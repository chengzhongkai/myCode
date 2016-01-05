/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include "echonetlitelite.h"

//=============================================================================
int gNumELLObjects = 0;
ELL_Object_t gELLObjectAlloc[ELL_MAX_OBJECTS];

#ifdef REGISTER_NODE_PROFILE
extern uint8_t gEDTWork[ELL_EDT_WORK_SIZE];
static bool_t gNodeProfileRegistered = FALSE;
#endif

//=============================================================================
// Initialize ECHONET Lite Objects Workspace
//=============================================================================
void ELL_InitObjects(void)
{
    int cnt;

    for (cnt = 0; cnt < ELL_MAX_OBJECTS; cnt ++) {
        gELLObjectAlloc[cnt].eoj = 0;
        gELLObjectAlloc[cnt].num_epc = 0;
        gELLObjectAlloc[cnt].epc_list = NULL;
    }

#ifdef REGISTER_NODE_PROFILE
    gNodeProfileRegistered = FALSE;
#endif
}

//=============================================================================
static ELL_EPCDefine_t *ELL_findEPC(ELL_Object_t *obj, uint8_t epc)
{
#if 0
    int cnt;
    ELL_EPCDefine_t *item;

    ELL_Assert(obj != NULL);

    // --------------------------------------------- Search EPC Define Item ---
    item = obj->epc_list;
    for (cnt = 0; cnt < obj->num_epc; cnt ++) {
        if (item->epc == epc) return (item);
        item ++;
    }
#else
    ELL_EPCDefine_t *item;
    uint8_t min = 0, max = obj->num_epc;
    uint8_t idx = (max - min) / 2;
    uint8_t next;

    ELL_Assert(obj != NULL);

    // --------------------------------------------- Search EPC Define Item ---
    while (min < max) {
        ELL_Assert(idx < obj->num_epc);

        item = &obj->epc_list[idx];
        if (epc == item->epc) return (item);

        if (epc < item->epc) {
            max = idx;
        } else {
            min = idx;
        }
        next = ((max - min) / 2) + min;
        if (next == idx) break;
        idx = next;
    }
#endif

    return (NULL);
}

//=============================================================================
static uint8_t ELL_createPropertyMap(ELL_Object_t *obj,
                                     uint8_t flag, uint8_t edt[17])
{
    static const uint8_t bit[8] = {
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
    };

    int cnt, cnt2, num, ptr, bit_idx;
    uint8_t tmp;

    ELL_Assert(obj != NULL);

    // ------------------------------------------------ Count EPC with flag ---
    num = 0;
    for (cnt = 0; cnt < obj->num_epc; cnt ++) {
        if ((obj->epc_list[cnt].flag & flag) != 0) {
            num ++;
        }
    }
    if (flag == EPC_FLAG_RULE_GET) {
        num += 3;
    }

    // ------------------------------------------------ Create Property Map ---
    edt[0] = num;
    if (num < 16) {
        // --------------------------------------------- Make Property List ---
        ptr = 1;
        for (cnt = 0; cnt < obj->num_epc; cnt ++) {
            if ((obj->epc_list[cnt].flag & flag) != 0) {
                edt[ptr ++] = obj->epc_list[cnt].epc;
            }
        }
        if (flag == EPC_FLAG_RULE_GET) {
            edt[ptr ++] = 0x9d;
            edt[ptr ++] = 0x9e;
            edt[ptr ++] = 0x9f;
        }
        // sort
        if (num >= 2) {
            for (cnt = 1; cnt < num; cnt ++) {
                for (cnt2 = num; cnt2 > cnt; cnt2 --) {
                    if (edt[cnt2 - 1] > edt[cnt2]) {
                        tmp = edt[cnt2 -1];
                        edt[cnt2 - 1] = edt[cnt2];
                        edt[cnt2] = tmp;
                    }
                }
            }
        }
        return (num + 1);
    }
    else {
        // ------------------------------------------- Make Property Bitmap ---
        MEM_Set(&edt[1], 0, 16);

        for (cnt = 0; cnt < obj->num_epc; cnt ++) {
            if ((obj->epc_list[cnt].flag & flag) != 0) {
                ptr = obj->epc_list[cnt].epc & 0x0f;
                bit_idx = (obj->epc_list[cnt].epc >> 4) - 8;
                if (bit_idx >= 0) {
                    edt[ptr + 1] |= bit[bit_idx];
                }
            }
        }
        if (flag == EPC_FLAG_RULE_GET) {
            edt[0x0d + 1] |= 0x02; // 0x9d
            edt[0x0e + 1] |= 0x02; // 0x9e
            edt[0x0f + 1] |= 0x02; // 0x9f
        }
        return (17);
    }
}

