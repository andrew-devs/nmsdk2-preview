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
#include <task.h>

#include "am_bsp.h"

#include "lorawan.h"

#include "application_task.h"
#include "application_task_cli.h"

static TaskHandle_t application_task_handle;
static QueueHandle_t lorawan_receive_queue;
static lorawan_rx_packet_t packet;

static void application_setup_lorawan()
{
    lorawan_set_app_eui_by_str("b4c231a359bc2e3d");
    lorawan_set_app_key_by_str("01c3f004a2d6efffe32c4eda14bcd2b4");
    lorawan_set_nwk_key_by_str("3f4ca100e2fc675ea123f4eb12c4a012");

    lorawan_receive_queue = lorawan_receive_register(1, 2);

    lorawan_command_t command;
    command.eCommand = LORAWAN_START;
    lorawan_send_command(&command);
}

static void application_task(void *parameter)
{
    application_task_cli_register();

    am_hal_gpio_pinconfig(AM_BSP_GPIO_LED0, g_AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_state_write(AM_BSP_GPIO_LED0, AM_HAL_GPIO_OUTPUT_SET);

    while (1)
    {
        if (xQueueReceive(lorawan_receive_queue, &packet, pdMS_TO_TICKS(500)) == pdPASS)
        {
            am_util_stdio_printf("\n\rReceived Data\n\r");
            am_util_stdio_printf("COUNTER   : %-4d\n\r", packet.ui32DownlinkCounter);
            am_util_stdio_printf("PORT      : %-4d\n\r", packet.ui32Port);
            am_util_stdio_printf("SLOT      : %-4d\n\r", packet.i16ReceiveSlot);
            am_util_stdio_printf("DATA RATE : %-4d\n\r", packet.i16DataRate);
            am_util_stdio_printf("RSSI      : %-4d\n\r", packet.i16RSSI);
            am_util_stdio_printf("SNR       : %-4d\n\r", packet.i16SNR);
            am_util_stdio_printf("SIZE      : %-4d\n\r", packet.ui32Length);
            am_util_stdio_printf("PAYLOAD   :\n\r");
            for (int i = 0; i < packet.ui32Length; i++)
            {
                am_util_stdio_printf("%02x ", packet.pui8Payload[i]);
            }
            am_util_stdio_printf("\n\r\n\r");
        }
        am_hal_gpio_state_write(AM_BSP_GPIO_LED0, AM_HAL_GPIO_OUTPUT_TOGGLE);
    }
}

void application_task_create(uint32_t priority)
{
    xTaskCreate(application_task, "application", 512, 0, priority, &application_task_handle);
}