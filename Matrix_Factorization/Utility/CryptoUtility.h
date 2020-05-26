#ifndef CRYPTO_UTILITY_H__
#define CRYPTO_UTILITY_H__

#pragma once
#include <cassert>
#include <stdint.h>
#include <vector>
#include <bitset>
#include "Communicator.h"
#include "../GraphData.hpp"

#define PRECISION_BIT_LENGTH 16
#define DIM 10

namespace Utility
{
	std::ostream& operator<<( std::ostream& dest, __int128_t value );
	std::ostream& operator<<( std::ostream& dest, __uint128_t value );
	
	class VectorOperation
	{
	public:
		static std::vector<uint64_t> Add(const std::vector<uint64_t>& x, const std::vector<uint64_t>& y);
		static std::vector<uint64_t> Sub(const std::vector<uint64_t>& x, const std::vector<uint64_t>& y);
		static std::vector<uint64_t> Mul(const uint64_t x, const std::vector<uint64_t>& y);
		static std::vector<uint64_t> Mul(const std::vector<uint64_t>& x, const uint64_t y);
		static std::vector<uint64_t> Mul(const std::vector<uint64_t>& x, const std::vector<uint64_t>& y);
		static std::vector<uint64_t> Dot(const std::vector<uint64_t>& x, const std::vector<uint64_t>& y);
	};
	
	class CryptoUtility
	{
	public:
		static std::vector<uint64_t> SampleSmallInput(int length);
		
		static std::vector<unsigned char> SampleByteArray(int length);

		static std::vector<unsigned char> ComputeHash(std::vector<unsigned char> msg);
		
		static int32_t * SampleInt32Array(int length);
		
		static std::vector<uint64_t>  SampleUInt64Array(int length);
		
		static std::vector<int> buildMaskIndex(const std::vector<int>& itemsPerUser);
		
		template<typename T>
		static void shuffle(std::vector<T>& data, const std::vector<unsigned char>& seed);

		template<typename T>
		static void shuffle(std::vector<T>& data, __int128& seed);
	};
	
	class ArithmeticOperation
	{
	public:
		static uint64_t add(uint64_t x, uint64_t y){return x + y;}
		static uint64_t sub(uint64_t x, uint64_t y){return x - y;}
		static uint64_t mul(uint64_t x, uint64_t y){return (x * y) >> PRECISION_BIT_LENGTH;}
		static uint64_t div(uint64_t x, uint64_t y){return (x << PRECISION_BIT_LENGTH) / y;}
	};
	
	class ArrayEncoder
	{
	public:
		static std::vector<uint64_t> UCharVec2UInt64tVec(std::vector<unsigned char> array);
		
		static std::vector<unsigned char> Encode(std::vector<uint64_t> array);	
		static std::vector<unsigned char> Hash(std::vector<uint64_t> array);
		
		static std::vector<uint64_t>  Decodeuint64_t(std::vector<unsigned char> &array);
		
		static std::vector<unsigned char> EncodeUInt64Array(std::vector<std::vector<uint64_t> > array);
		static std::vector<std::vector<uint64_t> > DecodeUInt64Array(std::vector<unsigned char> array);
		
		/// Convert a 4-byte array to unsigned int value
		static int byteArrayToInt32(unsigned char *byteArray);

		/// Convert an unsigned int to 4-byte array
		static unsigned char * int32ToByteArray(unsigned int val);
		
		static std::vector<unsigned char> MacVectorToByteArray(std::vector<uint64_t> MACedEdges);
		
		static std::vector<unsigned char> MacVectorToByteArray(uint64_t *MACedEdges, int size);
		
		static std::vector<uint64_t> ByteArrayToMacVector(std::vector<unsigned char> byteArray);
	};
	
	// 40:  	87, 167, 195, 203, 213, 285, 293, 299, 389, 437
	class DistributedMACGenerator
	{
	public:
		DistributedMACGenerator(uint64_t alpha, uint64_t beta);
		
		uint64_t computeMAC(int party, std::vector<uint64_t> &data, uint64_t random);
		
		__int128 Z64;
	private:
		const uint64_t p = 0x10000000000 - 293;
		
		uint64_t alpha;
		uint64_t beta;
		
		std::vector<uint64_t> coeff;
	};
	
	class SPDZ2kMAC
	{
	public:
		SPDZ2kMAC(uint64_t alpha, Communicator *com);
		uint64_t computeMAC(int party, uint64_t data, uint64_t random);
		uint64_t singleCheck(int party, uint64_t mac, uint64_t share);
// 		uint64_t multipleCheck(int party, std::vector<uint64_t> mac, std::vector<uint64_t> share);
// 	private:
		uint64_t alpha;
		Communicator *com;
	};
	
	class TestUtility
	{
	public:
		static void PrintByteArray(const std::vector<unsigned char>& array, const std::string& str);
		static void PrintVector(const std::vector<int16_t>& input, const std::string& str);
		static void PrintVector(const std::vector<uint16_t>& input, const std::string& str);
		static void PrintVector(const std::vector<uint64_t>& input, const std::string& str);
		static void PrintVector(const std::vector<std::vector<uint64_t> >& input, const std::string& str, float scale);
		static void PrintMask(const std::vector<uint64_t>& masks, const std::vector<int>& maskIndex, const std::vector<int>& itemsPerUser, const std::string& str);
		static std::vector<std::vector<uint64_t> > GenerateInput(const std::vector<int>& itemsPerUser, int inputLength);
	};
}

#endif
