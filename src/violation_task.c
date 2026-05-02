// ViolationTask: consumes sensor events, checks light color,
// publishes violation records to log queue if light was RED.

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "events.h"

extern QueueHandle_t xSensorEventQueue;
extern QueueHandle_t xLogQueue;
extern volatile light_state_t g_current_light;

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
            }
        }
    }
}
