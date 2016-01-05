/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include "mdns.h"
#include "platform.h"
#include "mqx_utils.h"

//=============================================================================
typedef struct {
	uint16_t id;
	uint16_t flags;
	uint16_t qd_count;
	uint16_t an_count;
	uint16_t ns_count;
	uint16_t ar_count;

	uint16_t len;
	const uint8_t *data;
} MDNS_Header_t;

#define MDNS_Bytes2Uint16(buf) (((uint16_t)((buf)[0]) << 8) | (uint16_t)((buf)[1]))

//=============================================================================
#define MDNS_TASK_STACK_SIZE (1024)
#define MDNS_TASK_NAME "mDNS Responder"

#define MDNS_IPADDR ((224 << 24) | (251)) // 224.0.0.251
#define MDNS_PORTNO (5353)

//=============================================================================
static _task_id gMDNS_TaskId;

#define MDNS_HOSTNAME_SIZE (32) // less than 255
static uint8_t gFQDN[MDNS_HOSTNAME_SIZE + 8];
static int gFQDN_len = 0;

static uint8_t gResponsePacket[12 + (MDNS_HOSTNAME_SIZE + 8) + 14 + 20];
static int gResponsePacket_len = 0;

#define MDNS_RECV_BUF_SIZE (1024)
static uint8_t gMDNS_RecvBuf[MDNS_RECV_BUF_SIZE];

//=============================================================================
static void MDNS_Task(uint32_t param);

//=============================================================================
uint32_t MDNS_StartTask(const char *hostname, uint32_t priority)
{
	uint32_t result;
	int len;
	const uint8_t local[] = {
		5, 'l', 'o', 'c', 'a', 'l', 0
	};

	len = strlen(hostname);
	if (len > MDNS_HOSTNAME_SIZE) {
		return (MQX_ERROR);
	}

	gFQDN[0] = len;
	strcpy((char *)&gFQDN[1], hostname);
	memcpy(&gFQDN[len + 1], local, 7);

	gFQDN_len = len + 8;

	result = MQX_CreateTask(&gMDNS_TaskId, MDNS_Task, MDNS_TASK_STACK_SIZE,
							priority, MDNS_TASK_NAME, 0);

	return (result);
}

//=============================================================================
static bool MDNS_ParseHeader(const uint8_t *buf, uint32_t len,
							 MDNS_Header_t *header)
{
	if (buf == NULL || len < 12 || header == NULL) return (FALSE);

	header->id = MDNS_Bytes2Uint16(buf);
	buf += 2;
	header->flags = MDNS_Bytes2Uint16(buf);
	buf += 2;
	header->qd_count = MDNS_Bytes2Uint16(buf);
	buf += 2;
	header->an_count = MDNS_Bytes2Uint16(buf);
	buf += 2;
	header->ns_count = MDNS_Bytes2Uint16(buf);
	buf += 2;
	header->ar_count = MDNS_Bytes2Uint16(buf);
	buf += 2;

	header->len = len - 12;
	header->data = buf;

#if 0
	printf("[mDNS] recv packet\n");
	printf("ID: 0x%04X\n", header->id);
	printf("Flags: 0x%04X\n", header->flags);
	printf("QDCOUNT: 0x%04X\n", header->qd_count);
	printf("ANCOUNT: 0x%04X\n", header->an_count);
	printf("NSCOUNT: 0x%04X\n", header->ns_count);
	printf("ARCOUNT: 0x%04X\n", header->ar_count);

	printf("Data:\n");
	for (int cnt = 0; cnt < header->len; cnt ++) {
		printf("0x%02X ", header->data[cnt]);
		if (((cnt + 1) & 0xf) == 0) printf("\n");
	}
	printf("\n");
#endif

	return (TRUE);
}

//=============================================================================
static void MDNS_SendResponsePacket(uint32_t sock)
{
    struct sockaddr_in addr;
    int send_size;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = MDNS_IPADDR;
    addr.sin_port = MDNS_PORTNO;

    send_size = sendto(sock, (char *)gResponsePacket, gResponsePacket_len,
					   RTCS_MSG_NOLOOP,
					   (struct sockaddr *)&addr, sizeof(addr));
}

