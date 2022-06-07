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
#ifndef _LORAWAN_H_
#define _LORAWAN_H_

#include <stdint.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <LmHandler.h>

typedef enum
{
    LORAWAN_JOIN = 0,
    LORAWAN_RESET,
    LORAWAN_SYNC_APP,
    LORAWAN_SYNC_MAC,
    LORAWAN_CLASS_SET,
} lorawan_command_e;

typedef struct
{
    lorawan_command_e eCommand;
    void *pvParameters;
} lorawan_command_t;

typedef struct
{
    uint32_t ui32DownlinkCounter;
    int16_t  i16DataRate;
    int16_t  i16RSSI;
    int16_t  i16SNR;
    int16_t  i16ReceiveSlot;
    uint32_t ui32Port;
    int32_t  ui32Length;
    uint8_t *pui8Payload;
} lorawan_rx_packet_t;

typedef struct 
{
    LmHandlerMsgTypes_t tType;
    uint32_t    ui32Port;
    uint32_t    ui32Length;
    uint8_t    *pui8Data;
} lorawan_tx_packet_t;

extern void lorawan_send_command(lorawan_command_t *pCommand);

extern void lorawan_set_device_eui_by_str(const char *pcDeviceEUI);
extern void lorawan_set_device_eui_by_bytes(const uint8_t *pui8DeviceEUI);
extern void lorawan_get_device_eui(uint8_t *pui8DeviceEUI);

extern void lorawan_set_app_eui_by_str(const char *pcAppEUI);
extern void lorawan_set_app_eui_by_bytes(const uint8_t *pui8AppEUI);
extern void lorawan_get_app_eui(uint8_t *pui8AppEUI);

extern void lorawan_set_app_key_by_str(const char *pcAppKey);
extern void lorawan_set_app_key_by_bytes(const uint8_t *pui8AppKey);
extern void lorawan_get_app_key(uint8_t *pui8AppKey);

extern void lorawan_set_nwk_key_by_str(const char *pcNwkKey);
extern void lorawan_set_nwk_key_by_bytes(const uint8_t *pui8NwkKey);
extern void lorawan_get_nwk_key(uint8_t *pui8NwkKey);

extern void lorawan_transmit(uint32_t ui32Port, uint32_t ui32Ack, uint32_t ui32Length, uint8_t *pui8Data);
extern QueueHandle_t lorawan_receive_register(uint32_t ui32Port, uint32_t elements);
extern void lorawan_receive_unregister(QueueHandle_t handle);

#endif