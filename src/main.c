#include "init.h"
#include "modbus.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/**
	* @brief  Timer of ms from lpc chip is turned on
*/

volatile uint32_t systick = 0;

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

// void uart2_echo_task()
// {	
// 	if (LPC_UART2->LSR & UART_LSR_RDR) {
// 		LPC_UART2->THR = LPC_UART2->RBR;
// 	}
// }

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
		// modbus_net_process(&modbus);
		// modem_error_handler();
		// network_task();		
		// led_task();
	}
}
