/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include <stdio.h>

#include "echonetlitelite.h"
#include "connection.h"

#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//=============================================================================
static uint8_t gRecvBuffer[1024];

//=============================================================================
const uint8_t gNodeProfileEPCDefine[] = {
  0x80,  1, 0x30, EPC_FLAG_RULE_GET,
  0x82,  4, 0x01, 0x0a, 0x01, 0x00, EPC_FLAG_RULE_GET,
  0x83, 17, 0xfe, 0x00, 0x00, 0x7e,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, EPC_FLAG_RULE_GET,
  0x8a,  3, 0x00, 0x00, 0x7e, EPC_FLAG_RULE_GET,

  0xd3,  3, 0x00, 0x00, 0x01, EPC_FLAG_RULE_GET, // N of Instances
  0xd4,  2, 0x00, 0x01, EPC_FLAG_RULE_GET, // N of Classes
  0xd5,  4, 0x01, 0x01, 0x30, 0x01, EPC_FLAG_RULE_ANNO, // Instance List [ANNO]
  0xd6,  4, 0x01, 0x01, 0x30, 0x01, EPC_FLAG_RULE_GET, // Node Instance List
  0xd7,  3, 0x01, 0x00, 0x30, EPC_FLAG_RULE_GET, // Node Class List

  0x00 /*** Terminator ***/
};

//=============================================================================
#define AC_EPC_RULE_SET (EPC_FLAG_RULE_SET | EPC_FLAG_RULE_SETUP)
#define AC_EPC_RULE_GET (EPC_FLAG_RULE_GET | EPC_FLAG_RULE_GETUP)
const uint8_t gAirconEPCDefine[] = {
  0x80,  1, 0x30, AC_EPC_RULE_SET | AC_EPC_RULE_GET | EPC_FLAG_RULE_ANNO,
  0x81,  1, 0x00, AC_EPC_RULE_SET | AC_EPC_RULE_GET | EPC_FLAG_RULE_ANNO,
  0x82,  4, 0x00, 0x00, 0x44, 0x00, AC_EPC_RULE_GET,
  0x88,  1, 0x42, AC_EPC_RULE_GET | EPC_FLAG_RULE_ANNO,

  0xb0,  1, 0x41, AC_EPC_RULE_SET | AC_EPC_RULE_GET | EPC_FLAG_RULE_ANNO,
  0xb3,  1, 0x19, AC_EPC_RULE_SET | AC_EPC_RULE_GET,

  0x00 /*** Terminator ***/
};

//=============================================================================
const uint8_t gSuperClassEPCRange[] = {
  0x80, ELL_PROP_LIST, 0x02, 0x30, 0x31,
  0x81, ELL_PROP_NG_LIST, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x87, ELL_PROP_MIN_MAX, 0x01, 0x01, 0x00, 0x64,
  0x8f, ELL_PROP_LIST, 0x02, 0x41, 0x42,
  0x93, ELL_PROP_LIST, 0x02, 0x41, 0x42,
  0x97, ELL_PROP_MIN_MAX, 0x02, 0x01, 0x00, 0x17, 0x01, 0x00, 0x3b,
  0x98, ELL_PROP_MIN_MAX, 0x03, 0x02, 0x00, 0x01, 0x27, 0x0f,
                                      0x01, 0x01, 0x0c,
                                      0x01, 0x01, 0x1f,
  0x99, ELL_PROP_MIN_MAX, 0x01, 0x02, 0x00, 0x00, 0xff, 0xff,

  0x00 /*** Terminator ***/
};

