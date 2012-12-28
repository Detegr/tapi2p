#include "config.h"
#include <stdlib.h>
#include <string.h>

static int item_compare(const void* a, const void* b)
{
	return strncmp((const char*)a,((struct configitem*)b)->key,ITEM_MAXLEN);
}

static int section_compare(const void* a, const void* b)
{
	return strncmp((const char*)a,((struct configsection*)b)->name,ITEM_MAXLEN);
}

static int section_sort(const void* a, const void* b)
{
	return strncmp((*(struct configsection**)a)->name,(*(struct configsection**)b)->name,ITEM_MAXLEN);
}

static void item_free(struct configitem* item)
{
	free(item->key);
	if(item->val) free(item->val);
}

static void section_free(struct configsection* sect)
{
	free(sect->name);
	for(unsigned int i=0; i<sect->items; ++i)
	{
		item_free(sect->item[i]);
	}
	free(sect->item);
}

void config_free(struct config* conf)
{
	if(conf->filename) free(conf->filename);
	for(unsigned int i=0; i<conf->sections; ++i)
	{
		section_free(conf->section[i]);
	}
	free(conf->section);
}

static int item_find(const char* needle, struct configsection* haystack)
{
	int max=haystack->items-1;
	int min=0;
	while(max>=min)
	{
		int mid=(unsigned int)((max-min)/2);
		int c = item_compare(needle, haystack->item[mid]);
		if(c<0) min=mid+1;
		else if(c>0) max=mid-1;
		else return mid;
	}
	return -1;
}

void config_init(struct config* conf, const char* filename)
{
	conf->filename = strdup(filename);
	conf->sections = 0;
	conf->size = 0;
	conf->section=NULL;
}

static void item_add(struct configsection* section, const char* key, const char* val)
{
	/* Allocate more space if needed */
	if(section->items+1 > section->size)
	{
		struct configitem** newi = (struct configitem**)malloc(section->size?section->size*2:10 * sizeof(struct configitem*));
		memcpy(newi, section->item, section->size);
		section->size = section->size?section->size*2:10;
		/* Free old item */
		struct configitem** tmp=section->item;
		section->item = newi;
		free(tmp);
	}
	/* Assign new item */
	struct configitem* newitem = (struct configitem*)malloc(sizeof(struct configitem));
	newitem->key = strdup(key);
	if(val) newitem->val = strdup(val);
	else newitem->val = NULL;
	section->item[section->items++] = newitem;
}

void config_add(struct config* conf, const char* section, const char* key, const char* val)
{
	struct configsection* sect;
	if(conf->sections > 0 && (sect=config_find_section(section, conf)))
	{
		item_add(sect, key, val);
	}
	else
	{
		section_new(conf, section);
		config_add(conf, section, key, val);
	}
}

static void section_new(struct config* conf, const char* name)
{
	if(conf->sections+1 > conf->size)
	{
		struct configsection** news = (struct configsection**)malloc(conf->size?conf->size*2:10 * sizeof(struct configsection*));
		memcpy(news, conf->section, conf->size);
		conf->size = conf->size?conf->size*2:10;
		/* Free old item */
		struct configsection** tmp=conf->section;
		conf->section = news;
		free(tmp);
	}
	/* Assign new item */
	struct configsection* newsection = (struct configsection*)malloc(sizeof(struct configsection));
	newsection->name = strdup(name);
	newsection->items = 0;
	newsection->size = 0;
	conf->section[conf->sections++] = newsection;

	qsort(conf->section, conf->sections, sizeof(struct configsection*), section_sort);
}

struct configsection* config_find_section(const char* needle, struct config* haystack)
{
	int max=haystack->sections-1;
	int min=0;
	while(max>=min)
	{
		int mid=min + (unsigned int)((max-min)/2);
		int c = section_compare(needle, haystack->section[mid]);
		if(c<0) min=mid+1;
		else if(c>0) max=mid-1;
		else return haystack->section[mid];
	}
	return NULL;
}

struct configitem* config_find_item(const char* needle, struct config* haystack, const char* section)
{
	if(section)
	{
		struct configsection* sect;
		if(haystack->sections > 0 && (sect=config_find_section(section, haystack)))
		{
			int item=item_find(needle, sect);
			if(item != -1) return sect->item[item];
		}
	}
	else
	{
		for(unsigned int i=0; i<haystack->sections; ++i)
		{
			int item=item_find(needle, haystack->section[i]);
			if(item != -1) return haystack->section[i]->item[item];
		}
	}
	return NULL;
}

void config_flush(struct config* conf, FILE* stream)
{
	for(unsigned int i=0; i<conf->sections; ++i)
	{
		fprintf(stream, "[%s]\n", conf->section[i]->name);
		for(unsigned int j=0; j<conf->section[i]->items; ++j)
		{
			struct configitem* item = conf->section[i]->item[j];
			if(item->val)
			{
				fprintf(stream, "\t%s=%s\n", item->key, item->val);
			}
			else fprintf(stream, "\t%s\n", item->key);
		}
	}
}

int main()
{
	struct config c;
	config_init(&c, "test.conf");
	config_add(&c, "Section", "Key", "Value");
	config_add(&c, "Aasi", "Don", "Key");
	config_add(&c, "Aasi", "Cee", NULL);
	config_add(&c, "Baa", "Boh", "Dóh");
	config_flush(&c, stdout);

	printf("%s in config: %d\n", "Don", config_find_item("Dan", &c, NULL) != NULL);

	return 0;
}
