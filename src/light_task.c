// src/light_task.c
// Traffic light state machine — cycles the onboard RGB LED.

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"

#include "board.h"
#include "config.h"
#include "events.h"

// Shared with ViolationTask. Volatile because two tasks read/write it.
volatile light_state_t g_current_light = LIGHT_RED;

static void SetLight(uint8_t color_pins)
{
    GPIOPinWrite(LED_PORT_BASE, LED_ALL_PINS, color_pins);
}

void LightTask(void *arg)
{
    (void)arg;
    for (;;) {
        g_current_light = LIGHT_GREEN;
        SetLight(COLOR_GREEN);
        vTaskDelay(pdMS_TO_TICKS(LIGHT_GREEN_MS));

        g_current_light = LIGHT_YELLOW;
        SetLight(COLOR_YELLOW);
        vTaskDelay(pdMS_TO_TICKS(LIGHT_YELLOW_MS));

        g_current_light = LIGHT_RED;
        SetLight(COLOR_RED);
        vTaskDelay(pdMS_TO_TICKS(LIGHT_RED_MS));
    }
}
