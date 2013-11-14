#ifndef TAPI2P_CORE_UTIL_H
#define TAPI2P_CORE_UTIL_H

int new_socket(const char* addr, const char* port);
int check_writability(int fd);
void set_nonblocking(int fd);
void set_blocking(int fd);

#endif
