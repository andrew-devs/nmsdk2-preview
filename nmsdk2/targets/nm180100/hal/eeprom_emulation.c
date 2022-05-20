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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <am_hal_flash.h>

#include "eeprom_emulation.h"

#define EEPROM_UNALLOCATED    0x00
#define EEPROM_ALLOCATED      0x01

#define SIZE_OF_DATA 2                                            /* 2 bytes */
#define SIZE_OF_VIRTUAL_ADDRESS 2                                 /* 2 bytes */
#define SIZE_OF_VARIABLE (SIZE_OF_DATA + SIZE_OF_VIRTUAL_ADDRESS) /* 4 bytes */

#define MAX_ACTIVE_VARIABLES (AM_HAL_FLASH_PAGE_SIZE / SIZE_OF_VARIABLE) - 1

typedef enum {
    EEPROM_PAGE_STATUS_ERASED = 0xFF,
    EEPROM_PAGE_STATUS_RECEIVING = 0xAA,
    EEPROM_PAGE_STATUS_ACTIVE = 0x00,
} eeprom_page_status_e;

/* Since the data to be written to flash must be read from ram, the data used to
 * set the pages' status, is explicitly written to the ram beforehand. */
static uint32_t EEPROM_PAGE_STATUS_ACTIVE_VALUE =
    ((uint32_t)EEPROM_PAGE_STATUS_ACTIVE << 24) | 0x00FFFFFF;
static uint32_t EEPROM_PAGE_STATUS_RECEIVING_VALUE =
    ((uint32_t)EEPROM_PAGE_STATUS_RECEIVING << 24) | 0x00FFFFFF;

static inline eeprom_page_status_e eeprom_page_get_status(eeprom_page_t *page)
{
    return (eeprom_page_status_e)((*(page->pui32StartAddress) >> 24) & 0xFF);
}

static inline int eeprom_page_set_active(eeprom_page_t *page)
{
    return am_hal_flash_program_main(
        AM_HAL_FLASH_PROGRAM_KEY, &EEPROM_PAGE_STATUS_ACTIVE_VALUE,
        page->pui32StartAddress, SIZE_OF_VARIABLE >> 2);
}

static inline int eeprom_page_set_receiving(eeprom_page_t *page)
{
    return am_hal_flash_program_main(
        AM_HAL_FLASH_PROGRAM_KEY, &EEPROM_PAGE_STATUS_RECEIVING_VALUE,
        page->pui32StartAddress, SIZE_OF_VARIABLE >> 2);
}

static bool eeprom_page_validate_empty(eeprom_page_t *page)
{
    uint32_t *address = page->pui32StartAddress;

    while (address <= page->pui32EndAddress) {
        uint32_t value = *address;
        if (value != 0xFFFFFFFF) {
            return false;
        }
        address++;
    }

    return true;
}

static bool eeprom_page_write(eeprom_page_t *page, uint16_t virtual_address,
                              uint16_t data)
{
    /* Start at the second word. The fist one is reserved for status and erase count. */
    uint32_t *address = page->pui32StartAddress + 1;
    uint32_t virtualAddressAndData;

    /* Iterate through the page from the beginning, and stop at the fist empty word. */
    while (address <= page->pui32EndAddress)
    {
        /* Empty word found. */
        if (*address == 0xFFFFFFFF)
        {
            virtualAddressAndData =
                ((uint32_t)(virtual_address << 16) & 0xFFFF0000) |
                (uint32_t)(data);

            if (am_hal_flash_program_main(AM_HAL_FLASH_PROGRAM_KEY,
                                          &virtualAddressAndData, address,
                                          SIZE_OF_VARIABLE >> 2) != 0)
            {
                return EEPROM_STATUS_ERROR;
            }
            return EEPROM_STATUS_OK;
        }
        else
        {
            address++;
        }
    }

    return EEPROM_STATUS_ERROR;
}

