#include "assert.h"

#include "inc/hw_types.h"
#include "driverlib/interrupt.h"

#include "console.h"

void assert(bool condition, char *msg)
{
    if (!condition) {
        IntMasterDisable();

        console_printf("ASSERTION FAILED: ");
        console_printf(msg);

        while (true) {
        }
    }
}
