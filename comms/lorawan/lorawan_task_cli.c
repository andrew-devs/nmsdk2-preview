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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <am_bsp.h>
#include <am_mcu_apollo.h>
#include <am_util.h>

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <queue.h>
#include <timers.h>

#include <LmHandler.h>
#include <LoRaMacTypes.h>

#include <eeprom_emulation.h>
#include <lorawan_eeprom_config.h>

#include "lorawan_config.h"

#include "console_task.h"
#include "lorawan.h"
#include "lorawan_task.h"
#include "lorawan_task_cli.h"

static portBASE_TYPE
lorawan_task_cli_entry(char *pui8OutBuffer, size_t ui32OutBufferLength, const char *pui8Command);

CLI_Command_Definition_t lorawan_task_cli_definition = {
    (const char *const) "lorawan",
    (const char *const) "lorawan:  LoRaWAN Application Layer Commands.\r\n",
    lorawan_task_cli_entry,
    -1};

static size_t argc;
static char *argv[8];
static char argz[128];

#define LM_BUFFER_SIZE 242
static uint8_t lorawan_cli_transmit_buffer[LM_BUFFER_SIZE];

static TimerHandle_t periodic_transmit_timer = NULL;

static void print_hex_array(char *pui8OutBuffer, uint8_t *array, uint32_t length)
{
    char byte[4];
    for (int i = 0; i < length; i++)
    {
        am_util_stdio_sprintf(byte, "%02X ", array[i]);
        strcat(pui8OutBuffer, byte);
    }
}

static void periodic_transmit_callback(TimerHandle_t handle)
{
    uint32_t ui32Count = (uint32_t)pvTimerGetTimerID(handle);
    ui32Count++;
    vTimerSetTimerID(handle, (void *)ui32Count);

    am_util_stdio_sprintf((char *)lorawan_cli_transmit_buffer, "%d", ui32Count);
    uint32_t length = strlen((char *)lorawan_cli_transmit_buffer);

    lorawan_transmit(
        LORAWAN_DEFAULT_PORT, LORAMAC_HANDLER_UNCONFIRMED_MSG, length, lorawan_cli_transmit_buffer);
}

void lorawan_task_cli_register()
{
    FreeRTOS_CLIRegisterCommand(&lorawan_task_cli_definition);
    argc = 0;
}

static void convert_hex_string(const char *in, size_t inlen, uint8_t *out, size_t *outlen)
{
    size_t n = 0;
    char cNum[3];
    *outlen = 0;
    while (n < inlen)
    {
        switch (in[n])
        {
        case '\\':
            n++;
            switch (in[n])
            {
            case 'x':
                n++;
                memset(cNum, 0, 3);
                memcpy(cNum, &in[n], 2);
                n++;
                out[*outlen] = strtol(cNum, NULL, 16);
                break;
            }
            break;
        default:
            out[*outlen] = in[n];
            break;
        }
        *outlen = *outlen + 1;
        n++;
    }
}

static void lorawan_task_cli_help(char *pui8OutBuffer, size_t argc, char **argv)
{
    strcat(pui8OutBuffer, "\r\nusage: lorawan <command>\r\n");
    strcat(pui8OutBuffer, "\r\n");
    strcat(pui8OutBuffer, "supported commands are:\r\n");
    strcat(pui8OutBuffer, "  start    start the LoRaWAN stack\r\n");
    strcat(pui8OutBuffer, "  stop     stop the LoRaWAN stack\r\n");
    strcat(pui8OutBuffer, "  class    get/set class\r\n");
    strcat(pui8OutBuffer, "  clear    reformat eeprom\r\n");
    strcat(pui8OutBuffer, "  datetime get/set/sync time\r\n");
    strcat(pui8OutBuffer, "  join\r\n");
    strcat(pui8OutBuffer, "  keys\r\n");
    strcat(pui8OutBuffer, "  periodic\r\n");
    strcat(pui8OutBuffer, "  send\r\n");
}

