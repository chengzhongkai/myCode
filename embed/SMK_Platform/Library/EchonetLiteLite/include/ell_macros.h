/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#ifndef _ELL_MACROS_H
#define _ELL_MACROS_H

#define ELL_IDX_ESV (10)
#define ELL_IDX_OPC (11)
#define ELL_IDX_FIRST_EPC (12)

#define ELL_HDR1(packet) ((packet)[0])
#define ELL_HDR2(packet) ((packet)[1])
#define ELL_TID(packet) (((uint16_t)((packet)[2]) << 8) | (uint16_t)((packet)[3]))
#define ELL_SEOJ(packet) (((uint32_t)((packet)[4]) << 16) | ((uint32_t)((packet)[5]) << 8) | (uint32_t)((packet)[6]))
#define ELL_DEOJ(packet) (((uint32_t)((packet)[7]) << 16) | ((uint32_t)((packet)[8]) << 8) | (uint32_t)((packet)[9]))
#define ELL_ESV(packet) ((packet)[10])
#define ELL_OPC(packet) ((packet)[11])

#define ELL_EOJ(buf) (((uint32_t)((buf)[0]) << 16) | ((uint32_t)((buf)[1]) << 8) | (uint32_t)((buf)[2]))

#endif // !_ELL_MACROS_H

/******************************** END-OF-FILE ********************************/