//=============================================================================
const uint8_t gAirconEPCRange[] = {
  0x80, ELL_PROP_LIST, 0x02, 0x30, 0x31,
  0x8f, ELL_PROP_LIST, 0x02, 0x41, 0x42,
  0xb0, ELL_PROP_LIST, 0x06, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
  0xb1, ELL_PROP_LIST, 0x02, 0x41, 0x42,
  0xb2, ELL_PROP_LIST, 0x03, 0x41, 0x42, 0x43,
  0xb3, ELL_PROP_MIN_MAX, 0x01, 0x01, 0x00, 0x32,
  0xb4, ELL_PROP_MIN_MAX, 0x01, 0x01, 0x00, 0x64,
  0xb5, ELL_PROP_MIN_MAX, 0x01, 0x01, 0x00, 0x32,
  0xb6, ELL_PROP_MIN_MAX, 0x01, 0x01, 0x00, 0x32,
  0xb7, ELL_PROP_MIN_MAX, 0x01, 0x01, 0x00, 0x32,

  0xbf, ELL_PROP_MIN_MAX, 0x01, 0x81, 0x81, 0x7d,
  0xa0, ELL_PROP_LIST, 0x09, 0x31, 0x32, 0x33, 0x34, 0x035, 0x36, 0x37, 0x38, 0x41,
  0xa1, ELL_PROP_LIST, 0x04, 0x41, 0x42, 0x43, 0x44,
  0xa3, ELL_PROP_LIST, 0x04, 0x31, 0x41, 0x42, 0x43,
  0xa4, ELL_PROP_LIST, 0x05, 0x41, 0x42, 0x43, 0x44, 0x45,
  0xa5, ELL_PROP_LIST, 0x1f, 0x41, 0x42, 0x43, 0x44,
                             0x51, 0x52, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
                             0x5a, 0x5b, 0x5c, 0x5d, 0x5f, 0x60, 0x61, 0x62,
                             0x63, 0x64, 0x65, 0x66, 0x67, 0x69, 0x6a, 0x6c,
                             0x6d, 0x6e, 0x6F,
  // 0xaa, ELL_PROP_LIST, 0x04, 0x40, 0x41, 0x42, 0x43,
  // 0xab, ELL_PROP_LIST, 0x02, 0x40, 0x41,
  0xc0, ELL_PROP_LIST, 0x03, 0x41, 0x42, 0x43,
  0xc1, ELL_PROP_LIST, 0x02, 0x41, 0x42,
  0xc2, ELL_PROP_LIST, 0x09, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x41,
  0xc4, ELL_PROP_LIST, 0x09, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x41,
  // 0xc6, ELL_PROP_BITMAP, 0x01, 0x03,
  0xc7, ELL_PROP_BINARY, 0x08, /***/
  // 0xc8, ELL_PROP_BITMAP, 0x01, 0x03,
  0xc9, ELL_PROP_BINARY, 0x08, /***/
  // 0xca, ELL_PROP_BITMAP, 0x01, 0x03,
  0xcb, ELL_PROP_BINARY, 0x08, /***/
  0xcc, ELL_PROP_LIST, 0x05, 0x40, 0x41, 0x42, 0x43, 0x44,
  // 0xcd, ELL_PROP_BITMAP, 0x01, 0x03,
  0xce, ELL_PROP_LIST, 0x03, 0x40, 0x41, 0x42,
  0xcf, ELL_PROP_LIST, 0x02, 0x41, 0x42,
  0x90, ELL_PROP_LIST, 0x04, 0x41, 0x42, 0x43, 0x44,
  0x91, ELL_PROP_MIN_MAX, 0x02, 0x01, 0x00, 0x17, 0x01, 0x00, 0x3b,
  0x92, ELL_PROP_MIN_MAX, 0x02, 0x01, 0x00, 0xff, 0x01, 0x00, 0x3b,
  0x94, ELL_PROP_LIST, 0x04, 0x41, 0x42, 0x43, 0x44,
  0x95, ELL_PROP_MIN_MAX, 0x02, 0x01, 0x00, 0x17, 0x01, 0x00, 0x3b,
  0x96, ELL_PROP_MIN_MAX, 0x02, 0x01, 0x00, 0xff, 0x01, 0x00, 0x3b,

  0x00 /*** Terminator ***/
};

//=============================================================================
static bool_t AC_SetProperty(ELL_Object_t *obj,
                             uint8_t epc, uint8_t pdc, const uint8_t *edt)
{
    printf("Set Callback\n");

    return (ELL_SetProperty(obj, epc, pdc, edt));
}

