// SensorTask — dual-FSR axle detection (primary) + HC-SR04 approach indicator.
//
// Hardware:
//   FSR_A on PE3 (ADC0 CH0), front axle
//   FSR_B on PE2 (ADC0 CH1), rear axle
//   HC-SR04 on PB0 (TRIG) + PB1 (ECHO), approach detector
//
// State machine:
//   IDLE  -> A pressed  -> A_HIT  -> B pressed within window -> emit + LOCKED
//                                 -> timeout                 -> LOCKED (pedestrian)
//                                 -> A released w/o B        -> IDLE
//   LOCKED -> both FSRs released -> IDLE (re-armed)
//
// Modularity:
//   This is the ONLY file that talks to physical sensors.
//   It posts opaque sensor_event_t through xSensorEventQueue.
//   Downstream tasks have zero awareness of FSR vs HC-SR04 vs anything else.

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "driverlib/uart.h"

#include "config.h"
#include "events.h"

extern QueueHandle_t     xSensorEventQueue;
extern SemaphoreHandle_t xUartMutex;

// ===== UART helpers (mutex-protected) =====
static void uart_puts_locked(const char *s) {
    xSemaphoreTake(xUartMutex, portMAX_DELAY);
    while (*s) UARTCharPut(UART0_BASE, *s++);
    xSemaphoreGive(xUartMutex);
}

// ===== Sensor reads =====

static void FSR_ReadBoth(uint32_t *a, uint32_t *b)
{
    uint32_t buf[2];
    ADCIntClear(ADC0_BASE, 1);
    ADCProcessorTrigger(ADC0_BASE, 1);
    while (!ADCIntStatus(ADC0_BASE, 1, false)) {}
    ADCIntClear(ADC0_BASE, 1);
    ADCSequenceDataGet(ADC0_BASE, 1, buf);
    *a = buf[0];   // PE3 = CH0
    *b = buf[1];   // PE2 = CH1
}

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
    uint16_t cm = pulse_us / 7;
    if (cm == 0 || cm > 400) return 0xFFFF;
    return cm;
}

// ===== Axle-detection state machine =====

typedef enum { S_IDLE, S_A_HIT, S_LOCKED } detect_state_t;

void SensorTask(void *arg)
{
    (void)arg;
    detect_state_t state = S_IDLE;
    TickType_t a_hit_at = 0;
    const TickType_t period = pdMS_TO_TICKS(FSR_POLL_PERIOD_MS);
    uint32_t hcsr04_counter = 0;

    for (;;) {
        uint32_t a, b;
        FSR_ReadBoth(&a, &b);
        bool a_press       = (a > FSR_PRESS_THRESHOLD);
        bool b_press       = (b > FSR_PRESS_THRESHOLD);
        bool both_released = (a < FSR_RELEASE_THRESHOLD) && (b < FSR_RELEASE_THRESHOLD);
        TickType_t now     = xTaskGetTickCount();

        switch (state) {
        case S_IDLE:
            if (a_press && !b_press) {
                state = S_A_HIT;
                a_hit_at = now;
            } else if (a_press && b_press) {
                // Both at once — count as a vehicle (single thick axle / simultaneous press)
                sensor_event_t ev = SENSOR_VEHICLE_DETECTED;
                xQueueSend(xSensorEventQueue, &ev, 0);
                state = S_LOCKED;
            }
            // Lone B press = pedestrian (wrong axle order); silently ignored
            break;

        case S_A_HIT: {
            uint32_t elapsed_ms = (now - a_hit_at) * portTICK_PERIOD_MS;
            if (b_press && elapsed_ms >= FSR_AB_GAP_MS_MIN
                        && elapsed_ms <= FSR_AB_GAP_MS_MAX) {
                sensor_event_t ev = SENSOR_VEHICLE_DETECTED;
                xQueueSend(xSensorEventQueue, &ev, 0);
                state = S_LOCKED;
            } else if (elapsed_ms > FSR_AB_GAP_MS_MAX) {
                state = S_LOCKED;   // timed out — could've been a pedestrian
            } else if (!a_press && !b_press) {
                state = S_IDLE;     // A released without B ever firing — pedestrian
            }
            break;
        }

        case S_LOCKED:
            if (both_released) state = S_IDLE;
            break;
        }

        // HC-SR04 approach indicator (lower priority, polled less often)
        if (++hcsr04_counter >= HCSR04_POLL_EVERY_N_TICKS) {
            hcsr04_counter = 0;
            uint16_t cm = HCSR04_ReadCm();
            if (cm < HCSR04_APPROACH_CM_MAX && cm > 5) {
                uart_puts_locked("    [APPROACH] vehicle within 40cm\r\n");
            }
        }

        vTaskDelay(period);
    }
}
