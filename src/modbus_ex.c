////// Пример использования ModBUS

////#include "modbus_ex.h"
////#include <stdint.h>
////// 1. Реализовать эти функции:

////// Должна возвращать значение таймера в мс
////uint32_t get_timer_ms(void)
////{
////    return systick;
////}

////// Чтение из uart в dst
////uint32_t uart2_read(uint8_t *dst, uint32_t length)
////{
////    // !!! Обязательно проверять на равенство dst == NULL и если это так то очищать приемную очередь uart
////    uint32_t cnt = 0;
//// 
////    while (LPC_UART2->LSR & UART_LSR_RDR && cnt < length) {
////        if (dst) {
////            dst[cnt++] = LPC_UART2->RBR;
////        } else {
////            LPC_UART2->RBR;
////        }
////    }
//// 
////    return cnt;

////}

////// Запись из src в uart
////uint32_t uart2_write(const uint8_t *src, uint32_t length)
////{
////    uint32_t cnt = 0;
//// 
////    if (length > UART_TX_FIFO_SIZE)
////        length = UART_TX_FIFO_SIZE;
//// 
////    while (cnt < length) {
////        LPC_UART2->THR = src[cnt++];
////    }
//// 
////    return cnt;
////}

////// 2. Описать регистры и контекст ModBUS
////// #define HOLDING_REGS_COUNT 16
////// uint16_t registers_block[HOLDING_REGS_COUNT];

////// modbus_t modbus = {
//////     .get_timer = get_timer_ms,
//////     .read = uart2_read,
//////     .write = uart2_write,

//////     .tx_idle = NULL,
//////     .drv_ctl = NULL,

//////     .unit_id = 33,

//////     .holding_regs = registers_block,
//////     .holding_regs_count = HOLDING_REGS_COUNT,
////// };


////// Вызывать в основном цикле
//////modbus_net_process(&modbus);

////// Вызывать в ISR
//////void modbus_int_rx(&modbus);
//////void modbus_int_tx(&modbus);