static int eeprom_page_transfer(eeprom_handle_t *pHandle, uint16_t virtual_address, uint16_t data)
{
    int status;
    uint32_t *pui32ActiveAddress;
    uint32_t *pui32ReceivingAddress;
    uint32_t ui32EraseCount;
    bool bNewData = false;

    /* If there is no receiving page predefined, set it to cycle through all allocated pages. */
    if (pHandle->receiving_page == -1)
    {
        pHandle->receiving_page = pHandle->active_page + 1;

        if (pHandle->receiving_page >= pHandle->allocated_pages)
        {
            pHandle->receiving_page = 0;
        }

        /* Check if the new receiving page really is erased. */
        if (!eeprom_page_validate_empty(&(pHandle->pages[pHandle->receiving_page])))
        {
            /* If this page is not truly erased, it means that it has been written to
             * from outside this API, this could be an address conflict. */
            am_hal_flash_page_erase(
                AM_HAL_FLASH_PROGRAM_KEY,
                AM_HAL_FLASH_ADDR2INST(
                    (uint32_t)(pHandle->pages[pHandle->receiving_page].pui32StartAddress)),
                AM_HAL_FLASH_ADDR2PAGE(
                    (uint32_t)(pHandle->pages[pHandle->receiving_page].pui32StartAddress)));
        }
    }

    /* Set the status of the receiving page */
    eeprom_page_set_receiving(&(pHandle->pages[pHandle->receiving_page]));

    /* If an address was specified, write it to the receiving page */
    if (virtual_address != 0)
    {
        eeprom_page_write(&(pHandle->pages[pHandle->receiving_page]), virtual_address, data);
    }

    /* Start at the last word. */
    pui32ActiveAddress = pHandle->pages[pHandle->receiving_page].pui32EndAddress;

    /* Iterate through all words in the active page. Each time a new virtual
     * address is found, write it and it's data to the receiving page */
    while (pui32ActiveAddress > pHandle->pages[pHandle->active_page].pui32StartAddress)
    {
        // 0x0000 and 0xFFFF are not valid virtual addresses.
        if ((uint16_t)(*pui32ActiveAddress >> 16) == 0x0000 ||
            (uint16_t)(*pui32ActiveAddress >> 16) == 0xFFFF)
        {
            bNewData = false;
        }
        /*
        // Omit when transfer is initiated from inside the eeprom_init() function.
        else if (address != 0 && (uint16_t)(*pui32ActiveAddress >> 16) > numberOfVariablesDeclared)
        {
        // A virtual address outside the virtual address space, defined by the
        // number of variables declared, are considered garbage.
        newVariable = false;
        }
        */
        else
        {
            pui32ReceivingAddress =
                pHandle->pages[pHandle->receiving_page].pui32StartAddress + 1;

            /* Start at the beginning of the receiving page. Check if the variable is
             * already transfered. */
            while (pui32ReceivingAddress <=
                   pHandle->pages[pHandle->receiving_page].pui32EndAddress)
            {
                /* Variable found, and is therefore already transferred. */
                if ((uint16_t)(*pui32ActiveAddress >> 16) ==
                    (uint16_t)(*pui32ReceivingAddress >> 16))
                {
                    bNewData = false;
                    break;
                }
                /* Empty word found. All transferred variables are checked.  */
                else if (*pui32ReceivingAddress == 0xFFFFFFFF)
                {
                    bNewData = true;
                    break;
                }
                pui32ReceivingAddress++;
            }
        }

        if (bNewData)
        {
            /* Write the new variable to the receiving page. */
            eeprom_page_write(&(pHandle->pages[pHandle->receiving_page]),
                              (uint16_t)(*pui32ActiveAddress >> 16),
                              (uint16_t)(*pui32ActiveAddress));
        }
        pui32ActiveAddress--;
    }

    /* Update erase count */
    ui32EraseCount = eeprom_erase_counter(pHandle);

    /* If a new page cycle is started, increment the erase count. */
    if (pHandle->receiving_page == 0)
        ui32EraseCount++;

    /* Set the first byte, in this way the page status is not altered when the erase count is written. */
    ui32EraseCount = ui32EraseCount | 0xFF000000;

    /* Write the erase count obtained to the active page head. */
    status = am_hal_flash_program_main(
        AM_HAL_FLASH_PROGRAM_KEY, &ui32EraseCount,
        pHandle->pages[pHandle->receiving_page].pui32StartAddress, SIZE_OF_VARIABLE >> 2);
    if (status != 0)
    {
        return status;
    }

    /* Erase the old active page. */
    status = am_hal_flash_page_erase(
        AM_HAL_FLASH_PROGRAM_KEY,
        AM_HAL_FLASH_ADDR2INST(
            (uint32_t)(pHandle->pages[pHandle->receiving_page].pui32StartAddress)),
        AM_HAL_FLASH_ADDR2PAGE(
            (uint32_t)(pHandle->pages[pHandle->receiving_page].pui32StartAddress)));
    if (status != 0)
    {
        return status;
    }

    /* Set the receiving page to be the new active page. */
    status = eeprom_page_set_active(&(pHandle->pages[pHandle->receiving_page]));
    if (status != 0)
    {
        return status;
    }

    pHandle->active_page = pHandle->receiving_page;
    pHandle->receiving_page = -1;

    return 0;
}

