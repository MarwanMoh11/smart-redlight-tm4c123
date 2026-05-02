// ViolationTask — pure event dispatcher.
// Consumes sensor events. On RED-phase detection, builds a
// violation_record_t and fans it out to alert, camera, and log queues.
// No hardware access. No UART. Pure logic — easily unit-testable.

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "events.h"

extern QueueHandle_t xSensorEventQueue;
extern QueueHandle_t xAlertQueue;
extern QueueHandle_t xCameraQueue;
extern QueueHandle_t xLogQueue;
extern volatile light_state_t g_current_light;

void ViolationTask(void *arg)
{
    (void)arg;
    sensor_event_t ev;

    for (;;) {
        if (xQueueReceive(xSensorEventQueue, &ev, portMAX_DELAY) != pdTRUE) continue;
        if (ev != SENSOR_VEHICLE_DETECTED) continue;
        if (g_current_light != LIGHT_RED) continue;   // legal pass

        violation_record_t rec = {
            .timestamp_unix = xTaskGetTickCount(),
            .light_at_event = LIGHT_RED,
            .sensor_value   = 0
        };

        // Fan out to all consumers. Drop on full (xQueueSend with 0 timeout).
        xQueueSend(xAlertQueue,  &rec, 0);
        xQueueSend(xCameraQueue, &rec, 0);
        xQueueSend(xLogQueue,    &rec, 0);
    }
}
