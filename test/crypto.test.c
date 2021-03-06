#include "../crypto/privatekey.h"
#include "../crypto/publickey.h"
#include "../crypto/aes.h"
#include "../crypto/keygen.h"
#include <stdio.h>

int main()
{
	FILE* priv=fopen("test", "r");
	FILE* pub=fopen("test.pub", "r");
	if(!priv || !pub)
	{
		printf("Generating keypair...\n");
		if(generate("test", T2PPRIVATEKEY|T2PPUBLICKEY) == -1)
		{
			fprintf(stderr, "Failed to create keypair!\n");
			return -1;
		}
	}
	else {fclose(priv); fclose(pub);}
	size_t len;
	struct pubkey pubkey;
	pubkey_init(&pubkey);
	pubkey_load(&pubkey, "test.pub");
	enc_t* data=aes_encrypt_random_pass((unsigned char*)"foobar", strlen("foobar"), 64, &pubkey, &len);
	pubkey_free(&pubkey);
	size_t declen;
	struct privkey privkey;
	privkey_init(&privkey);
	privkey_load(&privkey, "test");
	unsigned char* decdata=aes_decrypt_with_key(data, &privkey, &declen);
	for(unsigned int i=0; i<declen; ++i)
	{
		printf("%c", decdata[i]);
	}
	printf("\n");
	free(data);
}
