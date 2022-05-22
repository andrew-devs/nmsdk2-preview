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

#include "lorawan_task.h"
#include "lorawan_task_cli.h"
#include "console_task.h"
#include "task_message.h"
#include "ota_config.h"

#define LORAWAN_DEFAULT_CLASS CLASS_A

#define FRAGMENTATION_DATA_FRAGMENT 0x08

TaskHandle_t lorawan_task_handle;
QueueHandle_t lorawan_task_queue;

uint8_t psLmDataBuffer[LM_BUFFER_SIZE];
LmHandlerAppData_t LmAppData;
LmHandlerMsgTypes_t LmMsgType;

static uint8_t FragDataBlockAuthReqBuffer[5];

static LmHandlerParams_t parameters;
static LmHandlerCallbacks_t LmCallbacks;
static LmhpFragmentationParams_t lmh_fragmentation_parameters;
static LmhpComplianceParams_t LmComplianceParams;

static volatile bool TransmitPending = false;
static volatile bool MacProcessing = false;
static volatile bool ClockSynchronized = false;
static volatile bool McSessionStarted = false;
static volatile bool TransferCompleted = false;

static uint32_t timeout = portMAX_DELAY;

static uint8_t FragmentSize;
static uint8_t FragmentNumber;
static uint8_t FragmentReceived;
static uint8_t PrepareFlashStorage;
static LmHandlerAppData_t fragComplete;

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

static void TclProcessCommand(LmHandlerAppData_t *appData)
{
    // Only handle the reset command as that is indicative of the
    // beginning of compliance testing.  All the other compliance test
    // commands are handle by the Compliance state machine
    switch (appData->Buffer[0])
    {
    case 0x01:
        am_util_stdio_printf("Tcl: LoRaWAN MAC layer reset requested\r\n");
        break;
    case 0x05:
        am_util_stdio_printf("Tcl: Duty cycle set to %d\r\n",
                             appData->Buffer[1]);
        break;
    }
}

/*
 * LoRaMAC Application Layer Callbacks
 */
static void OnClassChange(DeviceClass_t deviceClass)
{
    DisplayClassUpdate(deviceClass);
    switch (deviceClass)
    {
    default:
    case CLASS_A:
    {
        McSessionStarted = false;
    }
    break;
    case CLASS_B:
    {
        LmHandlerAppData_t appData = {
            .Buffer = NULL,
            .BufferSize = 0,
            .Port = 0,
        };
        LmHandlerSend(&appData, LORAMAC_HANDLER_UNCONFIRMED_MSG);
        McSessionStarted = true;
    }
    break;
    case CLASS_C:
    {
        McSessionStarted = true;
    }
    break;
    }
}

static void OnBeaconStatusChange(LoRaMacHandlerBeaconParams_t *params)
{
    DisplayBeaconUpdate(params);

    switch (params->State)
    {
    case LORAMAC_HANDLER_BEACON_RX:
        break;
    case LORAMAC_HANDLER_BEACON_LOST:
        break;
    case LORAMAC_HANDLER_BEACON_NRX:
        break;
    default:
        break;
    }
}

static void OnMacProcess(void)
{
    /*
    // this is called inside an IRQ
    MacProcessing = true;
    timeout = 0;
#if defined(AM_BSP_GPIO_LED4)
    am_hal_gpio_state_write(AM_BSP_GPIO_LED4, AM_HAL_GPIO_OUTPUT_SET);
#endif // defined(AM_BSP_GPIO_LED4)
*/
    lorawan_task_wake();
}

