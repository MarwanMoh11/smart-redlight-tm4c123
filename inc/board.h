#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"

// ===== Onboard RGB LED (traffic light) =====
#define LED_PORT_BASE       GPIO_PORTF_BASE
#define LED_PORT_PERIPH     SYSCTL_PERIPH_GPIOF
#define LED_RED_PIN         GPIO_PIN_1
#define LED_BLUE_PIN        GPIO_PIN_2
#define LED_GREEN_PIN       GPIO_PIN_3
#define LED_ALL_PINS        (LED_RED_PIN | LED_BLUE_PIN | LED_GREEN_PIN)

// Yellow = red + green (positive logic on TM4C123GXL onboard RGB)
#define COLOR_RED           LED_RED_PIN
#define COLOR_YELLOW        (LED_RED_PIN | LED_GREEN_PIN)
#define COLOR_GREEN         LED_GREEN_PIN
#define COLOR_OFF           0

// ===== Onboard buttons (manual override) =====
#define BTN_PORT_BASE       GPIO_PORTF_BASE
#define BTN_SW1_PIN         GPIO_PIN_4   // force red
#define BTN_SW2_PIN         GPIO_PIN_0   // reset counter (NMI by default - needs unlock)

// ===== HC-SR04 ultrasonic sensor (placeholder for FSRs) =====
#define HCSR04_PORT_BASE    GPIO_PORTB_BASE
#define HCSR04_PORT_PERIPH  SYSCTL_PERIPH_GPIOB
#define HCSR04_TRIG_PIN     GPIO_PIN_0   // PB0
#define HCSR04_ECHO_PIN     GPIO_PIN_1   // PB1

// ===== Buzzer + flash LED placeholder =====
#define BUZZER_PORT_BASE    GPIO_PORTE_BASE
#define BUZZER_PORT_PERIPH  SYSCTL_PERIPH_GPIOE
#define BUZZER_PIN          GPIO_PIN_1   // PE1
#define FLASH_PIN           GPIO_PIN_2   // PE2

// ===== HM-10 Bluetooth on UART2 (PD6/PD7) =====
#define HM10_UART_BASE      UART2_BASE
#define HM10_UART_PERIPH    SYSCTL_PERIPH_UART2
#define HM10_GPIO_PERIPH    SYSCTL_PERIPH_GPIOD
#define HM10_GPIO_BASE      GPIO_PORTD_BASE
#define HM10_RX_PIN         GPIO_PIN_6
#define HM10_TX_PIN         GPIO_PIN_7
#define HM10_RX_CFG         GPIO_PD6_U2RX
#define HM10_TX_CFG         GPIO_PD7_U2TX
#define HM10_BAUD           9600

// ===== Debug UART0 (ICDI virtual COM @ 115200) =====
#define DEBUG_UART_BASE     UART0_BASE
#define DEBUG_UART_PERIPH   SYSCTL_PERIPH_UART0
#define DEBUG_GPIO_PERIPH   SYSCTL_PERIPH_GPIOA
#define DEBUG_GPIO_BASE     GPIO_PORTA_BASE
#define DEBUG_RX_PIN        GPIO_PIN_0
#define DEBUG_TX_PIN        GPIO_PIN_1
#define DEBUG_RX_CFG        GPIO_PA0_U0RX
#define DEBUG_TX_CFG        GPIO_PA1_U0TX
#define DEBUG_BAUD          115200

// ===== DS3231 RTC on I2C0 (PB2/PB3) =====
// NOTE: PB2/PB3 conflicts with nothing here, but FYI HCSR04 uses PB0/PB1.
#define RTC_I2C_BASE        I2C0_BASE
#define RTC_I2C_PERIPH      SYSCTL_PERIPH_I2C0
#define RTC_GPIO_PERIPH     SYSCTL_PERIPH_GPIOB
#define RTC_GPIO_BASE       GPIO_PORTB_BASE
#define RTC_SCL_PIN         GPIO_PIN_2   // PB2
#define RTC_SDA_PIN         GPIO_PIN_3   // PB3
#define RTC_SCL_CFG         GPIO_PB2_I2C0SCL
#define RTC_SDA_CFG         GPIO_PB3_I2C0SDA
#define DS3231_ADDR         0x68

#endif // BOARD_H
