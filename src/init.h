#include "chip.h"
//#include "board.h"
#include "string.h"
#include <stdio.h>

/* Transmit and receive ring buffers */
// STATIC RINGBUFF_T txring, rxring;

/* Transmit and receive ring buffer sizes */
// #define UART_SRB_SIZE 128	/* Send */
// #define UART_RRB_SIZE 32	/* Receive */


/* Timer macros functions */
#define TME_START(tme) (tme = systick)
#define TME_CHECK(tme, t) (systick - (tme) >= t)

/* Main timer value */
extern volatile uint32_t systick;

/* Timer for module checking */
extern volatile uint32_t module_tme;

/* Transmit and receive buffers */
// static uint8_t rxbuff[UART_RRB_SIZE], txbuff[UART_SRB_SIZE];

void init(void);
