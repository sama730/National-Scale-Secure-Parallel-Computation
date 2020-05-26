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
	// Generate random value for each input wire and output wire of mult/cmult/dot/ndot gates
	PreprocessingShare * MaskShareBuilder::Build(LayeredArithmeticCircuit *lc, AESRNG *rng, const std::vector<int>& itemsPerUser, const std::vector<int>& maskIndex)
	{
		Timer t;
		
		int numUsers = itemsPerUser.size();
		int sumNumItems = 0;
		for(int idx = 0; idx < itemsPerUser.size(); idx++)
		{
			sumNumItems += itemsPerUser[idx];
		}
		
		int numMasks = 6*DIM*numUsers + (2*DIM + 4)*sumNumItems;
		int numInputMasks = DIM*(numUsers + sumNumItems) + 2*sumNumItems;
		
		// Generate random mask for each wire in the input layer
		std::vector<uint64_t> maskShare = rng->GetUInt64Array(numInputMasks);
		
		// Fill the rest with 0
		maskShare.resize(numMasks);
		
		// Count the number of masks for the output wires of the mult type gates
		int nBeaverMasks = 0;
		for (int layer = 0; layer < lc->Depth; layer++)
		{
			nBeaverMasks += DIM*lc->operator[](layer)->CMulGates.size();
			nBeaverMasks += DIM*lc->operator[](layer)->MulGates.size();
			nBeaverMasks +=     lc->operator[](layer)->DotGates.size();
			nBeaverMasks += DIM*lc->operator[](layer)->NDotGates.size();
		}
		
		std::vector<uint64_t> beaverMask = rng->GetUInt64Array(nBeaverMasks);
		
		// Now we generate beaver share for non-linear gates
		// z <- x*y: beaverShare[z] = [lz + lx*ly]
		// As this won't agree with current lz (mask[z] is a random value),
		// lz needs to be corrected later.
		std::vector<std::vector<uint64_t> > beaverShare;
		
		int count = 0;
		for (int layer = 0;layer < lc->Depth;layer++)
		{
			auto cMulGates = lc->operator[](layer)->CMulGates;
			if(cMulGates.size() > 0)
			{
				beaverShare.push_back(rng->GetMaskArray(DIM*cMulGates.size()));
				
				for(auto &g : cMulGates)
				{
					int wireIndex = g->OutputWire;
					for(int idx = 0; idx < DIM; idx++)
					{
						maskShare[maskIndex[wireIndex] + idx] = beaverMask[count];
						count++;
					}
				}
			}
			
			auto mulGates = lc->operator[](layer)->MulGates;
			if (mulGates.size() > 0)
			{
				beaverShare.push_back(rng->GetMaskArray(DIM*mulGates.size()));
				
				for(auto &g : mulGates)
				{
					int wireIndex = g->OutputWire;
					for(int idx = 0; idx < DIM; idx++)
					{
						maskShare[maskIndex[wireIndex] + idx] = beaverMask[count];
						count++;
					}
				}
			}
			
			auto dotGates = lc->operator[](layer)->DotGates;
			if (dotGates.size() > 0)
			{
				beaverShare.push_back(rng->GetMaskArray(dotGates.size()));
				
				for(auto &g : dotGates)
				{
					int wireIndex = g->OutputWire;
					maskShare[maskIndex[wireIndex]] = beaverMask[count];
					count++;
				}
			}
			
			auto nDotGates = lc->operator[](layer)->NDotGates;
			if (nDotGates.size() > 0)
			{
				beaverShare.push_back(rng->GetMaskArray(DIM*nDotGates.size()));
				
				for(auto &g : nDotGates)
				{
					int wireIndex = g->OutputWire;
					for(int idx = 0; idx < DIM; idx++)
					{
						maskShare[maskIndex[wireIndex] + idx] = beaverMask[count];
						count++;
					}
				}
			}
		}
		
// 		TestUtility::PrintVector(beaverShare, "Alice beaver share", 1);
		
// 		std::cout << "AlicePreprocessingBuilder::PreprocessingShare..." << std::endl;
		PreprocessingShare *share = new PreprocessingShare(std::move(maskShare), std::move(beaverShare), maskIndex);
		
// 		t.Tick("AlicePreprocessingBuilder::Build");
		return share;
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

	const std::vector<uint64_t>& PreprocessingBuilder::getMasks()
	{
		return privateMasks;
	}
	
	const std::vector<uint64_t>& PreprocessingBuilder::getUntruncatedMasks()
	{
		return unTruncatedMasks;
	}

	uint64_t PreprocessingBuilder::getMasks(int idx)
	{
		return privateMasks[idx];
	}
	
	void PreprocessingBuilder::setMasks(const std::vector<uint64_t>& value)
	{
		privateMasks = value;
	}
	
	void PreprocessingBuilder::setMasks(std::vector<uint64_t>&& value)
	{
		privateMasks = std::move(value);
	}

	void PreprocessingBuilder::setUntruncatedMasks(const std::vector<uint64_t>& m)
	{
		unTruncatedMasks = m;
	}
	
	std::vector<std::vector<uint64_t> > PreprocessingBuilder::getBobCorrection() 
	{
		return privateBobCorrection;
	}

	void PreprocessingBuilder::addBobCorrection(std::vector<uint64_t> value)
	{
		privateBobCorrection.push_back(value);
	}
	
	void PreprocessingBuilder::setBobCorrection(const std::vector<std::vector<uint64_t> >& value)
	{
		privateBobCorrection = value;
	}
	
	void PreprocessingBuilder::BuildPreprocessing(std::vector<unsigned char>& seedAlice, std::vector<unsigned char>& seedBob, LayeredArithmeticCircuit *lc, const std::vector<int>& itemsPerUser, const std::vector<int>& maskIndex)
	{
		Timer t;
		
		
		int numUsers = itemsPerUser.size();
		int sumNumItems = 0;
		for(int idx = 0; idx < itemsPerUser.size(); idx++)
		{
			sumNumItems += itemsPerUser[idx];
		}
		
		int numMasks = 6*DIM*numUsers + (2*DIM + 4)*sumNumItems;
		int numInputMasks = DIM*(numUsers + sumNumItems) + 2*sumNumItems;
		
// 		std::vector<int> maskIndex = CryptoUtility::buildMaskIndex(itemsPerUser);
		
		this->setSeedAlice(seedAlice);
		this->setSeedBob(seedBob);
		
		AESRNG *rngAlice = new AESRNG(seedAlice.data());
		AESRNG *rngBob = new AESRNG(seedBob.data());
		
		// Generate random masks for input wires
		// Generate (incorrected) random masks and beaver triplets for output wires of mult-type gates
		std::cout << "Get alice share..." << std::endl;
		PreprocessingShare *aliceShare = MaskShareBuilder::Build(lc, rngAlice, itemsPerUser, maskIndex);
		
		std::cout << "Get bob share..." << std::endl;
		PreprocessingShare *bobShare   = MaskShareBuilder::Build(lc, rngBob, itemsPerUser, maskIndex);
		
		std::cout << "Get correction for bos's share..." << std::endl;
		// Now, we need to correct the bob's shared mask for the output wires of mult-type gates
		setBobCorrection(std::vector<std::vector<uint64_t> >());

		std::cout << "Reconstruct incorrect private masks" << std::endl;
		std::vector<uint64_t> privateMask(numMasks);
		
		// Reconstruct the mask for input wires
	    	for(int idx = 0; idx < numMasks; idx++)
		{
			privateMask[idx] = aliceShare->maskShare[idx] + bobShare->maskShare[idx];
		}
		
// 		std::vector<unsigned char> hashMask = ArrayEncoder::Hash(privateMask);
// 		TestUtility::PrintByteArray(hashMask, "hash mask");
		
		std::cout << "Reconstruct beaver" << std::endl;
		// Reconstruct beaver triplets for mult-type gates
		std::vector<uint64_t> beaver;
		for(int idx = 0; idx < aliceShare->beaverShare.size(); idx++)
		{
			for(int jdx = 0; jdx < aliceShare->beaverShare[idx].size(); jdx++)
			{
				beaver.push_back(aliceShare->beaverShare[idx][jdx] + bobShare->beaverShare[idx][jdx]);
			}
		}
		
		unTruncatedMasks.resize(numMasks);
		for(int idx = 0; idx < numMasks; idx++)
		{
			unTruncatedMasks[idx] = 0;
		}
		
// 		std::vector<unsigned char> hashBeaver = ArrayEncoder::Hash(beaver);
// 		TestUtility::PrintByteArray(hashBeaver, "hash beaver");
		
		std::cout << "Beaver size: " << beaver.size() << std::endl;
		
		std::cout << "Compute bob's correction" << std::endl;
		// Propagate the mask and get the corrections for bob 
		std::vector<uint64_t> bobCorrections;
		int count = 0;
		
		for (int layer = 0;layer < lc->Depth;layer++)
		{
			ArithmeticLayer *bl = lc->operator[](layer);
			
			// Propagate the mask share for linear gates
			
// 			std::cout << "Add gates" << std::endl;
			/// (x + lx) + (y + ly) = (x + y) + (lx + ly)
			for (auto &g : bl->AddGates)
			{
				int leftIndex = maskIndex[g->LeftWire];
				int rightIndex = maskIndex[g->RightWire];
				int outIndex = maskIndex[g->OutputWire];
				
				for(int idx = 0; idx < DIM; idx++)
				{
					privateMask[outIndex + idx] = privateMask[leftIndex + idx] + privateMask[rightIndex + idx];
				}
			}
			
// 			std::cout << "NAdd gates" << std::endl;
			// Sum (x_i + lx_i) = (Sum x_i) + (Sum lx_i)
			for (auto &g : bl->NAddGates)
			{
				int outIndex = maskIndex[g->OutputWire];
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					privateMask[outIndex + kdx] = 0;
				}
				
				for(int idx = 0; idx < g->InputWires.size(); idx++)
				{
					int inputIndex = maskIndex[g->InputWires[idx]];
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						privateMask[outIndex + kdx] += privateMask[inputIndex + kdx];
					}
				}
			}
			
// 			std::cout << "CAdd gates" << std::endl;
			/// (x + lx) + c = (x + c) + lx
			for (auto &g : bl->CAddGates)
			{
				int inIndex = maskIndex[g->InputWire];
				int outIndex = maskIndex[g->OutputWire];
				
				for(int idx = 0; idx < DIM; idx++)
				{
					privateMask[outIndex + idx] = privateMask[inIndex + idx];
				}
			}
			
// 			std::cout << "Sub gates" << std::endl;
			/// (x + lx) - (y + ly) = (x - y) + (lx - ly)
			// Sub gates are for: rating - <u, v>
			// The dimension of sub gate is 1 per wire
			for (auto &g : bl->SubGates)
			{
				int leftIndex = maskIndex[g->LeftWire];
				int rightIndex = maskIndex[g->RightWire];
				int outIndex = maskIndex[g->OutputWire];
				
				privateMask[outIndex] = privateMask[leftIndex] - privateMask[rightIndex];
			}
		
// 			std::cout << "CMul gates" << std::endl;
			// CMul: beaver[z] = lz + lx*ly = lz (ly = 0 for const wire)
			bobCorrections.resize(0);
			auto cMulGates = lc->operator[](layer)->CMulGates;
			
			if (cMulGates.size() > 0)
			{
				for (auto &g : bl->CMulGates)
				{
					int outIndex = maskIndex[g->OutputWire];
					for(int idx = 0; idx < DIM; idx++)
					{
						// Compute floor(lz/2^d)
						unTruncatedMasks[outIndex] = beaver[count];
						privateMask[outIndex] = (beaver[count] >> PRECISION_BIT_LENGTH);
						
						// Compute [lz/2^d]_2  =  floor(lz/2^d) - [lz/2^d]_1
						bobCorrections.push_back(privateMask[outIndex] - aliceShare->maskShare[outIndex]);
						
						count++; outIndex++;
					}
				}
				
				addBobCorrection(bobCorrections);
			}
			
// 			std::cout << "Mul gates" << std::endl;
			// Correct maskShare[z] for output of mul gate
			// lz = beaver[z] - lx*ly
			bobCorrections.resize(0);
			auto mulGates = lc->operator[](layer)->MulGates;
			
			if (mulGates.size() > 0)
			{
				for (int i = 0; i < mulGates.size(); i++)
				{
					MultiplicationGate *gate = mulGates[i];
					int leftIndex = maskIndex[gate->LeftWire];
					int rightIndex = maskIndex[gate->RightWire];
					int outIndex = maskIndex[gate->OutputWire];
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						// Multiplication between a scalar and a vector
						unTruncatedMasks[outIndex] = beaver[count] - privateMask[leftIndex]*privateMask[rightIndex];
						privateMask[outIndex] = (beaver[count] - privateMask[leftIndex]*privateMask[rightIndex]) >> PRECISION_BIT_LENGTH;
						
						bobCorrections.push_back(privateMask[outIndex] - aliceShare->maskShare[outIndex]);
						
						count++; rightIndex++; outIndex++;
					}
				}
				
				addBobCorrection(std::move(bobCorrections));
			}
			
