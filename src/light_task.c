// Traffic light cycle on onboard RGB. Updates g_current_light and
// prints phase-start markers over UART (mutex-protected).

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"

#include "board.h"
#include "config.h"
#include "events.h"

extern SemaphoreHandle_t xUartMutex;

volatile light_state_t g_current_light = LIGHT_RED;

static void uart_puts_locked(const char *s) {
    xSemaphoreTake(xUartMutex, portMAX_DELAY);
    while (*s) UARTCharPut(UART0_BASE, *s++);
    xSemaphoreGive(xUartMutex);
}

static void SetColor(uint8_t pins)
{
    GPIOPinWrite(GPIO_PORTF_BASE,
                 GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, pins);
}

void LightTask(void *arg)
{
    (void)arg;
    for (;;) {
        g_current_light = LIGHT_GREEN;
        uart_puts_locked("\r\n--- GREEN ---\r\n");
        SetColor(GPIO_PIN_3);
        vTaskDelay(pdMS_TO_TICKS(LIGHT_GREEN_MS));

        g_current_light = LIGHT_YELLOW;
        uart_puts_locked("--- YELLOW ---\r\n");
        SetColor(GPIO_PIN_1 | GPIO_PIN_3);
        vTaskDelay(pdMS_TO_TICKS(LIGHT_YELLOW_MS));

        g_current_light = LIGHT_RED;
        uart_puts_locked("--- RED (detection active) ---\r\n");
        SetColor(GPIO_PIN_1);
        vTaskDelay(pdMS_TO_TICKS(LIGHT_RED_MS));
    }
}
