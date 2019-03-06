#include "init.h"
#include "modem.h"
#include "chip.h"

#define MODEM_RX_BUF_LEN 128
#define MODEM_TX_BUF_LEN 128

#define SBUS_RX_BUF_LEN 128
#define SBUS_TX_BUF_LEN 128

volatile uint32_t recv_tme;

uint8_t modem_rx_buffer[MODEM_RX_BUF_LEN];
volatile uint32_t modem_rx_count;
volatile uint32_t modem_rx_flag;

uint8_t modem_tx_buffer[MODEM_TX_BUF_LEN];
volatile uint32_t modem_tx_count;
volatile uint32_t modem_tx_idx;

volatile modem_state_t module_state;
volatile mtask_state_t mtask_state;
volatile sms_task_state_t sms_task_state;

volatile uint32_t module_tme;

// sbus here: move all for sensors bus to sbus.c and sbus.h
uint8_t sbus_rx_buffer[SBUS_RX_BUF_LEN];
volatile uint32_t sbus_rx_count;
volatile uint32_t sbus_rx_flag;

uint8_t sbus_tx_buffer[SBUS_TX_BUF_LEN];
volatile uint32_t sbus_tx_count;
volatile uint32_t sbus_tx_idx;

volatile sbus_task_state_t sbus_task_state;

volatile uint32_t sbus_tme;

const char *phone_numbers[1] = {"+79655766572"};

const char *sms_text[2] = {
	"Test sms message",
	"Error sms message"
};

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
			TME_START(recv_tme);
			//if (byte == '\n') {
			//	modem_rx_flag = 1;
			//}			
		} 
		// else {
		// 	modem_rx_count = 0;
		// }
	}
}

void uart2_tx_handler()
{
	uint32_t written = Chip_UART_Send(LPC_UART2, &sbus_tx_buffer[sbus_tx_idx], sbus_tx_count);	
	
	sbus_tx_idx += written;
	sbus_tx_count -= written;
	
	if (!sbus_tx_count) {
		// Отключаем прерывание tx
		Chip_UART_IntDisable(LPC_UART2, UART_IER_THREINT);
	}
}