// 			std::cout << "Dot gates" << std::endl;
			bobCorrections.resize(0);
			auto dotGates = lc->operator[](layer)->DotGates;
			
			if (dotGates.size() > 0)
			{
				for (int i = 0; i < dotGates.size(); i++)
				{
					DotProductGate *gate = dotGates[i];
					int leftIndex = maskIndex[gate->LeftWire];
					int rightIndex = maskIndex[gate->RightWire];
					int outIndex = maskIndex[gate->OutputWire];
					
					uint64_t temp = 0;
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						temp += (privateMask[leftIndex + kdx]*privateMask[rightIndex + kdx]);
					}
					
					unTruncatedMasks[outIndex] = beaver[count] - temp;
					privateMask[outIndex] = (beaver[count] - temp) >> PRECISION_BIT_LENGTH;
					count++;
					
					bobCorrections.push_back(privateMask[outIndex] - aliceShare->maskShare[outIndex]);
				}
				
				addBobCorrection(std::move(bobCorrections));
			}
			
// 			std::cout << "NDot gates" << std::endl;
			bobCorrections.resize(0);
			auto nDotGates = lc->operator[](layer)->NDotGates;
			
			// Dot gate structures
			// Sum isReal_i*...
			// No need for truncation as isReal = 0/1
			// Left wires: InputWires[0] --> InputWires[#wire/2 - 1]
			// Right wires: InputWires[#wires/2] --> InputWires[#wires - 1]
			// Left wire: single value
			// Right wire: vector of size DIM
			if (nDotGates.size() > 0)
			{
				for (int i = 0; i < nDotGates.size(); i++)
				{
					NaryDotGate *gate = nDotGates[i];
					
					int outIndex = maskIndex[gate->OutputWire];
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{			
						uint64_t temp = 0;
						int step = gate->InputWires.size()/2;
						
						for(int jdx = 0; jdx < step; jdx++)
						{
							temp += privateMask[maskIndex[gate->InputWires[jdx]]]*privateMask[maskIndex[gate->InputWires[jdx + step]] + kdx];
						}
						
						unTruncatedMasks[outIndex] = beaver[count] - temp;
						privateMask[outIndex] = (beaver[count] - temp);
						bobCorrections.push_back(privateMask[outIndex] - aliceShare->maskShare[outIndex]);
						count++; outIndex++;
					}
				}
				
				addBobCorrection(std::move(bobCorrections));
			}
		}
		
		std::cout << "set masks" << std::endl;
		setMasks(std::move(privateMask));
	}
}