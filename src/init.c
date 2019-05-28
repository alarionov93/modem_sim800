#include "init.h"
#include "modem.h"
#include "modbus.h"

// 1_8 TX2 0_3 RX2
// 0_6 TX1 0_7 RX1

static void Init_UART_PinMux(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_6, (IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));	/* RXD0 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_7, (IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));	/* TXD0 */

	// !!! Через Chip_IPCON_PinMuxSet почему-то не работает (баг в библиотеке)
	// txd2 @ pio1_8
	*((uint32_t *)0x40044018) |= 0x3; 
	// rxd2 @ pio0_3
	*((uint32_t *)0x400440c0) |= 0x3;

	// txd1 @ pio 0_6
	*((uint32_t *)0x400440c4) |= 0x3;
	// rxd1 @ pio 0_7
	*((uint32_t *)0x400440c8) |= 0x3;
}


// Init all staff here
void init(void)
{
	SystemCoreClockUpdate();
	/* 1ms (1000Hz) interrupt */
	SysTick_Config(SystemCoreClock / 1000);

	// Включение клоков uart
	Chip_Clock_EnablePeriphClock(SYSCON_CLOCK_UART0); // включаем clock uart0
	LPC_SYSCON->UART0CLKDIV = 1;
	LPC_SYSCON->SYSAHBCLKCTRL |= (1ul << 19); // включаем clock uart1
	LPC_SYSCON->UART1CLKDIV = 1;
	LPC_SYSCON->SYSAHBCLKCTRL |= (1ul << 20); // включаем clock uart2
	LPC_SYSCON->UART2CLKDIV = 1; // !!! ВОТ ИЗ-ЗА ЭТОГО НЕ РАБОТАЛО. Chip_UART_Init делает это только для uart0

	// Включение клока блока IOCON
	Chip_Clock_EnablePeriphClock(SYSCON_CLOCK_IOCON);
	// Настройка функций на ножках, где uart
	Init_UART_PinMux();
	
	/* Setup UART2 */
	Chip_UART_Init(LPC_UART2);
	Chip_UART_SetBaud(LPC_UART2, 38400);
	
	Chip_UART_Init(LPC_UART1);
	Chip_UART_SetBaud(LPC_UART1, 9600);

	Chip_Clock_EnablePeriphClock(SYSCON_CLOCK_GPIO);
	
	Chip_GPIO_Init(LPC_GPIO);

	// Configure pwr pins of sim800 module
	Chip_GPIO_SetPinDIR(LPC_GPIO, 0, 1, true);
	Chip_GPIO_SetPinDIR(LPC_GPIO, 0, 8, true);
	Chip_GPIO_SetPinDIR(LPC_GPIO, 2, 6, true);
	Chip_GPIO_SetPinDIR(LPC_GPIO, 1, 5, true);

	/* Setup UART0 for modem 9600 */
	Chip_UART_Init(LPC_UART0);
	Chip_UART_SetBaud(LPC_UART0, 9600);
	Chip_UART_ConfigData(LPC_UART0, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));	
	Chip_UART_TXEnable(LPC_UART0);


	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
	Chip_UART_SetupFIFOS(LPC_UART0, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | UART_FCR_TRG_LEV1));
	Chip_UART_SetupFIFOS(LPC_UART1, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | UART_FCR_TRG_LEV1));
	Chip_UART_SetupFIFOS(LPC_UART2, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | UART_FCR_TRG_LEV1));

	/* Enable receive data and transmit data */
	Chip_UART_IntEnable(LPC_UART0, UART_IER_RBRINT);
	Chip_UART_IntEnable(LPC_UART1, UART_IER_RBRINT | UART_IER_THREINT);
	Chip_UART_IntEnable(LPC_UART2, UART_IER_RBRINT | UART_IER_THREINT);

	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(UART0_IRQn, 1);
	NVIC_SetPriority(UART1_IRQn, 1);
	NVIC_SetPriority(UART2_IRQn, 1);
	NVIC_EnableIRQ(UART0_IRQn);
	NVIC_EnableIRQ(UART1_IRQn);
	NVIC_EnableIRQ(UART2_IRQn);

	init_sim_800();
}