//=============================================================================
static bool_t ELL_defineEPC(ELL_Object_t *obj,
                            uint8_t epc, uint8_t pdc, const uint8_t *edt,
                            uint8_t flag)
{
    int cnt;
    ELL_EPCDefine_t *epc_def;

    // ELL_Assert(obj != NULL && pdc > 0 && edt != NULL);
    ELL_Assert(obj != NULL);

#if 1
    ELL_Printf("Define EPC(%02X), %d,", epc, pdc);
    for (cnt = 0; cnt < pdc; cnt ++) {
        ELL_Printf(" %02X", edt[cnt]);
    }
    ELL_Printf(", Access Rule(%02X)\n", flag);
#endif

    for (cnt = 0; cnt < obj->num_epc; cnt ++) {
        if (obj->epc_list[cnt].epc == 0) {
            epc_def = &obj->epc_list[cnt];

            epc_def->epc = epc;
            epc_def->pdc = pdc;
#if (ELL_USE_FIXED_PDC != 0)
            if (edt == NULL) {
                epc_def->edt = NULL;
            } else {
                epc_def->edt = MEM_Alloc(pdc);
                ELL_Assert(epc_def->edt != NULL);
                MEM_Copy(epc_def->edt, edt, pdc);
            }
#else
            if (edt == NULL) {
                epc_def->edt = NULL;
            } else {
                epc_def->edt = MEM_Alloc(pdc + 1);
                ELL_Assert(epc_def->edt != NULL);
                epc_def->edt[0] = pdc;
                MEM_Copy(&epc_def->edt[1], edt, pdc);
            }
#endif
            epc_def->flag = flag;
            epc_def->range = ELL_PROP_BINARY;

            return (TRUE);
        }
    }

    return (FALSE);
}

//=============================================================================
static void ELL_sortEPC(ELL_Object_t *obj)
{
    int cnt1, cnt2;
    ELL_EPCDefine_t tmp;
    ELL_EPCDefine_t *epc1, *epc2;

    for (cnt1 = 0; cnt1 <(obj->num_epc - 1); cnt1 ++) {
        for (cnt2 = (obj->num_epc - 1); cnt2 > cnt1; cnt2 --) {
            epc1 = &obj->epc_list[cnt2 - 1];
            epc2 = &obj->epc_list[cnt2];
            if (epc1->epc > epc2->epc) {
                tmp = *epc1;
                *epc1 = *epc2;
                *epc2 = tmp;
            }
        }
    }
}

//=============================================================================
#if 0
static void ELL_freeObject(ELL_Object_t *obj)
{
    int cnt;

    ELL_Assert(obj != NULL);

    for (cnt = 0; cnt < obj->num_epc; cnt ++) {
        MEM_Free(obj->epc_list[cnt].edt);
    }
    MEM_Free(obj->epc_list);

    obj->eoj = 0;
}
#endif

