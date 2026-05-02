#ifndef EVENTS_H
#define EVENTS_H

#include <stdint.h>

// Traffic light state — written by light_task, read by violation_task
typedef enum {
    LIGHT_GREEN  = 0,
    LIGHT_YELLOW = 1,
    LIGHT_RED    = 2
} light_state_t;

// Sensor event — produced by sensor_task, consumed by violation_task
typedef enum {
    SENSOR_NONE              = 0,
    SENSOR_VEHICLE_DETECTED  = 1
} sensor_event_t;

// Violation record — produced by violation_task, consumed by alert + log tasks
typedef struct {
    uint32_t      timestamp_unix;   // from DS3231
    light_state_t light_at_event;   // always RED for actual violations
    uint16_t      sensor_value;     // raw distance (cm) today, ADC counts later
} violation_record_t;

#endif // EVENTS_H