uint32_t eeprom_init(uint32_t ui32StartAddress, uint32_t ui32NumberOfPages, eeprom_handle_t *pHandle)
{
    if (pHandle == NULL)
    {
        return EEPROM_STATUS_ERROR;
    }

    if (ui32NumberOfPages < 2)
    {
        return EEPROM_STATUS_ERROR;
    }

    if (pHandle->pages == NULL)
    {
        return EEPROM_STATUS_ERROR;
    }

    pHandle->allocated = EEPROM_ALLOCATED;
    pHandle->active_page = -1;
    pHandle->receiving_page = -1;
    pHandle->allocated_pages = ui32NumberOfPages;

    /* Initialize the address of each page */
    uint32_t i;
    for (i = 0; i < ui32NumberOfPages; i++)
    {
        uint32_t ui32PageStart = ui32StartAddress + i * AM_HAL_FLASH_PAGE_SIZE;
        uint32_t ui32PageEnd = ui32PageStart + (AM_HAL_FLASH_PAGE_SIZE - 1);
        pHandle->pages[i].pui32StartAddress = (uint32_t *)(ui32PageStart);
        pHandle->pages[i].pui32EndAddress = (uint32_t *)(ui32PageEnd);
    }

    /* Check status of each page */
    for (i = 0; i < ui32NumberOfPages; i++)
    {
        switch (eeprom_page_get_status(&(pHandle->pages[i])))
        {
        case EEPROM_PAGE_STATUS_ACTIVE:
            if (pHandle->active_page == -1) {
                pHandle->active_page = i;
            } else {
                // More than one active page found. This is an invalid system state.
                return EEPROM_STATUS_ERROR;
            }
            break;
        case EEPROM_PAGE_STATUS_RECEIVING:
            if (pHandle->receiving_page == -1) {
                pHandle->receiving_page = i;
            } else {
                // More than one receiving page found. This is an invalid system state.
                return EEPROM_STATUS_ERROR;
            }
            break;
        case EEPROM_PAGE_STATUS_ERASED:
            // Validate if the page is really erased, and erase it if not.
            if (!eeprom_page_validate_empty(&(pHandle->pages[i]))) {
                am_hal_flash_page_erase(AM_HAL_FLASH_PROGRAM_KEY,
                                        AM_HAL_FLASH_ADDR2INST((uint32_t)(
                                            pHandle->pages[i].pui32StartAddress)),
                                        AM_HAL_FLASH_ADDR2PAGE((uint32_t)(
                                            pHandle->pages[i].pui32StartAddress)));
            }
            break;
        default:
            // Undefined page status, erase page.
            am_hal_flash_page_erase(
                AM_HAL_FLASH_PROGRAM_KEY,
                AM_HAL_FLASH_ADDR2INST((uint32_t)(pHandle->pages[i].pui32StartAddress)),
                AM_HAL_FLASH_ADDR2PAGE((uint32_t)(pHandle->pages[i].pui32StartAddress)));
            break;
        }
    }

    if ((pHandle->receiving_page == -1) && (pHandle->active_page == -1)) {
        return EEPROM_STATUS_ERROR;
    }

    if (pHandle->receiving_page == -1) {
        return EEPROM_STATUS_OK;
    } else if (pHandle->active_page == -1) {
        pHandle->active_page = pHandle->receiving_page;
        pHandle->receiving_page = -1;
        eeprom_page_set_active(&(pHandle->pages[pHandle->active_page]));
    } else {
        eeprom_page_transfer(pHandle, 0, 0);
    }

    return EEPROM_STATUS_OK;
}