static void OnJoinRequest(LmHandlerJoinParams_t *params)
{
    am_util_stdio_printf("\r\n");
    DisplayJoinRequestUpdate(params);

    if (params->Status == LORAMAC_HANDLER_ERROR)
    {
        LmHandlerJoin();
    }
    else
    {
        LmHandlerRequestClass(LORAWAN_DEFAULT_CLASS);
    }

    console_print_prompt();
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
    case 0:
        am_util_stdio_printf("MAC command received\r\n");
        am_util_stdio_printf("Size: %d\r\n", appData->BufferSize);
        break;

    case 3:
        if (appData->BufferSize == 1)
        {
            switch (appData->Buffer[0])
            {
            case 0:
                LmHandlerRequestClass(CLASS_A);
                break;
            case 1:
                LmHandlerRequestClass(CLASS_B);
                break;
            case 2:
                LmHandlerRequestClass(CLASS_C);
                break;
            default:
                break;
            }
        }
        break;

    case LM_APPLICATION_PORT:
        // process application specific data here
        break;

    case LM_MULTICAST_PORT:
        break;

    case LM_FUOTA_PORT:

        break;

    case LM_CLOCKSYNC_PORT:
        // override application layer time sync here if needed
        break;

    case LM_COMPLIANCE_PORT:
        TclProcessCommand(appData);
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

    FragDataBlockAuthReqBuffer[0] = 0x05;
    FragDataBlockAuthReqBuffer[1] = rx_crc & 0x000000FF;
    FragDataBlockAuthReqBuffer[2] = (rx_crc >> 8) & 0x000000FF;
    FragDataBlockAuthReqBuffer[3] = (rx_crc >> 16) & 0x000000FF;
    FragDataBlockAuthReqBuffer[4] = (rx_crc >> 24) & 0x000000FF;

    TransferCompleted = true;
    TransmitPending = true;

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

void application_handle_uplink()
{
    if (TransmitPending)
    {
        if (LmHandlerIsBusy() == true)
        {
            return;
        }

        if (McSessionStarted == false)
        {
            if (TransferCompleted)
            {
                TransferCompleted = false;
                TransmitPending = false;

                fragComplete.Buffer = FragDataBlockAuthReqBuffer;
                fragComplete.BufferSize = 5;
                fragComplete.Port = LM_FUOTA_PORT;

                LmHandlerSend(&fragComplete, LORAMAC_HANDLER_UNCONFIRMED_MSG);
            }
            else
            {
                TransmitPending = false;
                LmHandlerSend(&LmAppData, LmMsgType);
            }
        }
    }
}

void application_handle_command()
{
    task_message_t TaskMessage;

    // do not block on message receive as the LoRa MAC state machine decides
    // when it is appropriate to sleep.  We also do not explicitly go to
    // sleep directly and simply do a task yield.  This allows other timing
    // critical radios such as BLE to run their state machines.
    if (xQueueReceive(lorawan_task_queue, &TaskMessage, 0) == pdPASS)
    {
        switch (TaskMessage.ui32Event)
        {
        case JOIN:
            LmHandlerJoin();
            break;
        case SEND:
            TransmitPending = true;
            break;
        case SYNC_APP:
            LmhpClockSyncAppTimeReq();
            break;
        case SYNC_MAC:
            LmHandlerDeviceTimeReq();
            break;
        case WAKE:
            break;
        }
    }
}

void application_setup()
{
    LmMsgType = LORAMAC_HANDLER_UNCONFIRMED_MSG;

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

    FragmentSize = 0;
    FragmentNumber = 0;
    FragmentReceived = 0;
    PrepareFlashStorage = 0;
}

static char *otaStatusMessage[] = {"Success", "Error", "Failure", "Pending"};

static void dump_ota_status(void)
{
    uint32_t *pOtaDesc =
        (uint32_t *)(OTA_POINTER_LOCATION & ~(AM_HAL_FLASH_PAGE_SIZE - 1));
    uint32_t i;

    // Check if the current content at OTA descriptor is valid
    for (i = 0; i < AM_HAL_SECURE_OTA_MAX_OTA + 1; i++)
    {
        // Make sure the image address looks okay
        if (pOtaDesc[i] == 0xFFFFFFFF)
        {
            break;
        }
        if (((pOtaDesc[i] & 0x3) == AM_HAL_OTA_STATUS_ERROR) ||
            ((pOtaDesc[i] & ~0x3) >= 0x100000))
        {
            break;
        }
    }
    if (pOtaDesc[i] == 0xFFFFFFFF)
    {
        am_util_stdio_printf("\r\nValid Previous OTA state\r\n");
        // It seems in last boot this was used as OTA descriptor
        // Dump previous OTA information
        am_hal_ota_status_t otaStatus[AM_HAL_SECURE_OTA_MAX_OTA];
        am_hal_get_ota_status(pOtaDesc, AM_HAL_SECURE_OTA_MAX_OTA, otaStatus);
        for (uint32_t i = 0; i < AM_HAL_SECURE_OTA_MAX_OTA; i++)
        {
            if ((uint32_t)otaStatus[i].pImage == 0xFFFFFFFF)
            {
                break;
            }
            {
                am_util_stdio_printf(
                    "\r\nPrevious OTA: Blob Addr: 0x%x - Result %s\r\n",
                    otaStatus[i].pImage, otaStatusMessage[otaStatus[i].status]);
            }
        }
    }
    else
    {
        am_util_stdio_printf("\r\nNo Previous OTA state\r\n");
    }
}

void lorawan_wake_on_radio_irq()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(lorawan_task_handle, 0, eNoAction, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void lorawan_wake_on_timer_irq()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(lorawan_task_handle, 0, eNoAction, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void lorawan_task(void *pvParameters)
{
    FreeRTOS_CLIRegisterCommand(&LoRaWANCommandDefinition);
    lorawan_task_queue = xQueueCreate(10, sizeof(task_message_t));

    am_util_stdio_printf("\r\n\r\nLoRaWAN Application Demo Original\r\n\r\n");
    console_print_prompt();

    dump_ota_status();

    application_setup();

    am_hal_gpio_pinconfig(17, g_AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_state_write(17, AM_HAL_GPIO_OUTPUT_SET);

    TransmitPending = false;
    timeout = portMAX_DELAY;
    while (1)
    {
        application_handle_command();
        LmHandlerProcess();
        application_handle_uplink();

        xTaskNotifyWait(0, 1, NULL, portMAX_DELAY);
        am_hal_gpio_state_write(AM_BSP_GPIO_LED4, AM_HAL_GPIO_OUTPUT_TOGGLE);
/*
        if (MacProcessing)
        {
            taskENTER_CRITICAL();
            MacProcessing = false;
            timeout = portMAX_DELAY;
            taskEXIT_CRITICAL();
        }
        else
        {
#if defined(AM_BSP_GPIO_LED4)
#endif // defined(AM_BSP_GPIO_LED4)
        }
*/
    }
}

void lorawan_task_create(uint32_t priority)
{
    xTaskCreate(
        lorawan_task,
        "lorawan",
        512, 0, priority,
        &lorawan_task_handle);
}

void lorawan_task_wake()
{
    xTaskNotify(lorawan_task_handle, 0, eNoAction);
}