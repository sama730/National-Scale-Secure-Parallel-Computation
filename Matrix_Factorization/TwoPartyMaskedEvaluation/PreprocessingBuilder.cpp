#include <future>
#include <thread>
#include <iostream>
#include "PreprocessingBuilder.h"
#include "PreprocessingShare.h"

#include "../Utility/Timer.h"
#include "../Utility/ISecureRNG.h"
#include "../Utility/CryptoUtility.h"

using namespace Circuit;
using namespace Utility;

namespace TwoPartyMaskedEvaluation
{
	std::vector<int64_t> MaskShareBuilder::Build(LayeredArithmeticCircuit *lc, AESRNG *rng, const std::vector<int>& itemsPerUser)
	{
		Timer t;
		
		int numUsers = itemsPerUser.size();
		int sumNumItems = 0;
		for(int idx = 0; idx < itemsPerUser.size(); idx++)
		{
			sumNumItems += itemsPerUser[idx];
		}
		
		int numMasks = 6*DIM*numUsers + (2*DIM + 4)*sumNumItems;
		
		std::vector<int64_t> maskShare = rng->GetMaskArray(numMasks);

// 		t.Tick("AES time");
		
		std::vector<int> maskIndex = CryptoUtility::buildMaskIndex(itemsPerUser);
		
		for (int layer = 0; layer < lc->Depth; layer++)
		{
			ArithmeticLayer *bl = lc->operator[](layer);

			/// (x + lx) + (y + ly) = (x + y) + (lx + ly)
			for (auto &g : bl->AddGates)
			{
				int leftIndex = maskIndex[g->LeftWire];
				int rightIndex = maskIndex[g->RightWire];
				int outIndex = maskIndex[g->OutputWire];
				
				for(int idx = 0; idx < DIM; idx++)
				{
					maskShare[outIndex] = maskShare[leftIndex] + maskShare[rightIndex];
					leftIndex++; rightIndex++; outIndex++;
				}
			}
			
			for (auto &g : bl->NAddGates)
			{
				int outIndex = maskIndex[g->OutputWire];
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					maskShare[outIndex + kdx] = 0;
				}
				
				for(int idx = 0; idx < g->InputWires.size(); idx++)
				{
					int inputIndex = maskIndex[g->InputWires[idx]];
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						maskShare[outIndex + kdx] += maskShare[inputIndex + kdx];
					}
				}
			}
			
			/// (x + lx) + c = (x + c) + lx
			for (auto &g : bl->CAddGates)
			{
				int inIndex = maskIndex[g->InputWire];
				int outIndex = maskIndex[g->OutputWire];
				
				for(int idx = 0; idx < DIM; idx++)
				{
					maskShare[outIndex] = maskShare[inIndex];
					inIndex++; outIndex++;
				}
			}
			
			/// (x + lx) - (y + ly) = (x - y) + (lx - ly)
			for (auto &g : bl->SubGates)
			{
				int leftIndex = maskIndex[g->LeftWire];
				int rightIndex = maskIndex[g->RightWire];
				int outIndex = maskIndex[g->OutputWire];
				
				maskShare[outIndex] = maskShare[leftIndex] - maskShare[rightIndex];
				leftIndex++; rightIndex++; outIndex++;
			}

			__int128 temp;
			/// (x + lx)*c = x*c + lx*c
			for (auto &g : bl->CMulGates)
			{
				int inIndex = maskIndex[g->InputWire];
				int outIndex = maskIndex[g->OutputWire];
				
				for(int idx = 0; idx < DIM; idx++)
				{
					temp = g->constant;
					temp *= (__int128)maskShare[inIndex + idx];
					temp = (temp >> PRECISION_BIT_LENGTH);
					maskShare[outIndex + idx] = temp;
					
					maskShare[outIndex + idx] = maskShare[inIndex + idx];
				}
			}
		}
		
