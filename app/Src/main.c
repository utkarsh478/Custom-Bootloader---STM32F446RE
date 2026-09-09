#include "stm32f446xx.h"

#define APP_BASE 0x08020000UL

static void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms * 1600U; i++) {
        __NOP();
    }
}

int main(void)
{
    SCB->VTOR = APP_BASE;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER &= ~(3U << 10);
    GPIOA->MODER |=  (1U << 10);

    for (;;) {
        GPIOA->BSRR = (1U << 5);
        delay_ms(3000);
        GPIOA->BSRR = (1U << 5) << 16;
        delay_ms(3000);
    }
}
