/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include "ell_adapter.h"

//=============================================================================
void MWADbg_DumpMemory(const uint8_t *buf, int len)
{
    int cnt;
    int wrap = 0;
    for (cnt = 0; cnt < len; cnt ++) {
        ELL_Printf("%02X ", buf[cnt]);
        wrap ++;
        if (wrap == 16) {
            ELL_Printf("\n");
            wrap = 0;
        }
    }
    if (wrap > 0) ELL_Printf("\n");
}

//=============================================================================
void MWADbg_PrintFrame(MWA_Frame_t *frame)
{
    ELL_Assert(frame != NULL);

    ELL_Printf("FT: %04X\n", frame->frame_type);
    ELL_Printf("CN:   %02X\n", frame->cmd_no);
    ELL_Printf("FN:   %02X\n", frame->frame_no);
    ELL_Printf("DL: %04X\n", frame->len);
    ELL_Printf("FD: \n");
    MWADbg_DumpMemory(frame->data, frame->len);
    ELL_Printf("FCC:  %02X\n", frame->fcc);
    ELL_Printf("\n");
}

/******************************** END-OF-FILE ********************************/
