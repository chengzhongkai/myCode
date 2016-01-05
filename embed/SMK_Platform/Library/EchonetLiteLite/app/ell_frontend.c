/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include <stdio.h>

#include "echonetlitelite.h"
#include "connection.h"
#include "ell_adapter.h"

#include <arpa/inet.h>

#include <getopt.h>
#include <time.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

//=============================================================================
extern void printNetworkInterfaces(void);

//=============================================================================
static uint8_t gRecvBuffer[1024];
static uint8_t gSendBuffer[1024];

static UDP_Handle_t gUDP_ell;
static UDP_Handle_t gUDP_server;

static bool_t gStartup = FALSE;

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
    0xd5,  4, 0x01, 0x01, 0x30, 0x01,
    EPC_FLAG_RULE_ANNO, // Instance List Notification
    0xd6,  4, 0x01, 0x01, 0x30, 0x01,
    EPC_FLAG_RULE_GET, // Node Instance List
    0xd7,  3, 0x01, 0x01, 0x30, EPC_FLAG_RULE_GET,
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
bool_t ELL_ConstructObjects(void)
{
    ELL_InitObjects();

    // Construct Node Profile Object
    ELL_RegisterObject(0x0ef001, gNodeProfileEPC);

    // Construct Controller Object
    ELL_RegisterObject(0x05ff01, gControllerEPC);

    return (TRUE);
}

//=============================================================================
bool_t ELL_SendPacket(void *handle, void *dest, const uint8_t *buf, int len)
{
    if (handle == NULL) return (FALSE);

    if (dest == NULL) {
        ELLDbg_PrintPacket("to", 0, buf, len);
        return (UDP_Multicast(handle, buf, len));
    } else {
        IPv4Addr_t addr = *((IPv4Addr_t *)dest);
        ELLDbg_PrintPacket("to", addr, buf, len);
        return (UDP_Send(handle, addr, buf, len));
    }
}

//=============================================================================
socket_t mListenSock;
bool_t UDP_OpenTCP(const char *if_addr, int port, UDP_Handle_t *handle)
{
    socket_t sock;
    struct sockaddr_in addr;
    struct sockaddr_in client;
    int len;

    if (if_addr == NULL || handle == NULL) return (FALSE);

    printf("Open TCP %s:%d...\n", if_addr, port);

    mListenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (isInvalidSocket(mListenSock)) return (FALSE);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(if_addr); // INADDR_ANY;
    bind(mListenSock, (struct sockaddr *)&addr, sizeof(addr));

    listen(mListenSock, 1);

    sock = accept(mListenSock, (struct sockaddr *)&client, &len);

    handle->sock = sock;
    handle->port = port;
    handle->multicast_addr = INADDR_ANY;
    handle->if_addr = inet_addr(if_addr);

    return (TRUE);
}

//=============================================================================
void UDP_CloseTCP(UDP_Handle_t *handle)
{
    if (handle == NULL) return;

    close(handle->sock);

    if (mListenSock != 0) {
        close(mListenSock);
        mListenSock = 0;
    }
}

//=============================================================================
UDP_Handle_t *UDP_Select(UDP_Handle_t *udp1, UDP_Handle_t *udp2, int timeout)
{
    fd_set fds;
    int maxfd;
    struct timeval tv;

    ELL_Assert(udp1 != NULL && udp2 != NULL);

    FD_ZERO(&fds);
    FD_SET(udp1->sock, &fds);
    FD_SET(udp2->sock, &fds);
    maxfd = (udp1->sock > udp2->sock) ? udp1->sock : udp2->sock;

    if (timeout == 0) {
        tv.tv_sec = 0;
        tv.tv_usec = 0;
    } else {
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
    }

    select(maxfd + 1, &fds, NULL, NULL, &tv);

    if (FD_ISSET(udp1->sock, &fds)) {
        return (udp1);
    } else if (FD_ISSET(udp2->sock, &fds)) {
        return (udp2);
    }
    return (NULL);
}

