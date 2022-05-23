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
#include "console_task.h"
#include "ota_config.h"

#define LORAWAN_DEFAULT_CLASS CLASS_A

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


static volatile bool ClockSynchronized = false;
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

/*
 * LoRaMAC Application Layer Callbacks
 */
static void OnClassChange(DeviceClass_t deviceClass)
{
    DisplayClassUpdate(deviceClass);
    switch (deviceClass)
    {
    case CLASS_A:
    {
        McSessionStarted = false;
    }
        break;
    case CLASS_B:
    {
        McSessionStarted = true;
    }
        break;
    case CLASS_C:
    {
        McSessionStarted = true;
    }
        break;
    default:
        break;
    }
}

static void OnBeaconStatusChange(LoRaMacHandlerBeaconParams_t *params)
{
    DisplayBeaconUpdate(params);
}

static void OnMacProcess(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    lorawan_task_wake_from_isr(&xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void OnJoinRequest(LmHandlerJoinParams_t *params)
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
        console_print_prompt();
    }

}

static void OnMacMlmeRequest(LoRaMacStatus_t status, MlmeReq_t *mlmeReq,
                             TimerTime_t nextTxDelay)
{
    am_util_stdio_printf("\r\n");
    DisplayMacMlmeRequestUpdate(status, mlmeReq, nextTxDelay);
    console_print_prompt();
}

static void OnMacMcpsRequest(LoRaMacStatus_t status, McpsReq_t *mcpsReq,
                             TimerTime_t nextTxDelay)
{
    am_util_stdio_printf("\r\n");
    DisplayMacMcpsRequestUpdate(status, mcpsReq, nextTxDelay);
    am_util_stdio_printf("FPORT       : %d\r\n", mcpsReq->Req.Unconfirmed.fPort);
    am_util_stdio_printf("BUFFERSIZE  : %d\r\n\r\n", mcpsReq->Req.Unconfirmed.fBufferSize);
    console_print_prompt();
}

static void OnNetworkParametersChange(CommissioningParams_t *params)
{
    am_util_stdio_printf("\r\n");
    DisplayNetworkParametersUpdate(params);
    console_print_prompt();
}

static void OnNvmDataChange(LmHandlerNvmContextStates_t state, uint16_t size)
{
    am_util_stdio_printf("\r\n");
    DisplayNvmDataChange(state, size);
    console_print_prompt();
}

static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
    am_util_stdio_printf("\r\n");
    DisplayRxUpdate(appData, params);

    switch (appData->Port)
    {
    case LM_APPLICATION_PORT:
        // process application specific data here
        break;

   default:
        break;
    }

    console_print_prompt();
}

static void OnSysTimeUpdate(bool isSynchronized, int32_t timeCorrection)
{
    ClockSynchronized = isSynchronized;
    am_util_stdio_printf("\r\n");
    am_util_stdio_printf("Clock Synchronized: %d\r\n", ClockSynchronized);
    am_util_stdio_printf("Correction: %d\r\n", timeCorrection);
    am_util_stdio_printf("\r\n");
}

static void OnTxData(LmHandlerTxParams_t *params)
{
    am_util_stdio_printf("\r\n");
    DisplayTxUpdate(params);
    console_print_prompt();
}

static void OnFragProgress(uint16_t counter, uint16_t blocks, uint8_t size,
                           uint16_t lost)
{
    am_util_stdio_printf(
        "\r\n###### =========== FRAG_DECODER ============ ######\r\n");
    am_util_stdio_printf(
        "######               PROGRESS                ######\r\n");
    am_util_stdio_printf(
        "###### ===================================== ######\r\n");
    am_util_stdio_printf("RECEIVED    : %5d / %5d Fragments\r\n", counter,
                         blocks);
    am_util_stdio_printf("              %5d / %5d Bytes\r\n", counter * size,
                         blocks * size);
    am_util_stdio_printf("LOST        :       %7d Fragments\r\n\r\n", lost);
}

static void OnFragDone(int32_t status, uint32_t size)
{
    uint32_t rx_crc = Crc32((uint8_t *)OTA_FLASH_ADDRESS, size);

    uint8_t FragDataBlockAuthReqBuffer[5];

    FragDataBlockAuthReqBuffer[0] = 0x05;
    FragDataBlockAuthReqBuffer[1] = rx_crc & 0x000000FF;
    FragDataBlockAuthReqBuffer[2] = (rx_crc >> 8) & 0x000000FF;
    FragDataBlockAuthReqBuffer[3] = (rx_crc >> 16) & 0x000000FF;
    FragDataBlockAuthReqBuffer[4] = (rx_crc >> 24) & 0x000000FF;

    lorawan_transmit(201, LORAMAC_HANDLER_UNCONFIRMED_MSG, 5, FragDataBlockAuthReqBuffer);

    am_util_stdio_printf("\r\n");
    am_util_stdio_printf(
        "###### =========== FRAG_DECODER ============ ######\r\n");
    am_util_stdio_printf(
        "######               FINISHED                ######\r\n");
    am_util_stdio_printf(
        "###### ===================================== ######\r\n");
    am_util_stdio_printf("STATUS : %ld\r\n", status);
    am_util_stdio_printf("SIZE   : %ld\r\n", size);
    am_util_stdio_printf("CRC    : %08lX\n\n", rx_crc);
}

