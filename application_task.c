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
#include <timers.h>

#include "am_bsp.h"

#include "lorawan.h"
#include "lorawan_task_cli.h"

#include "application_task.h"
#include "application_task_cli.h"

static TaskHandle_t application_task_handle;
static QueueHandle_t lorawan_receive_queue;
static lorawan_rx_packet_t packet;

void application_button_handler()
{
    lorawan_transmit(1, LORAMAC_HANDLER_UNCONFIRMED_MSG, 0, NULL);
    portYIELD();
}

void application_pm_lorawan(lorawan_pm_state_e state)
{
    if (state == LORAWAN_PM_SLEEP)
    {
        am_hal_gpio_state_write(AM_BSP_GPIO_LED1, AM_HAL_GPIO_OUTPUT_SET);
//        am_hal_gpio_state_write(AM_BSP_GPIO_LORA_EN, AM_HAL_GPIO_OUTPUT_CLEAR);
    }
    else if (state == LORAWAN_PM_WAKE)
    {
        am_hal_gpio_state_write(AM_BSP_GPIO_LED1, AM_HAL_GPIO_OUTPUT_CLEAR);
//        am_hal_gpio_state_write(AM_BSP_GPIO_LORA_EN, AM_HAL_GPIO_OUTPUT_SET);
    }
}

static void application_setup_task()
{
    am_hal_gpio_pinconfig(AM_BSP_GPIO_LED0, g_AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_state_write(AM_BSP_GPIO_LED0, AM_HAL_GPIO_OUTPUT_SET);

    am_hal_gpio_pinconfig(AM_BSP_GPIO_LED1, g_AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_state_write(AM_BSP_GPIO_LED1, AM_HAL_GPIO_OUTPUT_SET);

    am_hal_gpio_pinconfig(AM_BSP_GPIO_LORA_EN, g_AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_state_write(AM_BSP_GPIO_LORA_EN, AM_HAL_GPIO_OUTPUT_SET);

    am_hal_gpio_pinconfig(AM_BSP_GPIO_SENSORS_EN, g_AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_state_write(AM_BSP_GPIO_SENSORS_EN, AM_HAL_GPIO_OUTPUT_CLEAR);

    am_hal_gpio_pinconfig(AM_BSP_GPIO_USB_DETECT, g_AM_HAL_GPIO_INPUT);

    am_hal_gpio_pinconfig(AM_BSP_GPIO_BUTTON0, g_AM_HAL_GPIO_INPUT);
    am_hal_gpio_interrupt_register(AM_BSP_GPIO_BUTTON0, application_button_handler);
    am_hal_gpio_interrupt_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_BUTTON0));
    am_hal_gpio_interrupt_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_BUTTON0));
}

static void application_setup_lorawan()
{
    lorawan_set_app_eui_by_str("b4c231a359bc2e3d");
    lorawan_set_app_key_by_str("01c3f004a2d6efffe32c4eda14bcd2b4");
    lorawan_set_nwk_key_by_str("3f4ca100e2fc675ea123f4eb12c4a012");

    lorawan_receive_queue = lorawan_receive_register(1, 2);
    lorawan_power_management_register(application_pm_lorawan);

    // start the LoRaWAN stack
    lorawan_command_t command = { .eCommand = LORAWAN_START, .pvParameters = NULL };
    lorawan_send_command(&command);
}

static void application_task(void *parameter)
{
    application_task_cli_register();

    application_setup_task();
    application_setup_lorawan();

    while (1)
    {
        if (xQueueReceive(lorawan_receive_queue, &packet, 0) == pdPASS)
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

        vTaskDelay(pdMS_TO_TICKS(15000));
        lorawan_transmit(1, LORAMAC_HANDLER_UNCONFIRMED_MSG, 0, NULL);
    }
}

void application_task_create(uint32_t priority)
{
    xTaskCreate(application_task, "application", 512, 0, priority, &application_task_handle);
}