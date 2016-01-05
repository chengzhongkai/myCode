/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include <stdio.h>

#include "echonetlitelite.h"
#include "connection.h"

#include <arpa/inet.h>

#include <getopt.h>
#include <time.h>
#include <stdlib.h>

//=============================================================================
extern void printNetworkInterfaces(void);

//=============================================================================
static uint8_t gRecvBuffer[1024];
static uint8_t gSendBuffer[1024];

//=============================================================================
const uint8_t gNodeProfileEPC[] = {
    0x80,  1, 0x30, EPC_FLAG_RULE_GET,
    0x82,  4, 0x01, 0x0a, 0x01, 0x00, EPC_FLAG_RULE_GET,
    0x83, 17, 0xfe, 0x00, 0x00, 0x7e,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, EPC_FLAG_RULE_GET,
    0x8a,  3, 0x00, 0x00, 0x7e, EPC_FLAG_RULE_GET,
    0xd3,  3, 0x00, 0x00, 0x02, EPC_FLAG_RULE_GET, // N of Instances
    0xd4,  2, 0x00, 0x02, EPC_FLAG_RULE_GET, // N of Classes
    0xd5,  7, 0x02, 0x00, 0x11, 0x01, 0x05, 0xfd, 0x01,
    EPC_FLAG_RULE_ANNO, // Instance List Notification
    0xd6,  7, 0x02, 0x00, 0x11, 0x01, 0x05, 0xfd, 0x01,
    EPC_FLAG_RULE_GET, // Node Instance List
    0xd7,  5, 0x02, 0x00, 0x11, 0x05, 0xfd, EPC_FLAG_RULE_GET,
    // Node Class List

    0x00 /*** Terminator ***/
};

//=============================================================================
const uint8_t gControllerEPC[] = {
    0x80,  1, 0x30, EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO,
    0x81,  1, 0x00, EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO,
    0x82,  4, 0x00, 0x00, 0x44, 0x00, EPC_FLAG_RULE_GET,

    0x00 /*** Terminator ***/
};

//=============================================================================
void ELL_ConstructObjects(void)
{
    ELL_InitObjects();

    // Construct Node Profile Object
    ELL_RegisterObject(0x0ef001, gNodeProfileEPC);

    // Construct Controller Object
    ELL_RegisterObject(0x05ff01, gControllerEPC);
}

//=============================================================================
uint8_t str2esv(const char *str)
{
    if (strcasecmp(str, "seti") == 0) {
        return (ESV_SetI);
    } else if (strcasecmp(str, "setc") == 0) {
        return (ESV_SetC);
    } else if (strcasecmp(str, "get") == 0) {
        return (ESV_Get);
    } else if (strcasecmp(str, "inf") == 0) {
        return (ESV_INF);
    } else if (strcasecmp(str, "infc") == 0) {
        return (ESV_INFC);
    } else if (strcasecmp(str, "inf_req") == 0) {
        return (ESV_INF_REQ);
    } else if (strcasecmp(str, "setget") == 0) {
        return (ESV_SetGet);
    }
    return (ESV_None);
}

//=============================================================================
uint32_t str2hex(const char *str)
{
    char ch;
    int res = 0;
    while ((ch = *str ++) != '\0') {
        res <<= 4;
        if (ch >= '0' && ch <= '9') {
            res += (ch - '0');
        } else if (ch >= 'a' && ch <= 'f') {
            res += (ch - 'a') + 10;
        } else if (ch >= 'A' && ch <= 'F') {
            res += (ch - 'A') + 10;
        } else {
            return (0);
        }
    }
    return (res);
}

//=============================================================================
uint8_t str2hexarray(const char *str, char sep, uint8_t *array, int num)
{
    int cnt, cnt2;
    char work[8];
    uint32_t var;
    uint8_t ret = 0;

    assert(str != NULL && array != NULL);

    for (cnt = 0; cnt < num; cnt ++) {
        for (cnt2 = 0; cnt2 < 8; cnt2 ++) {
            work[cnt2] = *str ++;
            if (work[cnt2] == sep || work[cnt2] == '\0') {
                work[cnt2] = '\0';
                break;
            }
        }
        if (cnt2 == 8) return (0);

        var = str2hex(work);
        if (cnt == 0) {
            ret = (uint8_t)var;
        } else if (var <= 0xff) {
            array[cnt - 1] = (uint8_t)var;
        } else {
            array[cnt - 1] = 0;
        }
    }

    return (ret);
}

//=============================================================================
int count_char_in_str(const char *str, char ch)
{
    int count;

    count = 0;
    for ( ; *str != '\0'; str ++) {
        if (*str == ch) count ++;
    }
    return (count);
}

//=============================================================================
void help(const char *cmd)
{
    printf("\nUsage: $ %s [options]\n\n"
           "options:\n"
           "\t-t TIMEOUT, --timeout=TIMEOUT\n"
           "\t-o TO_ADDR, --to_addr=TO_ADDR, as IP Address sending packet to\n"
           "\t-i IF_ADDR, --if_addr=IF_ADDR, as Interface IP Address\n"
           "\t-e ESV, --esv=ESV(SetI, SetC, Get, INF, INFC, INF_REQ)\n"
           "\t-d DEOJ, --deoj=DEOJ\n"
           "\t-s SEOJ, --seoj=SEOJ\n"
           "\t--epc=EPC,EDT,...EDT\n"
           "\t--tid=TID\n"
           "\nselect IF_ADDR from below:\n", cmd);
    printNetworkInterfaces();
    printf("\n");
}

