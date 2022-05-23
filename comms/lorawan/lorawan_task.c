/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2022, Northern Mechatronics, Inc.
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

#include "lorawan_task.h"
#include "lorawan_task_cli.h"
#include "lmh_callbacks.h"
#include "lmhp_fragmentation.h"
#include "console_task.h"
#include "ota_config.h"

typedef struct 
{
    LmHandlerMsgTypes_t tType;
    uint32_t    ui32Port;
    uint32_t    ui32Length;
    uint8_t    *ui8Data;
} lorawan_packet_t;

static TaskHandle_t lorawan_task_handle;
static QueueHandle_t lorawan_task_command_queue;
static QueueHandle_t lorawan_task_transmit_queue;

#define LM_BUFFER_SIZE 242
static uint8_t psLmDataBuffer[LM_BUFFER_SIZE];

static LmHandlerParams_t         lmh_parameters;
static LmHandlerCallbacks_t      lmh_callbacks;
static LmhpFragmentationParams_t lmhp_fragmentation_parameters;
static LmhpComplianceParams_t    LmComplianceParams;

static volatile bool McSessionStarted = false;

/*
 * Board ID is called by the LoRaWAN stack to
 * uniquely identify this device.
 * 
 * This example uses the processor ID
 */
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

static void lorawan_task_handle_uplink()
{
    if (McSessionStarted)
    {
        return;
    }

    lorawan_packet_t packet;
    if (xQueuePeek(lorawan_task_transmit_queue, &packet, 0) == pdPASS)
    {
        if (LmHandlerIsBusy() == true)
        {
            return;
        }

        xQueueReceive(lorawan_task_transmit_queue, &packet, 0);

        LmHandlerAppData_t app_data;

        if (packet.ui32Length > 0)
        {
            memcpy(psLmDataBuffer, packet.ui8Data, packet.ui32Length);
        }
        app_data.Port = packet.ui32Port;
        app_data.BufferSize = packet.ui32Length;
        app_data.Buffer = psLmDataBuffer;

        LmHandlerSend(&app_data, packet.tType);
    }
}

static void lorawan_task_handle_command()
{
    lorawan_command_t command;

    // do not block on message receive as the LoRa MAC state machine decides
    // when it is appropriate to sleep.  We also do not explicitly go to
    // sleep directly and simply do a task yield.  This allows other timing
    // critical radios such as BLE to run their state machines.
    if (xQueueReceive(lorawan_task_command_queue, &command, 0) == pdPASS)
    {
        switch (command.eCommand)
        {
        case LORAWAN_JOIN:
            LmHandlerJoin();
            break;
        case LORAWAN_SYNC_APP:
            LmhpClockSyncAppTimeReq();
            break;
        case LORAWAN_SYNC_MAC:
            LmHandlerDeviceTimeReq();
            break;
        default:
            break;
        }
    }
}

static void lorawan_task_setup()
{
    BoardInitMcu();
    BoardInitPeriph();

    lmh_parameters.Region = LORAMAC_REGION_US915;
    lmh_parameters.AdrEnable = true;
    lmh_parameters.TxDatarate = DR_0;
    lmh_parameters.PublicNetworkEnable = true;
    lmh_parameters.DataBufferMaxSize = LM_BUFFER_SIZE;
    lmh_parameters.DataBuffer = psLmDataBuffer;

    switch (lmh_parameters.Region)
    {
    case LORAMAC_REGION_EU868:
    case LORAMAC_REGION_RU864:
    case LORAMAC_REGION_CN779:
        lmh_parameters.DutyCycleEnabled = true;
        break;
    default:
        lmh_parameters.DutyCycleEnabled = false;
        break;
    }

    memset(&lmh_callbacks, 0, sizeof(LmHandlerCallbacks_t));
    lmh_callbacks_setup(&lmh_callbacks);
    LmHandlerInit(&lmh_callbacks, &lmh_parameters);
    LmHandlerSetSystemMaxRxError(20);

    LmHandlerPackageRegister(PACKAGE_ID_COMPLIANCE, &LmComplianceParams);

    LmHandlerPackageRegister(PACKAGE_ID_CLOCK_SYNC, NULL);

    LmHandlerPackageRegister(PACKAGE_ID_REMOTE_MCAST_SETUP, NULL);

    lmhp_fragmentation_setup(&lmhp_fragmentation_parameters);
    LmHandlerPackageRegister(PACKAGE_ID_FRAGMENTATION, &lmhp_fragmentation_parameters);
}

void lorawan_wake_on_radio_irq()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    lorawan_task_wake_from_isr(&xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void lorawan_wake_on_timer_irq()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    lorawan_task_wake_from_isr(&xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void lorawan_task(void *pvParameters)
{
    FreeRTOS_CLIRegisterCommand(&lorawan_command_definition);

    am_util_stdio_printf("\r\n\r\nLoRaWAN Application Demo Original\r\n\r\n");

    lorawan_task_setup();

    am_hal_gpio_pinconfig(AM_BSP_GPIO_LED4, g_AM_HAL_GPIO_OUTPUT);
    while (1)
    {
        am_hal_gpio_state_write(AM_BSP_GPIO_LED4, AM_HAL_GPIO_OUTPUT_SET);
        lorawan_task_handle_command();
        LmHandlerProcess();
        lorawan_task_handle_uplink();

        am_hal_gpio_state_write(AM_BSP_GPIO_LED4, AM_HAL_GPIO_OUTPUT_CLEAR);
        xTaskNotifyWait(0, 1, NULL, portMAX_DELAY);
    }
}

void lorawan_task_create(uint32_t ui32Priority)
{
    xTaskCreate(
        lorawan_task,
        "lorawan",
        512, 0, ui32Priority,
        &lorawan_task_handle);

    lorawan_task_command_queue = xQueueCreate(8, sizeof(lorawan_command_t));
    lorawan_task_transmit_queue = xQueueCreate(8, sizeof(lorawan_packet_t));
}

void lorawan_task_wake()
{
    xTaskNotify(lorawan_task_handle, 0, eNoAction);
}

void lorawan_task_wake_from_isr(BaseType_t *higher_priority_task_woken)
{
    xTaskNotifyFromISR(lorawan_task_handle, 0, eNoAction, higher_priority_task_woken);
}

void lorawan_send_command(lorawan_command_t *pCommand)
{
    xQueueSend(lorawan_task_command_queue, pCommand, 0);
    lorawan_task_wake();
}

void lorawan_transmit(uint32_t ui32Port, uint32_t ui32Ack, uint32_t ui32Length, uint8_t *ui8Data)
{
    lorawan_packet_t packet;

    packet.tType      = ui32Ack ? LORAMAC_HANDLER_CONFIRMED_MSG : LORAMAC_HANDLER_UNCONFIRMED_MSG;
    packet.ui32Port   = ui32Port;
    packet.ui32Length = ui32Length;

    if (ui32Length > 0)
    {
        packet.ui8Data = ui8Data;
        /*
        uint8_t *payload = pvPortMalloc(ui32Length);
        memcpy(payload, ui8Data, ui32Length);
        packet.ui8Data = payload;
        */
    }
    else
    {
        packet.ui8Data = NULL;
    }

    xQueueSend(lorawan_task_transmit_queue, &packet, 0);

    lorawan_task_wake();
}