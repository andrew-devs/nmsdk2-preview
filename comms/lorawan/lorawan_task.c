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
#include <LmhpClockSync.h>
#include <LmhpCompliance.h>
#include <LmhpFragmentation.h>
#include <LmhpRemoteMcastSetup.h>
#include <board.h>
#include <radio.h>

#include "lorawan.h"
#include "lorawan_config.h"

#include "lmh_callbacks.h"
#include "lmhp_fragmentation.h"
#include "lorawan_task.h"
#include "lorawan_task_cli.h"

static uint32_t lorawan_stack_started;
static lorawan_power_management_t lorawan_pm_callback;
static TaskHandle_t lorawan_task_handle;
static QueueHandle_t lorawan_task_command_queue;
static QueueHandle_t lorawan_task_transmit_queue;

#define LM_BUFFER_SIZE 242
static uint8_t psLmDataBuffer[LM_BUFFER_SIZE];

static LmHandlerParams_t lmh_parameters;
static LmHandlerCallbacks_t lmh_callbacks;
static LmhpFragmentationParams_t lmhp_fragmentation_parameters;
static LmhpComplianceParams_t lmhp_compliance_parameters;

static void lorawan_stack_start();
static void lorawan_stack_stop();

static void lorawan_task_handle_power_management(lorawan_pm_state_e state)
{
    if (lorawan_pm_callback == NULL)
    {
        return;
    }

    if (state == LORAWAN_PM_SLEEP)
    {
        if (Radio.GetStatus() == RF_IDLE)
        {
            lorawan_pm_callback(state);
        }
    }
    else if (state == LORAWAN_PM_WAKE)
    {
        lorawan_pm_callback(state);
    }
}

static void lorawan_task_handle_uplink()
{
    if (LmhpRemoteMcastSessionStateStarted())
    {
        return;
    }

    lorawan_tx_packet_t packet;
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
            memcpy(psLmDataBuffer, packet.pui8Data, packet.ui32Length);
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
        if (command.eCommand == LORAWAN_START)
        {
            lorawan_stack_start(); 
            return;
        }

        if (lorawan_stack_started)
        {
            switch (command.eCommand)
            {
            case LORAWAN_STOP:
                lorawan_stack_stop();
                break;
            case LORAWAN_JOIN:
                LmHandlerJoin();
                break;
            case LORAWAN_SYNC_APP:
                LmhpClockSyncAppTimeReq();
                break;
            case LORAWAN_SYNC_MAC:
                LmHandlerDeviceTimeReq();
                break;
            case LORAWAN_CLASS_SET:
                LmHandlerRequestClass((DeviceClass_t)command.pvParameters);
                break;
            default:
                break;
            }
        }
    }
}

static void lorawan_stack_start()
{
    if (lorawan_stack_started)
    {
        return;
    }
    lorawan_task_handle_power_management(LORAWAN_PM_WAKE);

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

    LmHandlerInit(&lmh_callbacks, &lmh_parameters);
    LmHandlerSetSystemMaxRxError(20);

    LmHandlerPackageRegister(PACKAGE_ID_COMPLIANCE, &lmhp_compliance_parameters);
    LmHandlerPackageRegister(PACKAGE_ID_CLOCK_SYNC, NULL);
    LmHandlerPackageRegister(PACKAGE_ID_REMOTE_MCAST_SETUP, NULL);

    lmhp_fragmentation_setup(&lmhp_fragmentation_parameters);
    LmHandlerPackageRegister(PACKAGE_ID_FRAGMENTATION, &lmhp_fragmentation_parameters);

    if (LmHandlerJoinStatus() == LORAMAC_HANDLER_SET)
    {
        LmHandlerRequestClass(LORAWAN_DEFAULT_CLASS);
        LmHandlerDeviceTimeReq();
    }
    Radio.Sleep();

    lorawan_stack_started = true;
}

void lorawan_stack_stop()
{
    LoRaMacStop();
    LoRaMacDeInitialization();
    BoardDeInitMcu();
    lorawan_task_handle_power_management(LORAWAN_PM_SLEEP);
    xQueueReset(lorawan_task_transmit_queue);
    lorawan_stack_started = false;
}

void lorawan_wake_on_radio_irq()
{
    lorawan_task_wake();
}

void lorawan_wake_on_timer_irq()
{
    lorawan_task_wake();
}

static void lorawan_task(void *pvParameters)
{
    lorawan_stack_started = false;
    lorawan_task_cli_register();

    while (1)
    {
        lorawan_task_handle_command();

        if (lorawan_stack_started)
        {
            LmHandlerProcess();
            lorawan_task_handle_uplink();
        }

        lorawan_task_handle_power_management(LORAWAN_PM_SLEEP);
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        lorawan_task_handle_power_management(LORAWAN_PM_WAKE);
    }
}

void lorawan_task_create(uint32_t ui32Priority)
{
    xTaskCreate(lorawan_task, "lorawan", 512, 0, ui32Priority, &lorawan_task_handle);

    lorawan_task_command_queue = xQueueCreate(8, sizeof(lorawan_command_t));
    lorawan_task_transmit_queue = xQueueCreate(8, sizeof(lorawan_tx_packet_t));

    memset(&lmh_callbacks, 0, sizeof(LmHandlerCallbacks_t));
    lmh_callbacks_setup(&lmh_callbacks);

    lorawan_pm_callback = NULL;
}

void lorawan_task_wake()
{
    BaseType_t xHigherPriorityTaskWoken;

    if (xPortIsInsideInterrupt() == pdTRUE)
    {
        xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(lorawan_task_handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
        xTaskNotifyGive(lorawan_task_handle);
    }
}

void lorawan_send_command(lorawan_command_t *pCommand)
{
    // prevent context switch until task notification is completed
    taskENTER_CRITICAL();

    xQueueSend(lorawan_task_command_queue, pCommand, 0);
    lorawan_task_wake();

    taskEXIT_CRITICAL();
}

void lorawan_transmit(uint32_t ui32Port, uint32_t ui32Ack, uint32_t ui32Length, uint8_t *pui8Data)
{
    lorawan_tx_packet_t packet;

    packet.tType = ui32Ack ? LORAMAC_HANDLER_CONFIRMED_MSG : LORAMAC_HANDLER_UNCONFIRMED_MSG;
    packet.ui32Port = ui32Port;
    packet.ui32Length = ui32Length;

    if (ui32Length > 0)
    {
        packet.pui8Data = pui8Data;
    }
    else
    {
        packet.pui8Data = NULL;
    }

    // prevent context switch until task notification is completed
    taskENTER_CRITICAL();

    xQueueSend(lorawan_task_transmit_queue, &packet, 0);
    lorawan_task_wake();

    taskEXIT_CRITICAL();
}

void lorawan_power_management_register(lorawan_power_management_t pHandler)
{
    lorawan_pm_callback = pHandler;
}
