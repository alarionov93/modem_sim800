#include "init.h"
#include "modem.h"
#include "chip.h"

#define MODEM_RX_BUF_LEN 128
#define MODEM_TX_BUF_LEN 128

uint8_t modem_rx_buffer[MODEM_RX_BUF_LEN];
volatile uint32_t modem_rx_count;
volatile uint32_t modem_rx_flag;

uint8_t modem_tx_buffer[MODEM_TX_BUF_LEN];
volatile uint32_t modem_tx_count;
volatile uint32_t modem_tx_idx;

volatile modem_state_t module_state;
mtask_state_t mtask_state;
sms_task_state_t sms_task_state;

volatile uint32_t module_tme;

//const char[][2] sms_text = {
//	"Test",
//	"Error"
//};

int cond_to_send;

void uart_tx_handler()
{
	uint32_t written = Chip_UART_Send(LPC_UART0, &modem_tx_buffer[modem_tx_idx], modem_tx_count);	
	
	modem_tx_idx += written;
	modem_tx_count -= written;
	
	if (!modem_tx_count) {
		// Отключаем прерывание tx
		Chip_UART_IntDisable(LPC_UART0, UART_IER_THREINT);
	}
}



void uart_rx_handler()
{
	while (LPC_UART0->LSR & UART_LSR_RDR) {
		uint8_t byte = LPC_UART0->RBR;
		
		if (modem_rx_count < MODEM_RX_BUF_LEN) {
			modem_rx_buffer[modem_rx_count++] = byte;
			if (byte == '\n') {
				modem_rx_flag = 1;
			}			
		} else {
			modem_rx_count = 0;
		}
	}
}

void UART_IRQHandler(void)
{
	switch (LPC_UART0->IIR & 0xe) {
		case UART_IIR_INTID_RDA:
		case UART_IIR_INTID_CTI:
			uart_rx_handler();
			break;
		case UART_IIR_INTID_THRE:
			uart_tx_handler();
			break;
	}
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
	
	mtask_state = MTASK_INIT;
	sms_task_state = SMS_TASK_INIT;
	/* Инициализация таймеров */
	TME_START(module_tme);
}

void modem_write(const char *cmd, uint32_t size)
{
	memcpy(modem_tx_buffer, cmd, size);
	modem_tx_idx = 0;
	modem_tx_count = size;
	uart_tx_handler();
	Chip_UART_IntEnable(LPC_UART0, UART_IER_THREINT);
}

void led_toggle(void);

void ping_modem()
{
	//led_toggle();
	modem_write("AT\r\n", 4);	
}

void modem_send_pin()
{
	modem_write("AT+CPIN=1234\r\n", 12);
}

void modem_send_sms(char sms_text[]) 
{
    int8_t answer;
		char end_symbol[1];
		sprintf (end_symbol, "%c", 26);
    char init_sms_str[] = "AT+CMGS=\"+79655766572\"";
	  // move sms sending code to this function
}

void modem_conf_sms()
{
	modem_write("AT+CMGF=1\r\n", 11); 				// sets the SMS mode to text
	task_delay(200);
	modem_write("AT+CSCS=\"GSM\"\r\n", 15);
	task_delay(200);
}

void modem_check_gsm(void)
{
	modem_write("AT+CREG?\r\n", 10);
	task_delay(100);
}

void modem_error_handler(void)
{
	switch (module_state)
	{
		case M_STATE_OFF:
			// turn on modem
			led_toggle();
			break;
		case M_STATE_ERROR:
			// init again
			led_toggle();
			break;
		case M_STATE_OFFLINE:
			// reboot, or reinit gsm
			led_toggle();
			break;
	}
}

void modem_task()
{		
	switch (mtask_state) 
	{
		case MTASK_INIT:
			if (TME_CHECK(module_tme, 5000))
			{
				mtask_state = MTASK_REG;
			}
		case MTASK_REG:
			modem_write("AT+CREG?\r\n", 10);
			task_delay(100);
			if (modem_rx_flag)
			{
				modem_rx_flag = 0;
				if (1) // check rx buffer for the desired answer
				{
					mtask_state = MTASK_CONF_SMS;
					module_state = M_STATE_ONLINE;
					TME_START(module_tme);
				} else
				{
					module_state = M_STATE_ERROR;
				}
			}
			break;

		case MTASK_CONF_SMS:
			modem_conf_sms();
			// Проверка условия приема ответа и смены состояния
			if (modem_rx_flag) 
			{
				modem_rx_flag = 0;
				// Проверка ответа
				if (1) { // check rx buffer for the desired answer
					mtask_state = MTASK_WORK;
					TME_START(module_tme);
				} else
				{
					module_state = M_STATE_ERROR;
				}
			}			
			break;
		case MTASK_WORK:							
			if (TME_CHECK(module_tme, 10000)) {
				TME_START(module_tme);
				ping_modem();
				if (modem_rx_flag) // modem is on
				{
					modem_rx_flag = 0;
					module_state = M_STATE_ON;
					modem_check_gsm();
					if(modem_rx_flag) // modem is online (check answer here)
					{
						modem_rx_flag = 0;
						module_state = M_STATE_ONLINE;
					} else
					{
						module_state = M_STATE_OFFLINE;
					}
				} else
				{
					module_state = M_STATE_OFF;
				}
			}
			break;
	}
}

void sms_task()
{
	const char delim_symbol[1] = {
		(char) 13
	};
	const char end_symbol[1] = {
		(char) 26
	};
	//sprintf (end_symbol, "%c", 26);
  	char init_sms_str[] = "AT+CMGS=\"+79655766572\"\r\n";

	switch (sms_task_state) 
	{
		case SMS_TASK_INIT:
			if(mtask_state == MTASK_WORK) // wait for modem registered to network
			{
				sms_task_state = SMS_TASK_WORK;
				cond_to_send = 1;
			}
			break;
		case SMS_TASK_WORK:
			if(cond_to_send == 1)
			{	
				cond_to_send = 0;
				modem_write(init_sms_str, strlen(init_sms_str));    // send the SMS number
				task_delay(100);
				modem_write(delim_symbol, strlen(delim_symbol));
				task_delay(100);
				modem_write("Test\r\n", 6);
				task_delay(100);
				modem_write(end_symbol, strlen(end_symbol));
				task_delay(500);
				if (0) // check rx buffer for the CMGS = 1 or ERROR
				{
					module_state = M_STATE_ERROR;
				}
			}
			break;
	}
}



