#include "init.h"
#include "modem.h"
#include "modbus.h"
//#include "chip.h"

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
volatile gsm_state_t gsm_state;

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
volatile int module_err_cnt = 0;
volatile int gsm_check_cnt = 0;

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

void uart1_tx_handler()
{
	uint32_t written = Chip_UART_Send(LPC_UART1, &sbus_tx_buffer[sbus_tx_idx], sbus_tx_count);	
	
	sbus_tx_idx += written;
	sbus_tx_count -= written;
	
	if (!sbus_tx_count) {
		// Отключаем прерывание tx
		Chip_UART_IntDisable(LPC_UART1, UART_IER_THREINT);
	}
}



void uart1_rx_handler()
{
	while (LPC_UART1->LSR & UART_LSR_RDR) {
		uint8_t byte = LPC_UART1->RBR;
		
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

void UART1_IRQHandler(void)
{
	switch (LPC_UART1->IIR & 0xe) {
		case UART_IIR_INTID_RDA:
		case UART_IIR_INTID_CTI:
			uart1_rx_handler();
			break;
		case UART_IIR_INTID_THRE:
			uart1_tx_handler();
			break;
	}
}

void UART2_IRQHandler(void)
{
	switch (LPC_UART2->IIR & 0xe) {
		case UART_IIR_INTID_RDA:
		case UART_IIR_INTID_CTI:
			// uart2_rx_handler();
			modbus_int_rx(&modbus);
			break;
		case UART_IIR_INTID_THRE:
			modbus_int_tx(&modbus);
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
	//sms_task_state = SMS_TASK_INIT;
	/* Инициализация таймеров */
	TME_START(module_tme);
}

void modem_write(const char *cmd, uint32_t size)
{
	memset(modem_rx_buffer, 0, sizeof(modem_rx_buffer));// clean buffer before sending
	memcpy(modem_tx_buffer, cmd, size);
	modem_tx_idx = 0;
	modem_tx_count = size;
	uart_tx_handler();
	Chip_UART_IntEnable(LPC_UART0, UART_IER_THREINT);
}

void sbus_write(const char *cmd, uint32_t size)
{
	memset(sbus_tx_buffer, 0, sizeof(sbus_tx_buffer));// clean buffer before sending
	memcpy(sbus_tx_buffer, cmd, size);
	sbus_tx_idx = 0;
	sbus_tx_count = size;
	uart1_tx_handler();
	Chip_UART_IntEnable(LPC_UART1, UART_IER_THREINT);
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

void stat_led_on(void)
{
	Chip_GPIO_WritePortBit(LPC_GPIO, 0, 8, true);
}

void stat_led_off(void)
{
	Chip_GPIO_WritePortBit(LPC_GPIO, 0, 8, false);
}

void ping_modem(void)
{
	stat_led_on();
	modem_write("AT\r\n", 4);
}

void gsm_check(void)
{
	stat_led_on();
	modem_write("AT+CREG?\r\n", 10);
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

int modem_check_answer(const char *expected_answer)
{
	char *p;
	p = strstr(modem_rx_buffer, expected_answer);
	if(p)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int sbus_check_answer(const char *expected_answer)
{
	char *p;
	p = strstr(sbus_rx_buffer, expected_answer);
	if(p)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void change_mtask_state(mtask_state_t new_state)
{
	TME_START(module_tme);
	mtask_state = new_state;
}

// TODO: check red led on circuit, and show errors with it
//void modem_error_handler(void)
//{
//	if (TME_CHECK(module_tme, 5000)) {
//		switch (module_state)
//		{
//			case M_STATE_OFF:
//				// turn on modem
//				led_toggle();
//				break;
//			case M_STATE_ERROR:
//				// init again
//				led_toggle();
//				break;
//			case M_STATE_OFFLINE:
//				// reboot, or reinit gsm
//				led_toggle();
//				break;
//		}
//	}
//}

// TODO: move all same actions in recieve from module here
void do_recive_routines(void)
{
	
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
				if (modem_check_answer("0,1")) // check answer of modem
				{
					module_state = M_STATE_ON;
					modem_write("AT+CMGF=1\r\n;AT+CSCS=\"GSM\"\r\n", 26);
					modem_rx_count = 0;
					change_mtask_state(MTASK_CONF_SMS);
				}
				else
				{
					// change all states to ERROR TODO here: if 0,2 -> search of network is performing now, handle this situation
					module_state = M_STATE_ERROR;
					modem_rx_count = 0;
					change_mtask_state(MTASK_INIT);
				}
			}
			break;
		case MTASK_CONF_SMS:// check if AT+CMGF=1\r\nAT+CSCS=\"GSM\"\r\n sending is available
			// Проверка условия приема ответа и смены состояния
			if (modem_rx_count > 0 && TME_CHECK(recv_tme, 100)) 
			{
				// Проверка ответа
				if (modem_check_answer("OK")) { // check rx buffer for the desired answer
					// sms has been configured in this moment
					module_state = M_STATE_ON;
					change_mtask_state(MTASK_WORK);
				}
				else
				{
					module_state = M_STATE_ERROR;
					change_mtask_state(MTASK_INIT);
				}
				modem_rx_count = 0;
			}			
			break;
		case MTASK_WORK:							
			if (TME_CHECK(module_tme, 10000)) {
				if (gsm_check_cnt++ > 12 /*2 mins*/ && module_state == M_STATE_ON)
				{
					//here should be gsm coverage check
					gsm_check_cnt = 0;
					gsm_check();
					change_mtask_state(MTASK_CHECK_GSM);
				}
				else
				{
					ping_modem();
					change_mtask_state(MTASK_CHECK_PING);
				}
			}
			
			cond_to_send = registers_block[2]; // value to check if sms sending is on
			//cond_to_send = 1; //tmp
			if(cond_to_send == 1) // sms need to send
			{	
				//cond_to_send = 0;// tmp
				modem_send_sms("Test sms msg."); // send sms with 1 call of modem_write
				change_mtask_state(MTASK_CHECK_SMS_IS_SENT);
			}
			
			break;
		case MTASK_CHECK_GSM:
			if (modem_rx_count > 0 && TME_CHECK(recv_tme, 100)) // modem is online (add timer check here)
			{
				stat_led_off();
				if (modem_check_answer("0,1")) //check answer of sim800 here
				{
					//module_state = M_STATE_ONLINE; // check if this works? OR:
					module_state = M_STATE_ON;
					gsm_state = M_STATE_ONLINE;//(OFFLINE);1/0
					change_mtask_state(MTASK_WORK);
				}
				else
				{
					// error codes needed here and in other places with error!
					gsm_state = M_STATE_OFFLINE;
					module_err_cnt++;
					if (module_err_cnt > 10)
					{
						module_err_cnt = 0;
					}
					module_state = M_STATE_ERROR;
					change_mtask_state(MTASK_WORK);
				}
				modem_rx_count = 0;
			}
			break;
		case MTASK_CHECK_PING:
			if (modem_rx_count > 0 && TME_CHECK(recv_tme, 100)) // modem is on (add timer check here)
			{
				stat_led_off();
				if (modem_check_answer("OK"))
				{
					module_state = M_STATE_ON;
					change_mtask_state(MTASK_WORK);
				}
				else
				{
					module_state = M_STATE_ERROR;
				}
				modem_rx_count = 0;
			}
			break;
		case MTASK_CHECK_SMS_IS_SENT:

			if (modem_rx_count > 0 && TME_CHECK(recv_tme, 100))
			{
				if (modem_check_answer("CMGS=1")) // check rx buffer for the CMGS = 1
				{
					cond_to_send = 0; //tmp
					change_mtask_state(MTASK_WORK);
				}
				else
				{
					module_state = M_STATE_ERROR;
				}
				modem_rx_count = 0;
			}
			break;
	}
}


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
				if (sbus_check_answer("123\r\n"))
				{
					// cond_to_send = 1;
					// sms_task_state = SMS_TASK_WORK;
					sbus_rx_flag = 0;
					// sbus_write("ok.\r\n", 5);
					led_toggle();
				}
			}
			break;
		case SBUS_TASK_IDLE:
			break;
	}
}


