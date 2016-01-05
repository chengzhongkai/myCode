/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include <stdio.h>
#include <memory.h>

#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//=============================================================================
void printNetworkInterfaces(void)
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
#if 0 // for IPv6
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

/******************************** END-OF-FILE ********************************/
