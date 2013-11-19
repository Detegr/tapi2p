#ifndef TAPI2P_PIPEMANAGER_H
#define TAPI2P_PIPEMANAGER_H

#define MAX_PIPE_LISTENERS 255

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "event.h"

void pipe_init(void);
void pipe_add(int fd);
void pipe_remove(int fd);
pipeevt_t* poll_event_from_pipes(void);
int send_event_to_pipes(pipeevt_t* e);
int send_event_to_pipes_simple(EventType t, const unsigned char* data, unsigned int data_len);

#endif
