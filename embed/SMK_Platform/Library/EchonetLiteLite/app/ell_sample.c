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
  0xd3,  3, 0x00, 0x00, 0x03, EPC_FLAG_RULE_GET, // N of Instances
  0xd4,  2, 0x00, 0x03, EPC_FLAG_RULE_GET, // N of Classes (includes Node Prof)
  0xd5, 10, 0x03, 0x00, 0x11, 0x01, 0x05, 0xfd, 0x01, 0x05, 0xfd, 0x02,
  EPC_FLAG_RULE_ANNO, // Instance List Notification
  0xd6, 10, 0x03, 0x00, 0x11, 0x01, 0x05, 0xfd, 0x01, 0x05, 0xfd, 0x02,
  EPC_FLAG_RULE_GET, // Node Instance List
  0xd7,  5, 0x02, 0x00, 0x11, 0x05, 0xfd, EPC_FLAG_RULE_GET, // Node Class List

  0x00 /*** Terminator ***/
};

//=============================================================================
const uint8_t gTempSensorEPCDefine[] = {
  0x80,  1, 0x30,
  EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO,
  0x81,  1, 0x00,
  EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO,
  0x82,  4, 0x00, 0x00, 0x44, 0x00, EPC_FLAG_RULE_GET,
  0xe0,  2, 0x00, 0x00, EPC_FLAG_RULE_GET,

  0x00 /*** Terminator ***/
};

//=============================================================================
const uint8_t gTempSensorEPCRange[] = {
  0x80, ELL_PROP_LIST, 0x02, 0x30, 0x31,
  0x81, ELL_PROP_NG_LIST, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x00 /*** Terminator ***/
};

//=============================================================================
const uint8_t gSwitchEPCDefine[] = {
  0x80,  1, 0x30,
  EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO,
  0x81,  1, 0x00,
  EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET | EPC_FLAG_RULE_ANNO,
  0x82,  4, 0x00, 0x00, 0x44, 0x00, EPC_FLAG_RULE_GET,
  0x88,  1, 0x42, EPC_FLAG_RULE_GET,
  0x89,  2, 0x00, 0x00, EPC_FLAG_RULE_GET,
  0x8a,  3, 0x00, 0x00, 0x7e, EPC_FLAG_RULE_GET,
  0xe0,  12, 'S', 'W', 'I', 'T', 'C', 'H', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  EPC_FLAG_RULE_SET | EPC_FLAG_RULE_GET,

  0x00 /*** Terminator ***/
};

//=============================================================================
const uint8_t gSwitchEPCRange[] = {
  0x80, ELL_PROP_LIST, 0x02, 0x30, 0x31,
  0x81, ELL_PROP_NG_LIST, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x00 /*** Terminator ***/
};

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

  // Construct Temperature Sensor Object
  obj = ELL_RegisterObject(0x001101, gTempSensorEPCDefine);
  if (obj == NULL) {
    return (FALSE);
  }
  ELL_AddRangeRule(obj, gTempSensorEPCRange);

  // Construct Switch Object 1
  obj = ELL_RegisterObject(0x05fd01, gSwitchEPCDefine);
  if (obj == NULL) {
    return (FALSE);
  }
  ELL_AddRangeRule(obj, gSwitchEPCRange);

  // Construct Switch Object 2
  obj = ELL_RegisterObject(0x05fd02, gSwitchEPCDefine);
  if (obj == NULL) {
    return (FALSE);
  }
  ELL_AddRangeRule(obj, gSwitchEPCRange);

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

  // ----------------------------------------------- Init ECHONET Lite Lite ---
  if (!ELL_ConstructObjects()) {
    /*** Initialization Error ***/
    return (0);
  }

  printf("\n");

  // ------------------------------------------------------ Open Connection ---
  if (!UDP_Open(av[1], "224.0.23.0", 3610, &udp)) {
    printf("Cannot open %s\n", av[1]);
    return (0);
  }
  ELL_SetSendPacketCallback(ELL_SendPacket);

  // ----------------------------------------------------- Initial Announce ---
  ELL_StartUp(&udp);

  // ------------------------------------------------------------ Main Loop ---
  while (1) {
    recv_size = UDP_Recv(&udp, gRecvBuffer, 1024, 0, &recv_addr);
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