void uart2_rx_handler()
{
	while (LPC_UART2->LSR & UART_LSR_RDR) {
		uint8_t byte = LPC_UART2->RBR;
		
		if (sbus_rx_count < SBUS_RX_BUF_LEN) {
			sbus_rx_buffer[sbus_rx_count++] = byte;
			if (byte == '\n') {
				sbus_rx_flag = 1;
			}			
		} else {
			sbus_rx_count = 0;
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

void UART2_IRQHandler(void)
{
	switch (LPC_UART2->IIR & 0xe) {
		case UART_IIR_INTID_RDA:
		case UART_IIR_INTID_CTI:
			uart2_rx_handler();
			break;
		case UART_IIR_INTID_THRE:
			uart2_tx_handler();
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

void sbus_write(const char *cmd, uint32_t size)
{
	memcpy(sbus_tx_buffer, cmd, size);
	sbus_tx_idx = 0;
	sbus_tx_count = size;
	uart2_tx_handler();
	Chip_UART_IntEnable(LPC_UART2, UART_IER_THREINT);
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
	uint8_t msg_buf[50];
    
  char phone_number[] = "+79655766572";
  	// char sms_text[] = "Test";
  sprintf(msg_buf, "AT+CMGS=\"%s\"\xD%s\r\n\x19", phone_number, sms_text);
	modem_write(msg_buf, strlen(msg_buf));    // send the SMS
}

void modem_check_gsm(void)
{
	modem_write("AT+CREG?\r\n", 10);
}

void modem_check_answer(void)
{

}

void change_mtask_state(mtask_state_t new_state)
{
	TME_START(module_tme);
	mtask_state = new_state;
}

void modem_error_handler(void)
{
	if (TME_CHECK(module_tme, 5000)) {
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
}

void modem_task()
{		
	switch (mtask_state) 
	{
		case MTASK_INIT:
			if (TME_CHECK(module_tme, 10000))
			{
				modem_write("AT+CREG?\r\n", 10);
				change_mtask_state(MTASK_CHECK_REG);
				//cond_to_send = 1;
			}
			break;
		case MTASK_CHECK_REG:
			if (modem_rx_count > 0 && TME_CHECK(recv_tme, 100))
			{
				if (1) // check answer of modem
				{
					modem_write("AT+CMGF=1\r\n;AT+CSCS=\"GSM\"\r\n", 26);
					change_mtask_state(MTASK_CONF_SMS);
				}

				modem_rx_count = 0;
				// else
				// {
				// 	// change all states to ERROR
				// 	module_state = M_STATE_ERROR;
				// }
			}
			break;
		case MTASK_CONF_SMS:// check if AT+CMGF=1\r\nAT+CSCS=\"GSM\"\r\n sending is available
			// Проверка условия приема ответа и смены состояния
			if (modem_rx_count > 0 && TME_CHECK(recv_tme, 100)) 
			{
				// Проверка ответа
				if (1) { // check rx buffer for the desired answer
					// sms has been configured in this moment
					mtask_state = MTASK_WORK;
				}
				// else
				// {
				// 	module_state = M_STATE_ERROR;
				// }
				modem_rx_count = 0;
			}			
			break;
		case MTASK_WORK:							
			if (TME_CHECK(module_tme, 10000)) {
				ping_modem();
				change_mtask_state(MTASK_CHECK_PING);
			}

			if(cond_to_send == 1) // sms need to send
			{	
				cond_to_send = 0;
				modem_send_sms("Test sms msg."); // send sms with 1 call of modem_write
				change_mtask_state(MTASK_CHECK_SMS_IS_SENT);
			}
			
			break;
		case MTASK_CHECK_PING:
			if (modem_rx_count > 0 && TME_CHECK(recv_tme, 100)) // modem is on (add timer check here)
			{
				// modem_rx_count shows bytes count from answer
				if (1)
				{
					module_state = M_STATE_ON;
					change_mtask_state(MTASK_WORK);
				}
				modem_rx_count = 0;
			}
			break;
		case MTASK_CHECK_SMS_IS_SENT:

			if (modem_rx_count > 0 && TME_CHECK(recv_tme, 100))
			{
				if (1/*strncmp(modem_rx_buffer, "CMGS = \'1\'", 10)*/) // check rx buffer for the CMGS = 1
				{
					change_mtask_state(MTASK_WORK);
				}
				modem_rx_count = 0;
			}
			break;
	}
}

// void sms_task()
// {
// 	const char delim_symbol[1] = {
// 		(char) 13
// 	};
// 	const char end_symbol[1] = {
// 		(char) 26
// 	};
// 	//sprintf (end_symbol, "%c", 26);
//   	char init_sms_str[] = "AT+CMGS=\"+79655766572\"\r\n";

// 	switch (sms_task_state) 
// 	{
// 		case SMS_TASK_INIT:
// 			if(mtask_state == MTASK_WORK) // wait for modem registered to network
// 			{
// 				sms_task_state = SMS_TASK_WORK;
// 				cond_to_send = 0;
// 			}
// 			break;
// 		case SMS_TASK_WORK:
// 			if(cond_to_send == 1)
// 			{	
// 				cond_to_send = 0;
// 				//modem_write(init_sms_str, strlen(init_sms_str));    // send the SMS number
// 				//task_delay(100);
// 				//modem_write(delim_symbol, strlen(delim_symbol));
// 				//task_delay(100);
// 				//modem_write("Test\r\n", 6);
// 				//task_delay(100);
// 				//modem_write(end_symbol, strlen(end_symbol));
// 				//task_delay(500);
// 				if (strncmp(modem_rx_buffer, "CMGS = \'1\'", 10)) // check rx buffer for the CMGS = 1
// 				{
// 					sms_task_state = SMS_TASK_IDLE;
// 				} else
// 				{
// 					module_state = M_STATE_ERROR;
// 				}
// 			} else
// 			{
// 				sms_task_state = SMS_TASK_IDLE;
// 			}
// 			break;
// 		case SMS_TASK_IDLE:
// 			if (sbus_rx_flag == 1)
// 			{
// 				if (0/* recieved cmd to send sms from sensors bus */)
// 				{
// 					cond_to_send = 1;
// 					sms_task_state = SMS_TASK_WORK;
// 				}
// 			}
// 			break;
// 	}
// }

// move to sbus.c
void sbus_task()
{
	switch(sbus_task_state)
	{
		case SBUS_TASK_INIT:
			sbus_task_state = SBUS_TASK_WORK;
			break;
		case SBUS_TASK_WORK:
			if (sbus_rx_flag == 1)
			{
				if (1/* recieved cmd to send sms from sensors bus */)
				{
					// cond_to_send = 1;
					// sms_task_state = SMS_TASK_WORK;
					sbus_rx_flag = 0;
					sbus_write("ok.\r\n", 5);
					led_toggle();
				}
			}
			break;
		case SBUS_TASK_IDLE:
			break;
	}
}



