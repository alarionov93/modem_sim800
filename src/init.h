#ifndef INIT_H
#define INIT_H

#include "chip.h"
#include "string.h"
#include <stdio.h>

/* Timer macros functions */
#define TME_START(tme) (tme = systick)
#define TME_CHECK(tme, t) (systick - (tme) >= t)

/* Main timer value */
extern volatile uint32_t systick;

void init(void);
void task_delay(uint32_t ms);

#endif
