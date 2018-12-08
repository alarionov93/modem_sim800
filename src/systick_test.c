#include "board.h"

#define TICKRATE_HZ     (1000)	/* 1000 ticks per second */

uint8_t modem_rx_buffer[128];
uint32_t modem_rx_count;
uint32_t modem_rx_flag;

uint8_t modem_tx_buffer[128];
uint32_t modem_tx_count = 128;
uint32_t modem_tx_idx = 0;


void uart_rx_handler()
{
	while (LPC_UART0->LSR & UART_LSR_RDR) {
		uint8_t byte = LPC_UART0->RBR;
		
		if (modem_rx_count < 128 - 1) {			
			modem_rx_buffer[modem_rx_count++] = byte;
			if (byte == '\n') {
				modem_rx_flag = 1;
			}			
		}
	}
}

void send_to_modem()
{
	uint32_t written = Chip_UART_Send(LPC_UART0, &modem_tx_buffer[modem_tx_idx], modem_tx_count);
	modem_tx_idx += written;
	modem_tx_count -= written;
}

void uart_tx_handler()
{
	if (modem_tx_count > 0) {		
		send_to_modem();
	}
}


void UART_IRQHandler(void)
{			
	if (LPC_UART0->IIR == UART_IIR_INTID_RDA) {
		uart_rx_handler();
	}
	if (LPC_UART0->IIR == UART_IIR_INTID_THRE) {
		uart_tx_handler();
	}
}

/**
 * @brief	Handle interrupt from SysTick timer
 * @return	Nothing
 */
static volatile uint32_t tick_ct = 0;
void SysTick_Handler(void)
{
	if ((tick_ct++ % 1000) == 0) {					
		if (Chip_GPIO_GetPinState(LPC_GPIO, 0, 8))		
			Chip_GPIO_WritePortBit(LPC_GPIO, 0, 8, false);
		else
			Chip_GPIO_WritePortBit(LPC_GPIO, 0, 8, true);				
	}
}

void task_process_modem()
{
	if (modem_rx_flag) {
		modem_rx_flag = 0;
		
		// work with buffer
		
		modem_rx_count = 0;
	}
}

/**
 * @brief	main routine for blinky example
 * @return	Function should not exit.
 */
int main(void)
{
	SystemCoreClockUpdate();

	Chip_Clock_EnablePeriphClock(SYSCON_CLOCK_IOCON);
	Chip_Clock_EnablePeriphClock(SYSCON_CLOCK_GPIO);
	
	Chip_GPIO_Init(LPC_GPIO);
	Chip_GPIO_SetPinDIR(LPC_GPIO, 0, 8, true);
	
	Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_6, (IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));	/* RXD */
	Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_7, (IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));	/* TXD */
	
	
	/* Setup UART for 115.2K8N1 */
	Chip_UART_Init(LPC_UART0);
	Chip_UART_SetBaud(LPC_UART0, 115200);
	Chip_UART_ConfigData(LPC_UART0, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_UART0, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV0));
	Chip_UART_TXEnable(LPC_UART0);
	
	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_UART0, UART_IER_RBRINT);
	//Chip_UART_IntEnable(LPC_UART0, UART_IER_THREINT);
	NVIC_EnableIRQ(UART0_IRQn);
	
	/* Enable and setup SysTick Timer at a periodic rate */
	SysTick_Config(SystemCoreClock / TICKRATE_HZ);

	/* LED is toggled in interrupt handler */
	while (1) {
		//__WFI();
		task_timer();
		
		task_process_modem();
	}
}
