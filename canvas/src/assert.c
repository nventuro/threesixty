#include "assert.h"

#include "utils/uartstdio.h"

void assert(bool condition, char *msg)
{
    if (!condition) {
        UARTprintf("ASSERTION FAILED: ");
        UARTprintf(msg);

        while (true) {
        }
    }
}