/*
 * crc.h
 *
 *  Created on: Sep 13, 2026
 *      Author: utkarsh
 */

#ifndef CRC_H_
#define CRC_H_

#include <stdint.h>

void     crc32_init(void);
void     crc32_reset(void);
void     crc32_update(const uint8_t *data, uint32_t len);
uint32_t crc32_value(void);
uint32_t crc32_compute(const uint8_t *data, uint32_t len);


#endif /* CRC_H_ */
