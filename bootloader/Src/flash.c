/*
 * flash.c
 *
 *  Created on: Sep 11, 2026
 *      Author: utkarsh
 */

#include "stm32f446xx.h"
#include "flash.h"

#define FLASH_ALL_ERRORS  (FLASH_SR_WRPERR | FLASH_SR_PGAERR |FLASH_SR_PGPERR |FLASH_SR_PGSERR | FLASH_SR_OPERR)

static uint8_t flash_wait_busy(void){
	uint32_t timeout = 0x00FFFFFFUL;

	while ((FLASH->SR & FLASH_SR_BSY) != 0U){
		if(timeout-- == 0){
			return FLASH_ERR_BUSY;
		}
	}
}

static void flash_clear_errors(void){
	FLASH->SR = FLASH_ALL_ERRORS | FLASH_SR_EOP;
}

static uint8_t flash_check_errors(void){
    if ((FLASH->SR & FLASH_ALL_ERRORS) != 0U) {
        flash_clear_errors();
        return FLASH_ERR_WRITE;
    }
    return FLASH_OK;
}



static void flash_flush_caches(void){
    uint32_t acr = FLASH->ACR;

    FLASH->ACR &= ~(FLASH_ACR_ICEN | FLASH_ACR_DCEN);

    FLASH->ACR |=  FLASH_ACR_ICRST;
    FLASH->ACR &= ~FLASH_ACR_ICRST;
    FLASH->ACR |=  FLASH_ACR_DCRST;
    FLASH->ACR &= ~FLASH_ACR_DCRST;

    FLASH->ACR = acr;
}

void flash_unlock(void){
    if ((FLASH->CR & FLASH_CR_LOCK) != 0U) {
        FLASH->KEYR = 0x45670123UL;
        FLASH->KEYR = 0xCDEF89ABUL;
    }
}

void flash_lock(void){
	FLASH->CR = FLASH_CR_LOCK;

}

uint8_t flash_erase_sector(uint8_t sector){
    uint8_t status;

    if (sector < FLASH_FIRST_FREE_SECTOR || sector > FLASH_LAST_SECTOR) {
        return FLASH_ERR_PROTECTED;
    }

    status = flash_wait_busy();
    if (status != FLASH_OK) {
        return status;
    }

    flash_clear_errors();

    FLASH->CR &= ~FLASH_CR_PSIZE;
	FLASH->CR |=  FLASH_CR_PSIZE_1;          /* 0b10 = x32, needs 2.7-3.6 V */

    FLASH->CR &= ~FLASH_CR_SNB;
    FLASH->CR |=  ((uint32_t)sector << FLASH_CR_SNB_Pos);
    FLASH->CR |=  FLASH_CR_SER;
    FLASH->CR |=  FLASH_CR_STRT;

    status = flash_wait_busy();

    FLASH->CR &= ~FLASH_CR_SER;
    FLASH->CR &= ~FLASH_CR_SNB;

    if (status != FLASH_OK) {
	    return status;
    }

    status = flash_check_errors();
    flash_flush_caches();

    return status;
}


//write in the flash
uint8_t flash_write(uint32_t addr, const uint8_t *data, uint32_t len){

	uint8_t status;
	uint32_t i;
	uint32_t word;


	//4byte address  beacuse psize = x32
	//
	if ((addr & 3U) != 0U || (len & 3U) != 0U) {
	    return FLASH_ERR_ALIGN;
	}

	if(addr < 0x0800C00UL || (addr + len) > 0x08080000UL){
		return FLASH_ERR_PROTECTED;
	}

	status = flash_wait_busy();
	if(status != FLASH_OK){
		return status;
	}

	flash_clear_errors();

	FLASH->CR &= ~FLASH_CR_PSIZE;    //rst
	FLASH->CR |= FLASH_CR_PSIZE_1;   //32bit data parellelism(flash controller programs 32 bits at a time.)
	FLASH->CR |= FLASH_CR_PG;        // programming active

	//
    for (i = 0U; i < len; i += 4U) {
        word = (uint32_t)data[i]
             | ((uint32_t)data[i + 1U] << 8)
             | ((uint32_t)data[i + 2U] << 16)
             | ((uint32_t)data[i + 3U] << 24);

        *(volatile uint32_t *)(addr + i) = word;

        status = flash_wait_busy();
        if (status != FLASH_OK) {
            break;
        }

        status = flash_check_errors();
        if (status != FLASH_OK) {
            break;
        }
    }

    FLASH->CR &= ~(FLASH_CR_PG);

    flash_flush_caches();

    if (status != FLASH_OK) {
        return status;
    }

    //verifiying the data
    for (i = 0U; i < len; i += 4U) {
		word = (uint32_t)data[i]
			 | ((uint32_t)data[i + 1U] << 8)
			 | ((uint32_t)data[i + 2U] << 16)
			 | ((uint32_t)data[i + 3U] << 24);

		if (*(volatile uint32_t *)(addr + i) != word) {
			return FLASH_ERR_VERIFY;
		}
    }

    return FLASH_OK;


}
