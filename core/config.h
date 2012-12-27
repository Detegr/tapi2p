#ifndef TAPI2P_CONFIG_H
#define TAPI2P_CONFIG_H

#include <stdio.h>
#define ITEM_MAXLEN 255

struct configitem
{
	char* key;
	char* val;
};

struct configsection
{
	char* name;
	unsigned int items;
	unsigned int size;
	struct configitem** item;
};

struct config
{
	char* filename;
	unsigned int sections;
	unsigned int size;
	struct configsection** section;
};

static int item_compare(const void* a, const void* b);
static int section_compare(const void* a, const void* b);
static int section_sort(const void* a, const void* b);

static void item_add(struct configsection* section, const char* key, const char* val);
static int item_find(const char* needle, struct configsection* haystack);
static void item_free(struct configitem* item);

static void section_new(struct config* conf, const char* name);
static void section_free(struct configsection* sect);

struct configsection* config_find_section(const char* needle, struct config* haystack);
struct configitem* config_find_item(const char* needle, struct config* haystack, const char* section);
void config_init(struct config* conf, const char* filename);
void config_free(struct config* conf);
void config_load(struct config* conf, const char* filename);
void config_add(struct config* conf, const char* section, const char* key, const char* val);
void config_flush(struct config* conf, FILE* stream);

#endif
