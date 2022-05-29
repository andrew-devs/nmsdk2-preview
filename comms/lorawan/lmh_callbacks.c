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
#include <am_mcu_apollo.h>
#include <am_util.h>

#include <FreeRTOS.h>
#include <list.h>

#include <LmHandlerMsgDisplay.h>

#include "lorawan_config.h"

#include "lorawan.h"
#include "lorawan_task.h"
#include "lmh_callbacks.h"
#include "console_task.h"

static List_t lorawan_receive_callback_list;

static void lmh_rx_callback_service(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);

static void lmh_on_mac_process(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    lorawan_task_wake_from_isr(&xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void lmh_on_nvm_data_change(LmHandlerNvmContextStates_t state, uint16_t size)
{
    am_util_stdio_printf("\r\n");
    DisplayNvmDataChange(state, size);
    console_print_prompt();

    lorawan_task_wake();
}

static void lmh_on_network_parameters_change(CommissioningParams_t *params)
{
    am_util_stdio_printf("\r\n");
    DisplayNetworkParametersUpdate(params);
    console_print_prompt();

    lorawan_task_wake();
}

static void lmh_on_mac_mcps_request(LoRaMacStatus_t status, McpsReq_t *mcpsReq,
                             TimerTime_t nextTxDelay)
{
    am_util_stdio_printf("\r\n");
    DisplayMacMcpsRequestUpdate(status, mcpsReq, nextTxDelay);
    am_util_stdio_printf("FPORT       : %d\r\n", mcpsReq->Req.Unconfirmed.fPort);
    am_util_stdio_printf("BUFFERSIZE  : %d\r\n\r\n", mcpsReq->Req.Unconfirmed.fBufferSize);
    console_print_prompt();

    lorawan_task_wake();
}

static void lmh_on_mac_mlme_request(LoRaMacStatus_t status, MlmeReq_t *mlmeReq,
                             TimerTime_t nextTxDelay)
{
    am_util_stdio_printf("\r\n");
    DisplayMacMlmeRequestUpdate(status, mlmeReq, nextTxDelay);
    console_print_prompt();

    lorawan_task_wake();
}

static void lmh_on_join_request(LmHandlerJoinParams_t *params)
{
    if (params->Status == LORAMAC_HANDLER_ERROR)
    {
        LmHandlerJoin();
    }
    else
    {
        am_util_stdio_printf("\r\n");
        DisplayJoinRequestUpdate(params);
        
        LmHandlerRequestClass(LORAWAN_DEFAULT_CLASS);
        LmHandlerDeviceTimeReq();
    }

    lorawan_task_wake();
}

static void lmh_on_tx_data(LmHandlerTxParams_t *params)
{
    am_util_stdio_printf("\r\n");
    DisplayTxUpdate(params);
    console_print_prompt();

    lorawan_task_wake();
}

static void lmh_on_rx_data(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
    am_util_stdio_printf("\r\n");
    DisplayRxUpdate(appData, params);
    console_print_prompt();

    lmh_rx_callback_service(appData, params);

    lorawan_task_wake();
}

static void lmh_on_class_change(DeviceClass_t deviceClass)
{
    DisplayClassUpdate(deviceClass);
    console_print_prompt();

    lorawan_task_wake();
}

static void lmh_on_beacon_status_change(LoRaMacHandlerBeaconParams_t *params)
{
    DisplayBeaconUpdate(params);
    console_print_prompt();

    lorawan_task_wake();
}

static void lmh_on_sys_time_update(bool isSynchronized, int32_t timeCorrection)
{
    am_util_stdio_printf("\r\n");
    am_util_stdio_printf("Clock Synchronized: %d\r\n", isSynchronized);
    am_util_stdio_printf("Correction: %d\r\n", timeCorrection);
    am_util_stdio_printf("\r\n");
    console_print_prompt();

    lorawan_task_wake();
}

void lmh_callbacks_setup(LmHandlerCallbacks_t *cb)
{
    cb->GetBatteryLevel = NULL;
    cb->GetTemperature = NULL;
    cb->GetRandomSeed = NULL;
    cb->OnMacProcess = lmh_on_mac_process;
    cb->OnNvmDataChange = lmh_on_nvm_data_change;
    cb->OnNetworkParametersChange = lmh_on_network_parameters_change;
    cb->OnMacMcpsRequest = lmh_on_mac_mcps_request;
    cb->OnMacMlmeRequest = lmh_on_mac_mlme_request;
    cb->OnJoinRequest = lmh_on_join_request;
    cb->OnTxData = lmh_on_tx_data;
    cb->OnRxData = lmh_on_rx_data;
    cb->OnClassChange = lmh_on_class_change;
    cb->OnBeaconStatusChange = lmh_on_beacon_status_change;
    cb->OnSysTimeUpdate = lmh_on_sys_time_update;

    vListInitialise(&lorawan_receive_callback_list);
}

void lorawan_receive_register(uint32_t ui32Port, QueueHandle_t pHandle)
{
    ListItem_t *list_item = pvPortMalloc(sizeof(ListItem_t));
    vListInitialiseItem(list_item);

    list_item->xItemValue = ui32Port;
    list_item->pvOwner = pHandle;

    vListInsertEnd(&lorawan_receive_callback_list, list_item);
}

void lorawan_receive_unregister(uint32_t ui32Port, QueueHandle_t pHandle)
{
    ListItem_t *pItem = listGET_HEAD_ENTRY(&lorawan_receive_callback_list);

    while (pItem != listGET_END_MARKER(&lorawan_receive_callback_list))
    {
        if ((pItem->pvOwner == pHandle) &&
            (pItem->xItemValue == ui32Port))
        {
            listREMOVE_ITEM(pItem);
            return;
        }

        pItem = listGET_NEXT(pItem);
    }
}

void lmh_rx_callback_service(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
    ListItem_t *pItem = listGET_HEAD_ENTRY(&lorawan_receive_callback_list);

    while (pItem != listGET_END_MARKER(&lorawan_receive_callback_list))
    {
        if (pItem->pvOwner)
        {
            lorawan_rx_packet_t packet;
            packet.ui32DownlinkCounter = params->DownlinkCounter;
            packet.i16DataRate = params->Datarate;
            packet.i16ReceiveSlot = params->RxSlot;
            packet.i16RSSI = params->Rssi;
            packet.i16SNR = params->Snr;
            packet.ui32Port = appData->Port;
            packet.ui32Length = appData->BufferSize;
            packet.pui8Payload = appData->Buffer;

            xQueueSend(pItem->pvOwner, &packet, 0);
        }

        pItem = listGET_NEXT(pItem);
    }
}