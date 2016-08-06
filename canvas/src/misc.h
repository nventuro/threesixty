#ifndef _MISC_H_
#define _MISC_H_

#include <stdint.h>

#define BIT(n) (1 << (n))

void misc_init(void);

uint32_t misc_getSysMS(void);

void misc_busyWaitMS(uint32_t ms);

void misc_enterCritical(void);

void misc_exitCritical(void);

#endif
