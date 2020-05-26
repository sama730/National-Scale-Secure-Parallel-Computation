#include <openssl/sha.h>
#include "PreprocessingParty.h"
#include "CrossCheckException.h"
#include "../TwoPartyMaskedEvaluation/PreprocessingBuilder.h"
#include "../Utility/Timer.h"

#include <string>
#include <sstream>
#include <iostream>

using namespace Utility;
using namespace TwoPartyMaskedEvaluation;
using namespace Circuit;

namespace CrossCheck
{

	PreprocessingParty::PreprocessingParty(int player, Communicator *communicator, bool isAlice)
	{
		this->player = player;
		this->communicator = communicator;
		this->isAlice = isAlice;
	}

	void PreprocessingParty::RunPreprocessing(LayeredArithmeticCircuit *lc, std::vector<uint64_t>& masks, PreprocessingShare * &share, std::vector<uint64_t>& unTruncatedMasks, const std::vector<int>& itemsPerUser)
	{
		std::cout << "Run Preprocessing Party" << std::endl;
		std::vector<int> shareIndex = CryptoUtility::buildMaskIndex(itemsPerUser);
		
		if(player == Alice || player == Bob)
		{
			std::cout << "Alice & Bob build" << std::endl;
			BuildPreprocessing(masks, unTruncatedMasks, lc, itemsPerUser, shareIndex);
			
			std::cout << "Alice & Bob receive" << std::endl;
			share = ReceivePreprocessing(lc, itemsPerUser, shareIndex);
		}
		else
		{
			std::cout << "Charlie & David receive" << std::endl;
			share = ReceivePreprocessing(lc, itemsPerUser, shareIndex);
			
			std::cout << "Charlie & David build" << std::endl;
			BuildPreprocessing(masks, unTruncatedMasks, lc, itemsPerUser, shareIndex);
		}
// 		std::future<std::vector<uint64_t> > f1 = std::async(std::launch::async, [&]{return BuildPreprocessing(lc, itemsPerUser, shareIndex);} );
// 		std::future<PreprocessingShare *> f2 = std::async(std::launch::async, [&]{return ReceivePreprocessing(lc, itemsPerUser, shareIndex);} );
// 		
// 		f1.wait();
// 		f2.wait();
// 		
// 		masks = f1.get();
// 		share = f2.get();
	}
	
	void PreprocessingParty::BuildPreprocessing(std::vector<uint64_t>& masks, std::vector<uint64_t>& unTruncatedMasks, LayeredArithmeticCircuit *lc, const std::vector<int>& itemsPerUser, const std::vector<int>& shareIndex)
	{
		std::cout << "Build preprocessing" << std::endl;
		std::vector<unsigned char> seedAlice, seedBob;
		
		int stop;
		
		// Pair (Alice, Bob) generates seeds and sends to the other pair
		if (isAlice)
		{
			std::cout << "Sending seeds..." << std::endl;
			GenerateSeeds(seedAlice, seedBob);
			
			communicator->SendEvaluationPartner(seedAlice.data(), seedAlice.size());
			communicator->SendEvaluationPartner(seedBob.data(), seedBob.size());
		}
		else
		{
			std::cout << "Receiving seeds..." << std::endl;
			seedAlice.resize(32);
			seedBob.resize(32);
			communicator->AwaitEvaluationPartner(seedAlice.data(), seedAlice.size());
			communicator->AwaitEvaluationPartner(seedBob.data(), seedBob.size());
		}
		
		
		TestUtility::PrintByteArray(seedAlice, "Seed Alice");
		TestUtility::PrintByteArray(seedBob, "Seed Bob");
		
		// Both Alice and Bob sends seeds for the other pair
		communicator->SendAlice(seedAlice.data(), seedAlice.size());
		communicator->SendBob(seedBob.data(), seedBob.size());
		
		PreprocessingBuilder *builder = new PreprocessingBuilder();
		Timer t;
		builder->BuildPreprocessing(seedAlice, seedBob, lc, itemsPerUser, shareIndex);
		t.Tick("BuildPreprocessing -- builder->BuildPreprocessing(seedAlice, seedBob, lc)");
		
		std::vector<unsigned char> maskCorrection = ArrayEncoder::EncodeUInt64Array(builder->getBobCorrection());
		
		std::cout << "Sending mask correction" << std::endl;
		for(int idx = 0; idx < 10; idx++) std::cout << (int)maskCorrection[maskCorrection.size() - 1 - idx] << " ";
		std::cout << std::endl;
		
		if(isAlice)
		{
// 			std::cout << "Alice sending bob mask correction: " << maskCorrection.size() << std::endl;
			uint64_t size = maskCorrection.size();
			communicator->SendBob((unsigned char *)(&size), sizeof(uint64_t));
			communicator->SendBob(maskCorrection.data(), maskCorrection.size());
			t.Tick("SendBob maskCorrection");
			std::vector<unsigned char> hash = CryptoUtility::ComputeHash(maskCorrection);
		}
		else{
// 			std::cout << "Bob sending bob hash: " << maskCorrection.size() << std::endl;
			std::vector<unsigned char> hash = CryptoUtility::ComputeHash(maskCorrection);
			uint64_t size = hash.size();
			communicator->SendBob((unsigned char *)(&size), sizeof(uint64_t));
			communicator->SendBob(hash.data(), hash.size());
			t.Tick("SendBob hash");
		}
		
		masks = builder->getMasks();
		unTruncatedMasks = builder->getUntruncatedMasks();
	}
	
