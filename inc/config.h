#ifndef CONFIG_H
#define CONFIG_H

// ===== Traffic light timing (milliseconds) =====
#define LIGHT_GREEN_MS              6000
#define LIGHT_YELLOW_MS             2000
#define LIGHT_RED_MS                5000

// ===== FSR axle-detection (primary sensor — proposal spec) =====
// PE3 = FSR_A (front axle), PE2 = FSR_B (rear axle)
// Empirically calibrated: idle reads ~10-15, firm finger press ~1500-3700.
#define FSR_PRESS_THRESHOLD         1000    // ADC count above which = press detected
#define FSR_RELEASE_THRESHOLD       200     // hysteresis: re-arm when both drop below
#define FSR_AB_GAP_MS_MIN           50      // shortest valid axle-to-axle gap
#define FSR_AB_GAP_MS_MAX           3000    // longest valid axle-to-axle gap
#define FSR_POLL_PERIOD_MS          50      // sensor task poll rate (20 Hz)

// ===== HC-SR04 (secondary, "approach indicator" only — NOT a violation trigger) =====
// Calibration: pulse_us / 7 = cm  (Tiva polling-loop empirical divisor)
#define HCSR04_APPROACH_CM_MAX      40
#define HCSR04_POLL_EVERY_N_TICKS   10      // poll once per 10 sensor ticks = every 500ms

// ===== Alert timings =====
#define ALERT_BUZZER_MS             500
#define ALERT_FLASH_MS              20
#define ALERT_TONE_HZ               2000    // unused (no buzzer in this build)

// ===== Camera (ESP32-CAM placeholder) =====
#define CAMERA_PROTOCOL_BYTE_HDR    0xAA
#define CAMERA_PROTOCOL_BYTE_CMD    0x55
#define CAMERA_PROTOCOL_CMD_CAPTURE 0x01

// ===== FreeRTOS task priorities (higher = more urgent) =====
#define PRIO_VIOLATION              4   // top: react to sensor events instantly
#define PRIO_LIGHT                  3   // visible heartbeat
#define PRIO_ALERT                  3
#define PRIO_CAMERA                 2
#define PRIO_LOG                    2
#define PRIO_SENSOR                 1   // bottom: runs whenever there's spare CPU

// ===== Task stack sizes (in words) =====
#define STACK_SMALL                 256
#define STACK_MED                   512
#define STACK_LARGE                 1024

#endif // CONFIG_H
