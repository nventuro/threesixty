#include "misc.h"

#include "inc/hw_types.h"

#include "driverlib/interrupt.h"

#include <stdbool.h>

static bool misc_were_ints_enabled;

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