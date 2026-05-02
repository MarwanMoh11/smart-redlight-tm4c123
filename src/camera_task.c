// CameraTask — sends a capture command to the ESP32-CAM module.
// Today: prints a placeholder "CAPTURE_REQUEST" line over UART0.
// Tomorrow: sends a binary capture command over UART2 (PD6/PD7) to
// the ESP32-CAM, which photographs the vehicle and saves to SD card.
// Sole responsibility: turning a violation_record_t into a photo.

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "inc/hw_memmap.h"
#include "driverlib/uart.h"

#include "events.h"

extern QueueHandle_t xCameraQueue;

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

// Future: when ESP32-CAM is wired to UART2 (PD6/PD7),
// replace this with a binary protocol send:
//
// static void Camera_SendCapture(uint32_t timestamp)
// {
//     uint8_t cmd[8] = { 0xAA, 0x55, 0x01, /* timestamp bytes */, 0xFF };
//     for (int i = 0; i < 8; i++) UARTCharPut(UART2_BASE, cmd[i]);
// }

void CameraTask(void *arg)
{
    (void)arg;
    violation_record_t rec;

    for (;;) {
        if (xQueueReceive(xCameraQueue, &rec, portMAX_DELAY) == pdTRUE) {
            // Today: text placeholder. Tomorrow: Camera_SendCapture(rec.timestamp_unix);
            uart_puts("    [CAMERA] capture_request tick=");
            uart_dec(rec.timestamp_unix);
            uart_puts("  -> img_");
            uart_dec(rec.timestamp_unix);
            uart_puts(".jpg\r\n");
        }
    }
}
