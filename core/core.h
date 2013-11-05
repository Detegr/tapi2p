#ifndef TAPI2P_CORE_H
#define TAPI2P_CORE_H

#define FILE_PART_BYTES 524288 // 512kb file part size

#include "event.h"

int core_start(void);
void core_stop(void);
int core_socket(void);
void send_to_all(evt_t* e);
void send_data_to_peer(struct peer* p, evt_t* e);

#endif