//=============================================================================
static void MDNS_HandleQuery(uint32_t sock, MDNS_Header_t *header)
{
	if (header == NULL) {
		return;
	}

	if (header->qd_count > 0 && header->data != NULL) {
		if (memcmp(header->data, gFQDN, gFQDN_len) == 0) {
			// ------------------------------------------------ Query Match ---
			const uint8_t qtype_qclass[] = {0x00, 0x01, 0x00, 0x01};
			if (memcmp(&header->data[gFQDN_len], qtype_qclass, 4) == 0) {
				// ------------------------------------------ Send Response ---
				MDNS_SendResponsePacket(sock);
			}
		}
	}
}

//=============================================================================
static void MDNS_MakeResponsePacket(uint32_t ipaddr)
{
	const uint8_t header[] = {
		0x00, 0x00, 0x84, 0x00,
		0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01
	};
	const uint8_t ipv4[] = {
		0x00, 0x01, 0x80, 0x01,
		0x00, 0x00, 0x78, 0x00,
		0x00, 0x04,
		0x00, 0x00, 0x00, 0x00
	};
	const uint8_t nsec[] = {
		0xc0, 0x0c,
		0x00, 0x2f, 0x80, 0x01,
		0x00, 0x00, 0x78, 0x00,
		0x00, 0x08,
		0xc0, 0x0c, 0x00, 0x04, 0x40, 0x00, 0x00, 0x00
	};

	memcpy(gResponsePacket, header, 12);
	memcpy(&gResponsePacket[12], gFQDN, gFQDN_len);
	memcpy(&gResponsePacket[12 + gFQDN_len], ipv4, 14);
	gResponsePacket[12 + gFQDN_len + 10] = (ipaddr >> 24) & 0xff;
	gResponsePacket[12 + gFQDN_len + 11] = (ipaddr >> 16) & 0xff;
	gResponsePacket[12 + gFQDN_len + 12] = (ipaddr >> 8) & 0xff;
	gResponsePacket[12 + gFQDN_len + 13] = ipaddr & 0xff;
	memcpy(&gResponsePacket[12 + gFQDN_len + 14], nsec, 20);

	gResponsePacket_len = 12 + gFQDN_len + 14 + 20;
}

//=============================================================================
static void MDNS_Task(uint32_t param)
{
	uint32_t sock;
    struct sockaddr_in addr;
    struct ip_mreq mreq;
    uint16_t addrlen;
    IPCFG_IP_ADDRESS_DATA ip_data;
	MDNS_Header_t header;
	int recv_size;

	// -------------------------------------------------------- Open Socket ---
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == RTCS_SOCKET_ERROR) {
		err_printf("[mDNS] open error\n");
		_task_block();
	}

	// -------------------------------------------------------- Bind Socket ---
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = MDNS_PORTNO;

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) != RTCS_OK) {
		err_printf("[mDNS] bind error\n");
		shutdown(sock, 0);
		_task_block();
	}

    // --------------------------------------------- Join Multicast Address ---
	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_multiaddr.s_addr = MDNS_IPADDR; // inet_addr("224.0.0.251");
    ipcfg_get_ip(BSP_DEFAULT_ENET_DEVICE, &ip_data);
	mreq.imr_interface.s_addr = ip_data.ip;
	if (setsockopt(sock, SOL_IGMP, RTCS_SO_IGMP_ADD_MEMBERSHIP,
				   &mreq, sizeof(mreq)) != RTCS_OK) {
		err_printf("[mDNS] setsockopt error\n");
		shutdown(sock, 0);
		_task_block();
	}

	MDNS_MakeResponsePacket(ip_data.ip);

	// ---------------------------------------------------------- Main Loop ---
	while (1) {
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = MDNS_PORTNO;
		addrlen = sizeof(addr);

		recv_size = recvfrom(sock, gMDNS_RecvBuf, MDNS_RECV_BUF_SIZE, 0,
							 (struct sockaddr *)&addr, &addrlen);
		if (recv_size > 0 && ip_data.ip != addr.sin_addr.s_addr) {
			if (MDNS_ParseHeader(gMDNS_RecvBuf, recv_size, &header)) {
				if (header.id == 0x0000 && header.flags == 0x0000) {
					// --------------------------------------- Query Packet ---
					MDNS_HandleQuery(sock, &header);
				}
			}
		}
	}
}

/******************************** END-OF-FILE ********************************/
