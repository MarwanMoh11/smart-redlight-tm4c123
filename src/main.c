// Step B: traffic light + HC-SR04 distance printing.

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"

#include "board.h"
#include "config.h"

extern void LightTask(void *arg);
extern void SensorTask(void *arg);

static void HardwareInit(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // RGB on Port F
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,
                          GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPinWrite(GPIO_PORTF_BASE,
                 GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);

    // HC-SR04 on Port B (PB0=trig, PB1=echo)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)) {}
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_0);
    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE,  GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_1,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, 0);

    // UART0 on PA0/PA1 @ 115200
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

    xTaskCreate(LightTask,  "Light",  256, NULL, 2, NULL);
    xTaskCreate(SensorTask, "Sensor", 512, NULL, 4, NULL);

    vTaskStartScheduler();
    for (;;) {}
    return 0;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{ (void)xTask; (void)pcTaskName; for(;;){} }
void vApplicationMallocFailedHook(void) { for(;;){} }
