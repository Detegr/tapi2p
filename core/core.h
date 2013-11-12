#ifndef TAPI2P_CORE_H
#define TAPI2P_CORE_H

#define SOCKET_ONEWAY -2

// Length of the password used to AES encrypt data
#define PW_LEN 80

#include "event.h"

int core_start(void);
void core_stop(void);
int core_socket(void);
void send_to_all(evt_t* e);
void send_data_to_peer(struct peer* p, evt_t* e);

#endif
