// #include "chip.h"
// #include "board.h"
// #include "string.h"
#include "init.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

volatile int led_stat;

typedef enum MODULE_STATE {
	M_STATE_OFF, // 0
	M_STATE_ON, // 1 if module is turned on
	M_STATE_ERROR, // 2 if module is not answering or smth else
	M_STATE_INIT, // 3 while turning on pwr on module
	M_STATE_ONLINE, // 4 if module answered to AT with OK and registered to network
	M_STATE_OFFLINE, // 5 if module NOT answered to AT with OK or NOT registered to network
} ModuleState_t;

volatile ModuleState_t module_state = M_STATE_OFF;

#define MODEM_TX_BUF_LEN 128


// const char inst2[] = "Press a key to echo it back or ESC to quit\r\n";

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

uint8_t data[32]; // test uart buffer

/*****************************************************************************
 * Private functions
 ****************************************************************************/


/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
	* @brief  Timer of ms from lpc chip is turned on
  */
void SysTick_Handler(void)
{
	systick++;
}

/**
	* @brief  Delay task
	* @param  ms delay
  */
void task_delay(uint32_t ms)
{
	uint32_t ticks = systick;
	while (!TME_CHECK(ticks, ms))
        __NOP();
}

void init_sim_800(void)
{
	module_state = M_STATE_INIT;
	Chip_GPIO_WritePortBit(LPC_GPIO, 0, 8, false);
	Chip_GPIO_WritePortBit(LPC_GPIO, 2, 6, true); // set pwr to sim800
	Chip_GPIO_WritePortBit(LPC_GPIO, 1, 5, true);
	task_delay(500);
	Chip_GPIO_WritePortBit(LPC_GPIO, 1, 5, false);
	task_delay(1000);
	Chip_GPIO_WritePortBit(LPC_GPIO, 1, 5, true);
	while(Chip_GPIO_GetPinState(LPC_GPIO, 3, 3) != 1) {		
		__NOP();
		module_state = M_STATE_ERROR;
	}
	module_state = M_STATE_ON;
}

// void check_sim800_module(void)
// {
// 	// send AT to sim 800 here
// 	// led blinking here for test
// 	// if (led_stat == 0)
// 	// {
// 	// 	Chip_GPIO_WritePortBit(LPC_GPIO, 0, 8, true);
// 	// 	led_stat = 1;
// 	// } else 
// 	// {
// 	// 	Chip_GPIO_WritePortBit(LPC_GPIO, 0, 8, false);
// 	// 	led_stat = 0;
// 	// }
	
// 	/* Send initial messages */
// 	Chip_UART_SendBlocking(LPC_UART0, "AT\r\n", 4);
// 	// recieve module answer here?
// }

uint8_t modem_rx_buffer[128];
uint32_t modem_rx_count;
uint32_t modem_rx_flag;

uint8_t modem_tx_buffer[MODEM_TX_BUF_LEN];
uint32_t modem_tx_count = 128;
uint32_t modem_tx_idx = 0;

//memset(modem_tx_buffer, 0, (int) MODEM_TX_BUF_LEN);

uint8_t last_cmd[];
const char recv_answer;
uint8_t check_answer(const char* buff, const char* expected_answer) 
{
	if (strstr(buff, expected_answer) != NULL)
	{
		return 1;
	} else
	{
		return 0;
	}
} 
#define CMD_0 "AT\r\n"
#define CMD_1 "AT+CPIN?\r\n"
#define CMD_2 "AT+CREG?\r\n"

void task_process_modem()
{
	char recv_buff[128];
	uint8_t next_cmd[128];
	if (modem_rx_flag) {
		modem_rx_flag = 0;
		for (int i = 0; i < sizeof(recv_buff); i++)
		{
			recv_buff[i] = modem_rx_buffer[i];
		}
		//recv_buff = (char*) modem_rx_buffer;

			if (!strcmp(last_cmd,CMD_0))
			{
				if (check_answer(recv_buff, "OK") == 1) 
				{
					// ok
					memset(modem_tx_buffer, 0, (int) MODEM_TX_BUF_LEN);
					//next_cmd = ;
					for (int i = 0; i < sizeof("AT+CPIN?\r\n"); i++)
					{
						modem_tx_buffer[i] = "AT+CPIN?\r\n"[i];
					}
				} else 
				{
					// handle error
					module_state == M_STATE_ERROR;
				}
			}
			else if (!strcmp(last_cmd,CMD_1))
			{
				if (check_answer(recv_buff, "READY") == 1) 
				{
					// ok
					memset(modem_tx_buffer, 0, (int) MODEM_TX_BUF_LEN);
//					next_cmd = "AT+CREG?\r\n";
					for (int i = 0; i < sizeof("AT+CREG?\r\n"); i++)
					{
						modem_tx_buffer[i] = "AT+CREG?\r\n"[i];
					}
				} else
				{
					// handle error
					module_state == M_STATE_ERROR;
				}
			}
			else if (!strcmp(last_cmd,CMD_2)) 
			{
				if (check_answer(recv_buff, "1") == 1) 
				{
					// ok
					memset(modem_tx_buffer, 0, (int) MODEM_TX_BUF_LEN);
					module_state == M_STATE_ONLINE;
				} else 
				{
					// handle error
					module_state == M_STATE_ERROR;
				}
			}
		modem_rx_count = 0;
	}
}

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
	} else {
		// clean buffer to put in it new AT cmd ?
		// clean buffer in other place, only put new cmd ?
		// memset(modem_tx_buffer, '\0', MODEM_TX_BUF_LEN);
		for (int i = 0; i < MODEM_TX_BUF_LEN; i++)
		{
			if (modem_tx_buffer[i] != 0)
			{
				last_cmd[i] = modem_tx_buffer[i];
			}
		}
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
	* @brief  Check if module is on and working
  	* @param  None
	* @retval None
  */
void check_task(void)
{
	/* Check module 1 times per 5 sec */
	if (TME_CHECK(module_tme, 1000)) {
		TME_START(module_tme);

		task_process_modem();
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
		check_task();
		if (module_state == M_STATE_ONLINE)
		{
			Chip_GPIO_WritePortBit(LPC_GPIO, 0, 8, true);
		}
	}
}
