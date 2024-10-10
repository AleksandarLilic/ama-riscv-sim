#ifndef CSR_H
#define CSR_H

#define CSR_TOHOST 0x51e
#define CSR_MSCRATCH 0x340
#define CSR_MCYCLE 0xb00
#define CSR_MINSTRET 0xb02
#define CSR_MCYCLEH 0xb80
#define CSR_MINSTRETH 0xb82
// read-only CSRs
#define CSR_MISA 0x301
#define CSR_MHARTID 0xf14
// read only user CSRs
#define CSR_CYCLE 0xC00
#define CSR_TIME 0xC01
#define CSR_INSTRET 0xC02
#define CSR_CYCLEH 0xC80
#define CSR_TIMEH 0xC81
#define CSR_INSTRETH 0xC82

#endif