//=============================================================================
// Register ECHONET Lite Object
//=============================================================================
ELL_Object_t *ELL_RegisterObject(uint32_t eoj, const uint8_t *define)
{
    int cnt, cnt2;
    int num_epc;
    const uint8_t *ptr;
    uint8_t epc, pdc, flag;
    const uint8_t *edt;
    ELL_Object_t *obj;
    uint8_t prop_map[17];
#ifdef OVERWRITE_PROP_MAP
    uint8_t num_prop_map, prop_map_flag;
#endif

    ELL_Assert(define != NULL);

#if 1
	ELL_Printf("\nRegister Object (EOJ=0x%06X)\n", eoj);
#endif

    // ------------------------------------------------------------------------
    if ((eoj & 0x0000ff) == 0x00) return (NULL);
    if (define == NULL) return (NULL);
#ifdef REGISTER_NODE_PROFILE
    if ((eoj & 0xffff00) == 0x0ef000) return (NULL);
    if (gNodeProfileRegistered) return (NULL);
#endif

    // --------------------------------------------------------- Count EPCs ---
#ifdef OVERWRITE_PROP_MAP
    num_prop_map = 0;
    prop_map_flag = 0;
    num_epc = 0;
    ptr = define;
    while (num_epc < (128 - 3)) {
        epc = *ptr ++; // pop EPC
        if (epc < 0x80) break;
        pdc = *ptr ++; // pop PDC
        ptr += pdc + 1; // skip EDT and flag
        num_epc ++;

        if (epc == 0x9d || epc == 0x9e || epc == 0x9f) {
            num_prop_map ++;
            // 0x9D => 0x02, 0x9E => 0x04, 0x9F => 0x08
            prop_map_flag |= 1 << (epc & 0x03);
        }
    }
    if ((num_epc + 3 - num_prop_map) >= 128) {
        return (NULL);
    }
#else
    num_epc = 0;
    ptr = define;
    while (num_epc < (128 - 3)) {
        epc = *ptr ++; // pop EPC
        if (epc < 0x80) break;
        pdc = *ptr ++; // pop PDC
        ptr += pdc + 1; // skip EDT and flag

        if (epc != 0x9d && epc != 0x9e && epc != 0x9f) {
            num_epc ++;
        }
    }
    if ((num_epc + 3) >= 128) {
        return (NULL);
    }
#endif

    // --------------------------------------------- Find Free Object Space ---
    for (cnt = 0; cnt < ELL_MAX_OBJECTS; cnt ++) {
        if (gELLObjectAlloc[cnt].eoj == 0) {
            obj = &gELLObjectAlloc[cnt];
            break;
        }
    }
    if (cnt == ELL_MAX_OBJECTS) {
        return (NULL);
    }

    // ------------------------------------------------------- Set Contents ---
    obj->eoj = eoj;
    // +3, because EPC 0x9D, 0x9E, 0x9F is generated automatically
#ifdef OVERWRITE_PROP_MAP
    obj->num_epc = (uint8_t)num_epc + 3 - num_prop_map;
#else
    obj->num_epc = (uint8_t)num_epc + 3;
#endif
    obj->epc_list = MEM_Alloc(sizeof(ELL_EPCDefine_t) * obj->num_epc);
    ELL_Assert(obj->epc_list != NULL);
    for (cnt2 = 0; cnt2 < obj->num_epc; cnt2 ++) {
        MEM_Set(&obj->epc_list[cnt2], 0, sizeof(ELL_EPCDefine_t));
    }

    ptr = define;
    for (cnt2 = 0; cnt2 < num_epc; cnt2 ++) {
        epc = *ptr;
        ptr ++;
        pdc = *ptr;
        ptr ++;
        if (pdc == 0) {
            edt = NULL;
        } else {
            edt = ptr;
            ptr += pdc;
        }
        flag = *ptr;
        ptr ++;
#ifdef OVERWRITE_PROP_MAP
        ELL_defineEPC(obj, epc, pdc, edt, flag);
#else
        if (epc != 0x9d && epc != 0x9e && epc != 0x9f) {
            ELL_defineEPC(obj, epc, pdc, edt, flag);
        }
        else {
            // to skip
            cnt2 --;
        }
#endif
    }

    // ---------------------------- Add Property Maps(EPC 0x9d, 0x9e, 0x9f) ---
#ifdef OVERWRITE_PROP_MAP
    if ((prop_map_flag & 0x08) == 0) {
        pdc = ELL_createPropertyMap(obj, EPC_FLAG_RULE_GET, prop_map);
        ELL_defineEPC(obj, 0x9f, pdc, prop_map, EPC_FLAG_RULE_GET);
    }

    if ((prop_map_flag & 0x04) == 0) {
        pdc = ELL_createPropertyMap(obj, EPC_FLAG_RULE_SET, prop_map);
        ELL_defineEPC(obj, 0x9e, pdc, prop_map, EPC_FLAG_RULE_GET);
    }

    if ((prop_map_flag & 0x02) == 0) {
        pdc = ELL_createPropertyMap(obj, EPC_FLAG_RULE_ANNO, prop_map);
        ELL_defineEPC(obj, 0x9d, pdc, prop_map, EPC_FLAG_RULE_GET);
    }
#else
    pdc = ELL_createPropertyMap(obj, EPC_FLAG_RULE_GET, prop_map);
    ELL_defineEPC(obj, 0x9f, pdc, prop_map, EPC_FLAG_RULE_GET);

    pdc = ELL_createPropertyMap(obj, EPC_FLAG_RULE_SET, prop_map);
    ELL_defineEPC(obj, 0x9e, pdc, prop_map, EPC_FLAG_RULE_GET);

    pdc = ELL_createPropertyMap(obj, EPC_FLAG_RULE_ANNO, prop_map);
    ELL_defineEPC(obj, 0x9d, pdc, prop_map, EPC_FLAG_RULE_GET);
#endif

    // ----------------------------------------------------------- Sort EPC ---
    ELL_sortEPC(obj);

    gNumELLObjects ++;

    return (obj);
}