// 		t.Tick("MaskShareBuilder");

		return maskShare;
	}

	std::vector<unsigned char> PreprocessingBuilder::getSeedAlice()
	{
		return privateSeedAlice;
	}

	void PreprocessingBuilder::setSeedAlice(const std::vector<unsigned char>& value)
	{
		privateSeedAlice = value;
	}

	std::vector<unsigned char> PreprocessingBuilder::getSeedBob()
	{
		return privateSeedBob;
	}

	void PreprocessingBuilder::setSeedBob(const std::vector<unsigned char>& value)
	{
		privateSeedBob = value;
	}

	const std::vector<int64_t>& PreprocessingBuilder::getMasks()
	{
		return privateMasks;
	}

	int64_t PreprocessingBuilder::getMasks(int idx)
	{
		return privateMasks[idx];
	}
	
	void PreprocessingBuilder::setMasks(const std::vector<int64_t>& value)
	{
		privateMasks = value;
	}
	
	void PreprocessingBuilder::setMasks(std::vector<int64_t>&& value)
	{
		privateMasks = std::move(value);
	}

	std::vector<std::vector<int64_t> > PreprocessingBuilder::getBobCorrection() 
	{
		return privateBobCorrection;
	}

	void PreprocessingBuilder::addBobCorrection(std::vector<int64_t> value)
	{
		privateBobCorrection.push_back(value);
	}
	
	void PreprocessingBuilder::setBobCorrection(const std::vector<std::vector<int64_t> >& value)
	{
		privateBobCorrection = value;
	}
	
	void PreprocessingBuilder::BuildPreprocessing(std::vector<unsigned char>& seedAlice, std::vector<unsigned char>& seedBob, LayeredArithmeticCircuit *lc, const std::vector<int>& itemsPerUser)
	{
		Timer t;
		
		
		int numUsers = itemsPerUser.size();
		int sumNumItems = 0;
		for(int idx = 0; idx < itemsPerUser.size(); idx++)
		{
			sumNumItems += itemsPerUser[idx];
		}
		
		int numMasks = 6*DIM*numUsers + (2*DIM + 4)*sumNumItems;
		
		this->setSeedAlice(seedAlice);
		this->setSeedBob(seedBob);
		
		AESRNG *rng = new AESRNG(seedBob.data());
		
		std::future<PreprocessingShare *> f1 = std::async(std::launch::async, [&]{ return AlicePreprocessingBuilder::Build(lc, seedAlice, itemsPerUser); });
		std::future<std::vector<int64_t> > f2 = std::async(std::launch::async, [&]{ return MaskShareBuilder::Build(lc, rng, itemsPerUser); });
		
		f1.wait();
		f2.wait();
		
		
// 		std::cout << "Get alice share..." << std::endl;
		PreprocessingShare *aliceShare = f1.get();
		
// 		std::cout << "Get maskShare..." << std::endl;
		std::vector<int64_t> maskShare  = f2.get();
		
// 		t.Tick("Generate share and mask");
		
		setBobCorrection(std::vector<std::vector<int64_t> >());

		std::vector<int64_t> privateMask(numMasks);
		
// 		std::cout << "Adding mask shares..." << std::endl;
		for(int idx = 0; idx < numMasks; idx++)
		{
			privateMask[idx] = maskShare[idx] + aliceShare->maskShare[idx];
// 			std::stringstream ss;
// 			ss << "private mask idx: " << maskShare[idx] << " " << aliceShare->maskShare[idx] << " " << privateMask[idx] << std::endl;
// 			std::cout << ss.str() << std::endl;
		}
		
		for (int layer = 0; layer < lc->Depth; layer++)
		{
			ArithmeticLayer *bl = lc->operator[](layer);
			for (auto &g : bl->CMulGates)
			{
// 				int inIndex = maskIndex[g->InputWire];
				int outIndex = aliceShare->maskIndex[g->OutputWire];
				for(int idx = 0; idx < DIM; idx++)
				{
					privateMask[outIndex + idx] = ((g->constant*privateMask[outIndex + idx]) >> PRECISION_BIT_LENGTH);
				}
			}
			
			for (auto &g : bl->AddGates)
			{
				int leftIndex = aliceShare->maskIndex[g->LeftWire];
				int rightIndex = aliceShare->maskIndex[g->RightWire];
				int outIndex = aliceShare->maskIndex[g->OutputWire];
				
				for(int idx = 0; idx < DIM; idx++)
				{
					privateMask[outIndex + idx] = privateMask[leftIndex + idx] + privateMask[rightIndex + idx];
				}
			}
		}
				
				
		setMasks(std::move(privateMask));
		
// 		t.Tick("Get private mask");
		
		std::vector<int> maskIndex = CryptoUtility::buildMaskIndex(itemsPerUser);
		
// 		std::cout << "Adding beaver shares..." << std::endl;
		for (int layer = 0; layer < lc->Depth; layer++)
		{
			auto mulGates = lc->operator[](layer)->MulGates;
			int count = mulGates.size();
			
			if (count > 0)
			{
				std::vector<int64_t> beaverShare(DIM*count);

				for (int i = 0; i < count; i++)
				{
					MultiplicationGate *gate = mulGates[i];
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						// Multiplication between a scalar and a vector
						beaverShare[DIM*i + kdx] = getMasks(maskIndex[gate->LeftWire])*getMasks(maskIndex[gate->RightWire] + kdx);
					}
				}
				
				std::vector<int64_t> beaverShareAlice = aliceShare->GetNextBeaverTriples();
				
				for(int idx = 0; idx < DIM*count; idx++)
				{
					beaverShare[idx] -= beaverShareAlice[idx];
				}
				
				addBobCorrection(std::move(beaverShare));
			}
			
			auto dotGates = lc->operator[](layer)->DotGates;
			count = dotGates.size();
			
			if (count > 0)
			{
				std::vector<int64_t> beaverShare(count);
				std::vector<int64_t> beaverShareAlice = aliceShare->GetNextBeaverTriples();
				
				for (int i = 0; i < count; i++)
				{
					DotProductGate *gate = dotGates[i];
					beaverShare[i] = 0;
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						beaverShare[i] += getMasks(maskIndex[gate->LeftWire] + kdx)*getMasks(maskIndex[gate->RightWire] + kdx);
					}
					
					beaverShare[i] -= beaverShareAlice[i];
				}
				
				addBobCorrection(std::move(beaverShare));
			}
			
			auto nDotGates = lc->operator[](layer)->NDotGates;
			count = nDotGates.size();
			
			if (count > 0)
			{
				std::vector<int64_t> beaverShare(DIM*count);
				std::vector<int64_t> beaverShareAlice = aliceShare->GetNextBeaverTriples();
				
				for (int i = 0; i < count; i++)
				{
					NaryDotGate *gate = nDotGates[i];
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{						
						int step = gate->InputWires.size()/2;
						
						for(int jdx = 0; jdx < step; jdx++)
						{
							beaverShare[DIM*i + kdx] += getMasks(maskIndex[gate->InputWires[jdx]])*getMasks(maskIndex[gate->InputWires[jdx + step]] + kdx);
						}
						
						beaverShare[DIM*i + kdx] = (beaverShare[DIM*i + kdx]);
					}
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						beaverShare[DIM*i + kdx] -= beaverShareAlice[DIM*i + kdx];
					}
				}
				
				addBobCorrection(std::move(beaverShare));
			}
		}
		