//=============================================================================
void printPacket(const uint8_t *buf, int len)
{
    int cnt;
    int wrap = 0;
    for (cnt = 0; cnt < len; cnt ++) {
        printf("%02X ", buf[cnt]);
        wrap ++;
        if (wrap == 16) {
            printf("\n");
            wrap = 0;
        }
    }
    if (wrap > 0) printf("\n");
}

//=============================================================================
#define setEOJBytes(buf, eoj) do {              \
        *((uint8_t *)(buf) + 0) = (uint8_t)((eoj) >> 16);   \
        *((uint8_t *)(buf) + 1) = (uint8_t)((eoj) >> 8);   \
        *((uint8_t *)(buf) + 2) = (uint8_t)(eoj);          \
    } while (0)
#define setLenBytes(buf, len) do {              \
        *((uint8_t *)(buf) + 3) = (uint8_t)((len) >> 8);   \
        *((uint8_t *)(buf) + 4) = (uint8_t)(len);          \
    } while (0)
#define getLenBytes(buf) (((uint16_t)((buf)[5]) << 8) | (uint16_t)((buf)[6]))

static bool_t Mid_SetProperty(ELL_Object_t *obj,
                              uint8_t epc, uint8_t pdc, const uint8_t *edt)
{
    MWA_Frame_t request, reply;
    int send_size, recv_size;
    bool_t result;

    if (obj == NULL || edt == NULL || pdc == 0) return (FALSE);

    request.frame_type = 0x0003;
    request.cmd_no = 0x10;
    request.frame_no = 0x00;
    request.len = pdc + 6;
    request.data = MEM_Alloc(pdc + 6);
    if (request.data == NULL) return (FALSE);

    setEOJBytes(request.data, obj->eoj);
    setLenBytes(request.data, (pdc + 1));
    ((uint8_t *)request.data)[5] = epc;
    MEM_Copy((uint8_t *)&request.data[6], edt, pdc);

    result = FALSE;

    send_size = MWA_PackFrame(&request, gSendBuffer, 1024);
    if (send_size > 0) {
        MWADbg_PrintFrame(&request);
        send(gUDP_server.sock, gSendBuffer, send_size, 0);

        recv_size = UDP_Recv(&gUDP_server, gRecvBuffer, 1024, 1000, NULL);
        if (recv_size > 0) {
            if (MWA_UnpackFrame(gRecvBuffer, recv_size, &reply)) {
                MWADbg_PrintFrame(&reply);

                if (reply.data[3] == 0 && reply.data[4] == 0) {
                    result = TRUE;
                }
            } else {
                ELL_Printf("Packet Error\n");
                printPacket(gRecvBuffer, recv_size);
                ELL_Printf("\n");
            }
        }
    }

    MEM_Free((uint8_t *)request.data);

    return (result);
}

//=============================================================================
static uint8_t Mid_GetProperty(ELL_Object_t *obj,
                               uint8_t epc, uint8_t *edt, int max)
{
    MWA_Frame_t request, reply;
    uint8_t fd[6];
    int send_size, recv_size;
    uint8_t result;

    if (obj == NULL || edt == NULL) return (FALSE);

    request.frame_type = 0x0003;
    request.cmd_no = 0x10;
    request.frame_no = 0x00;
    request.len = 0x06;
    request.data = fd;

    setEOJBytes(request.data, obj->eoj);
    setLenBytes(request.data, 1);
    ((uint8_t *)request.data)[5] = epc;

    result = 0;

    send_size = MWA_PackFrame(&request, gSendBuffer, 1024);
    if (send_size > 0) {
        MWADbg_PrintFrame(&request);
        send(gUDP_server.sock, gSendBuffer, send_size, 0);

        recv_size = UDP_Recv(&gUDP_server, gRecvBuffer, 1024, 1000, NULL);
        if (recv_size > 0) {
            if (MWA_UnpackFrame(gRecvBuffer, recv_size, &reply)) {
                MWADbg_PrintFrame(&reply);

                if (reply.data[3] == 0 && reply.data[4] == 0) {
                    result = getLenBytes(reply.data);
                    result --;
                    if (max < result) {
                        result = 0;
                    } else {
                        MEM_Copy(edt, &reply.data[8], result);
                    }
                }
            } else {
                ELL_Printf("Packet Error\n");
                printPacket(gRecvBuffer, recv_size);
                ELL_Printf("\n");
            }
        }
    }

    return (result);
}

