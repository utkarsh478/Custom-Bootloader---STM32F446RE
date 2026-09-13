/*
 * uart.c
 *
 *  Created on: Sep 11, 2026
 *      Author: utkarsh
 */


#include "stm32f446xx.h"
#include "uart.h"

/* 16 MHz HSI, 115200 baud, OVER8 = 0
 * 16000000 / (16 * 115200) = 8.681
 * mantissa = 8, fraction = round(0.681 * 16) = 11
 */
#define UART_BRR_115200_16MHZ	((8U << 4) | 11U)


void uart_init(void){

	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

	/* PA2 = TX, PA3 = RX, both to alternate function */
	GPIOA->MODER &= ~((3U << (2 * 2)) | (3U << (3 * 2)));
	GPIOA->MODER |=  ((2U << (2 * 2)) | (2U << (3 * 2)));

	/* AF7 = USART2 */
	GPIOA->AFR[0] &= ~((0xFU << (2 * 4)) | (0xFU << (3 * 4)));
	GPIOA->AFR[0] |=  ((7U   << (2 * 4)) | (7U   << (3 * 4)));

	/* Push-pull, high speed, no pull */
	GPIOA->OTYPER  &= ~((1U << 2) | (1U << 3));
	GPIOA->OSPEEDR |=  ((3U << (2 * 2)) | (3U << (3 * 2)));
	GPIOA->PUPDR   &= ~((3U << (2 * 2)) | (3U << (3 * 2)));

	USART2->CR1 = 0U;                      /* 8N1, no parity, OVER8 = 0 */
	USART2->CR2 = 0U;                      /* 1 stop bit */
	USART2->CR3 = 0U;                      /* no flow control */

	USART2->BRR = UART_BRR_115200_16MHZ;

	USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

}

void uart_send_byte(uint8_t b){
	while ((USART2->SR & USART_SR_TXE) == 0U){}
		USART2->DR = (uint32_t)b;
}

void uart_send(const uint8_t *data, uint32_t len){
	for(uint32_t i = 0U;i < len ;i++){
		uart_send_byte(data[i]);
	}
}

void uart_send_str(const char *s){
	while(*s != '\0'){
		uart_send_byte((uint8_t)*s);
		s++;
	}
}

uint8_t uart_recv_ready(void)
{
    return ((USART2->SR & USART_SR_RXNE) != 0U) ? 1U : 0U;
}

uint8_t uart_recv_byte(void)
{
    while ((USART2->SR & USART_SR_RXNE) == 0U) { }
    return (uint8_t)(USART2->DR & 0xFFU);
}

uint8_t uart_recv_byte_to(uint8_t *b, uint32_t timeout)
{
    while (timeout-- > 0U) {
        if ((USART2->SR & USART_SR_ORE) != 0U) {
            (void)USART2->SR;
            (void)USART2->DR;
        }
        if ((USART2->SR & USART_SR_RXNE) != 0U) {
            *b = (uint8_t)(USART2->DR & 0xFFU);
            return UART_OK;
        }
    }
    return UART_TIMEOUT;
}