// 		t.Tick("Beaver thing");
	}

	PreprocessingShare *AlicePreprocessingBuilder::Build(LayeredArithmeticCircuit *lc, std::vector<unsigned char>& seed, const std::vector<int>& itemsPerUser)
	{
		Timer t;
// 		std::cout << "AlicePreprocessingBuilder::Init rng..." << std::endl;
		AESRNG *rng = new AESRNG(seed.data());
		
// 		std::cout << "AlicePreprocessingBuilder::MaskShareBuilder..." << std::endl;
		auto maskShare = MaskShareBuilder::Build(lc, rng, itemsPerUser);
				
		std::vector<std::vector<int64_t> > beaverShare;
		
		for (int layer = 0;layer < lc->Depth;layer++)
		{
			auto mulGates = lc->operator[](layer)->MulGates;
			int count = mulGates.size();
			if (count > 0)
			{
				beaverShare.push_back(rng->GetMaskArray(DIM*count));
			}
			
			auto dotGates = lc->operator[](layer)->DotGates;
			count = dotGates.size();
			if (count > 0)
			{
				beaverShare.push_back(rng->GetMaskArray(count));
			}
			
			auto nDotGates = lc->operator[](layer)->NDotGates;
			count = nDotGates.size();
			if (count > 0)
			{
				beaverShare.push_back(rng->GetMaskArray(DIM*count));
			}
		}
		
// 		TestUtility::PrintVector(beaverShare, "Alice beaver share", 1);
		
// 		std::cout << "AlicePreprocessingBuilder::PreprocessingShare..." << std::endl;
		PreprocessingShare *share = new PreprocessingShare(std::move(maskShare), std::move(beaverShare), CryptoUtility::buildMaskIndex(itemsPerUser));
		
// 		t.Tick("AlicePreprocessingBuilder::Build");
		return share;
	}
	
	PreprocessingShare *BobPreprocessingBuilder::Build(LayeredArithmeticCircuit *lc, std::vector<unsigned char>& seed, std::vector<std::vector<int64_t> >&& beaverShare, const std::vector<int>& itemsPerUser)
	{
// 		Timer t;
// 		std::cout << "Bob: Init rng..." << std::endl;
		AESRNG *rng = new AESRNG(seed.data());
		
// 		std::cout << "Bob: MaskShareBuilder::Build..." << std::endl;
		auto maskShare = MaskShareBuilder::Build(lc, rng, itemsPerUser);
		
// 		std::cout << "Bob: MaskShareBuilder::End..." << std::endl;
		PreprocessingShare *share = new PreprocessingShare(std::move(maskShare), std::move(beaverShare), CryptoUtility::buildMaskIndex(itemsPerUser));
		
// 		t.Tick("BobPreprocessingBuilder::Build");
		
		return share; 
	}
}