#ifdef REGISTER_NODE_PROFILE
/*** CODING... ***/
//=============================================================================
ELL_Object_t *ELL_RegisterNodeProfile(uint32_t eoj, const uint8_t *define)
{
    int cnt, cnt2;
    int num_epc;
    const uint8_t *ptr;
    uint8_t epc, pdc, flag;
    const uint8_t *edt;
    ELL_Object_t *obj;
    uint8_t prop_map[17];

    ELL_Assert(define != NULL);

    // ------------------------------------------------------------------------
    if ((eoj & 0x0000ff) == 0x00) return (NULL);
    if ((eoj & 0xffff00) != 0x0ef000) return (NULL);
    if (define == NULL) return (NULL);
    if (gNodeProfileRegistered) return (NULL);

    // --------------------------------------------------------- Count EPCs ---
    num_epc = 0;
    ptr = define;
    while (num_epc < (128 - 8)) {
        epc = *ptr ++; // pop EPC
        if (epc < 0x80) break;
        switch (epc) {
        case 0x9d: case 0x9e: case 0x9f:
        case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
            return (NULL);
        default:
            break;
        }
        pdc = *ptr ++; // pop PDC
        ptr += pdc + 1; // skip EDT and flag
        num_epc ++;
    }
    if ((num_epc + 8) >= 128) {
        return (NULL);
    }

    // --------------------------------------------- Find Free Object Space ---
    for (cnt = 0; cnt < ELL_MAX_OBJECTS; cnt ++) {
        if (gELLObjectAlloc[cnt].eoj == 0) {
            obj = &gELLObjectAlloc[cnt];
            break;
        }
    }
    if (cnt == ELL_MAX_OBJECTS) {
        return (NULL);
    }

    // ------------------------------------------------------- Set Contents ---
    obj->eoj = eoj;
    // +8, because EPC 0x9D-0x9F and 0xD3-0xD7 is generated automatically
    obj->num_epc = (uint8_t)num_epc + 8;
    obj->epc_list = MEM_Alloc(sizeof(ELL_EPCDefine_t) * obj->num_epc);
    ELL_Assert(obj->epc_list != NULL);
    for (cnt2 = 0; cnt2 < obj->num_epc; cnt2 ++) {
        MEM_Set(&obj->epc_list[cnt2], 0, sizeof(ELL_EPCDefine_t));
    }

    ptr = define;
    for (cnt2 = 0; cnt2 < num_epc; cnt2 ++) {
        epc = *ptr;
        ptr ++;
        pdc = *ptr;
        ptr ++;
        edt = ptr;
        ptr += pdc;
        flag = *ptr;
        ptr ++;
        ELL_defineEPC(obj, epc, pdc, edt, flag);
    }

    // ---------------------------- Add Property Maps(EPC 0x9d, 0x9e, 0x9f) ---
    pdc = ELL_createPropertyMap(obj, EPC_FLAG_RULE_GET, prop_map);
    ELL_defineEPC(obj, 0x9f, pdc, prop_map, EPC_FLAG_RULE_GET);

    pdc = ELL_createPropertyMap(obj, EPC_FLAG_RULE_SET, prop_map);
    ELL_defineEPC(obj, 0x9e, pdc, prop_map, EPC_FLAG_RULE_GET);

    pdc = ELL_createPropertyMap(obj, EPC_FLAG_RULE_ANNO, prop_map);
    ELL_defineEPC(obj, 0x9d, pdc, prop_map, EPC_FLAG_RULE_GET);

    // ----------------------- Add Classes and Objects Info (EPC 0xD3-0xD7) ---
    gEDTWork[0] = (gNumELLObjects >> 16) & 0xff;
    gEDTWork[1] = (gNumELLObjects >> 8) & 0xff;
    gEDTWork[2] = gNumELLObjects & 0xff;
    ELL_defineEPC(obj, 0xd3, 3, gEDTWork, EPC_FLAG_RULE_GET);


    // ----------------------------------------------------------- Sort EPC ---
    ELL_sortEPC(obj);

    gNumELLObjects ++;

    gNodeProfileRegistered = TRUE;

    return (obj);
}
#endif

