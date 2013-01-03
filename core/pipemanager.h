#ifndef TAPI2P_PIPEMANAGER_H
#define TAPI2P_PIPEMANAGER_H

#define MAX_PIPE_LISTENERS 255

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "event.h"

static fd_set pipeset;
static int pipe_fds[MAX_PIPE_LISTENERS];
static int pipe_slot[2];
static int fd_max;

void pipe_init(void);
void pipe_add(int fd);
void pipe_remove(int fd);
struct Event* poll_event(void);

#endif
