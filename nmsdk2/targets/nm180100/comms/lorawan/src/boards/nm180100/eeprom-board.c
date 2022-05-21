/*!
 * \file      eeprom-board.c
 *
 * \brief     Target board EEPROM driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include <am_mcu_apollo.h>
#include "utilities.h"
#include "eeprom-board.h"
#include "eeprom_emulation.h"
#include "lorawan_eeprom_config.h"

eeprom_handle_t lorawan_eeprom_handle;

LmnStatus_t EepromMcuWriteBuffer( uint16_t addr, uint8_t *buffer, uint16_t size )
{
    if (eeprom_write_array_len(&lorawan_eeprom_handle, addr + 1, buffer, size) != EEPROM_STATUS_OK)
    {
        return LMN_STATUS_ERROR;
    }

    return LMN_STATUS_OK;
}

LmnStatus_t EepromMcuReadBuffer( uint16_t addr, uint8_t *buffer, uint16_t size )
{
    if (eeprom_read_array_len(&lorawan_eeprom_handle, addr + 1, buffer, size) != EEPROM_STATUS_OK)
    {
        return LMN_STATUS_ERROR;
    }

    return LMN_STATUS_OK;
}

void EepromMcuSetDeviceAddr( uint8_t addr )
{
}

LmnStatus_t EepromMcuGetDeviceAddr( void )
{
    return LMN_STATUS_ERROR;
}
