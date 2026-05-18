// Smart Red Light — 6-task FreeRTOS architecture matching proposal.
//
// Sensors:     FSR_A (PE3), FSR_B (PE2), HC-SR04 (PB0/PB1)
// Outputs:     onboard RGB LED (PF1/2/3) for traffic light + alert flicker
// Logging:     UART0 (PA0/PA1) to PC at 115200 baud, picocom-friendly
// Camera:      ESP32-CAM UART2 (PD6/PD7) — wired, init commented out
//              until ESP32-CAM firmware is flashed.
//
// Concurrency: 6 tasks, 4 priority levels, 4 queues, 1 mutex.

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
#include "driverlib/uart.h"
#include "driverlib/adc.h"
#include "driverlib/pin_map.h"

#include "config.h"
#include "events.h"

extern void LightTask(void *arg);
extern void SensorTask(void *arg);
extern void ViolationTask(void *arg);
extern void AlertTask(void *arg);
extern void CameraTask(void *arg);
extern void LogTask(void *arg);

QueueHandle_t     xSensorEventQueue;
QueueHandle_t     xAlertQueue;
QueueHandle_t     xCameraQueue;
QueueHandle_t     xLogQueue;
SemaphoreHandle_t xUartMutex;

static void HardwareInit(void)
{
    // 80 MHz from PLL + 16 MHz crystal
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // ===== Port F: onboard RGB LED =====
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,
                          GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPinWrite(GPIO_PORTF_BASE,
                 GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);

    // ===== Port B: HC-SR04 (PB0 TRIG out, PB1 ECHO in) =====
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)) {}
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_0);
    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE,  GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_1,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, 0);

    // ===== Port E: FSR_A (PE3 = CH0) and FSR_B (PE2 = CH1) on ADC0 =====
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)) {}
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)) {}
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    ADCHardwareOversampleConfigure(ADC0_BASE, 64);
    ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_CH0);   // step 0 = FSR_A
    ADCSequenceStepConfigure(ADC0_BASE, 1, 1,
                             ADC_CTL_CH1 | ADC_CTL_IE | ADC_CTL_END); // step 1 = FSR_B
    ADCSequenceEnable(ADC0_BASE, 1);
    ADCIntClear(ADC0_BASE, 1);

    // ===== UART0: PC debug console (ICDI virtual COM @ 115200) =====
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)) {}
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) {}
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE);

    // ===== UART2: ESP32-CAM (PD6 RX, PD7 TX) — uncomment when integrating =====
    // SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
    // SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    // while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART2)) {}
    // while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD)) {}
    // GPIOPinConfigure(GPIO_PD6_U2RX);
    // GPIOPinConfigure(GPIO_PD7_U2TX);
    // GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    // UARTConfigSetExpClk(UART2_BASE, SysCtlClockGet(), 115200,
    //                     UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
    //                     UART_CONFIG_PAR_NONE);
}

int main(void)
{
    HardwareInit();

    xSensorEventQueue = xQueueCreate(8,  sizeof(sensor_event_t));
    xAlertQueue       = xQueueCreate(4,  sizeof(violation_record_t));
    xCameraQueue      = xQueueCreate(4,  sizeof(violation_record_t));
    xLogQueue         = xQueueCreate(16, sizeof(violation_record_t));
    xUartMutex        = xSemaphoreCreateMutex();

    xTaskCreate(LightTask,     "Light", STACK_SMALL, NULL, PRIO_LIGHT,     NULL);
    xTaskCreate(SensorTask,    "Sens",  STACK_MED,   NULL, PRIO_SENSOR,    NULL);
    xTaskCreate(ViolationTask, "Viol",  STACK_SMALL, NULL, PRIO_VIOLATION, NULL);
    xTaskCreate(AlertTask,     "Alert", STACK_SMALL, NULL, PRIO_ALERT,     NULL);
    xTaskCreate(CameraTask,    "Cam",   STACK_SMALL, NULL, PRIO_CAMERA,    NULL);
    xTaskCreate(LogTask,       "Log",   STACK_MED,   NULL, PRIO_LOG,       NULL);

    vTaskStartScheduler();
    for (;;) {}
    return 0;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{ (void)xTask; (void)pcTaskName; for(;;){} }
void vApplicationMallocFailedHook(void) { for(;;){} }