//=============================================================================
bool_t ELL_AddRangeRule(ELL_Object_t *obj, const uint8_t *range)
{
    int num, cnt;
    uint8_t epc;
    ELL_PropertyType_t rtype;
    const uint8_t *rdef;
    const uint8_t *ptr;
    ELL_EPCDefine_t *epc_def;

    ELL_Assert(obj != NULL && range != NULL);

    // ------------------------------------------------------------------------
    ptr = range;
    while (*ptr >= 0x80) {
        epc = *ptr ++;
        rtype = (ELL_PropertyType_t)*ptr;
        ptr ++;
        rdef = ptr;

        /*** skip ***/
        switch (rtype) {
        case ELL_PROP_VARIABLE:
            rdef = NULL;
            break;
        case ELL_PROP_LIST:
            ptr += (*ptr + 1);
            break;
        case ELL_PROP_NG_LIST:
            ptr += (*ptr + 1);
            break;
        case ELL_PROP_MIN_MAX:
            num = *ptr;
            ptr ++;
            for (cnt = 0; cnt < num; cnt ++) {
                ptr += (*ptr & 0x7f) * 2 + 1;
            }
            break;
        case ELL_PROP_BINARY:
            ptr ++;
            break;
        case ELL_PROP_BITMAP:
            ptr += (*ptr + 1);
            break;
        default:
            /*** ERROR ***/
            return (FALSE);
        }

        epc_def = ELL_findEPC(obj, epc);
        if (epc_def != NULL) {
            epc_def->range = rtype;
            epc_def->rdef = (uint8_t *)rdef;
        }
    }

    return (TRUE);
}

//=============================================================================
bool_t ELL_AddCallback(ELL_Object_t *obj,
                       ELL_SetCallback_t *set_cb, ELL_GetCallback_t *get_cb)
{
    if (obj == NULL) return (FALSE);

    obj->set_cb = (void *)set_cb;
    obj->get_cb = (void *)get_cb;

    return (TRUE);
}

//=============================================================================
ELL_Object_t *ELL_FindObject(uint32_t eoj)
{
    int cnt;

    for (cnt = 0; cnt < gNumELLObjects; cnt ++) {
        if (gELLObjectAlloc[cnt].eoj == eoj) {
            return (&gELLObjectAlloc[cnt]);
        }
    }

    return (NULL);
}

//=============================================================================
uint32_t ELL_GetNextInstance(uint32_t eoj)
{
    int cnt;

    if ((eoj & 0xff) == 0) {
        for (cnt = 0; cnt < gNumELLObjects; cnt ++) {
            if ((gELLObjectAlloc[cnt].eoj & 0xffff00) == eoj) {
                return (gELLObjectAlloc[cnt].eoj);
            }
        }
    } else {
        for (cnt = 0; cnt < gNumELLObjects; cnt ++) {
            if (gELLObjectAlloc[cnt].eoj == eoj) {
                break;
            }
        }
        cnt ++;
        eoj &= 0xffff00;
        for ( ; cnt < gNumELLObjects; cnt ++) {
            if ((gELLObjectAlloc[cnt].eoj & 0xffff00) == eoj) {
                return (gELLObjectAlloc[cnt].eoj);
            }
        }
    }

    return (0);
}

//=============================================================================
bool_t ELL_InitProperty(ELL_Object_t *obj,
                        uint8_t epc, uint8_t pdc, const uint8_t *edt)
{
    ELL_EPCDefine_t *epc_def;

    if (obj == NULL || edt == NULL) {
        return (FALSE);
    }

    epc_def = ELL_findEPC(obj, epc);
    if (epc_def == NULL) {
        return (FALSE);
    }

#if (ELL_USE_FIXED_PDC != 0)
    if (epc_def->pdc == 0) {
        epc_def->edt = MEM_Alloc(pdc);
        ELL_Assert(epc_def->edt != NULL);
        epc_def->pdc = pdc;
    }
    else if (pdc != epc_def->pdc) {
        return (FALSE);
    }

    MEM_Copy(epc_def->edt, edt, pdc);
#else
    if (epc_def->pdc == 0) {
        epc_def->edt = MEM_Alloc(pdc + 1);
        ELL_Assert(epc_def->edt != NULL);
        epc_def->edt[0] = pdc;
        epc_def->pdc = pdc;
    }
    else if (pdc > epc_def->edt[0]) {
        return (FALSE);
    }

    MEM_Copy(&epc_def->edt[1], edt, pdc);
    epc_def->pdc = pdc;
#endif

    return (TRUE);
}

