// CameraTask — placeholder for ESP32-CAM. Mutex-protected UART writes.

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "inc/hw_memmap.h"
#include "driverlib/uart.h"

#include "events.h"

extern QueueHandle_t xCameraQueue;
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

void CameraTask(void *arg)
{
    (void)arg;
    violation_record_t rec;

    for (;;) {
        if (xQueueReceive(xCameraQueue, &rec, portMAX_DELAY) == pdTRUE) {
            xSemaphoreTake(xUartMutex, portMAX_DELAY);
            uart_puts("    [CAMERA] capture_request tick=");
            uart_dec(rec.timestamp_unix);
            uart_puts("  -> img_");
            uart_dec(rec.timestamp_unix);
            uart_puts(".jpg\r\n");
            xSemaphoreGive(xUartMutex);
        }
    }
}
