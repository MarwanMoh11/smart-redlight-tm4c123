// Smart Red Light — 6-task FreeRTOS architecture matching proposal.
// UART access is serialized via xUartMutex.

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
#include "driverlib/pin_map.h"

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
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,
                          GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPinWrite(GPIO_PORTF_BASE,
                 GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)) {}
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_0);
    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE,  GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_1,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, 0);

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
}

int main(void)
{
    HardwareInit();

    xSensorEventQueue = xQueueCreate(8,  sizeof(sensor_event_t));
    xAlertQueue       = xQueueCreate(4,  sizeof(violation_record_t));
    xCameraQueue      = xQueueCreate(4,  sizeof(violation_record_t));
    xLogQueue         = xQueueCreate(16, sizeof(violation_record_t));
    xUartMutex        = xSemaphoreCreateMutex();

    xTaskCreate(LightTask,     "Light",  256, NULL, 3, NULL);
    xTaskCreate(SensorTask,    "Sens",   512, NULL, 1, NULL);
    xTaskCreate(ViolationTask, "Viol",   256, NULL, 4, NULL);
    xTaskCreate(AlertTask,     "Alert",  256, NULL, 3, NULL);
    xTaskCreate(CameraTask,    "Cam",    256, NULL, 2, NULL);
    xTaskCreate(LogTask,       "Log",    512, NULL, 2, NULL);

    vTaskStartScheduler();
    for (;;) {}
    return 0;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{ (void)xTask; (void)pcTaskName; for(;;){} }
void vApplicationMallocFailedHook(void) { for(;;){} }
