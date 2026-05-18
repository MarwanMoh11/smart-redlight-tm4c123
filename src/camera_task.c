// CameraTask — ESP32-CAM capture command dispatcher.
//
// Today: text placeholder logged over UART0 (visible in picocom).
// Tomorrow: binary command over UART2 (PD6/PD7) to a flashed ESP32-CAM
//   running our companion firmware. Uncomment Camera_SendBinaryCapture()
//   below once the ESP32-CAM hardware is integrated.
//
// Protocol (when binary mode is enabled):
//   HDR  CMD  TIMESTAMP[4 LE bytes]  CHECKSUM
//   0xAA 0x55 [TS]                    XOR
//
// ESP32-CAM responds with 0xAA 0x55 0x02 [size_LE] on success.

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "inc/hw_memmap.h"
#include "driverlib/uart.h"

#include "config.h"
#include "events.h"

extern QueueHandle_t     xCameraQueue;
extern SemaphoreHandle_t xUartMutex;

// ===== UART0 helpers (placeholder text path) =====
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

// ===== ESP32-CAM binary protocol (UART2 — TO BE ENABLED) =====
// Wire to UART2 (PD6/PD7) and uncomment in main.c HardwareInit.
//
// static void Camera_SendBinaryCapture(uint32_t timestamp)
// {
//     uint8_t pkt[7];
//     pkt[0] = CAMERA_PROTOCOL_BYTE_HDR;
//     pkt[1] = CAMERA_PROTOCOL_BYTE_CMD;
//     pkt[2] = (timestamp >>  0) & 0xFF;
//     pkt[3] = (timestamp >>  8) & 0xFF;
//     pkt[4] = (timestamp >> 16) & 0xFF;
//     pkt[5] = (timestamp >> 24) & 0xFF;
//     pkt[6] = pkt[0] ^ pkt[1] ^ pkt[2] ^ pkt[3] ^ pkt[4] ^ pkt[5];   // XOR checksum
//     for (int i = 0; i < 7; i++) UARTCharPut(UART2_BASE, pkt[i]);
// }

void CameraTask(void *arg)
{
    (void)arg;
    violation_record_t rec;

    for (;;) {
        if (xQueueReceive(xCameraQueue, &rec, portMAX_DELAY) == pdTRUE) {
            // Today: text placeholder (visible on serial monitor)
            xSemaphoreTake(xUartMutex, portMAX_DELAY);
            uart_puts("    [CAMERA] capture_request tick=");
            uart_dec(rec.timestamp_unix);
            uart_puts("  -> img_");
            uart_dec(rec.timestamp_unix);
            uart_puts(".jpg\r\n");
            xSemaphoreGive(xUartMutex);

            // Tomorrow: binary command on UART2 to actual ESP32-CAM
            // Camera_SendBinaryCapture(rec.timestamp_unix);
        }
    }
}
