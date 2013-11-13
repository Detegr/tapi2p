#include "file.h"
#include "pathmanager.h"
#include <sys/stat.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "peer.h"
#include "event.h"
#include "core.h"

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

void sha_to_str(const unsigned char* sha, char* out)
{
	char sha_char[2];
	char* shap=out;
	for(int i=0; i<SHA_DIGEST_LENGTH; ++i)
	{
		sprintf(sha_char, "%02x", sha[i]);
		shap=stpcpy(shap, sha_char);
	}
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
		sha_to_str(fullsha, file_sha_as_str);

		char* mdpath=NULL;
		getpath(metadatapath(), file_sha_as_str, &mdpath);
		if(mdpath)
		{
			FILE* mdbin=fopen(mdpath, "w");
			free(mdpath);

			unsigned char fp=Metadata; // To distinguish the beginning of a
			fwrite(&fp, 1, 1, mdbin); //  metadata from the data when receiving
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

void check_or_create_metadata(const unsigned char* sha_data, size_t sha_size)
{
	struct config* conf=getconfig();
	struct stat buf;
	char sha_str[SHA_DIGEST_LENGTH*2+1];
	sha_to_str(sha_data, sha_str);
	char* mdpath=NULL;
	getpath(metadatapath(), sha_str, &mdpath);
	if(stat(mdpath, &buf) == -1)
	{
		FILE* f=fopen(mdpath, "w");
		if(f)
		{
			unsigned char type=Metadata;
			fwrite(&type, 1, 1, f);
			fwrite(sha_data, sha_size, 1, f);
			fclose(f);
		}
		else
		{
			fprintf(stderr, "Could not open file %s. Failed to write metadata.\n", mdpath);
		}
	}
	else
	{
		printf("Metadata %s already exists\n", sha_str);
	}
	free(mdpath);
}


static void* request_thread(void* args)
{
	fprequest_t* req=((void**)args)[0];
	struct peer* p=((void**)args)[1];
	const char* filename="testfile.out"; // For testing
	evt_t e;
	event_init(&e, RequestFilePart, (const unsigned char*)req, sizeof(*req));
	send_data_to_peer(p, &e);
	event_free_s(&e);
	free(req);
	free(args);
	return 0;
}

void request_file_part_from_peer(int partnum, const char* sha_str, struct peer* p)
{
	printf("Requesting file part for %s for peer %s:%u\n", sha_str, p->addr, p->port);
	fprequest_t* req=malloc(sizeof(fprequest_t));
	req->part=partnum;
	strncpy(req->sha_str, sha_str, SHA_DIGEST_LENGTH*2+1);
	void** data=malloc(2*sizeof(void*));
	data[0]=req;
	data[1]=p;
	pthread_t requestt;
	pthread_create(&requestt, NULL, &request_thread, data);
}
