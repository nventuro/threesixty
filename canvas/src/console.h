#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <stdarg.h>

void console_init(void);

/**
 * @brief      printf substitute, with reduced code size and stack depth, but
 *             with a couple less features.
 *
 *             The following formats are supported: 'd', 'u', 'c', 's', 'x',
 *             'X'. Padding and field with (limited to 255) are also supported.
 *
 *             Also, this function is not reentrant, so it must be called from
 *             only one execution thread.
 *
 * @param[in]  fmt        printf's fmt parameter.
 * @param[in]  <unnamed>  printf's extra paramters.
 */
void console_printf(const char *fmt, ...);

#endif
