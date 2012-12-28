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

#define ITERATIONS 4

char random_char()
{
	int c = rand() % 58;
	if(c<64) c+=64;
	return (char)c;
}

int main()
{
	for(int iteration=0; iteration<ITERATIONS; ++iteration)
	{
		struct config c;
		config_init(&c, CONFIG_FILENAME);
		srand(RANDOM_SEED+iteration);
		printf("Generating %d sections...\n", SECTIONS);
		char* sections[SECTIONS];
		char** keys[SECTIONS];
		int keys_per_section[SECTIONS];
		memset(sections, 0, SECTIONS * sizeof(char*));
		memset(keys, 0, SECTIONS * sizeof(char**));

		for(int i=0; i<SECTIONS; ++i)
		{
			int seclen = (rand() % SHORT_ITEM) + 2;
			char* sec = (char*)malloc(seclen * sizeof(char));
			for(int j=0; j<seclen-1; ++j)
			{
				sec[j] = random_char();
			}
			sec[seclen-1]=0;
			sections[i]=sec;

			int keys_for_this_section=(rand()%KEYS_PER_SECTION) + 1;
			keys[i] = (char**)malloc(keys_for_this_section * sizeof(char*));
			memset(keys[i], 0, keys_for_this_section * sizeof(char*));
			printf("Adding %d keys to section %d\n", keys_for_this_section, i);
			keys_per_section[i]=keys_for_this_section;
			for(int k=0; k<keys_for_this_section; ++k)
			{
				int keylen = (rand() % SHORT_ITEM) + 2;
				keys[i][k] = (char*)malloc(keylen * sizeof(char));
				for(int n=0; n<keylen-1; ++n)
				{
					keys[i][k][n]=random_char();
				}
				keys[i][k][keylen-1]=0;

				int vallen = (rand () % SHORT_ITEM) + 2;
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
		for(int i=0; i<SECTIONS; ++i)
		{
			printf("Searching section %d\n", i);
			assert(config_find_section(&c, sections[i]) != NULL);
		}
		for(int i=0; i<SECTIONS; ++i)
		{
			printf("Searching nonexistent section, round %d\n", i);
			int sectlen = (rand() % SHORT_ITEM) + 2;
			char sect[sectlen];
			for(int n=0; n<sectlen-1; ++n)
			{
				sect[n]=random_char();
			}
			sect[sectlen-1]=0;
			int exists=0;
			for(int j=0; j<SECTIONS; ++j)
			{
				if(strcmp(sections[j], sect) == 0) {exists=1;break;}
			}
			if(!exists)
			{
				assert(config_find_section(&c, sect) == NULL);
			}
			else i--;
		}
		for(int i=0; i<SECTIONS; ++i)
		{
			for(int j=0; j<keys_per_section[i]; ++j)
			{
				printf("Searching existing item from ALL sections, round %d, item %d\n", i, j);
				assert(config_find_item(&c, keys[i][j], NULL));
				printf("Searching existing item from correct section, round %d, item %d\n", i, j);
				assert(config_find_item(&c, keys[i][j], sections[i]));
				printf("Searching existing item from wrong section, round %d, item %d\n", i, j);
				int nonexistent=0;
				for(int z=0; z<SECTIONS; ++z)
				{
					if(config_find_item(&c, keys[i][j], sections[z]) != NULL) continue;
					else {nonexistent=1;break;}
				}
				assert(nonexistent);

				int exists_key=1;
				int exists_sec=1;
				int keylen = (rand() % SHORT_ITEM) + 6; // +6 to prevent generating too short names that would collide (at least 5 characters)
				int seclen = (rand() % SHORT_ITEM) + 6; // +6 to prevent generating too short names that would collide (at least 5 characters)
				char key[keylen];
				char sec[seclen];
				do
				{
					if(exists_key)
					{
						for(int n=0; n<keylen-1; ++n)
						{
							key[n]=random_char();
						}
						key[keylen-1]=0;
						exists_key=0;
					}
					if(exists_sec)
					{
						for(int g=0; g<seclen-1; ++g)
						{
							sec[g]=random_char();
						}
						sec[seclen-1]=0;
						exists_sec=0;
					}
					for(int x=0; x<SECTIONS; ++x)
					{
						for(int c=0; c<keys_per_section[x]; ++c)
						{
							if(strcmp(keys[x][c], key) == 0) exists_key=1;
							if(strcmp(sections[x], sec) == 0) exists_sec=1;
						}
					}
				} while(exists_key || exists_sec);
				printf("Searching nonexistent item from ALL sections, round %d, item %d\n", i, j);
				assert(config_find_item(&c, key, NULL) == NULL);
				printf("Searching nonexistent item from existing section, round %d, item %d\n", i, j);
				assert(config_find_item(&c, key, sections[i]) == NULL);
				printf("Searching nonexistent item from nonexistent section, round %d, item %d\n", i, j);
				assert(config_find_item(&c, key, sec) == NULL);
				printf("Searching existing item from nonexistent section, round %d, item %d\n", i, j);
				assert(config_find_item(&c, keys[i][j], sec) == NULL);
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
	}
	return 0;
}
