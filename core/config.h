#ifndef TAPI2P_CONFIG_H
#define TAPI2P_CONFIG_H

#define ITEM_MAXLEN 255

struct configitem
{
	char* key;
	char* val;
};

struct configsection
{
	char* name;
	struct configitem* item;
};

struct config
{
	char* filename;
	struct configitem* sections;
};

static int item_compare(const void* a, const void* b);

static void item_free(struct configitem* item);
static void section_free(struct configsection* sect);

void config_free(struct config* conf);
void config_load(struct config* conf, const char* filename);
void config_flush();

#endif
