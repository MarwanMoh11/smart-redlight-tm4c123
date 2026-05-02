// src/main.c — Step 3.1: traffic light cycle on onboard RGB.

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

#include "board.h"
#include "config.h"

extern void LightTask(void *arg);

static void HardwareInit(void)
{
    // 80 MHz from PLL + 16 MHz crystal
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // Port F: RGB LED (PF1=R, PF2=B, PF3=G)
    SysCtlPeripheralEnable(LED_PORT_PERIPH);
    while (!SysCtlPeripheralReady(LED_PORT_PERIPH)) {}
    GPIOPinTypeGPIOOutput(LED_PORT_BASE, LED_ALL_PINS);
    GPIOPinWrite(LED_PORT_BASE, LED_ALL_PINS, 0);
}

int main(void)
{
    HardwareInit();

    xTaskCreate(LightTask, "Light", STACK_MED, NULL, PRIO_LIGHT, NULL);

    vTaskStartScheduler();

    for (;;) {}
    return 0;
}

// FreeRTOS hooks
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask; (void)pcTaskName;
    for (;;) {}
}

void vApplicationMallocFailedHook(void)
{
    for (;;) {}
}
