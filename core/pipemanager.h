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
evt_t* poll_event(void);
int send_event(evt_t* e);

#endif
