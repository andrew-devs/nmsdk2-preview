/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2021, Northern Mechatronics, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <am_bsp.h>
#include <am_mcu_apollo.h>
#include <am_util.h>

#include <FreeRTOS.h>
#include <queue.h>

#include <LmHandler.h>
#include <LmHandlerMsgDisplay.h>
#include <LmhpClockSync.h>
#include <LmhpCompliance.h>
#include <LmhpFragmentation.h>
#include <LmhpRemoteMcastSetup.h>
#include <NvmDataMgmt.h>
#include <board.h>
#include <timer.h>
#include <utilities.h>

#include <lorawan_power.h>

#include "lorawan_task.h"
#include "lorawan_task_cli.h"

static TaskHandle_t  lorawan_task_handle;
static QueueHandle_t lorawan_command_queue;
static QueueHandle_t lorawan_transmit_queue;

typedef uint32_t lorawan_command_t;

typedef struct
{
    LmHandlerMsgTypes_t type;
    uint32_t            port;
    uint32_t            size;
    uint8_t            *payload;
} lorawan_packet_t;

static void lorawan_task(void *parameter);

void lorawan_wake_on_radio_irq()
{
    xTaskNotifyGive(lorawan_task_handle);
}

void lorawan_wake_on_timer()
{
    xTaskNotifyGive(lorawan_task_handle);
}

static void lorawan_task_handle_command()
{
    lorawan_command_t command;
    xQueueReceive(lorawan_command_queue, &command, 0);
}

static void lorawan_task_handle_uplink()
{
    lorawan_packet_t packet;
    xQueueReceive(lorawan_transmit_queue, &packet, 0);

    LmHandlerAppData_t app_data;
    app_data.Port       = packet.port;
    app_data.Buffer     = packet.payload;
    app_data.BufferSize = packet.size;
    LmHandlerSend(&app_data, packet.type);

    if (packet.size > 0)
    {
        vPortFree(packet.payload);
    }
}

void BoardGetUniqueId(uint8_t *id)
{
    am_util_id_t i;

    am_util_id_device(&i);

    id[0] = (uint8_t)(i.sMcuCtrlDevice.ui32ChipID0);
    id[1] = (uint8_t)(i.sMcuCtrlDevice.ui32ChipID0 >> 8);
    id[2] = (uint8_t)(i.sMcuCtrlDevice.ui32ChipID0 >> 16);
    id[3] = (uint8_t)(i.sMcuCtrlDevice.ui32ChipID0 >> 24);
    id[4] = (uint8_t)(i.sMcuCtrlDevice.ui32ChipID1);
    id[5] = (uint8_t)(i.sMcuCtrlDevice.ui32ChipID1 >> 8);
    id[6] = (uint8_t)(i.sMcuCtrlDevice.ui32ChipID1 >> 16);
    id[7] = (uint8_t)(i.sMcuCtrlDevice.ui32ChipID1 >> 24);
}

void lorawan_task_create(uint32_t priority)
{
    lorawan_command_queue  = xQueueCreate(10, sizeof(lorawan_command_t));
    lorawan_transmit_queue = xQueueCreate(10, sizeof(lorawan_packet_t));

    xTaskCreate(
        lorawan_task,
        "lorawan",
        512, 0, priority,
        &lorawan_task_handle);
}

void lorawan_task_command(uint32_t command)
{
    xQueueSend(lorawan_command_queue, &command, 0);
    xTaskNotifyGive(lorawan_task_handle);
}

void lorawan_task_transmit(LmHandlerMsgTypes_t type, uint32_t port, uint32_t size, uint8_t *data)
{
    lorawan_packet_t packet;
    uint8_t *payload;
    
    if (size > 0)
    {
        payload  = pvPortMalloc(size);
        memcpy(payload, data, size);
    }
    else
    {
        payload = NULL;
    }

    packet.type = type;
    packet.port = port;
    packet.size = size;
    packet.payload = payload;

    if (xQueueSend(lorawan_transmit_queue, &packet, 0))
    {
        xTaskNotifyGive(lorawan_task_handle);
    }
    else
    {
        if (payload)
        {
            vPortFree(payload);
        }
    }
}

void lorawan_task(void *parameter)
{
    lorawan_task_cli_register();

    am_hal_gpio_pinconfig(17, g_AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_state_write(17, AM_HAL_GPIO_OUTPUT_SET);

    while (1)
    {
        // 1. handle commands
        lorawan_task_handle_command();
        // 2. LmHandlerProcess();
        LmHandlerProcess();
        // 3. handle uplinks
        lorawan_task_handle_uplink();

        ulTaskNotifyTake(
            pdFALSE,
            portMAX_DELAY);

        am_hal_gpio_state_write(17, AM_HAL_GPIO_OUTPUT_TOGGLE);
        vTaskDelay(pdMS_TO_TICKS(500));

    }
}