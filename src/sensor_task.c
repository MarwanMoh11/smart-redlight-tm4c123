// HC-SR04 sensor task with software pedestrian filter.
// Posts SENSOR_VEHICLE_DETECTED to xSensorEventQueue when
// an object is present for VEHICLE_DWELL_MS_MIN to MAX,
// then leaves.

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

#include "config.h"
#include "events.h"

extern QueueHandle_t xSensorEventQueue;

static uint16_t HCSR04_ReadCm(void)
{
    uint32_t cycles_per_us = SysCtlClockGet() / 1000000;
    uint32_t timeout;

    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_PIN_0);
    SysCtlDelay(cycles_per_us * 10 / 3);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, 0);

    timeout = cycles_per_us * 30000 / 3;
    while (!GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_1)) {
        if (--timeout == 0) return 0xFFFF;
    }

    uint32_t hi_count = 0;
    timeout = cycles_per_us * 30000 / 3;
    while (GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_1)) {
        if (--timeout == 0) return 0xFFFF;
        hi_count++;
    }

    uint32_t pulse_us = (hi_count * 3) / cycles_per_us;
    uint16_t cm = pulse_us / 58;
    if (cm == 0 || cm > 400) return 0xFFFF;
    return cm;
}

typedef enum { S_IDLE, S_PRESENT } detect_state_t;

void SensorTask(void *arg)
{
    (void)arg;
    detect_state_t state = S_IDLE;
    TickType_t entered = 0;
    const TickType_t period = pdMS_TO_TICKS(1000 / SENSOR_POLL_HZ);
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        uint16_t cm = HCSR04_ReadCm();
        bool present = (cm != 0xFFFF) && (cm < VEHICLE_DISTANCE_CM_MAX);
        TickType_t now = xTaskGetTickCount();

        switch (state) {
        case S_IDLE:
            if (present) { state = S_PRESENT; entered = now; }
            break;

        case S_PRESENT:
            if (!present) {
                uint32_t dwell_ms = (now - entered) * portTICK_PERIOD_MS;
                if (dwell_ms >= VEHICLE_DWELL_MS_MIN &&
                    dwell_ms <= VEHICLE_DWELL_MS_MAX) {
                    sensor_event_t ev = SENSOR_VEHICLE_DETECTED;
                    xQueueSend(xSensorEventQueue, &ev, 0);
                }
                state = S_IDLE;
            } else if ((now - entered) * portTICK_PERIOD_MS >
                       VEHICLE_DWELL_MS_MAX) {
                state = S_IDLE;   // stationary object, abandon
            }
            break;
        }

        vTaskDelay(period);
    }
}
