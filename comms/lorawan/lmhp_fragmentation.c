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
#include <stdint.h>
#include <string.h>

#include <am_mcu_apollo.h>
#include <am_util.h>

#include <FreeRTOS.h>
#include <task.h>

#include "ota_config.h"

#include "lorawan_task.h"
#include "lmhp_fragmentation.h"

static uint8_t FragDataBlockAuthReqBuffer[5];

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

void lmhp_fragmentation_setup(LmhpFragmentationParams_t *parameters)
{
    parameters->OnProgress = OnFragProgress;
    parameters->OnDone = OnFragDone;
    parameters->DecoderCallbacks.FragDecoderWrite = FragDecoderWrite;
    parameters->DecoderCallbacks.FragDecoderRead = FragDecoderRead;
    parameters->DecoderCallbacks.FragDecoderErase = FragDecoderErase;
}