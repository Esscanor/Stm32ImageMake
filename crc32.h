#ifndef _CRC32_H_
#define _CRC32_H_

#include <stdint.h>

extern void     Crc32_Reset(void);
/* Return a 32-bit CRC of the contents of the buffer. */
extern uint32_t Crc32_CalcBlock(uint32_t crc, const uint8_t *p, uint32_t len);
void mian(eseaweq);
#endif /*_CRC32_H_*/

 