static void lorawan_task_cli_class(char *pui8OutBuffer, size_t argc, char **argv)
{
    DeviceClass_t cls;

    if (argc == 3)
    {
        if (strcmp(argv[2], "get") == 0)
        {
            am_util_stdio_sprintf(pui8OutBuffer, "\n\rCurrent Class: ");

            cls = LmHandlerGetCurrentClass();
            switch (cls)
            {
            case CLASS_A:
                strcat(pui8OutBuffer, "A");
                break;
            case CLASS_B:
                strcat(pui8OutBuffer, "B");
                break;
            case CLASS_C:
                strcat(pui8OutBuffer, "C");
                break;
            }
            strcat(pui8OutBuffer, "\n\r");
        }
    }
    else if (argc == 4)
    {
        if (strcmp(argv[2], "set") == 0)
        {
            switch (argv[3][0])
            {
            case 'a':
            case 'A':
                cls = CLASS_A;
                break;
            case 'b':
            case 'B':
                cls = CLASS_B;
                break;
            case 'c':
            case 'C':
                cls = CLASS_C;
                break;
            default:
                am_util_stdio_sprintf(pui8OutBuffer, "\n\rUnknown class requested.\n\r");
                return;
            }

            lorawan_command_t command;
            command.eCommand = LORAWAN_CLASS_SET;
            command.pvParameters = (void *)cls;
            lorawan_send_command(&command);
        }
    }
}

static void lorawan_task_cli_datetime(char *pui8OutBuffer, size_t argc, char **argv)
{
    if (argc == 2)
    {
        SysTime_t timestamp = SysTimeGet();
        struct tm localtime;

        SysTimeLocalTime(timestamp.Seconds, &localtime);

        am_util_stdio_sprintf(
            pui8OutBuffer,
            "\n\rUnix timestamp: %d\n\rStack Time: %02d/%02d/%04d %02d:%02d:%02d (UTC0)\n\r",
            timestamp.Seconds,
            localtime.tm_mon + 1,
            localtime.tm_mday,
            localtime.tm_year + 1900,
            localtime.tm_hour,
            localtime.tm_min,
            localtime.tm_sec);

        return;
    }

    lorawan_command_t command;
    if (argc >= 3)
    {
        if (strcmp(argv[2], "sync") == 0)
        {
            if (argc == 3)
            {
                command.eCommand = LORAWAN_SYNC_MAC;
            }
            else
            {
                command.eCommand = LORAWAN_SYNC_APP;
            }
            lorawan_send_command(&command);
        }
    }
}

static void lorawan_task_cli_keys(char *pui8OutBuffer, size_t argc, char **argv)
{
    uint8_t dev_eui[SE_EUI_SIZE];
    uint8_t app_eui[SE_EUI_SIZE];
    uint8_t app_key[SE_KEY_SIZE];
    uint8_t nwk_key[SE_KEY_SIZE];

    lorawan_get_device_eui(dev_eui);
    lorawan_get_app_eui(app_eui);
    lorawan_get_app_key(app_key);
    lorawan_get_nwk_key(nwk_key);

    strcat(pui8OutBuffer, "\n\r");

    strcat(pui8OutBuffer, "Device EUI  : ");
    print_hex_array(pui8OutBuffer, dev_eui, SE_EUI_SIZE);
    strcat(pui8OutBuffer, "\n\r");

    strcat(pui8OutBuffer, "App EUI     : ");
    print_hex_array(pui8OutBuffer, app_eui, SE_EUI_SIZE);
    strcat(pui8OutBuffer, "\n\r");

    strcat(pui8OutBuffer, "App Key     : ");
    print_hex_array(pui8OutBuffer, app_key, SE_KEY_SIZE);
    strcat(pui8OutBuffer, "\n\r");

    strcat(pui8OutBuffer, "Network Key : ");
    print_hex_array(pui8OutBuffer, nwk_key, SE_KEY_SIZE);
    strcat(pui8OutBuffer, "\n\r");

    strcat(pui8OutBuffer, "\n\r");
}

