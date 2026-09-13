/*
 * uart.h
 *
 *  Created on: Sep 11, 2026
 *      Author: utkarsh
 */

#ifndef UART_H_
#define UART_H_

#include <stdint.h>

#define UART_OK       0U
#define UART_TIMEOUT  1U


void    uart_init(void);
void    uart_send_byte(uint8_t b);
void    uart_send(const uint8_t *data, uint32_t len);
void    uart_send_str(const char *s);
uint8_t uart_recv_byte(void);
uint8_t uart_recv_ready(void);
uint8_t uart_recv_byte_to(uint8_t *b, uint32_t timeout);

#endif /* UART_H_ */
