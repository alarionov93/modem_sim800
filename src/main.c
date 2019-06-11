#include "init.h"
#include "modbus.h"
// #include "modbus2.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/**
	* @brief  Timer of ms from lpc chip is turned on
*/

volatile uint32_t systick = 0;

// extern modbus_t modbus2;

void SysTick_Handler(void)
{
	systick++;
}

void task_delay(uint32_t ms)
{
	uint32_t ticks = systick;
	while (!TME_CHECK(ticks, ms))
        __NOP();
}

void uart1_echo_task()
{	
	if (LPC_UART1->LSR & UART_LSR_RDR) {
		LPC_UART1->THR = LPC_UART1->RBR;
	}
}

/**
 * @brief	Main UART program body
 * @return	Always returns 1
 */
int main(void)
{	
	init();	
	
	for (;;) {
		modem_task();
		// sbus_task();
		modbus_net_process(&modbus);
		//uart1_echo_task();
		modbus_net_process(&modbus2);
		//modem_error_handler();
		// led_task();
	}
}