//=============================================================================
bool_t ELL_SetProperty(ELL_Object_t *obj,
                       uint8_t epc, uint8_t pdc, const uint8_t *edt)
{
    ELL_EPCDefine_t *epc_def;

    if (obj == NULL || edt == NULL) {
        return (FALSE);
    }

    epc_def = ELL_findEPC(obj, epc);
    if (epc_def == NULL) {
        return (FALSE);
    }
    // if (epc_def->pdc == 0) return (FALSE);

#if (ELL_USE_FIXED_PDC != 0)
    if (epc_def->pdc == 0) {
        if ((epc_def->flag & EPC_FLAG_ADAPTER) != 0) {
            epc_def->edt = MEM_Alloc(pdc);
            ELL_Assert(epc_def->edt != NULL);
            epc_def->pdc = pdc;
            if ((epc_def->flag & EPC_FLAG_RULE_ANNO) != 0) {
                epc_def->flag |= EPC_FLAG_NEED_ANNO;
            }
        } else {
            return (FALSE);
        }
    }
    if (pdc != epc_def->pdc) {
        return (FALSE);
    }

    if ((epc_def->flag & EPC_FLAG_RULE_ANNO) != 0
        && MEM_Compare(epc_def->edt, edt, pdc) != 0) {
        epc_def->flag |= EPC_FLAG_NEED_ANNO;
    }

    MEM_Copy(epc_def->edt, edt, pdc);
#else
    if ((epc_def->flag & EPC_FLAG_ADAPTER) != 0) {
        if (epc_def->pdc == 0) {
            epc_def->edt = MEM_Alloc(pdc + 1);
            ELL_Assert(epc_def->edt != NULL);
            epc_def->edt[0] = pdc;
            epc_def->pdc = pdc;
            if ((epc_def->flag & EPC_FLAG_RULE_ANNO) != 0) {
                epc_def->flag |= EPC_FLAG_NEED_ANNO;
            }
        }
        else if (pdc > epc_def->edt[0]) {
            MEM_Free(epc_def->edt);
            epc_def->edt = MEM_Alloc(pdc + 1);
            ELL_Assert(epc_def->edt != NULL);
            epc_def->edt[0] = pdc;
            epc_def->pdc = pdc;
            if ((epc_def->flag & EPC_FLAG_RULE_ANNO) != 0) {
                epc_def->flag |= EPC_FLAG_NEED_ANNO;
            }
        }
    }
    else if (epc_def->pdc == 0) {
        return (FALSE);
    }
    else if (pdc > epc_def->edt[0]) {
        return (FALSE);
    }

    if ((epc_def->flag & EPC_FLAG_RULE_ANNO) != 0
        && (pdc != epc_def->pdc 
            || MEM_Compare(&epc_def->edt[1], edt, pdc) != 0)) {
        epc_def->flag |= EPC_FLAG_NEED_ANNO;
    }

    MEM_Copy(&epc_def->edt[1], edt, pdc);
    epc_def->pdc = pdc;
#endif

    return (TRUE);
}

//=============================================================================
uint8_t ELL_GetProperty(ELL_Object_t *obj, uint8_t epc, uint8_t *edt, int max)
{
    ELL_EPCDefine_t *epc_def;

    ELL_Assert(obj != NULL && edt != NULL && max > 0);

    epc_def = ELL_findEPC(obj, epc);
    if (epc_def == NULL) return (0);
    if (epc_def->pdc > max) return (0);
    if (epc_def->pdc == 0) return (0);

#if (ELL_USE_FIXED_PDC != 0)
    MEM_Copy(edt, epc_def->edt, epc_def->pdc);
#else
    MEM_Copy(edt, &epc_def->edt[1], epc_def->pdc);
#endif

    return (epc_def->pdc);
}

