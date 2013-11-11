#include "file.h"
#include "pathmanager.h"
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char* SHA1_for_part(unsigned part, unsigned char* data, size_t datasize)
{
	unsigned char* ret=malloc(SHA_DIGEST_LENGTH);
	size_t bytes;
	if(part*FILE_PART_BYTES+FILE_PART_BYTES > datasize)
	{
		bytes=datasize-(part*FILE_PART_BYTES);
	}
	else bytes=FILE_PART_BYTES;

	SHA1(data+(part*FILE_PART_BYTES), bytes, ret);

	return ret;
}

void create_metadata_file(const char* from, char* file_sha_as_str)
{
	FILE* f=fopen(from, "r");
	unsigned char fullsha[SHA_DIGEST_LENGTH];
	if(f)
	{
		long filesize=0;
		unsigned char* filebuf;
		fseek(f,0,SEEK_END);
		filesize=ftell(f);
		fseek(f,0,SEEK_SET);
		filebuf=malloc(filesize);
		fread(filebuf, filesize, 1, f);
		fclose(f);
		int part_count=(filesize/FILE_PART_BYTES)+1;

#ifndef NDEBUG
		printf("File size: %u\n", filesize);
		printf("FilePart count: %d\n", part_count);
		printf("Part count * part size: %d\n", part_count*FILE_PART_BYTES);
		printf("SHA1 for file: ");
#endif
		SHA1(filebuf, filesize, fullsha);

		char sha_char[2];
		char* shap=file_sha_as_str;
		for(int i=0; i<SHA_DIGEST_LENGTH; ++i)
		{
			sprintf(sha_char, "%02x", fullsha[i]);
			shap=stpcpy(shap, sha_char);
		}

		char* mdpath=NULL;
		getpath(metadatapath(), file_sha_as_str, &mdpath);
		if(mdpath)
		{
			FILE* mdbin=fopen(mdpath, "w");
			free(mdpath);

			fwrite(fullsha, SHA_DIGEST_LENGTH, 1, mdbin);
			for(int i=0; i<part_count; ++i)
			{
				unsigned char* sha=SHA1_for_part(i, filebuf, filesize);
				fwrite(sha, SHA_DIGEST_LENGTH, 1, mdbin);
				free(sha);
			}
			fclose(mdbin);
		}
		free(filebuf);
	}
	else
	{
		printf("Failed to create metadata for file %s\n", from);
	}
}