uint32_t eeprom_format(eeprom_handle_t *pHandle)
{
    uint32_t ui32EraseCount = 0xFF000001;
    int i;
    int status;

    for (i = pHandle->allocated_pages - 1; i >= 0; i--)
    {
        if (!eeprom_page_validate_empty(&(pHandle->pages[i])))
        {
            status = am_hal_flash_page_erase(
                AM_HAL_FLASH_PROGRAM_KEY,
                AM_HAL_FLASH_ADDR2INST((uint32_t)(pHandle->pages[i].pui32StartAddress)),
                AM_HAL_FLASH_ADDR2PAGE((uint32_t)(pHandle->pages[i].pui32StartAddress)));
            if (status != 0)
            {
                return EEPROM_STATUS_ERROR;
            }
        }
    }

    pHandle->active_page = 0;
    pHandle->receiving_page = -1;

    status = am_hal_flash_program_main(
        AM_HAL_FLASH_PROGRAM_KEY, &ui32EraseCount,
        pHandle->pages[pHandle->active_page].pui32StartAddress, 1);

    if (status != 0)
    {
        return EEPROM_STATUS_ERROR;
    }

    status = eeprom_page_set_active(&(pHandle->pages[pHandle->active_page]));
    if (status != 0)
    {
        return EEPROM_STATUS_ERROR;
    }

    return EEPROM_STATUS_OK;
}

uint32_t eeprom_read(eeprom_handle_t *pHandle, uint16_t virtual_address, uint16_t *data)
{
    uint32_t *pui32Address;

    if (pHandle->allocated == -1) {
        return EEPROM_STATUS_ERROR;
    }

    pui32Address = (pHandle->pages[pHandle->active_page].pui32StartAddress);

    // 0x0000 and 0xFFFF are illegal addresses.
    if (virtual_address != 0x0000 && virtual_address != 0xFFFF) {
        while (pui32Address < pHandle->pages[pHandle->active_page].pui32EndAddress) {
            if ((uint16_t)(*pui32Address >> 16) == virtual_address) {
                *data = (uint16_t)(*pui32Address);
                return EEPROM_STATUS_OK;
            }
            pui32Address++;
        }
    }
    // Variable not found, return null value.
    *data = 0x0000;

    return EEPROM_STATUS_ERROR;
}

uint32_t eeprom_read_array(eeprom_handle_t *pHandle, uint16_t virtual_address, uint8_t *data, uint8_t *len)
{
    if (pHandle->allocated == -1) {
        return EEPROM_STATUS_ERROR;
    }

    uint16_t value;
    if (eeprom_read(pHandle, virtual_address, &value) != EEPROM_STATUS_OK)
    {
        return EEPROM_STATUS_ERROR;
    }

    *len = (value >> 8) & 0xFF;
    data[0] = value & 0xFF;
    for (int i = 1; i < *len; i++)
    {
        if (eeprom_read(pHandle, virtual_address + i, &value) != EEPROM_STATUS_OK)
        {
            *len = i;
            return EEPROM_STATUS_ERROR;
        }
        data[i] = value & 0xFF;
    }

    return EEPROM_STATUS_OK;
}

uint32_t eeprom_read_array_len(eeprom_handle_t *pHandle, uint16_t virtual_address, uint8_t *data, uint16_t size)
{
    uint16_t value;

    if (pHandle->allocated == -1) {
        return EEPROM_STATUS_ERROR;
    }

    for (int i = 0; i < size; i++)
    {
        if (eeprom_read(pHandle, virtual_address + i, &value) != EEPROM_STATUS_OK)
        {
            return EEPROM_STATUS_ERROR;
        }
        data[i] = value & 0xFF;
    }

    return EEPROM_STATUS_OK;
}

uint32_t eeprom_write(eeprom_handle_t *pHandle, uint16_t virtual_address, uint16_t data)
{
    if (pHandle->allocated == -1) {
        return EEPROM_STATUS_ERROR;
    }

    uint16_t stored_value;

    if (eeprom_read(pHandle, virtual_address, &stored_value) == EEPROM_STATUS_OK) {
        if (stored_value == data) {
            return EEPROM_STATUS_OK;
        }
    }

    if (eeprom_page_write(&(pHandle->pages[pHandle->active_page]), virtual_address, data) != EEPROM_STATUS_OK) {
        eeprom_page_transfer(pHandle, virtual_address, data);
    }

    return EEPROM_STATUS_OK;
}

