#include "misc.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"

#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"

#include <stdbool.h>

#define CPU_FREQ_HZ 50000000

static bool misc_were_ints_enabled;
static uint32_t misc_systick_ints = 0;

static void misc_systickISR(void);

void misc_init(void)
{
    SysTickIntRegister(misc_systickISR);

    SysTickPeriodSet(CPU_FREQ_HZ / 1000);

    // Enable the SysTick Interrupt.
    SysTickIntEnable();

    // Enable SysTick.
    SysTickEnable();
}

uint32_t misc_getSysMS(void)
{
    return misc_systick_ints;
}

void misc_enterCritical(void)
{
    misc_were_ints_enabled = !IntMasterDisable();
}

void misc_exitCritical(void)
{
    if (misc_were_ints_enabled) {
        IntMasterEnable();
    }
}

static void misc_systickISR(void)
{
    misc_systick_ints += 1;
}
