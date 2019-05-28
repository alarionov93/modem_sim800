#ifndef MODBUS_H
#define MODBUS_H

#include <stdint.h>

// Размер буфера
#define MODBUS_BUFFER_SIZE 256

/* ModBUS context */
typedef struct {
	// public interface
	uint32_t (*read)(uint8_t *dst, uint32_t length);
	uint32_t (*write)(const uint8_t *src, uint32_t length);
	uint32_t (*get_timer)();
	uint32_t (*tx_idle)();
	void (*drv_ctl)(uint32_t enable);

	uint32_t unit_id;
	uint16_t *holding_regs;
	uint32_t holding_regs_count;

	// private
	uint32_t rx_bytes;
	uint32_t tx_bytes;
	volatile uint32_t state;
	volatile uint32_t stamp;
	volatile uint32_t buf_cnt;
	volatile uint32_t buf_idx;
	uint8_t buf[MODBUS_BUFFER_SIZE];
} modbus_t;

void modbus_net_process(modbus_t *ctx);
void modbus_int_rx(void *p);
void modbus_int_tx(void *p);

#define HOLDING_REGS_COUNT 16
extern uint16_t registers_block[];
extern uint16_t registers_block2[];

extern modbus_t modbus;
extern modbus_t modbus2;

uint32_t get_timer_ms(void);

uint32_t uart2_read(uint8_t *dst, uint32_t length);

uint32_t uart2_write(const uint8_t *src, uint32_t length);

uint32_t uart1_read(uint8_t *dst, uint32_t length);

uint32_t uart1_write(const uint8_t *src, uint32_t length);

#endif
