// src/main.c — Phase 2 placeholder. Real task creation comes in Phase 3.

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

// Blink the onboard red LED via FreeRTOS to prove the scheduler runs.
static void BlinkTask(void *arg)
{
    (void)arg;
    for (;;) {
        GPIOPinWrite(LED_PORT_BASE, LED_RED_PIN, LED_RED_PIN);
        vTaskDelay(pdMS_TO_TICKS(250));
        GPIOPinWrite(LED_PORT_BASE, LED_RED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

static void HardwareInit(void)
{
    // 80 MHz from PLL + 16 MHz crystal
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    SysCtlPeripheralEnable(LED_PORT_PERIPH);
    while (!SysCtlPeripheralReady(LED_PORT_PERIPH)) {}
    GPIOPinTypeGPIOOutput(LED_PORT_BASE, LED_ALL_PINS);
    GPIOPinWrite(LED_PORT_BASE, LED_ALL_PINS, 0);
}

int main(void)
{
    HardwareInit();

    xTaskCreate(BlinkTask, "Blink", STACK_SMALL, NULL, 2, NULL);

    vTaskStartScheduler();

    // Should never reach here.
    for (;;) {}
    return 0;
}

// FreeRTOS hooks (required when configCHECK_FOR_STACK_OVERFLOW and
// configUSE_MALLOC_FAILED_HOOK are enabled in FreeRTOSConfig.h)
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask; (void)pcTaskName;
    for (;;) {}    // halt — set a breakpoint here in GDB
}

void vApplicationMallocFailedHook(void)
{
    for (;;) {}
}
