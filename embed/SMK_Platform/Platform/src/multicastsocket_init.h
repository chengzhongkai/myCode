#ifndef MULTI_SOCKET_H_
#define MULTI_SOCKET_H_

#include <rtcs.h>
extern uint32_t MulticastSocket_init(uint32_t port, _ip_address ip, _ip_address multicastip);
void MulticastSocket_Reset(void);


#endif /* MULTI_SOCKET_H_ */