	PreprocessingShare *PreprocessingParty::ReceivePreprocessing(LayeredArithmeticCircuit *lc, const std::vector<int>& itemsPerUser, const std::vector<int>& shareIndex)
	{
// 		Timer t;
		std::cout << "Waiting for seeds from Alice & Bob" << std::endl;
		std::vector<unsigned char> mySeed1(32), mySeed2(32);
		communicator->AwaitAlice(mySeed1.data(), mySeed1.size());
		communicator->AwaitBob(mySeed2.data(), mySeed2.size());
		
		PreprocessingShare *share;
		
		TestUtility::PrintByteArray(mySeed1, "Received Seed From Alice");
		TestUtility::PrintByteArray(mySeed2, "Received Seed From Bob");
		
// 		std::cout << "Received seeds..." << std::endl;
		if (!(mySeed1 == mySeed2))
		{
			throw InconsistentSeedException("Alice and Bob sent different seeds.");
		}
		
		AESRNG *rng = new AESRNG(mySeed1.data());
		share = MaskShareBuilder::Build(lc, rng, itemsPerUser, shareIndex);
		
		if (!isAlice)
		{
// 			std::cout << "Else of AlicePreprocessingBuilder::Build(lc, mySeed1.data());" << std::endl;
			uint64_t size;
			communicator->AwaitAlice((unsigned char *)(&size), sizeof(uint64_t));
			std::vector<unsigned char> maskCorrectionEncoding(size);
			communicator->AwaitAlice(maskCorrectionEncoding.data(), maskCorrectionEncoding.size());
			
			communicator->AwaitBob((unsigned char *)(&size), sizeof(uint64_t));
			std::vector<unsigned char> hash(size);
			communicator->AwaitBob(hash.data(), hash.size());
			std::vector<unsigned char> myHash = CryptoUtility::ComputeHash(maskCorrectionEncoding);
			
			std::cout << "Receiving mask correction" << std::endl;
			for(int idx = 0; idx < 10; idx++) std::cout << (int)maskCorrectionEncoding[maskCorrectionEncoding.size() - 1 - idx] << " ";
			std::cout << std::endl;
			
			TestUtility::PrintByteArray(hash, "hash");
			TestUtility::PrintByteArray(myHash, "myHash");
			
			if (!(hash == myHash))
			{
				throw InconsistentHashException("Preprocessing share production inconsistent.");
			}

			std::vector<std::vector<uint64_t> > maskCorrection = ArrayEncoder::DecodeUInt64Array(maskCorrectionEncoding);
			
			// Bob correct the masks for the wires
			
			int count = 0;
			for (int layer = 0;layer < lc->Depth;layer++)
			{
				ArithmeticLayer *bl = lc->operator[](layer);
				auto cMulGates = lc->operator[](layer)->CMulGates;
				
				if (cMulGates.size() > 0)
				{
					std::vector<uint64_t> correction = maskCorrection[count]; count++;
					int count2 = 0;
					
					for (auto &g : bl->CMulGates)
					{
						int outIndex = shareIndex[g->OutputWire];
						
						for(int idx = 0; idx < DIM; idx++)
						{
							share->maskShare[outIndex] = correction[count2];
							count2++; outIndex++;
						}
					}
				}
				
				// Correct maskShare[z] for output of mul gate
				// lz = beaver[z] - lx*ly
				auto mulGates = lc->operator[](layer)->MulGates;
				
				if (mulGates.size() > 0)
				{
					std::vector<uint64_t> correction = maskCorrection[count]; count++;
					int count2 = 0;
					
					for (int i = 0; i < mulGates.size(); i++)
					{
						MultiplicationGate *gate = mulGates[i];
						int outIndex = shareIndex[gate->OutputWire];
						
						for(int kdx = 0; kdx < DIM; kdx++)
						{
							share->maskShare[outIndex] = correction[count2];
							count2++; outIndex++;
						}
					}
				}
				
				auto dotGates = lc->operator[](layer)->DotGates;
				
				if (dotGates.size() > 0)
				{
					std::vector<uint64_t> correction = maskCorrection[count]; count++;
					for (int i = 0; i < dotGates.size(); i++)
					{
						
						DotProductGate *gate = dotGates[i];
						int outIndex = shareIndex[gate->OutputWire];
						
						share->maskShare[outIndex] = correction[i];
					}
				}
				
				auto nDotGates = lc->operator[](layer)->NDotGates;
				
				// Dot gate structures
				// Left wires: InputWires[0] --> InputWires[#wire/2 - 1]
				// Right wires: InputWires[#wires/2] --> InputWires[#wires - 1]
				// Left wire: single value
				// Right wire: vector of size DIM
				if (nDotGates.size() > 0)
				{
					std::vector<uint64_t> correction = maskCorrection[count]; count++;
					int count2 = 0;
					
					for (int i = 0; i < nDotGates.size(); i++)
					{
						NaryDotGate *gate = nDotGates[i];
						
						int outIndex = shareIndex[gate->OutputWire];
						
						for(int kdx = 0; kdx < DIM; kdx++)
						{						
							share->maskShare[outIndex] = correction[count2];
							count2++; outIndex++;
						}
					}
				}
			}
		}
		
		// Propagate the masks layer by layer
		for (int layer = 0;layer < lc->Depth;layer++)
		{
			ArithmeticLayer *bl = lc->operator[](layer);
			
			// Propagate the mask share for linear gates

			/// (x + lx) + (y + ly) = (x + y) + (lx + ly)
			for (auto &g : bl->AddGates)
			{
				int leftIndex = shareIndex[g->LeftWire];
				int rightIndex = shareIndex[g->RightWire];
				int outIndex = shareIndex[g->OutputWire];
				
				for(int idx = 0; idx < DIM; idx++)
				{
					share->maskShare[outIndex + idx] = share->maskShare[leftIndex + idx] + share->maskShare[rightIndex + idx];
				}
			}
			
			// Sum (x_i + lx_i) = (Sum x_i) + (Sum lx_i)
			for (auto &g : bl->NAddGates)
			{
				int outIndex = shareIndex[g->OutputWire];
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					share->maskShare[outIndex + kdx] = 0;
				}
				
				for(int idx = 0; idx < g->InputWires.size(); idx++)
				{
					int inputIndex = shareIndex[g->InputWires[idx]];
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						share->maskShare[outIndex + kdx] += share->maskShare[inputIndex + kdx];
					}
				}
			}
			
			/// (x + lx) + c = (x + c) + lx
			for (auto &g : bl->CAddGates)
			{
				int inIndex = shareIndex[g->InputWire];
				int outIndex = shareIndex[g->OutputWire];
				
				for(int idx = 0; idx < DIM; idx++)
				{
					share->maskShare[outIndex + idx] = share->maskShare[inIndex + idx];
				}
			}
			
			/// (x + lx) - (y + ly) = (x - y) + (lx - ly)
			// Sub gates are for: rating - <u, v>
			// The dimension of sub gate is 1 per wire
			for (auto &g : bl->SubGates)
			{
				int leftIndex = shareIndex[g->LeftWire];
				int rightIndex = shareIndex[g->RightWire];
				int outIndex = shareIndex[g->OutputWire];
				
				share->maskShare[outIndex] = share->maskShare[leftIndex] - share->maskShare[rightIndex];
			}
		}
		
// 		t.Tick("ReceivePreprocessing");
		return share;
	}
}
