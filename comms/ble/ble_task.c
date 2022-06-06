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

#include <am_bsp.h>
#include <am_mcu_apollo.h>
#include <am_util.h>

#include <FreeRTOS.h>
#include <queue.h>

#include <wsf_types.h>
#include <wsf_buf.h>
#include <wsf_heap.h>
#include <wsf_os.h>
#include <wsf_timer.h>
#include <wsf_trace.h>

#include <hci_drv_apollo.h>
#include <hci_drv_apollo3.h>

#include "hidapp_api.h"

#include "ble_task.h"
#include "ble_task_cli.h"

static TaskHandle_t ble_task_handle;

static wsfBufPoolDesc_t mainPoolDesc[] = {{16, 8}, {32, 4}, {192, 8}, {256, 8}};
static char wsf_trace_buffer[256];

extern void StackInitHidApp(void);

void am_ble_isr(void)
{
    HciDrvIntService();
}

uint8_t ble_task_tracer(const uint8_t *msg, long unsigned int len)
{
    memcpy(wsf_trace_buffer, msg, len);
    wsf_trace_buffer[len] = 0;

    am_util_stdio_printf(wsf_trace_buffer);

    return 1;
}

static void ble_task_setup()
{
    HciDrvRadioBoot(1);

    const uint8_t numPools = sizeof(mainPoolDesc) / sizeof(mainPoolDesc[0]);

    uint16_t memUsed;
    memUsed = WsfBufInit(numPools, mainPoolDesc);
    WsfHeapAlloc(memUsed);
    WsfOsInit();
    WsfTimerInit();

    WsfTraceRegisterHandler(ble_task_tracer);
    WsfTraceEnable(TRUE);

    StackInitHidApp();

    wsfHandlerId_t handlerId;
    handlerId = WsfOsSetNextHandler(HciDrvHandler);
    HciDrvHandlerInit(handlerId);
}

static void ble_task(void *pvParameters)
{
    ble_task_cli_register();
    ble_task_setup();

    HidAppStart();
    while (1)
    {
        wsfOsDispatcher();
    }
}

void ble_task_create(uint32_t ui32Priority)
{
    xTaskCreate(ble_task, "ble", 512, 0, ui32Priority, &ble_task_handle);
}
