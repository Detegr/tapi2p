#ifndef TAPI2P_CORE_H
#define TAPI2P_CORE_H

int core_start(void);
void core_stop(void);
int core_socket(void);
void send_to_all(unsigned char* data_to_enc, int len);

#endif
