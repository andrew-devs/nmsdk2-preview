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

#include "lorawan.h"

SecureElementNvmData_t lorawan_se = {.DevEui = {0},
                                     .JoinEui = {0},
                                     .Pin = {0},
                                     .KeyList = {
                                         {.KeyID = APP_KEY, .KeyValue = {0}},
                                         {.KeyID = NWK_KEY, .KeyValue = {0}},
                                         {.KeyID = J_S_INT_KEY, .KeyValue = {0}},
                                         {.KeyID = J_S_ENC_KEY, .KeyValue = {0}},
                                         {.KeyID = F_NWK_S_INT_KEY, .KeyValue = {0}},
                                         {.KeyID = S_NWK_S_INT_KEY, .KeyValue = {0}},
                                         {.KeyID = NWK_S_ENC_KEY, .KeyValue = {0}},
                                         {.KeyID = APP_S_KEY, .KeyValue = {0}},
                                         {.KeyID = MC_ROOT_KEY, .KeyValue = {0}},
                                         {.KeyID = MC_KE_KEY, .KeyValue = {0}},
                                         {.KeyID = MC_KEY_0, .KeyValue = {0}},
                                         {.KeyID = MC_APP_S_KEY_0, .KeyValue = {0}},
                                         {.KeyID = MC_NWK_S_KEY_0, .KeyValue = {0}},
                                         {.KeyID = MC_KEY_1, .KeyValue = {0}},
                                         {.KeyID = MC_APP_S_KEY_1, .KeyValue = {0}},
                                         {.KeyID = MC_NWK_S_KEY_1, .KeyValue = {0}},
                                         {.KeyID = MC_KEY_2, .KeyValue = {0}},
                                         {.KeyID = MC_APP_S_KEY_2, .KeyValue = {0}},
                                         {.KeyID = MC_NWK_S_KEY_2, .KeyValue = {0}},
                                         {.KeyID = MC_KEY_3, .KeyValue = {0}},
                                         {.KeyID = MC_APP_S_KEY_3, .KeyValue = {0}},
                                         {.KeyID = MC_NWK_S_KEY_3, .KeyValue = {0}},
                                         {.KeyID = SLOT_RAND_ZERO_KEY, .KeyValue = {0}},
                                     }};

uint8_t hex_char(const char ch)
{
    if ('0' <= ch && ch <= '9')
        return (uint8_t)(ch - '0');

    if ('a' <= ch && ch <= 'f')
        return (uint8_t)(ch - 'a' + 10);

    if ('A' <= ch && ch <= 'F')
        return (uint8_t)(ch - 'A' + 10);

    return 255;
}

void hex_to_bin(const char *str, uint8_t *array)
{
    while (*str)
    {
        uint8_t n1 = hex_char(*str++);
        uint8_t n2 = hex_char(*str++);

        if ((n1 == 255) || (n2 == 255))
        {
            return;
        }

        *array++ = (n1 << 4) + n2;
    }
}

void lorawan_set_device_eui_by_str(const char *pcDeviceEUI)
{
    hex_to_bin(pcDeviceEUI, lorawan_se.DevEui);
}

void lorawan_set_device_eui_by_bytes(const uint8_t *pui8DeviceEUI)
{
    memcpy(lorawan_se.DevEui, pui8DeviceEUI, SE_EUI_SIZE);
}

void lorawan_get_device_eui(uint8_t *pui8DeviceEUI)
{
    memcpy(pui8DeviceEUI, lorawan_se.DevEui, SE_EUI_SIZE);
}

void lorawan_set_app_eui_by_str(const char *pcAppEUI)
{
    hex_to_bin(pcAppEUI, lorawan_se.JoinEui);
}

void lorawan_set_app_eui_by_bytes(const uint8_t *pui8AppEUI)
{
    memcpy(lorawan_se.JoinEui, pui8AppEUI, SE_EUI_SIZE);
}

void lorawan_get_app_eui(uint8_t *pui8AppEUI)
{
    memcpy(pui8AppEUI, lorawan_se.JoinEui, SE_EUI_SIZE);
}

void lorawan_set_app_key_by_str(const char *pcAppKey)
{
    for (int i = 0; i < NUM_OF_KEYS; i++)
    {
        if (lorawan_se.KeyList[i].KeyID == APP_KEY)
        {
            hex_to_bin(pcAppKey, lorawan_se.KeyList[i].KeyValue);
            return;
        }
    }
}

void lorawan_set_app_key_by_bytes(const uint8_t *pui8AppKey)
{
    for (int i = 0; i < NUM_OF_KEYS; i++)
    {
        if (lorawan_se.KeyList[i].KeyID == APP_KEY)
        {
            memcpy(lorawan_se.KeyList[i].KeyValue, pui8AppKey, SE_KEY_SIZE);
            return;
        }
    }
}

void lorawan_get_app_key(uint8_t *pui8AppKey)
{
    for (int i = 0; i < NUM_OF_KEYS; i++)
    {
        if (lorawan_se.KeyList[i].KeyID == APP_KEY)
        {
            memcpy(pui8AppKey, lorawan_se.KeyList[i].KeyValue, SE_KEY_SIZE);
            return;
        }
    }
}

void lorawan_set_nwk_key_by_str(const char *pcNwkKey)
{
    for (int i = 0; i < NUM_OF_KEYS; i++)
    {
        if (lorawan_se.KeyList[i].KeyID == NWK_KEY)
        {
            hex_to_bin(pcNwkKey, lorawan_se.KeyList[i].KeyValue);
            return;
        }
    }
}

void lorawan_set_nwk_key_by_bytes(const uint8_t *pui8NwkKey)
{
    for (int i = 0; i < NUM_OF_KEYS; i++)
    {
        if (lorawan_se.KeyList[i].KeyID == NWK_KEY)
        {
            memcpy(lorawan_se.KeyList[i].KeyValue, pui8NwkKey, SE_KEY_SIZE);
            return;
        }
    }
}

void lorawan_get_nwk_key(uint8_t *pui8NwkKey)
{
    for (int i = 0; i < NUM_OF_KEYS; i++)
    {
        if (lorawan_se.KeyList[i].KeyID == NWK_KEY)
        {
            memcpy(pui8NwkKey, lorawan_se.KeyList[i].KeyValue, SE_KEY_SIZE);
            return;
        }
    }
}