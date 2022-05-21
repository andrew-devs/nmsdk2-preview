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
#include <string.h>

#include <am_mcu_apollo.h>
#include <am_util.h>

#include <FreeRTOS.h>
#include <task.h>

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

#include "loramac_layer5.h"
#include "lorawan_task.h"

static LmHandlerParams_t lmh_parameters;
static LmHandlerCallbacks_t lmh_callbacks;
static LmhpComplianceParams_t lmh_compliance_parameters;
//static LmhpFragmentationParams_t lmh_fragmentation_parameters;

#define LORAMAC_LAYER5_BUFFER_SIZE  (255)
static uint8_t loramac_layer5_databuffer[LORAMAC_LAYER5_BUFFER_SIZE];

static void cb_on_mac_process()
{
    lorawan_task_wake();
}

static void cb_on_join_request(LmHandlerJoinParams_t *params)
{
    DisplayJoinRequestUpdate(params);

    if (params->Status == LORAMAC_HANDLER_ERROR)
    {
        LmHandlerJoin();
    }
}

static void cb_on_network_parameters_changed(CommissioningParams_t *params)
{
    DisplayNetworkParametersUpdate(params);
}

static void cb_on_mac_mlme_request(LoRaMacStatus_t status, MlmeReq_t *mlmeReq, TimerTime_t nextTxDelay)
{
    DisplayMacMlmeRequestUpdate(status, mlmeReq, nextTxDelay);
}

static void cb_on_mac_mcps_request(LoRaMacStatus_t status, McpsReq_t *mcpsReq, TimerTime_t nextTxDelay)
{
    DisplayMacMcpsRequestUpdate(status, mcpsReq, nextTxDelay);
}

static void cb_on_systime_update(bool isSynchronized, int32_t timeCorrection)
{
}

static void cb_on_tx_data(LmHandlerTxParams_t *params)
{
    DisplayTxUpdate(params);
}

static void cb_on_rx_data(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
    DisplayRxUpdate(appData, params);
}

static void cb_on_class_change(DeviceClass_t deviceClass)
{
    DisplayClassUpdate(deviceClass);
}

static void cb_on_nvm_data_change(LmHandlerNvmContextStates_t state, uint16_t size)
{
    DisplayNvmDataChange(state, size);
}

static void cb_on_beacon_status_change(LoRaMacHandlerBeaconParams_t *params)
{
    DisplayBeaconUpdate(params);
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

void loramac_layer5_setup()
{
    BoardInitMcu();
    BoardInitPeriph();
    memset(loramac_layer5_databuffer, 0, LORAMAC_LAYER5_BUFFER_SIZE);

    lmh_parameters.Region = LORAMAC_REGION_US915;
    lmh_parameters.AdrEnable = true;
    lmh_parameters.TxDatarate = DR_0;
    lmh_parameters.PublicNetworkEnable = true;
    lmh_parameters.DataBufferMaxSize = LORAMAC_LAYER5_BUFFER_SIZE;
    lmh_parameters.DataBuffer = loramac_layer5_databuffer;

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
    // these are mandatory
    lmh_callbacks.OnMacProcess = cb_on_mac_process;
    lmh_callbacks.OnJoinRequest = cb_on_join_request;
    lmh_callbacks.OnNetworkParametersChange = cb_on_network_parameters_changed;
    lmh_callbacks.OnMacMlmeRequest = cb_on_mac_mlme_request;
    lmh_callbacks.OnMacMcpsRequest = cb_on_mac_mcps_request;
    lmh_callbacks.OnSysTimeUpdate = cb_on_systime_update;
    lmh_callbacks.OnTxData = cb_on_tx_data;
    lmh_callbacks.OnRxData = cb_on_rx_data;
    lmh_callbacks.OnClassChange = cb_on_class_change;
    lmh_callbacks.OnNvmDataChange = cb_on_nvm_data_change;
    lmh_callbacks.OnBeaconStatusChange = cb_on_beacon_status_change;

/*
    lmh_fragmentation_parameters.OnProgress = OnFragProgress;
    lmh_fragmentation_parameters.OnDone = OnFragDone;
    lmh_fragmentation_parameters.DecoderCallbacks.FragDecoderWrite = FragDecoderWrite;
    lmh_fragmentation_parameters.DecoderCallbacks.FragDecoderRead = FragDecoderRead;
    lmh_fragmentation_parameters.DecoderCallbacks.FragDecoderErase = FragDecoderErase;
*/
    LmHandlerErrorStatus_t status = LmHandlerInit(&lmh_callbacks, &lmh_parameters);
    if (status != LORAMAC_HANDLER_SUCCESS)
    {
    }
    LmHandlerSetSystemMaxRxError(20);
    LmHandlerPackageRegister(PACKAGE_ID_COMPLIANCE, &lmh_compliance_parameters);
    LmHandlerPackageRegister(PACKAGE_ID_CLOCK_SYNC, NULL);
    LmHandlerPackageRegister(PACKAGE_ID_REMOTE_MCAST_SETUP, NULL);

    /*
    LmHandlerPackageRegister(PACKAGE_ID_FRAGMENTATION, &lmh_fragmentation_parameters);

    FragmentSize = 0;
    FragmentNumber = 0;
    FragmentReceived = 0;
    PrepareFlashStorage = 0;
    */
}