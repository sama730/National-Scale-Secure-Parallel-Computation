#ifndef ISECURE_RNG_H___
#define ISECURE_RNG_H___

#pragma once

#include <vector>
#include <stdexcept>
#include <random>
#include <ctime>
#include <chrono>

#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include <emp-tool/utils/block.h>
#include <emp-tool/utils/prp.h>
#include <emp-tool/utils/prg.h>

#if !defined (__AES__)
    #error "AES-NI instructions not enabled"
#endif

namespace Utility
{	
	// class ISecureRNG
	// {
	// public:
	// 	~ISecureRNG(){}
 // 		// virtual unsigned char *GetByteArray(int length) = 0;
	// 	std::vector<uint16_t> GetUInt16Array(int length) = 0;
	// 	std::vector<uint32_t> GetUInt32Array(int length) = 0;
	// 	std::vector<uint64_t> GetUInt64Array(int length) = 0;
	// private:
	// 	virtual unsigned char * Generate(unsigned char *plaintext, int plaintext_len, unsigned char * key) = 0;
	// };

	// class InsecureSeededRNG : public ISecureRNG
	// {
	// public:
	// 	~InsecureSeededRNG(){}
	// 	InsecureSeededRNG(){};
	// 	unsigned char * GetByteArray(int length);
	// 	std::vector<uint16_t> GetUInt16Array(int length);
	// 	std::vector<uint32_t> GetUInt32Array(int length);
	// 	std::vector<uint64_t> GetUInt64Array(int length);

	// private:
	// 	unsigned char * Generate(unsigned char * plaintext, int plaintext_len, unsigned char * key){return nullptr;}
	// };

	class AESRNG// : public ISecureRNG
	{
	private:
		int counter;
		unsigned char * key;
		emp::PRG *prg; //((const char *)(&_key));
		// Return the cipher text
		unsigned char * Generate(unsigned char * plaintext, int plaintext_len, unsigned char * key);

	public:

		AESRNG(unsigned char *seed);
		
		~AESRNG()
		{
 			// delete [] key;
		
			delete prg;
		}

		unsigned char * GetByteArray();
		std::vector<uint16_t> GetUInt16Array(int length);
		std::vector<uint32_t> GetUInt32Array(int length);
		std::vector<uint64_t> GetUInt64Array(int length);
		std::vector<uint64_t> GetUInt128Array(int length);
		std::vector<uint64_t>  GetMaskArray(int length);
	};
}

#endif