static int8_t FragDecoderWrite(uint32_t offset, uint8_t *data, uint32_t size)
{
    uint32_t *destination = (uint32_t *)(OTA_FLASH_ADDRESS + offset);
    uint32_t source[64];
    uint32_t length = size >> 2;

    am_util_stdio_printf("\r\nDecoder Write: 0x%x, 0x%x, %d\r\n",
                         (uint32_t)destination, (uint32_t)source, length);
    memcpy(source, data, size);

    taskENTER_CRITICAL();

    am_hal_flash_program_main(AM_HAL_FLASH_PROGRAM_KEY, source, destination,
                              length);

    taskEXIT_CRITICAL();
    return 0;
}

static int8_t FragDecoderRead(uint32_t offset, uint8_t *data, uint32_t size)
{
    uint8_t *UnfragmentedData = (uint8_t *)(OTA_FLASH_ADDRESS);
    for (uint32_t i = 0; i < size; i++)
    {
        data[i] = UnfragmentedData[offset + i];
    }
    return 0;
}

static int8_t FragDecoderErase(uint32_t offset, uint32_t size)
{
    uint32_t totalPage = (size >> 13) + 1;
    uint32_t address = OTA_FLASH_ADDRESS;

    am_util_stdio_printf("\r\nErasing %d pages at 0x%x\r\n", totalPage,
                         address);

    for (int i = 0; i < totalPage; i++)
    {
        address += AM_HAL_FLASH_PAGE_SIZE;
        am_util_stdio_printf("Instance: %d, Page: %d\r\n",
                             AM_HAL_FLASH_ADDR2INST(address),
                             AM_HAL_FLASH_ADDR2PAGE(address));

        taskENTER_CRITICAL();

        am_hal_flash_page_erase(AM_HAL_FLASH_PROGRAM_KEY,
                                AM_HAL_FLASH_ADDR2INST(address),
                                AM_HAL_FLASH_ADDR2PAGE(address));

        taskEXIT_CRITICAL();
    }

    return 0;
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
            vPortFree(app_data.Buffer);
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
    LmHandlerParams_t parameters;
    LmHandlerCallbacks_t LmCallbacks;
    LmhpFragmentationParams_t lmh_fragmentation_parameters;
    LmhpComplianceParams_t LmComplianceParams;

    BoardInitMcu();
    BoardInitPeriph();

    parameters.Region = LORAMAC_REGION_US915;
    parameters.AdrEnable = true;
    parameters.TxDatarate = DR_0;
    parameters.PublicNetworkEnable = true;
    parameters.DataBufferMaxSize = LM_BUFFER_SIZE;
    parameters.DataBuffer = psLmDataBuffer;

    switch (parameters.Region)
    {
    case LORAMAC_REGION_EU868:
    case LORAMAC_REGION_RU864:
    case LORAMAC_REGION_CN779:
        parameters.DutyCycleEnabled = true;
        break;
    default:
        parameters.DutyCycleEnabled = false;
        break;
    }

    memset(&LmCallbacks, 0, sizeof(LmHandlerCallbacks_t));
    // these are mandatory
    LmCallbacks.OnMacProcess = OnMacProcess;
    LmCallbacks.OnJoinRequest = OnJoinRequest;
    LmCallbacks.OnNetworkParametersChange = OnNetworkParametersChange;
    LmCallbacks.OnMacMlmeRequest = OnMacMlmeRequest;
    LmCallbacks.OnMacMcpsRequest = OnMacMcpsRequest;
    LmCallbacks.OnSysTimeUpdate = OnSysTimeUpdate;
    LmCallbacks.OnTxData = OnTxData;
    LmCallbacks.OnRxData = OnRxData;
    LmCallbacks.OnClassChange = OnClassChange;
    LmCallbacks.OnNvmDataChange = OnNvmDataChange;
    LmCallbacks.OnBeaconStatusChange = OnBeaconStatusChange;

    lmh_fragmentation_parameters.OnProgress = OnFragProgress;
    lmh_fragmentation_parameters.OnDone = OnFragDone;
    lmh_fragmentation_parameters.DecoderCallbacks.FragDecoderWrite = FragDecoderWrite;
    lmh_fragmentation_parameters.DecoderCallbacks.FragDecoderRead = FragDecoderRead;
    lmh_fragmentation_parameters.DecoderCallbacks.FragDecoderErase = FragDecoderErase;

    LmHandlerErrorStatus_t status = LmHandlerInit(&LmCallbacks, &parameters);
    if (status != LORAMAC_HANDLER_SUCCESS)
    {
        am_util_stdio_printf("\r\n\r\nLoRaWAN application framework "
                             "initialization failed\r\n\r\n");
        console_print_prompt();
    }
    LmHandlerSetSystemMaxRxError(20);
    LmHandlerPackageRegister(PACKAGE_ID_COMPLIANCE, &LmComplianceParams);
    LmHandlerPackageRegister(PACKAGE_ID_CLOCK_SYNC, NULL);
    LmHandlerPackageRegister(PACKAGE_ID_REMOTE_MCAST_SETUP, NULL);
    LmHandlerPackageRegister(PACKAGE_ID_FRAGMENTATION, &lmh_fragmentation_parameters);
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
        // free at OnTxDone
        uint8_t *payload = pvPortMalloc(ui32Length);
        memcpy(payload, ui8Data, ui32Length);
        packet.ui8Data = payload;
    }
    else
    {
        packet.ui8Data = NULL;
    }

    packet.ui8Data = ui8Data;

    xQueueSend(lorawan_task_transmit_queue, &packet, 0);

    lorawan_task_wake();
}