#ifndef CRC_H
#define CRC_H

#include <stdint.h>

uint32_t    f_CRC16Tbl(void const *buf, uint32_t sizeBuf, uint32_t crc);
uint32_t    f_CRC16Calc(void const *buf, uint32_t sizeBuf, uint32_t crc);
void    f_CRC16Tbl_Next(uint8_t byte, uint32_t *crc);
uint8_t     f_LRC(void const *buf, uint32_t sizeBuf, uint8_t lrc);

#endif
