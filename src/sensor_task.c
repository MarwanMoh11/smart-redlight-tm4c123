// Step B: HC-SR04 distance reader. Prints over UART0 every 200ms.

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"

static void uart_puts(const char *s) {
    while (*s) UARTCharPut(UART0_BASE, *s++);
}

static void uart_dec(uint32_t v) {
    char buf[12];
    int n = 0;
    if (v == 0) { UARTCharPut(UART0_BASE, '0'); return; }
    while (v && n < 12) { buf[n++] = '0' + (v % 10); v /= 10; }
    while (n--) UARTCharPut(UART0_BASE, buf[n]);
}

static uint16_t HCSR04_ReadCm(void)
{
    uint32_t cycles_per_us = SysCtlClockGet() / 1000000;
    uint32_t timeout;

    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_PIN_0);
    SysCtlDelay(cycles_per_us * 10 / 3);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, 0);

    timeout = cycles_per_us * 30000 / 3;
    while (!GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_1)) {
        if (--timeout == 0) return 0xFFFF;
    }

    uint32_t hi_count = 0;
    timeout = cycles_per_us * 30000 / 3;
    while (GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_1)) {
        if (--timeout == 0) return 0xFFFF;
        hi_count++;
    }

    // Calibrated empirically: hi_count is in loop iterations,
    // and we found 1 cm ≈ N iterations.
    // Tune CM_PER_HI_COUNT_DIVIDER by pointing at known distances.
    #define CM_PER_HI_COUNT_DIVIDER 250   // adjust this number
    uint16_t cm = hi_count / CM_PER_HI_COUNT_DIVIDER;
    if (cm == 0 || cm > 400) return 0xFFFF;
    return cm;
}

void SensorTask(void *arg)
{
    (void)arg;
    uart_puts("\r\n=== Step B: sensor + light cycle ===\r\n\r\n");
    uint32_t n = 0;
    for (;;) {
        uint16_t cm = HCSR04_ReadCm();
        uart_puts("[");
        uart_dec(n++);
        uart_puts("] dist = ");
        if (cm == 0xFFFF) uart_puts("OUT_OF_RANGE");
        else { uart_dec(cm); uart_puts(" cm"); }
        uart_puts("\r\n");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
