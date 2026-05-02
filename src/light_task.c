// Traffic light cycle on onboard RGB. Updates g_current_light for
// other tasks (ViolationTask) to read.

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

volatile light_state_t g_current_light = LIGHT_RED;

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
        SetColor(GPIO_PIN_3);
        vTaskDelay(pdMS_TO_TICKS(LIGHT_GREEN_MS));

        g_current_light = LIGHT_YELLOW;
        SetColor(GPIO_PIN_1 | GPIO_PIN_3);
        vTaskDelay(pdMS_TO_TICKS(LIGHT_YELLOW_MS));

        g_current_light = LIGHT_RED;
        SetColor(GPIO_PIN_1);
        vTaskDelay(pdMS_TO_TICKS(LIGHT_RED_MS));
    }
}
