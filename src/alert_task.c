// AlertTask — handles violation-time alerting.
// Today: rapid red flicker on the onboard RGB.
// Tomorrow: also pulses PE2 (flash LED) for 20 ms and drives a buzzer.
// Sole responsibility: turning a violation_record_t into a physical alert.

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"

#include "events.h"
#include "config.h"

extern QueueHandle_t xAlertQueue;

static void Alert_Visual(void)
{
    const uint32_t flash_ms    = 50;
    const uint32_t total_ms    = ALERT_BUZZER_MS;
    const uint32_t flash_count = total_ms / (2 * flash_ms);

    for (uint32_t i = 0; i < flash_count; i++) {
        GPIOPinWrite(GPIO_PORTF_BASE,
                     GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);
        vTaskDelay(pdMS_TO_TICKS(flash_ms));
        GPIOPinWrite(GPIO_PORTF_BASE,
                     GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_1);
        vTaskDelay(pdMS_TO_TICKS(flash_ms));
    }
    GPIOPinWrite(GPIO_PORTF_BASE,
                 GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_1);
}

// Future hook: when flash LED hardware is wired to PE2,
// uncomment and call Alert_Flash(rec) before Alert_Visual().
//
// static void Alert_Flash(const violation_record_t *rec)
// {
//     (void)rec;
//     GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, GPIO_PIN_2);
//     vTaskDelay(pdMS_TO_TICKS(ALERT_FLASH_MS));
//     GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, 0);
// }
//
// Future hook: when buzzer hardware is wired,
// add Alert_Buzz() and call alongside.

void AlertTask(void *arg)
{
    (void)arg;
    violation_record_t rec;

    for (;;) {
        if (xQueueReceive(xAlertQueue, &rec, portMAX_DELAY) == pdTRUE) {
            // Alert_Flash(&rec);   // future
            Alert_Visual();
            // Alert_Buzz();        // future
        }
    }
}
