/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include "echonetlitelite.h"
#include "connection.h"

//=============================================================================
void ELLDbg_PrintPacket(const char *prefix, IPv4Addr_t addr,
                        const uint8_t *buf, int len)
{
    int cnt;

    if (addr != 0) {
        ELL_Printf("%s %d.%d.%d.%d\n", prefix,
                   addr & 0xff, (addr >> 8) & 0xff,
                   (addr >> 16) & 0xff, (addr >> 24) & 0xff);
    } else {
        ELL_Printf("%s 224.0.23.0\n", prefix);
    }

    for (cnt = 0; cnt < len; cnt ++) {
        ELL_Printf("%02X ", buf[cnt]);
    }
    ELL_Printf("\nTID: %04X, SEOJ: %06X, DEOJ: %06X, ESV: %02X(%s)\n\n",
               ELL_TID(buf), ELL_SEOJ(buf), ELL_DEOJ(buf), ELL_ESV(buf),
               ELL_ESVString(ELL_ESV(buf)));
}

//=============================================================================
void ELLDbg_PrintProperty(const ELL_Property_t *prop)
{
    int cnt;

    ELL_Printf("EPC: %02X, PDC: %02X\n", prop->epc, prop->pdc);
    ELL_Printf("EDT: ");
    if (prop->edt != NULL) {
        for (cnt = 0; cnt < prop->pdc; cnt ++) {
            ELL_Printf("%02X ", prop->edt[cnt]);
        }
    }
    ELL_Printf("\n\n");
}

/******************************** END-OF-FILE ********************************/
