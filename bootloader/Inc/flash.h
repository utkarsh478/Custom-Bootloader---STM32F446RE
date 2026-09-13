/*
 * flash.h
 *
 *  Created on: Sep 11, 2026
 *      Author: utkarsh
 */

#ifndef FLASH_H_
#define FLASH_H_

#include <stdint.h>

#define FLASH_OK            0U
#define FLASH_ERR_BUSY      1U
#define FLASH_ERR_PROTECTED 2U
#define FLASH_ERR_ALIGN     3U
#define FLASH_ERR_WRITE     4U
#define FLASH_ERR_VERIFY    5U

#define FLASH_FIRST_FREE_SECTOR  3U
#define FLASH_LAST_SECTOR        7U

void    flash_unlock(void);
void    flash_lock(void);
uint8_t flash_erase_sector(uint8_t sector);
uint8_t flash_write(uint32_t addr, const uint8_t *data, uint32_t len);



#endif /* FLASH_H_ */
