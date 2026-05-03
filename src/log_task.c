// LogTask: prints violation records to UART0

#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "inc/hw_memmap.h"
#include "driverlib/uart.h"

#include "events.h"

extern QueueHandle_t xLogQueue;

static void UART_PrintString(const char *text)
{
    while (*text != '\0')
    {
        UARTCharPut(UART0_BASE, *text);
        text++;
    }
}

void LogTask(void *arg)
{
    (void)arg;

    violation_record_t record;
    uint32_t violationCount = 0;
    char message[100];

    UART_PrintString("\r\n=== Smart Red Light System Online ===\r\n");
    UART_PrintString("Wave hand at sensor during RED phase to trigger violation.\r\n\r\n");

    while (1)
    {
        xQueueReceive(xLogQueue, &record, portMAX_DELAY);

        violationCount++;

        sprintf(message,
                ">>> VIOLATION #%lu at tick=%lu (light=RED)\r\n",
                violationCount,
                record.timestamp_unix);

        UART_PrintString(message);
    }
}
