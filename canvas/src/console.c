#include "console.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"

static void console_putchar(char c);
static void console_out(char c);
static void console_outDgt(char dgt);
static void console_divOut(unsigned int div);

static struct {
    char* bf;
    char buf[12];
    unsigned int num;
    char uc;
    char zs;
} console_data;

void console_init(void)
{
    // Initialize the UART.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Enable the UART peripheral for use.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    // Configure the UART for 115200, n, 8, 1
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200, (UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE | UART_CONFIG_WLEN_8));

    // Enable the UART operation.
    UARTEnable(UART0_BASE);
}

void console_printf(const char *fmt, ...)
{
    va_list va;
    char ch;
    char* p;

    va_start(va,fmt);

    while ((ch = *(fmt++))) {
        if (ch != '%') {
            console_putchar(ch);
        } else {
            char lz = 0;
            char w = 0;

            ch = *(fmt++);
            if (ch == '0') {
                ch = *(fmt++);
                lz = 1;
            }

            if ((ch >= '0') && (ch <= '9')) {
                w = 0;
                while (ch >= '0' && ch <= '9') {
                    w = (((w << 2) + w) << 1) + ch - '0';
                    ch = *fmt++;
                }
            }

            console_data.bf = console_data.buf;
            p = console_data.bf;
            console_data.zs = 0;

            switch (ch) {
                case 0:
                    goto abort;

                case 'u':
                case 'd':
                    console_data.num = va_arg(va, unsigned int);
                    if ((ch == 'd') && ((int) console_data.num < 0)) {
                        console_data.num = -(int) console_data.num;
                        console_out('-');
                    }

                    console_divOut(1000000000);
                    console_divOut(100000000);
                    console_divOut(10000000);
                    console_divOut(1000000);
                    console_divOut(100000);
                    console_divOut(10000);
                    console_divOut(1000);
                    console_divOut(100);
                    console_divOut(10);
                    console_outDgt(console_data.num);
                    break;

                case 'x':
                case 'X':
                    console_data.uc = ch == 'X';
                    console_data.num = va_arg(va, unsigned int);
                    console_divOut(0x10000000);
                    console_divOut(0x1000000);
                    console_divOut(0x100000);
                    console_divOut(0x10000);
                    console_divOut(0x1000);
                    console_divOut(0x100);
                    console_divOut(0x10);
                    console_outDgt(console_data.num);
                    break;

                case 'c':
                    console_out((char) (va_arg(va, int)));
                    break;

                case 's':
                    p = va_arg(va, char *);
                    break;

                case '%':
                    console_out('%');

                default:
                    break;
            }

            *console_data.bf = 0;
            console_data.bf = p;
            while (*console_data.bf++ && w > 0) {
                w--;
            }

            while (w-- > 0) {
                console_putchar(lz ? '0' : ' ');
            }

            while ((ch = *p++)) {
                console_putchar(ch);
            }
        }
    }

abort:
    console_putchar('\0');
    va_end(va);
}

static void console_putchar(char c)
{
    UARTCharPut(UART0_BASE, c);
}

static void console_out(char c)
{
    *console_data.bf++ = c;
}

static void console_outDgt(char dgt)
{
    console_out(dgt + (dgt < 10 ? '0' : (console_data.uc ? 'A' : 'a') - 10));
    console_data.zs = 1;
}

static void console_divOut(unsigned int div)
{
    unsigned char dgt = 0;

    while (console_data.num >= div) {
        console_data.num -= div;
        dgt++;
    }

    if (console_data.zs || (dgt > 0)) {
        console_outDgt(dgt);
    }
}