uint32_t eeprom_write_array(eeprom_handle_t *pHandle, uint16_t virtual_address, uint8_t *data, uint8_t len)
{
    if (pHandle->allocated == -1) {
        return EEPROM_STATUS_ERROR;
    }

    uint16_t stored_value;

    if (eeprom_read(pHandle, virtual_address, &stored_value) == EEPROM_STATUS_OK) {
        uint8_t stored_len = (stored_value >> 8) & 0xFF;
        if (stored_len != len) {
            for (int i = 0; i < stored_len; i++)
            {
                eeprom_delete(pHandle, virtual_address + i);
            }
        }
    }

    uint16_t value = (len << 8) | data[0];
    if (eeprom_page_write(
            &(pHandle->pages[pHandle->active_page]),
            virtual_address, value) != EEPROM_STATUS_OK) {
        eeprom_page_transfer(pHandle, virtual_address, value);
    }

    for (int i = 1; i < len; i++)
    {
        if (!eeprom_page_write(
                &(pHandle->pages[pHandle->active_page]), virtual_address + i, data[i])) {
            eeprom_page_transfer(pHandle, virtual_address + i, data[i]);
        }
    }
    
    return EEPROM_STATUS_OK;
}

uint32_t eeprom_write_array_len(eeprom_handle_t *pHandle, uint16_t virtual_address, uint8_t *data, uint16_t size)
{
    if (pHandle->allocated == -1) {
        return EEPROM_STATUS_ERROR;
    }

    uint32_t status = 0;
    for (int i = 0; i < size; i++)
    {
        status |= eeprom_write(pHandle, virtual_address + i, data[i]);

        if (status != EEPROM_STATUS_OK)
        {
            return status;
        }
    }

    return status;
}

uint32_t eeprom_delete(eeprom_handle_t *pHandle, uint16_t virtual_address)
{
    if (pHandle->allocated == -1) {
        return EEPROM_STATUS_ERROR;
    }

    bool bDeleted = false;

    uint32_t data = 0x0000FFFF;
    uint32_t *address = pHandle->pages[pHandle->active_page].pui32EndAddress;

    while (address > pHandle->pages[pHandle->active_page].pui32StartAddress)
    {
        if ((uint16_t)(*address >> 16) == virtual_address)
        {
            bDeleted = true;
            am_hal_flash_program_main(
                  AM_HAL_FLASH_PROGRAM_KEY,
                  &data, address,
                  SIZE_OF_VARIABLE >> 2);
        }
        address--;
    }

    if (bDeleted)
        return EEPROM_STATUS_OK;

    return EEPROM_STATUS_ERROR;
}

uint32_t eeprom_delete_array(eeprom_handle_t *pHandle, uint16_t virtual_address)
{
    if (pHandle->allocated == -1) {
        return EEPROM_STATUS_ERROR;
    }

    uint32_t status = 0;
    uint16_t stored_value;

    if (eeprom_read(pHandle, virtual_address, &stored_value) == EEPROM_STATUS_OK)
    {
        uint8_t stored_len = (stored_value >> 8) & 0xFF;
        for (int i = 0; i < stored_len; i++)
        {
            status |= eeprom_delete(pHandle, virtual_address + i);
            if (status != EEPROM_STATUS_OK)
            {
                return status;
            }
        }
    }

    return status;
}

uint32_t eeprom_erase_counter(eeprom_handle_t *pHandle)
{
    if (pHandle->active_page == -1)
    {
        return 0xFFFFFF;
    }

    uint32_t eraseCount;

    /* The number of erase cycles is the 24 LSB of the first word of the active page. */
    eraseCount = (*(pHandle->pages[pHandle->active_page].pui32StartAddress) & 0x00FFFFFF);

    /* if the page has never been erased, return 0. */
    if (eraseCount == 0xFFFFFF) {
        return 0;
    }

    return eraseCount;
}
