#ifndef CONFIG_H
#define CONFIG_H

// ===== Traffic light timing (milliseconds) =====
#define LIGHT_GREEN_MS              6000
#define LIGHT_YELLOW_MS             2000
#define LIGHT_RED_MS                5000

// ===== Sensor parameters (HC-SR04 placeholder for FSR) =====
#define SENSOR_POLL_HZ              50      // 20 ms period
#define VEHICLE_DISTANCE_CM_MAX     30      // "object present" threshold
#define VEHICLE_DWELL_MS_MIN        150     // pedestrian filter (software analog)
#define VEHICLE_DWELL_MS_MAX        2000    // ignore stationary objects

// When FSRs arrive, replace the above with:
// #define FSR_ADC_THRESHOLD        2000    // ~vehicle pressure
// #define FSR_AB_GAP_MS_MIN        50      // axle-to-axle gap
// #define FSR_AB_GAP_MS_MAX        500

// ===== Alert timings =====
#define ALERT_BUZZER_MS             500
#define ALERT_FLASH_MS              20

// ===== FreeRTOS task priorities (higher = more urgent) =====
#define PRIO_SENSOR                 4
#define PRIO_VIOLATION              3
#define PRIO_ALERT                  3
#define PRIO_LIGHT                  2
#define PRIO_LOG                    1

// ===== Task stack sizes (in words, not bytes) =====
#define STACK_SMALL                 128
#define STACK_MED                   256
#define STACK_LARGE                 512

#endif // CONFIG_H
