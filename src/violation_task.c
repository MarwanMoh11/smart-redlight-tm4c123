// ViolationTask: consumes sensor events, checks light state.
// On RED-phase violation: pushes record to log queue AND
// flickers the onboard RGB red↔off rapidly for a visible alert.

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

extern QueueHandle_t xSensorEventQueue;
extern QueueHandle_t xLogQueue;
extern volatile light_state_t g_current_light;

static void FlickerAlert(void)
{
    const uint32_t flash_ms     = 50;
    const uint32_t total_ms     = ALERT_BUZZER_MS;   // reuse 500ms constant
    const uint32_t flash_count  = total_ms / (2 * flash_ms);

    for (uint32_t i = 0; i < flash_count; i++) {
        // OFF
        GPIOPinWrite(GPIO_PORTF_BASE,
                     GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);
        vTaskDelay(pdMS_TO_TICKS(flash_ms));
        // RED only
        GPIOPinWrite(GPIO_PORTF_BASE,
                     GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_1);
        vTaskDelay(pdMS_TO_TICKS(flash_ms));
    }
    // Restore solid red (light is still in red phase)
    GPIOPinWrite(GPIO_PORTF_BASE,
                 GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_1);
}

void ViolationTask(void *arg)
{
    (void)arg;
    sensor_event_t ev;

    for (;;) {
        if (xQueueReceive(xSensorEventQueue, &ev, portMAX_DELAY) == pdTRUE) {
            if (ev != SENSOR_VEHICLE_DETECTED) continue;

            if (g_current_light == LIGHT_RED) {
                violation_record_t rec = {
                    .timestamp_unix = xTaskGetTickCount(),
                    .light_at_event = LIGHT_RED,
                    .sensor_value   = 0
                };
                xQueueSend(xLogQueue, &rec, 0);
                FlickerAlert();
            }
        }
    }
}