static void lorawan_task_cli_periodic(char *pui8OutBuffer, size_t argc, char **argv)
{
    uint32_t ui32Period;
    if (argc < 3)
    {
        return;
    }

    if (strcmp(argv[2], "stop") == 0)
    {
        if (periodic_transmit_timer)
        {
            xTimerStop(periodic_transmit_timer, portMAX_DELAY);
            xTimerDelete(periodic_transmit_timer, portMAX_DELAY);
            periodic_transmit_timer = NULL;
        }
    }
    else if (strcmp(argv[2], "start") == 0)
    {
        if (argc == 3)
        {
            ui32Period = 30;
        }
        else
        {
            ui32Period = atoi(argv[3]);
        }

        if (periodic_transmit_timer == NULL)
        {
            periodic_transmit_timer = xTimerCreate("lorawan periodic",
                                                   pdMS_TO_TICKS(ui32Period * 1000),
                                                   pdTRUE,
                                                   (void *)0,
                                                   periodic_transmit_callback);
            xTimerStart(periodic_transmit_timer, portMAX_DELAY);
        }
        else
        {
            xTimerChangePeriod(periodic_transmit_timer, pdMS_TO_TICKS(ui32Period), portMAX_DELAY);
        }
    }
}

static void lorawan_task_cli_send(char *pui8OutBuffer, size_t argc, char **argv)
{
    uint32_t port = LORAWAN_DEFAULT_PORT;
    uint32_t ack = LORAMAC_HANDLER_UNCONFIRMED_MSG;

    size_t length;
    convert_hex_string(
        argv[argc - 1], strlen(argv[argc - 1]), lorawan_cli_transmit_buffer, &length);
    lorawan_cli_transmit_buffer[length] = 0;

    if (argc == 5)
    {
        port = atoi(argv[2]);
        ack = atoi(argv[3]) ? LORAMAC_HANDLER_CONFIRMED_MSG : LORAMAC_HANDLER_UNCONFIRMED_MSG;
    }
    else if (argc == 4)
    {
        port = atoi(argv[2]);
    }

    lorawan_transmit(port, ack, length, lorawan_cli_transmit_buffer);
}

static portBASE_TYPE
lorawan_task_cli_entry(char *pui8OutBuffer, size_t ui32OutBufferLength, const char *pui8Command)
{
    pui8OutBuffer[0] = 0;

    strcpy(argz, pui8Command);
    FreeRTOS_CLIExtractParameters(argz, &argc, argv);

    if (strcmp(argv[1], "help") == 0)
    {
        lorawan_task_cli_help(pui8OutBuffer, argc, argv);
    }
    else if (strcmp(argv[1], "start") == 0)
    {
        lorawan_command_t command;
        command.eCommand = LORAWAN_START;
        lorawan_send_command(&command);
    }
    else if (strcmp(argv[1], "stop") == 0)
    {
        lorawan_command_t command;
        command.eCommand = LORAWAN_STOP;
        lorawan_send_command(&command);
    }
    else if (strcmp(argv[1], "class") == 0)
    {
        lorawan_task_cli_class(pui8OutBuffer, argc, argv);
    }
    else if (strcmp(argv[1], "clear") == 0)
    {
        eeprom_format(&lorawan_eeprom_handle);
    }
    else if (strcmp(argv[1], "datetime") == 0)
    {
        lorawan_task_cli_datetime(pui8OutBuffer, argc, argv);
    }
    else if (strcmp(argv[1], "join") == 0)
    {
        lorawan_command_t command;
        command.eCommand = LORAWAN_JOIN;
        lorawan_send_command(&command);
    }
    else if (strcmp(argv[1], "keys") == 0)
    {
        lorawan_task_cli_keys(pui8OutBuffer, argc, argv);
    }
    else if (strcmp(argv[1], "periodic") == 0)
    {
        lorawan_task_cli_periodic(pui8OutBuffer, argc, argv);
    }
    else if (strcmp(argv[1], "send") == 0)
    {
        lorawan_task_cli_send(pui8OutBuffer, argc, argv);
    }

    return pdFALSE;
}
