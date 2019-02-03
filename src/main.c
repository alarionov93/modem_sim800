#include "init.h"
#include "modem.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/**
	* @brief  Timer of ms from lpc chip is turned on
*/
void SysTick_Handler(void)
{
	systick++;
}

void led_toggle(void)
{
	static int led_stat;
	if (led_stat == 0)
	{
		Chip_GPIO_WritePortBit(LPC_GPIO, 0, 8, true);
		led_stat = 1;
	} else 
	{
		Chip_GPIO_WritePortBit(LPC_GPIO, 0, 8, false);
		led_stat = 0;
	}
}



//uint8_t last_cmd;
//const char recv_answer;
//uint8_t check_answer(const char* buff, const char* expected_answer) 
//{
//	if (strstr(buff, expected_answer) != NULL)
//	{
//		return 1;
//	} else
//	{
//		return 0;
//	}
//} 
//#define CMD_0 "AT\r\n"
//#define CMD_1 "AT+CPIN?\r\n"
//#define CMD_2 "AT+CREG?\r\n"

// void task_process_modem()
// {
//	// memset(modem_tx_buffer, 0, (int) MODEM_TX_BUF_LEN);
//	
//	// char recv_buff[128];
//	// uint8_t next_cmd[128];
//	// if (modem_rx_flag) {
//		// modem_rx_flag = 0;
//		// for (int i = 0; i < modem_rx_count; i++)
//		// {
//			// recv_buff[i] = modem_rx_buffer[i];
//		// }
//		//recv_buff = (char*) modem_rx_buffer;

// 			if (!strncmp(last_cmd,CMD_0, 4))
// 			{
// 				if (check_answer(recv_buff, "OK") == 1) 
// 				{
// 					// ok
// 					memset(modem_tx_buffer, 0, (int) MODEM_TX_BUF_LEN);
// 					strncpy(next_cmd, "AT+CPIN?\r\n", 10);
// 					for (int i = 0; i < sizeof(next_cmd); i++)
// 					{
// 						modem_tx_buffer[i] = next_cmd[i];
// 					}
// 				} else 
// 				{
// 					// handle error
// 					module_state == M_STATE_ERROR;
// 				}
// 			}
// 			else if (!strcmp(last_cmd,CMD_1))
// 			{
// 				if (check_answer(recv_buff, "READY") == 1) 
// 				{
// 					// ok
// 					memset(modem_tx_buffer, 0, (int) MODEM_TX_BUF_LEN);
// //					next_cmd = "AT+CREG?\r\n";
// 					for (int i = 0; i < sizeof("AT+CREG?\r\n"); i++)
// 					{
// 						modem_tx_buffer[i] = "AT+CREG?\r\n"[i];
// 					}
// 				} else
// 				{
// 					// handle error
// 					module_state == M_STATE_ERROR;
// 				}
// 			}
// 			else if (!strcmp(last_cmd,CMD_2)) 
// 			{
// 				if (check_answer(recv_buff, "1") == 1) 
// 				{
// 					// ok
// 					memset(modem_tx_buffer, 0, (int) MODEM_TX_BUF_LEN);
// 					module_state == M_STATE_ONLINE;
// 				} else 
// 				{
// 					// handle error
// 					module_state == M_STATE_ERROR;
// 				}
// 			}
//		// modem_rx_count = 0;
//	// }
// }

//void rx_task()
//{
//	if (modem_rx_flag) {
//		modem_rx_flag = 0;
//		if (strncmp(modem_rx_buffer, "OK\r\n", 4))
//		{
//			// show led
//			Chip_GPIO_WritePortBit(LPC_GPIO, 0, 8, true);
//		} else
//		{
//			// hide led
//			Chip_GPIO_WritePortBit(LPC_GPIO, 0, 8, false);
//		}
//		modem_rx_count = 0;
//	}
//}


///**
//	* @brief  Check if module is on and working
//  	* @param  None
//	* @retval None
//  */
//void check_task(void)
//{
//	/* Check module 1 times per 5 sec */
//	if (TME_CHECK(module_tme, 5000)) {
//		TME_START(module_tme);
//		
//		if (mtask_state == MTASK_IDLE) {
//			
//			ping_modem();
//		}
//	}
//}

/**
 * @brief	Main UART program body
 * @return	Always returns 1
 */
int main(void)
{	
	init();	
	
	for (;;) {
		modem_task();
		sms_task();
		//network_task();		
		//led_task();
	}
}