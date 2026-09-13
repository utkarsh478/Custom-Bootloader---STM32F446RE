/*
 * crc.c
 *
 *  Created on: Sep 13, 2026
 *      Author: utkarsh
 */


#include "stm32f446xx.h"
#include "crc.h"

void crc32_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
}

void crc32_reset(void)
{
    CRC->CR = CRC_CR_RESET;
}

void crc32_update(const uint8_t *data, uint32_t len)
{
    uint32_t i = 0U;

    while ((i + 4U) <= len) {
        CRC->DR = ((uint32_t)data[i]      << 24)
                | ((uint32_t)data[i + 1U] << 16)
                | ((uint32_t)data[i + 2U] << 8)
                |  (uint32_t)data[i + 3U];
        i += 4U;
    }

    	// left over bytes
    if (i < len) {
        uint32_t word = 0U;
        uint8_t  shift = 24U;

        while (i < len) {
            word |= (uint32_t)data[i] << shift;
            shift -= 8U;
            i++;
        }
        CRC->DR = word;          /* tail zero-padded in the low bytes */
    }
}

uint32_t crc32_value(void)
{
    return CRC->DR;
}

uint32_t crc32_compute(const uint8_t *data, uint32_t len)
{
    crc32_reset();
    crc32_update(data, len);
    return crc32_value();
}