//=============================================================================
bool_t ELL_HandleInitObject(const uint8_t *data, int len)
{
    int obj_num;
    int ptr;
    uint32_t eoj;
    int num_props;

    int cnt, cnt2;

    uint8_t *def = MEM_Alloc(2048);
    int def_ptr;
    uint8_t epc, pdc, access, flag;

    ELL_Object_t *obj;

    if (data == NULL) return (FALSE);

    obj_num = data[0];
    if (obj_num > 3) return (FALSE);

    ELL_InitObjects();
    ELL_RegisterObject(0x0ef001, gNodeProfileEPC);

    ptr = 1;
    def_ptr = 0;
    for (cnt = 0; cnt < obj_num; cnt ++) {
        eoj = ((uint32_t)data[ptr] << 16) | ((uint32_t)data[ptr + 1] << 8)
            | (uint32_t)data[ptr + 2];
        ptr += 3;
        num_props = data[ptr];
        ptr ++;
        for (cnt2 = 0; cnt2 < num_props; cnt2 ++) {
            epc = data[ptr ++];
            access = data[ptr ++];
            pdc = data[ptr ++];

            if (pdc > 0
                && (epc != 0x9d && epc != 0x9e && epc != 0x9f)) {
                def[def_ptr ++] = epc;
                def[def_ptr ++] = pdc;
                MEM_Set(&def[def_ptr], 0x00, pdc);
                def_ptr += pdc;

                flag = 0;
                if ((access & 0x01) != 0) {
                    flag |= EPC_FLAG_RULE_GET | EPC_FLAG_RULE_GETUP;
                }
                if ((access & 0x02) != 0) {
                    flag |= EPC_FLAG_RULE_SET | EPC_FLAG_RULE_SETUP;
                }
                if ((access & 0x04) != 0) {
                    flag |= EPC_FLAG_RULE_ANNO;
                }
                def[def_ptr ++] = flag;
            }
        }
        def[def_ptr] = 0x00;

        obj = ELL_RegisterObject(eoj, def);
        if (obj == NULL) return (FALSE);

        ELL_AddCallback(obj, Mid_SetProperty, Mid_GetProperty);
    }

    return (TRUE);
}

//=============================================================================
bool_t ELL_HandleNotifyProp(const uint8_t *data, int len)
{
    uint32_t eoj;
    uint8_t epc;
    uint16_t pdc;
    const uint8_t *edt;

    ELL_Object_t *obj;

    if (data == NULL || len < 7) return (FALSE);

    eoj = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8)
        | (uint32_t)data[2];
    pdc = ((uint16_t)data[3] << 8) | (uint16_t)data[4];
    epc = data[5];
    edt = &data[6];

    if ((pdc + 5) != len) return (FALSE);

    pdc --;
    if (pdc > 245) return (FALSE); // MAX 245 bytes

    obj = ELL_FindObject(eoj);
    if (obj == NULL) return (FALSE);

    return (ELL_SetProperty(obj, epc, pdc, edt));
}

