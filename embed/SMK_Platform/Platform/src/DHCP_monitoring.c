#include <mqx.h>
#include <message.h>
#include "NetConnect.h"

static DHCP_Check_Calllback *gDHCPCheckCallback = NULL;

void DHCP_Check_SetCallback(DHCP_Check_Calllback *func)
{
	gDHCPCheckCallback = func;
}

void DHCP_Check_Task_WakeUp(void *dummy)
{
	_task_id tid = (_task_id)dummy;

	_task_ready(_task_get_td(tid));
}

void DHCP_Check_Task(uint32_t dummy)
{
	GROUPWARE_MESSAGE_STRUCT_PTR DHCP_msgdata;
	_queue_number q_no = (_queue_number)dummy;

	uint32_t prev_ip[3] = {0};

	_msgq_open_system(q_no, 10,
					  &DHCP_Check_Task_WakeUp, (void *)_task_get_id());


	while(1)
	{

		DHCP_msgdata = _msgq_poll(_msgq_get_id(0, q_no));

		if(DHCP_msgdata == NULL)
		{
			_task_block();
		}
		else
		{
			if(DHCP_msgdata->data[0] == NET_CONNECT_MSG_STATUS)
			{
				if(prev_ip[0] != DHCP_msgdata->data[1]
				   || prev_ip[1] != DHCP_msgdata->data[2]
				   || prev_ip[2] != DHCP_msgdata->data[3])
				{
					if (gDHCPCheckCallback != NULL)
					{
						(*gDHCPCheckCallback)(DHCP_msgdata->data[1],
											  DHCP_msgdata->data[2],
											  DHCP_msgdata->data[3]);
					}

#if 0
					//IP address
					if((DHCP_msgdata->data[1] != 0)
					   && (DHCP_msgdata->data[2] != 0)
					   && (DHCP_msgdata->data[3] != 0))
					{
						printf("\nIpAddress :%d.%d.%d.%d\n",
							   (DHCP_msgdata->data[1] & 0xFF000000) >> 24,
							   (DHCP_msgdata->data[1] & 0x00FF0000) >> 16,
							   (DHCP_msgdata->data[1] & 0x0000FF00) >> 8,
							   (DHCP_msgdata->data[1] & 0x000000FF));

						printf("SubNetMask:%d.%d.%d.%d\n",
							   (DHCP_msgdata->data[2] & 0xFF000000) >> 24,
							   (DHCP_msgdata->data[2] & 0x00FF0000) >> 16,
							   (DHCP_msgdata->data[2] & 0x0000FF00) >> 8,
							   (DHCP_msgdata->data[2] & 0x000000FF));

						printf("GateWay   :%d.%d.%d.%d\n\n",
							   (DHCP_msgdata->data[3] & 0xFF000000) >> 24,
							   (DHCP_msgdata->data[3] & 0x00FF0000) >> 16,
							   (DHCP_msgdata->data[3] & 0x0000FF00) >> 8,
							   (DHCP_msgdata->data[3] & 0x000000FF));
					}
					else
					{
						printf("\nIpAddress :Error\n\n");
					}
#endif

					prev_ip[0] = DHCP_msgdata->data[1];
					prev_ip[1] = DHCP_msgdata->data[2];
					prev_ip[2] = DHCP_msgdata->data[3];
				}
			}
			else if(DHCP_msgdata->data[0] == NET_CONNECT_MSG_FATAL_ERROR)
			{
				printf("DHCP:Error\n");
			}

			_msg_free(DHCP_msgdata);
		}
	}
}
