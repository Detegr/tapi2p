#include "pathmanager.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

static char* basepath_str=NULL;
static char* configpath_str=NULL;
static char* keypath_str=NULL;
static char* selfkeypath_str=NULL;
static char* selfkeypath_pub_str=NULL;
static char* socketpath_str=NULL;
static struct config conf={0};
static struct config* conf_ptr=NULL;

static const char* getpath(const char* base, const char* add, char** to);

const char* basepath(void)
{
	if(!basepath_str)
	{
		char* tapiroot=getenv("TAPI2P_ROOT");
		if(tapiroot)
		{
			size_t rootlen=strnlen(tapiroot, PATH_MAX);
			int separator=0;
			if(tapiroot[rootlen-1]!='/') separator=1;
			basepath_str=(char*)malloc((rootlen+1+separator) * sizeof(char));
			strncpy(basepath_str, tapiroot, rootlen+1);
			if(separator)
			{
				basepath_str[rootlen]='/';
				basepath_str[rootlen+1]=0;
			}
		}
		else
		{
			const char* tapipath="/.config/tapi2p/";
			tapiroot=getenv("HOME");
			if(!tapiroot)
			{
				fprintf(stderr, "Couldn't get HOME directory\n");
				return NULL;
			}
			size_t pathlen=strlen(tapipath);
			size_t rootlen=strnlen(tapiroot, PATH_MAX);
			size_t newlen=pathlen + rootlen + 1;
			if(newlen>PATH_MAX)
			{
				fprintf(stderr, "Tapi2p path too long.\n");
				return NULL;
			}
			basepath_str=(char*)malloc(newlen * sizeof(char));
			stpncpy(stpncpy(basepath_str, tapiroot, rootlen+1), tapipath, pathlen+1);
		}
	}
	return basepath_str;
}

static const char* getpath(const char* base, const char* add, char** to)
{
	if(!*to)
	{
		size_t addlen=strlen(add);
		size_t baselen = strnlen(base, PATH_MAX);
		size_t newlen=addlen + baselen + 1;
		if(newlen>PATH_MAX)
		{
			fprintf(stderr, "Tapi2p path too long.\n");
			return NULL;
		}
		*to = (char*)malloc(newlen * sizeof(char));
		stpncpy(stpncpy(*to, base, baselen+1), add, addlen+1);
	}
	return *to;
}

const char* configpath(void)
{
	return getpath(basepath(), "config", &configpath_str);
}

const char* keypath(void)
{
	return getpath(basepath(), "keys/", &keypath_str);
}

const char* selfkeypath(void)
{
	return getpath(keypath(), "self", &selfkeypath_str);
}

const char* selfkeypath_pub(void)
{
	return getpath(keypath(), "self.pub", &selfkeypath_pub_str);
}

const char* socketpath(void)
{
	return getpath(basepath(), "t2p_core", &socketpath_str);
}

void pathmanager_free(void)
{
	if(basepath_str) free(basepath_str);
	if(configpath_str) free(configpath_str);
	if(keypath_str) free(keypath_str);
	if(selfkeypath_str) free(selfkeypath_str);
	if(socketpath_str) free(socketpath_str);
	if(selfkeypath_pub_str) free(selfkeypath_pub_str);
}

struct config* getconfig(void)
{
	if(!conf_ptr)
	{
		if(config_load(&conf, configpath()) == -1)
		{
			config_init(&conf);
		}
		conf_ptr=&conf;
	}
	return conf_ptr;
}
