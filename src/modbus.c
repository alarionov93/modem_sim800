#include "init.h"
#include "modbus.h"
#include "crc.h"
#include <stddef.h>

// Коды функций MODBUS
#define MODBUS_READ_HOLDING_REGISTERS			0x03
#define MODBUS_READ_INPUT_REGISTERS				0x04
#define MODBUS_PRESET_SINGLE_REGISTER			0x06
#define MODBUS_PRESET_MULTIPLE_REGISTER			0x10

// Коды ошибок MODBUS
#define MODBUS_ILLEGAL_FUNCTION					0x01
#define MODBUS_ILLEGAL_DATA_ADDRESS				0x02
#define MODBUS_ILLEGAL_DATA_VALUE				0x03

// Приватные макросы
#define MODBUS_BUF_LEN(ctx)		((ctx)->buf_cnt - (ctx)->buf_idx)

typedef enum {
	MODBUS_STATE_RECIEVE = 0,
	MODBUS_STATE_WAIT_END,
	MODBUS_STATE_SEND,
	MODBUS_STATE_RESET_DE,
} modbus_state_t;

// Прототипы приватных функций
static void reset_buffer(modbus_t *ctx);
static void send_buffer(modbus_t *ctx);
static void throw_error(modbus_t *ctx, uint8_t error);

void modbus_int_rx(void *p)
{
	modbus_t *ctx = (modbus_t *)p;

	// Делать что-то только в состоянии приема
	if (ctx->state != MODBUS_STATE_RECIEVE) {
		// Игнорируем данные, пришедшие не вовремя
		ctx->read(NULL, MODBUS_BUFFER_SIZE);
		return;
	}

	// Запоминаем время прихода байта
	ctx->stamp = ctx->get_timer();

	// Читаем в буффер скольк сможем
	uint32_t cnt = ctx->read(&ctx->buf[ctx->buf_cnt], MODBUS_BUFFER_SIZE - ctx->buf_cnt);
	ctx->buf_cnt += cnt;

	// Увеличиваем счетчик принятых байт
	ctx->rx_bytes += cnt;

	// Проверка превышения размера буфера
	if (ctx->buf_cnt >= MODBUS_BUFFER_SIZE)
		ctx->state = MODBUS_STATE_WAIT_END;
}

void modbus_int_tx(void *p)
{
	modbus_t *ctx = (modbus_t *)p;

	// Делать что-то только в состоянии передачи
	if (ctx->state != MODBUS_STATE_SEND)
		return;

	// Отправляем сколько сможем за раз
	uint32_t cnt = ctx->write(&ctx->buf[ctx->buf_idx], MODBUS_BUF_LEN(ctx));
	ctx->buf_idx += cnt;

	// Увеличиваем счетчик отправленных байт
	ctx->tx_bytes += cnt;

	// Если закинули все, меняем состояние
	if (MODBUS_BUF_LEN(ctx) == 0) {
		reset_buffer(ctx);
		ctx->stamp = ctx->get_timer();
		ctx->state = MODBUS_STATE_RESET_DE;
	}
}

void modbus_net_process(modbus_t *ctx)
{
	switch (ctx->state) {
		case MODBUS_STATE_RECIEVE:
			// Если приняты данные и выполнено условие конца фрейма
			if (ctx->buf_cnt > 0 && (ctx->get_timer() - ctx->stamp >= 5)) {
				ctx->state = MODBUS_STATE_WAIT_END;
			}
			break;
		case MODBUS_STATE_WAIT_END:
			{
				uint16_t crc;

				// Проверяем адрес
				if (ctx->buf[0] != ctx->unit_id)
					goto reset_receive;

				// Проверяем CRC
				crc = f_CRC16Tbl(ctx->buf, ctx->buf_cnt - sizeof(crc), 0xffff);
				if ((ctx->buf[ctx->buf_cnt - 1] << 8 | ctx->buf[ctx->buf_cnt - 2]) != crc)
					goto reset_receive;

				// Анализ кода функции
				switch (ctx->buf[1]) {
					case MODBUS_READ_HOLDING_REGISTERS:
						{
							uint16_t addr, count;

							addr = (uint16_t)ctx->buf[2] << 8 | ctx->buf[3];
							count = (uint16_t)ctx->buf[4] << 8 | ctx->buf[5];

							// Неверный адрес и/или кол-во регистров
							if (addr + count > ctx->holding_regs_count) {
								throw_error(ctx, MODBUS_ILLEGAL_DATA_ADDRESS);
								break;
							}

							// Формируем буфер ответа
							ctx->buf_idx = 0;
							ctx->buf_cnt = 2;
							ctx->buf[ctx->buf_cnt++] = count * 2;	// кол-во байт данных в ответе

							for (; count > 0; addr++, count--) {
								ctx->buf[ctx->buf_cnt++] = ctx->holding_regs[addr] >> 8;  	// старший байт
								ctx->buf[ctx->buf_cnt++] = ctx->holding_regs[addr] & 0xff;  // младший байт
							}
						}
						break;
					case MODBUS_PRESET_MULTIPLE_REGISTER:
						{
							uint16_t addr, count;

							addr = (uint16_t)ctx->buf[2] << 8 | ctx->buf[3];
							count = (uint16_t)ctx->buf[4] << 8 | ctx->buf[5];

							// Неверный адрес и/или кол-во регистров
							if (addr + count > ctx->holding_regs_count) {
								throw_error(ctx, MODBUS_ILLEGAL_DATA_ADDRESS);
								break;
							}

							// Неправильные данные
							if (ctx->buf[6] != count * 2) {
								throw_error(ctx, MODBUS_ILLEGAL_DATA_VALUE);
								break;
							}

							// Сохраняем полученные данные
							uint32_t a = addr;
							for (uint32_t offset = 0; count > 0; a++, count--, offset += 2) {
								ctx->holding_regs[a] = (ctx->buf[7 + offset] << 8) | (ctx->buf[8 + offset]);
							}

							// Формируем буфер ответа
							ctx->buf_idx = 0;
							ctx->buf_cnt = 6; // id8 + nfunc8 + addr16 + count16
						}
						break;
					// Функция не поддерживается
					default:
						throw_error(ctx, MODBUS_ILLEGAL_FUNCTION);
				} // switch код функции

				// Отправляем сформированный буфер
				send_buffer(ctx);
				break;

reset_receive:
				reset_buffer(ctx);
				ctx->state = MODBUS_STATE_RECIEVE;
				break;
			}
		case MODBUS_STATE_RESET_DE:
			if (ctx->tx_idle != NULL && ctx->drv_ctl != NULL) {
				// Если трансмиттер пуст
				if (ctx->tx_idle()) {
					// Отключаем драйвер
					ctx->drv_ctl(0);
					// Переходим в состояние приема запроса
					ctx->state = MODBUS_STATE_RECIEVE;
				}
			} else {
				// Переходим в состояние приема запроса
				ctx->state = MODBUS_STATE_RECIEVE;
			}
			break;
	}
}

