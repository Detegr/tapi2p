#ifndef TAPI2P_PATHMANAGER_H
#define TAPI2P_PATHMANAGER_H

#include <stdio.h>
#include "../dtgconf/src/config.h"

const char* basepath(void);
const char* configpath(void);
const char* keypath(void);
const char* selfkeypath(void);
const char* selfkeypath_pub(void);
const char* socketpath(void);

FILE* configfile(void);
struct config* getconfig(void);

void pathmanager_free(void);

#endif