//=============================================================================
uint8_t ELL_GetPropertySize(ELL_Object_t *obj, uint8_t epc)
{
    ELL_EPCDefine_t *epc_def;

    ELL_Assert(obj != NULL);

    epc_def = ELL_findEPC(obj, epc);
    if (epc_def == NULL) return (0);

    return (epc_def->pdc);
}

//=============================================================================
uint8_t ELL_GetPropertyFlag(ELL_Object_t *obj, uint8_t epc)
{
    ELL_EPCDefine_t *epc_def;

    ELL_Assert(obj != NULL);

    epc_def = ELL_findEPC(obj, epc);
    if (epc_def == NULL) return (0);

    return (epc_def->flag);
}

//=============================================================================
static bool_t ELL_checkMinMax(uint8_t pdc, const uint8_t *edt, uint8_t *def)
{
    int num, cnt;
    int size;
    int edt_ptr;
    bool_t is_signed;
    int32_t s_val, s_min, s_max;
    uint32_t u_val, u_min, u_max;

    ELL_Assert(edt != NULL && def != NULL);

    edt_ptr = 0;
    num = *def ++;
    for (cnt = 0; cnt < num; cnt ++) {
        is_signed = ((*def & 0x80) != 0);
        size = (*def ++) & 0x7f;
        switch (size) {
        case 1:
            u_val = edt[edt_ptr];
            edt_ptr ++;
            u_min = *def ++;
            u_max = *def ++;
            break;
        case 2:
            u_val = (((uint16_t)edt[edt_ptr] << 8) |(uint16_t)edt[edt_ptr + 1]);
            edt_ptr += 2;
            u_min = (((uint16_t)def[0] << 8) |(uint16_t)def[1]);
            def += 2;
            u_max = (((uint16_t)def[0] << 8) |(uint16_t)def[1]);
            def += 2;
            break;
        case 4:
            u_val = (((uint32_t)edt[edt_ptr] << 24)
                     |((uint32_t)edt[edt_ptr + 1] << 16)
                     |((uint32_t)edt[edt_ptr + 2] << 8)
                     |(uint32_t)edt[edt_ptr + 3]);
            edt_ptr += 4;
            u_min = (((uint32_t)def[0] << 24) |((uint32_t)def[1] << 16)
                     |((uint32_t)def[2] << 8) |(uint32_t)def[3]);
            def += 4;
            u_max = (((uint32_t)def[0] << 24) |((uint32_t)def[1] << 16)
                     |((uint32_t)def[2] << 8) |(uint32_t)def[3]);
            def += 4;
            break;
        default:
            return (FALSE);
        }
        if (is_signed) {
            switch (size) {
            case 1:
                s_val = (int8_t)u_val;
                s_min = (int8_t)u_min;
                s_max = (int8_t)u_max;
                break;
            case 2:
                s_val = (int16_t)u_val;
                s_min = (int16_t)u_min;
                s_max = (int16_t)u_max;
                break;
            case 4:
                s_val = (int32_t)u_val;
                s_min = (int32_t)u_min;
                s_max = (int32_t)u_max;
                break;
            default:
                return (FALSE);
            }
            if (s_val < s_min || s_val > s_max) {
                return (FALSE);
            }
        }
        else {
            if (u_val < u_min || u_val > u_max) {
                return (FALSE);
            }
        }
    }

    return (TRUE);
}

//=============================================================================
bool_t ELL_CheckProperty(ELL_Object_t *obj,
                         uint8_t epc, uint8_t pdc, const uint8_t *edt)
{
    ELL_EPCDefine_t *epc_def;
    int num, cnt;
    uint8_t *def;

    ELL_Assert(obj != NULL && edt != NULL);

    epc_def = ELL_findEPC(obj, epc);
    if (epc_def == NULL) return (FALSE);

    if (epc_def->range != ELL_PROP_VARIABLE
        && epc_def->pdc != pdc) return (FALSE);

    def = epc_def->rdef;
    switch (epc_def->range) {
    case ELL_PROP_VARIABLE:
        return (TRUE);
    case ELL_PROP_LIST:
        if (pdc != 1) {
            return (FALSE);
        }
        num = *def ++;
        for (cnt = 0; cnt < num; cnt ++) {
            if (edt[0] == *def) {
                return (TRUE);
            }
            def ++;
        }
        break;
    case ELL_PROP_NG_LIST:
        if (pdc != 1) {
            return (FALSE);
        }
        num = *def ++;
        for (cnt = 0; cnt < num; cnt ++) {
            if (edt[0] == *def) {
                return (FALSE);
            }
            def ++;
        }
        return (TRUE);
    case ELL_PROP_MIN_MAX:
        return (ELL_checkMinMax(pdc, edt, def));
    case ELL_PROP_BINARY:
        return (TRUE);
    case ELL_PROP_BITMAP:
        num = *def ++;
        for (cnt = 0; cnt < num; cnt ++) {
            if ((edt[cnt] &(~*def)) != 0) {
                return (FALSE);
            }
            def ++;
        }
        return (TRUE);
    default:
        break;
    }

    return (FALSE);
}