//=============================================================================
static uint8_t AC_GetProperty(ELL_Object_t *obj,
                              uint8_t epc, uint8_t *edt, int max)
{
    printf("Get Callback\n");

    return (ELL_GetProperty(obj, epc, edt, max));
}

//=============================================================================
bool_t ELL_ConstructObjects(void)
{
    ELL_Object_t *obj;

    ELL_InitObjects();

    // Construct Node Profile Object
    obj = ELL_RegisterObject(0x0ef001, gNodeProfileEPCDefine);
    if (obj == NULL) {
        return (FALSE);
    }

    // Construct Aircon Object
    obj = ELL_RegisterObject(0x013001, gAirconEPCDefine);
    if (obj == NULL) {
        return (FALSE);
    }
    ELL_AddRangeRule(obj, gSuperClassEPCRange);
    ELL_AddRangeRule(obj, gAirconEPCRange);
    ELL_AddCallback(obj, AC_SetProperty, AC_GetProperty);

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
static void printNetworkInterfaces(void)
{
    struct ifaddrs *ifa_list;
    struct ifaddrs *ifa;
    int n;
    char addrstr[256], netmaskstr[256];

    n = getifaddrs(&ifa_list);
    if (n != 0) {
        return;
    }

    for (ifa = ifa_list; ifa != NULL; ifa = ifa->ifa_next) {
        memset(addrstr, 0, sizeof(addrstr));
        memset(netmaskstr, 0, sizeof(netmaskstr));

        // printf("%s\n", ifa->ifa_name);
        // printf("  0x%.8x\n", ifa->ifa_flags);

        if (ifa->ifa_addr->sa_family == AF_INET) {
            inet_ntop(AF_INET,
                      &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr,
                      addrstr, sizeof(addrstr));

#if 0
            inet_ntop(AF_INET,
                      &((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr,
                      netmaskstr, sizeof(netmaskstr));
#endif

            // printf("  IPv4: %s netmask %s\n", addrstr, netmaskstr);

            printf("  %s (%s)\n", addrstr, ifa->ifa_name);
        }
#if 0
        else if (ifa->ifa_addr->sa_family == AF_INET6) {
            inet_ntop(AF_INET6,
                      &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr,
                      addrstr, sizeof(addrstr));

            inet_ntop(AF_INET6,
                      &((struct sockaddr_in6 *)ifa->ifa_netmask)->sin6_addr,
                      netmaskstr, sizeof(netmaskstr));

            printf("  IPv6: %s netmask %s\n", addrstr, netmaskstr);
        }
        else {
            printf("  else:%d\n", ifa->ifa_addr->sa_family);
        }
#endif
    }

    freeifaddrs(ifa_list);
}

//=============================================================================
int main(int ac, char *av[])
{
    UDP_Handle_t udp;

    int recv_size;
    IPv4Addr_t recv_addr;

    if (ac < 2) {
        printf("\nUsage: $ %s IP_ADDRESS\n\n"
               "select IP_ADDRESS from below:\n", av[0]);
        printNetworkInterfaces();
        printf("\n");
        exit(0);
        return (0);
    }

    // --------------------------------------------- Init ECHONET Lite Lite ---
    if (!ELL_ConstructObjects()) {
        /*** Initialization Error ***/
        return (0);
    }

    printf("\n");

    // ---------------------------------------------------- Open Connection ---
    if (!UDP_Open(av[1], "224.0.23.0", 3610, &udp)) {
        printf("Cannot open %s\n", av[1]);
        return (0);
    }
    ELL_SetSendPacketCallback(ELL_SendPacket);

    // --------------------------------------------------- Initial Announce ---
    ELL_StartUp(&udp);

    // ---------------------------------------------------------- Main Loop ---
    while (1) {
        recv_size = UDP_Recv(&udp, gRecvBuffer, 1024, 1000, &recv_addr);
        if (recv_size > 0) {
            ELLDbg_PrintPacket("from", recv_addr, gRecvBuffer, recv_size);

            ELL_HandleRecvPacket(gRecvBuffer, recv_size, &udp, &recv_addr);
        }
        else {
            ELL_CheckAnnounce(&udp);
        }
    }

    return (0);
}

/******************************** END-OF-FILE ********************************/