//=============================================================================
void ELL_HandleMidFrame_FE(MWA_Frame_t *request,
                           UDP_Handle_t *udp_ell,
                           UDP_Handle_t *udp_server)
{
    MWA_Frame_t reply;
    int send_size;

    if (request->frame_type == 0x0003 && request->cmd_no == 0x70) {
        uint8_t status[2] = { 0x00, 0x00 };

        if (ELL_HandleInitObject(request->data, request->len)) {
            ELL_StartUp(udp_ell);
            gStartup = TRUE;
        } else {
            status[0] = 0xff;
            status[1] = 0xff;
        }

        reply.frame_type = 0x0003;
        reply.cmd_no = 0xf0;
        reply.frame_no = request->frame_no;;
        reply.len = sizeof(status);
        reply.data = status;

        send_size = MWA_PackFrame(&reply, gSendBuffer, 1024);

        if (send_size > 0) {
            MWADbg_PrintFrame(&reply);
            send(udp_server->sock, gSendBuffer, send_size, 0);
        }
    }
    else if (request->frame_type == 0x0003 && request->cmd_no == 0x11) {
        uint8_t status[5];

        if (ELL_HandleNotifyProp(request->data, request->len)) {
            status[0] = 0x00;
            status[1] = 0x00;
        } else {
            status[0] = 0xff;
            status[1] = 0xff;
        }
        status[2] = request->data[0];
        status[3] = request->data[1];
        status[4] = request->data[2];

        reply.frame_type = 0x0003;
        reply.cmd_no = 0x91;
        reply.frame_no = request->frame_no;;
        reply.len = sizeof(status);
        reply.data = status;

        send_size = MWA_PackFrame(&reply, gSendBuffer, 1024);

        if (send_size > 0) {
            MWADbg_PrintFrame(&reply);
            send(udp_server->sock, gSendBuffer, send_size, 0);
        }
    }
    else if (request->frame_type == 0x0003 && request->cmd_no == 0x90) {
    }
}

//=============================================================================
int main(int ac, char *av[])
{
    // UDP_Handle_t udp_ell;
    // UDP_Handle_t udp_server;
    UDP_Handle_t *udp;
    int server_portno = 0;

    int recv_size;
    IPv4Addr_t recv_addr;

    if (ac < 3) {
        printf("\nUsage: $ %s IP_ADDRESS SERVER_PORTNO\n\n"
               "select IP_ADDRESS from below:\n", av[0]);
        printNetworkInterfaces();
        printf("\n");
        exit(0);
        return (0);
    }

    // --------------------------------------------------- Open Server Port ---
    server_portno = atoi(av[2]);
    if (!UDP_OpenTCP("127.0.0.1", server_portno, &gUDP_server)) {
        printf("Cannot open 127.0.0.1:%d\n", server_portno);
        return (0);
    }

#if 0
    // --------------------------------------------- Init ECHONET Lite Lite ---
    if (!ELL_ConstructObjects()) {
        /*** Initialization Error ***/
        return (0);
    }
    printf("\n");
#endif

    // ---------------------------------------------------- Open Connection ---
    if (!UDP_Open(av[1], "224.0.23.0", 3610, &gUDP_ell)) {
        printf("Cannot open %s\n", av[1]);
        return (0);
    }
    ELL_SetSendPacketCallback(ELL_SendPacket);

#if 0
    // --------------------------------------------------- Initial Announce ---
    ELL_StartUp(&gUDP_ell);
#endif

    printf("Connected...\n");

    // ---------------------------------------------------------- Main Loop ---
    while (1) {
        udp = UDP_Select(&gUDP_ell, &gUDP_server, 1000);

        if (udp == &gUDP_ell) {
            recv_size = UDP_Recv(&gUDP_ell, gRecvBuffer, 1024, 0, &recv_addr);
            if (recv_size > 0) {
                ELLDbg_PrintPacket("from", recv_addr, gRecvBuffer, recv_size);

                ELL_HandleRecvPacket(gRecvBuffer,
                                     recv_size, &gUDP_ell, &recv_addr);
            }
        }
        else if (udp == &gUDP_server) {
            recv_size = UDP_Recv(&gUDP_server, gRecvBuffer, 1024, 0, NULL);
            if (recv_size > 0) {
                MWA_Frame_t mid_frame;

                if (MWA_UnpackFrame(gRecvBuffer, recv_size, &mid_frame)) {
                    MWADbg_PrintFrame(&mid_frame);

                    ELL_HandleMidFrame_FE(&mid_frame, &gUDP_ell, &gUDP_server);
                } else {
                    printf("Packet Error\n");
                    printPacket(gRecvBuffer, recv_size);
                    printf("\n");
                }

                // Server use
                // [IP_ADDR:4] [TID:2] [EOJ:3] [ESV:1] [OPC:1] [EPC_PDC_EDT:n]
            }
        }
        else if (gStartup) {
            ELL_CheckAnnounce(&gUDP_ell);
        }
    }

    return (0);
}

/******************************** END-OF-FILE ********************************/