//=============================================================================
void ELL_ClearAnnounce(ELL_Object_t *obj)
{
    ELL_EPCDefine_t *epc_def;
    int cnt;

    if (obj == NULL) return;

    epc_def = obj->epc_list;
    for (cnt = 0; cnt < obj->num_epc; cnt ++) {
        epc_def->flag &= ~EPC_FLAG_NEED_ANNO;
        epc_def ++;
    }
}

//=============================================================================
void ELL_NeedAnnounce(ELL_Object_t *obj, uint8_t epc)
{
    ELL_EPCDefine_t *epc_def;

    if (obj == NULL) return;

    epc_def = ELL_findEPC(obj, epc);
    if (epc_def == NULL) {
        return;
    }

    if ((epc_def->flag & EPC_FLAG_RULE_ANNO) != 0) {
        epc_def->flag |= EPC_FLAG_NEED_ANNO;
    }

}

//=============================================================================
int ELL_GetAnnounceEPC(ELL_Object_t *obj, uint8_t *buf, int max)
{
    int cnt, num;

    ELL_Assert(obj != NULL && buf != NULL);

    num = 0;
    for (cnt = 0; cnt < obj->num_epc; cnt ++) {
        if (num < (max - 1)
            && (obj->epc_list[cnt].flag & EPC_FLAG_NEED_ANNO) != 0) {
            buf[num ++] = obj->epc_list[cnt].epc;
            obj->epc_list[cnt].flag &= ~EPC_FLAG_NEED_ANNO;
        }
    }
    buf[num] = 0;

    return (num);
}

//=============================================================================
uint8_t ELL_GetPropertyVirtual(ELL_Object_t *obj,
							   uint8_t epc, uint8_t *edt, int max)
{
	uint8_t flag;
	uint8_t pdc;

	if (obj == NULL || edt == NULL || max == 0) return (0);

	flag = ELL_GetPropertyFlag(obj, epc);
	if ((flag & EPC_FLAG_RULE_GET) == 0) {
		// --------------------------------------------- No Get Access Rule ---
		pdc = 0;
	} else if ((flag & EPC_FLAG_RULE_GETUP) != 0 && obj->get_cb != NULL) {
		// --------------------------------------- Has Callback for Getting ---
		pdc = (*(ELL_GetCallback_t *)obj->get_cb)(obj, epc, edt, max);
	} else {
		// ------------------------------------------ Get Property Directly ---
		pdc = ELL_GetProperty(obj, epc, edt, max);
	}

	return (pdc);
}

//=============================================================================
uint8_t ELL_SetPropertyVirtual(ELL_Object_t *obj,
							   uint8_t epc, uint8_t pdc, const uint8_t *edt)
{
	uint8_t flag;
	bool_t result;

	if (obj == NULL || edt == NULL || pdc == 0) return (0);

	result = FALSE;
	flag = ELL_GetPropertyFlag(obj, epc);
	if ((flag & EPC_FLAG_RULE_SET) == 0) {
		// --------------------------------------------- No Set Access Rule ---
	}
	else if (pdc == 0) {
		// ------------------------------------------------ PDC = 0, no EDT ---
	}
	else if ((flag & EPC_FLAG_RULE_SETUP) != 0 && obj->set_cb != NULL) {
		// --------------------------------------- Has Callback for Setting ---
		result = (*(ELL_SetCallback_t *)obj->set_cb)(obj, epc, pdc, edt);
	}
	else if (!ELL_CheckProperty(obj, epc, pdc, edt)) {
		// ------------------------------------ Failed to Check Value Range ---
	}
	else {
		// ------------------------------------------ Set Property Directly ---
		result = ELL_SetProperty(obj, epc, pdc, edt);
	}

	return (result);
}

/******************************** END-OF-FILE ********************************/
