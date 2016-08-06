#include "assert.h"

#include "inc/hw_types.h"

#include "driverlib/interrupt.h"

#include "utils/uartstdio.h"

void assert(bool condition, char *msg)
{
    if (!condition) {
        IntMasterDisable();

        UARTprintf("ASSERTION FAILED: ");
        UARTprintf(msg);

        while (true) {
        }
    }
}
