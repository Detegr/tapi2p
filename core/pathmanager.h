#ifndef TAPI2P_PATHMANAGER_H
#define TAPI2P_PATHMANAGER_H

#include <stdio.h>
#include "config.h"

static char* basepath_str=NULL;
static char* configpath_str=NULL;
static char* keypath_str=NULL;
static char* selfkeypath_str=NULL;
static char* socketpath_str=NULL;
static struct config conf={0};

static const char* getpath(const char* base, const char* add, char** to);

const char* basepath(void);
const char* configpath(void);
const char* keypath(void);
const char* selfkeypath(void);
const char* socketpath(void);

struct config* getconfig(void);

void pathmanager_free(void);

#endif
