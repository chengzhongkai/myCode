#include <rtcs.h>
#include <dns.h>
#include <ipcfg.h>

uint32_t DNS_Domain2IP(char *domain)
{
	HOSTENT_STRUCT_PTR DtoI_data;

//DNSリゾルバ設定
	//char DNS_Lnn[] = ".";
	char DNS_Lsn[] = "";
	IPCFG_IP_ADDRESS_DATA MyIP_addr;

	ipcfg_get_ip(BSP_DEFAULT_ENET_DEVICE,&MyIP_addr);

	DNS_First_Local_server[0].NAME_PTR = DNS_Lsn;
	DNS_First_Local_server[0].IPADDR = MyIP_addr.gateway;

//ドメイン名->IPアドレスリスト
	DtoI_data = gethostbyname(domain);

	return (*(uint32_t *)(DtoI_data->h_addr_list[0]));

}
