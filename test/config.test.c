#include "../core/config.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define CONFIG_FILENAME "test.conf"
#define SECTIONS 1024
#define KEYS_PER_SECTION 32
#define SHORT_ITEM 16
#define ITEM_MAXLEN 255
#define TOO_LONG_ITEM 512
#define RANDOM_SEED 6

char random_char()
{
	int c = rand() % 58;
	if(c<64) c+=64;
	return (char)c;
}

int main()
{
	struct config c;
	config_init(&c, CONFIG_FILENAME);
	srand(RANDOM_SEED);

	printf("Adding generating %d sections...\n", SECTIONS);
	char* sections[SECTIONS];
	char** keys[SECTIONS];
	int keys_per_section[SECTIONS];
	memset(sections, 0, SECTIONS * sizeof(char*));
	memset(keys, 0, SECTIONS * sizeof(char**));

	for(int i=0; i<SECTIONS; ++i)
	{
		int seclen = rand() % SHORT_ITEM;
		char* sec = (char*)malloc(seclen * sizeof(char));
		for(int j=0; j<seclen-1; ++j)
		{
			sec[j] = random_char();
		}
		sec[seclen-1]=0;
		sections[i]=sec;

		int keys_for_this_section=rand()%KEYS_PER_SECTION;
		keys[i] = (char**)malloc(keys_for_this_section * sizeof(char*));
		memset(keys[i], 0, keys_for_this_section * sizeof(char*));
		printf("Adding %d keys to section %d\n", keys_for_this_section, i);
		keys_per_section[i]=keys_for_this_section;
		for(int k=0; k<keys_for_this_section; ++k)
		{
			int keylen = rand() % SHORT_ITEM;
			keys[i][k] = (char*)malloc(keylen * sizeof(char));
			for(int n=0; n<keylen-1; ++n)
			{
				keys[i][k][n]=random_char();
			}
			keys[i][k][keylen-1]=0;

			int vallen = rand () % SHORT_ITEM;
			char* val = (char*)malloc(vallen * sizeof(char));
			for(int n=0; n<vallen-1; ++n)
			{
				val[n]=random_char();
			}
			val[vallen-1]=0;

			config_add(&c, sections[i], keys[i][k], val);
			free(val);
		}
	}
	//config_flush(&c, stdout);
	for(int i=0; i<SECTIONS; ++i)
	{
		for(int j=0; j<keys_per_section[i]; ++j)
		{
			free(keys[i][j]);
		}
		free(keys[i]);
		free(sections[i]);
	}
	config_free(&c);
	return 0;
}
