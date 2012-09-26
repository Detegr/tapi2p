#include "KeyGenerator.h"
#include "PrivateKey.h"
#include "PublicKey.h"
#include <fstream>
#include <iostream>

int main()
{
	RSA_KeyGenerator::WriteKeyPair("foobar");
	/*
	std::ifstream is("foobar.enc");
	std::string line;
	std::string file;
	if(is.is_open())
	{
		while(is.good())
		{
			unsigned char c;
			c=(unsigned char)is.get();
			if(is.good()) file+=c;
		}
	}
	is.close();
	RSA_PublicKey pub;
	pub.Load("foobar");

	unsigned char* out;
	size_t outlen;
	pub.Encrypt((const unsigned char*)"foobar", 6, &out, &outlen);
	RSA_PrivateKey p;
	p.Load("foobar");
	p.Decrypt(out,outlen);
	OPENSSL_free(out);
	*/
}
