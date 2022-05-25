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
#ifndef _EEPROM_EMULATION_H_
#define _EEPROM_EMULATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#define EEPROM_STATUS_OK          0
#define EEPROM_STATUS_ERROR       1

typedef struct {
    uint32_t *pui32StartAddress;
    uint32_t *pui32EndAddress;
} eeprom_page_t;

typedef struct {
    uint8_t allocated;
    int16_t active_page;
    int16_t receiving_page;
    int16_t allocated_pages;
    eeprom_page_t *pages;
} eeprom_handle_t;

uint32_t eeprom_init(uint32_t ui32StartAddress, uint32_t ui32NumberOfPages, eeprom_handle_t *pHandle);
uint32_t eeprom_format(eeprom_handle_t *pHandle);

uint32_t eeprom_read(eeprom_handle_t *pHandle, uint16_t address, uint16_t *data);
uint32_t eeprom_read_array(eeprom_handle_t *pHandle, uint16_t address, uint8_t *data, uint8_t *len);
uint32_t eeprom_read_array_len(eeprom_handle_t *pHandle, uint16_t address, uint8_t *data, uint16_t size);

uint32_t eeprom_write(eeprom_handle_t *pHandle, uint16_t address, uint16_t data);
uint32_t eeprom_write_array(eeprom_handle_t *pHandle, uint16_t address, uint8_t *data, uint8_t len);
uint32_t eeprom_write_array_len(eeprom_handle_t *pHandle, uint16_t address, uint8_t *data, uint16_t size);
uint32_t eeprom_delete(eeprom_handle_t *pHandle, uint16_t virtual_address);
uint32_t eeprom_delete_array(eeprom_handle_t *pHandle, uint16_t virtual_address);

uint32_t eeprom_erase_counter(eeprom_handle_t *pHandle);

#ifdef __cplusplus
}
#endif

#endif /* _EEPROM_EMULATION_H_ */