static void throw_error(modbus_t *ctx, uint8_t error)
{
	// Формируем буфер ответа
	ctx->buf_idx = 0;
	ctx->buf_cnt = 1;
	ctx->buf[ctx->buf_cnt++] |= 0x80;	// бит ошибки
	ctx->buf[ctx->buf_cnt++] = error; // код ошибки
}

static void send_buffer(modbus_t *ctx)
{
	uint16_t crc;

	// Добавляем CRC16 в конец пакета
	crc = f_CRC16Tbl(ctx->buf, ctx->buf_cnt, 0xffff);
	ctx->buf[ctx->buf_cnt++] = crc & 0xff;
	ctx->buf[ctx->buf_cnt++] = (crc >> 8);

	// Включаем драйвер
	if (ctx->drv_ctl != NULL)
		ctx->drv_ctl(1);

	// Переходим в состояние передачи ответа
	ctx->state = MODBUS_STATE_SEND;

	// Первоначально заполняем очередь
	modbus_int_tx(ctx);
}

static void reset_buffer(modbus_t *ctx)
{
	ctx->buf_cnt = 0;
	ctx->buf_idx = 0;
}
// 1. Реализовать эти функции:

// Должна возвращать значение таймера в мс
uint32_t get_timer_ms(void)
{
    return systick;
}

// Чтение из uart в dst
uint32_t uart2_read(uint8_t *dst, uint32_t length)
{
    // !!! Обязательно проверять на равенство dst == NULL и если это так то очищать приемную очередь uart
    uint32_t cnt = 0;
 
    while (LPC_UART2->LSR & UART_LSR_RDR && cnt < length) {
        if (dst) {
            dst[cnt++] = LPC_UART2->RBR;
        } else {
            LPC_UART2->RBR;
        }
    }
 
    return cnt;

}

// Запись из src в uart
uint32_t uart2_write(const uint8_t *src, uint32_t length)
{
    uint32_t cnt = 0;
 
    if (length > UART_TX_FIFO_SIZE)
        length = UART_TX_FIFO_SIZE;
 
    while (cnt < length) {
        LPC_UART2->THR = src[cnt++];
    }
 
    return cnt;
}

// Чтение из uart в dst
uint32_t uart1_read(uint8_t *dst, uint32_t length)
{
    // !!! Обязательно проверять на равенство dst == NULL
    // и если это так то очищать приемную очередь uart
    uint32_t cnt = 0;
 
    while (LPC_UART1->LSR & UART_LSR_RDR && cnt < length) {
        if (dst) {
            dst[cnt++] = LPC_UART1->RBR;
        } else {
            LPC_UART1->RBR;
        }
    }
 
    return cnt;

}

// Запись из src в uart
uint32_t uart1_write(const uint8_t *src, uint32_t length)
{
    uint32_t cnt = 0;
 
    if (length > UART_TX_FIFO_SIZE)
        length = UART_TX_FIFO_SIZE;
 
    while (cnt < length) {
        LPC_UART1->THR = src[cnt++];
    }
 
    return cnt;
}

uint16_t registers_block2[HOLDING_REGS_COUNT];

modbus_t modbus2 = {
    .get_timer = get_timer_ms,
    .read = uart2_read,
    .write = uart2_write,

    .tx_idle = NULL,
    .drv_ctl = NULL,

    .unit_id = 34,

    .holding_regs = registers_block2,
    .holding_regs_count = HOLDING_REGS_COUNT,
};

uint16_t registers_block[HOLDING_REGS_COUNT];

modbus_t modbus = {
    .get_timer = get_timer_ms,
    .read = uart1_read,
    .write = uart1_write,

    .tx_idle = NULL,
    .drv_ctl = NULL,

    .unit_id = 33,

    .holding_regs = registers_block,
    .holding_regs_count = HOLDING_REGS_COUNT,
};
