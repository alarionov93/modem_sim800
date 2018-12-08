#include "init.h"

/* Main timer value */
volatile uint32_t systick = 0;

/* Timer for module checking */
volatile uint32_t module_tme = 0;


static void Init_UART_PinMux(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_6, (IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));	/* RXD */
	Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_7, (IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));	/* TXD */
}


// Init all staff here
void init(void)
{
	/* 1ms (1000Hz) interrupt */
	SysTick_Config(SystemCoreClock / 1000);
//	Board_Init();
	Init_UART_PinMux();
//	Board_LED_Set(0, false);
	
	Chip_Clock_EnablePeriphClock(SYSCON_CLOCK_IOCON);
	Chip_Clock_EnablePeriphClock(SYSCON_CLOCK_GPIO);
	
	Chip_GPIO_Init(LPC_GPIO);

	// Configure pwr pins of sim800 module
	Chip_GPIO_SetPinDIR(LPC_GPIO, 0, 8, true);
	Chip_GPIO_SetPinDIR(LPC_GPIO, 2, 6, true);
	Chip_GPIO_SetPinDIR(LPC_GPIO, 1, 5, true);

	/* Setup UART for 9600 */
	Chip_UART_Init(LPC_UART0);
	Chip_UART_SetBaud(LPC_UART0, 9600);
	Chip_UART_ConfigData(LPC_UART0, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_UART0, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_UART0);

	/* Before using the ring buffers, initialize them using the ring
	   buffer init function */
	//RingBuffer_Init(&rxring, rxbuff, 1, UART_RRB_SIZE);
	//RingBuffer_Init(&txring, txbuff, 1, UART_SRB_SIZE);

	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
	Chip_UART_SetupFIFOS(LPC_UART0, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_UART0, (UART_IER_RBRINT | UART_IER_RLSINT));

	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(UART0_IRQn, 1);
	NVIC_EnableIRQ(UART0_IRQn);

	/* Инициализация таймеров */
	TME_START(module_tme);

	init_sim_800();
}
