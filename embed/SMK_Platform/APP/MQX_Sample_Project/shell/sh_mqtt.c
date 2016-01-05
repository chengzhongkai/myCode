/*****************************************************************************/
/* Copyright (c) 2015 SMK Corporation. All rights reserved.                  */
/*****************************************************************************/

#include <mqx.h>
#include <shell.h>
#include <ipcfg.h>

#include "Config.h"

#include "mqtt.h"
#include "mqtt_task.h"

extern _ip_address parse_ip(char *str);

//=============================================================================
static void on_recv_topic(MQTT_UTF8_t *topic, MQTT_Data_t *message)
{
	char temp[64];

	memcpy(temp, topic->ptr, topic->len);
	temp[topic->len] = '\0';
	printf("%s:", temp);

	memcpy(temp, message->ptr, message->len);
	temp[message->len] = '\0';
	printf("%s\n", temp);
}

//=============================================================================
static void on_error(uint32_t err_code)
{
}

//=============================================================================
static uint32_t mqtt_send(void *handle, const uint8_t *buf, uint32_t len)
{
	uint32_t sock = (uint32_t)handle;

	return (send(sock, (uint8_t *)buf, len, 0));
}

//=============================================================================
static uint32_t mqtt_recv(void *handle,
						  uint8_t *buf, uint32_t max, uint32_t timeout)
{
	uint32_t sock = (uint32_t)handle;

	return (recv(sock, buf, max, timeout));
}

//=============================================================================
static void mqtt_close(void *handle)
{
	uint32_t sock = (uint32_t)handle;

	shutdown(sock, 0);
}

//=============================================================================
static void *mqtt_open(void)
{
	uint32_t sock;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == RTCS_SOCKET_ERROR) {
		return (NULL);
	}

	return ((void *)sock);
}

//=============================================================================
static bool mqtt_connect(void *handle, uint32_t ipaddr, uint32_t portno)
{
	uint32_t sock = (uint32_t)handle;
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ipaddr;
	addr.sin_port = portno;

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) != RTCS_OK) {
		return (FALSE);
	}

	return (TRUE);
}

//=============================================================================
static void usage(void)
{
	printf("Usage: mqtt [ip addr]\n   Connect to MQTT Broker\n");
}

//=============================================================================
int32_t Shell_mqtt(int32_t argc, char *argv[])
{
	bool print_usage;
	bool short_help = FALSE;

	print_usage = Shell_check_help_request(argc, argv, &short_help);
	if (print_usage) {
		if (short_help) {
			printf("mqtt\n");
		} else {
			usage();
		}
		return (0);
	}

	if (argc != 2) {
		usage();
		return (0);
	}

	uint32_t ipaddr = parse_ip(argv[1]);

	MQTT_NetIF_t mqtt_netif;
	MQTT_Client_t *client;

	// --------------------------------------------------- Open and Connect ---
	void *handle;

	handle = mqtt_open();
	if (handle == NULL) {
		printf("socket open error\n");
		return (0);
	}
	if (!mqtt_connect(handle, ipaddr, 1883)) {
		printf("socket connect error\n");
		return (0);
	}

	// --------------------------------------------------- Register Net I/F ---
	mqtt_netif.handle = handle;
	mqtt_netif.send = mqtt_send;
	mqtt_netif.recv = mqtt_recv;
	mqtt_netif.close = mqtt_close;

	// ------------------------------------------------------ Create Handle ---
	client = MQTT_CreateHandle(&mqtt_netif);
	if (client != NULL) {
		MQTT_RegisterRecvTopicCB(client, on_recv_topic);
		MQTT_RegisterErrorCB(client, on_error);

		/*** Parameter ***/
		MQTT_SetClientID(client, "clientX");
		client->clean_session = TRUE;
		client->keep_alive = 5;

		/*** username and password ***/
		MQTT_UTF8_t username;
		MQTT_Data_t password;
		MQTT_Str2UTF8(&username, "smk");
		client->username = &username;
		password.len = 4;
		password.ptr = "test";
		client->password = &password;

		// ----------------------------------------------------- Start Task ---
		if (MQTT_StartTask(client, 9)) {
			_time_delay(3000);
#if 0
			MQTT_Subscribe(client, "smk/test", MQTT_QOS_0);
			MQTT_Subscribe(client, "slb/gateway", MQTT_QOS_1);
			MQTT_Subscribe(client, "smk/multi/remote", MQTT_QOS_2);
#endif

			MQTT_Subscribe(client, "smk/pubtest", MQTT_QOS_0);

			for (int cnt = 0; cnt < 30; cnt ++) {
				uint8_t msg[10];

				msg[0] = 'S';
				msg[1] = 'M';
				msg[2] = 'K';
				msg[3] = '0' + cnt;
				MQTT_Publish(client, "sensor/temp",
							 MQTT_QOS_0 + (cnt % 3), MQTT_RETAIN,
							 msg, 4);
				_time_delay(3000);
			}

			MQTT_Disconnect(client);
		}

		MQTT_ReleaseHandle(client);
		client = NULL;
	}

	return (0);
}

/******************************** END-OF-FILE ********************************/