//=============================================================================
int main(int ac, char *av[])
{
    UDP_Handle_t udp;

    int recv_size;
    IPv4Addr_t recv_addr;

    uint8_t esv = ESV_None;
    uint16_t tid = 0;
    uint32_t seoj = 0x0ef001;
    uint32_t deoj = 0x000000;
    int prop_idx = 0;
    ELL_Property_t prop_list[16];
    int32_t timeout = 3000;
    IPv4Addr_t to_addr = INADDR_ANY;
    char *if_addr = NULL;
    int send_size;
    bool_t verbose = FALSE;

    clock_t start_time;
    int32_t r_timeout;

    const struct option long_options[] = {
        // name, arg, null, result
        { "timeout", 1, NULL, 't' },
        { "to_addr", 1, NULL, 'o' },
        { "if_addr", 1, NULL, 'i' },
        { "esv",     1, NULL, 'e' },
        { "deoj",    1, NULL, 'd' },
        { "seoj",    1, NULL, 's' },
        { "epc",     1, NULL, 'z' },
        { "tid",     1, NULL, 'x' },
        { "verbose", 0, NULL, 'v' },
        { 0, 0, 0, 0 }
    };
    const char *short_options = "t:o:i:e:d:s:v";

    int res = 0;
    int opt_idx = 0;

    srand(time(NULL));
    tid = (uint16_t)rand();

    while ((res = getopt_long(ac, av,
                              short_options, long_options, &opt_idx)) != -1) {
        switch (res) {
        case 't':
            if (optarg != NULL) {
                timeout = atoi(optarg);
            }
            break;
        case 'o':
            to_addr = inet_addr(optarg);
            break;
        case 'i':
            if_addr = optarg;
            break;
        case 'e':
            esv = str2esv(optarg);
            break;
        case 'd':
            deoj = str2hex(optarg);
            break;
        case 's':
            seoj = str2hex(optarg);
            break;
        case 'z':
            if (prop_idx < 15) {
                prop_list[prop_idx].pdc = count_char_in_str(optarg, ',');
                prop_list[prop_idx].edt = malloc(sizeof(char)
                                                 * prop_list[prop_idx].pdc);
                // assert(prop_list[prop_idx].edt != NULL);
                prop_list[prop_idx].epc = str2hexarray(optarg, ',',
                                                       (uint8_t *)prop_list[prop_idx].edt,
                                                       prop_list[prop_idx].pdc + 1);
                prop_idx ++;

                prop_list[prop_idx].epc = 0;
            }
            break;
        case 'x':
            tid = (uint16_t)str2hex(optarg);
            break;
        case 'v':
            verbose = 1;
            break;
        case ':':
        case '?':
        default:
            help(av[0]);
            return (0);
        }
    }

    // ------------------------------------------------------------------------
    if (if_addr == NULL || esv == ESV_None || prop_idx == 0) {
        help(av[0]);
        return (0);
    }

    // --------------------------------------------- Init ECHONET Lite Lite ---
    ELL_ConstructObjects();

    printf("\n");

    // ---------------------------------------------------- Open Connection ---
    if (!UDP_Open(if_addr, "224.0.23.0", 3610, &udp)) {
        printf("Cannot open %s\n", av[1]);
        return (0);
    }

    // -------------------------------------------------------- Make Packet ---
    send_size = ELL_MakePacket(tid, seoj, deoj, esv, prop_list,
                               gSendBuffer, sizeof(gSendBuffer));

    ELLDbg_PrintPacket("to", to_addr, gSendBuffer, send_size);
    UDP_Send(&udp, to_addr, gSendBuffer, send_size);

    // ---------------------------------------------------------- Main Loop ---
    start_time = clock();
    // printf("start=%ld/%d\n", start_time, CLOCKS_PER_SEC);
    r_timeout = timeout;
    printf("wait %dms...\n", timeout);
    do {
        recv_size = UDP_Recv(&udp, gRecvBuffer, 1024, r_timeout, &recv_addr);
        if (recv_size > 0) {
            ELLDbg_PrintPacket("from", recv_addr, gRecvBuffer, recv_size);

            ELL_HandleRecvPacket(gRecvBuffer, recv_size, &udp, &recv_addr);

            if (to_addr == recv_addr
                && ELL_DEOJ(gRecvBuffer) == seoj
                && ELL_TID(gRecvBuffer) == tid) {
                break;
            }
        }
        else {
            // printf(".\n");
            r_timeout = timeout - ((clock() - start_time) * 30);
            if (r_timeout < 0) r_timeout = 1;
        }
    } while (0); // while (((clock() - start_time) * 30) < timeout);

    UDP_Close(&udp);

    return (0);
}

/******************************** END-OF-FILE ********************************/
