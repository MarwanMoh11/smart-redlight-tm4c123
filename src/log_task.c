// LogTask — timestamped UART output, mutex-protected.

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "inc/hw_memmap.h"
#include "driverlib/uart.h"

#include "events.h"

extern QueueHandle_t xLogQueue;
extern SemaphoreHandle_t xUartMutex;

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

void LogTask(void *arg)
{
    (void)arg;
    violation_record_t rec;
    uint32_t count = 0;

    xSemaphoreTake(xUartMutex, portMAX_DELAY);
    uart_puts("\r\n=== Smart Red Light System Online ===\r\n");
    uart_puts("Wave hand at sensor during RED phase to trigger violation.\r\n\r\n");
    xSemaphoreGive(xUartMutex);

    for (;;) {
        if (xQueueReceive(xLogQueue, &rec, portMAX_DELAY) == pdTRUE) {
            count++;
            xSemaphoreTake(xUartMutex, portMAX_DELAY);
            uart_puts(">>> VIOLATION #");
            uart_dec(count);
            uart_puts(" at tick=");
            uart_dec(rec.timestamp_unix);
            uart_puts(" (light=RED)\r\n");
            xSemaphoreGive(xUartMutex);
        }
    }
}
