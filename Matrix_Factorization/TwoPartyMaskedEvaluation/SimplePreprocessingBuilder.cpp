#include <openssl/rand.h>
#include <stdint.h>

#include "SimplePreprocessingBuilder.h"
#include "PreprocessingShareStorage.h"
#include "PreprocessingShare.h"
#include "../Utility/Timer.h"
#include "../Utility/ISecureRNG.h"
#include "../Utility/CryptoUtility.h"
#include "../Circuit/LayeredArithmeticCircuit.h"
#include "../Circuit/GradientDescendCircuitBuilder.h"

#define DIM 10
using namespace Circuit;
using namespace Utility;

namespace TwoPartyMaskedEvaluation
{
	PreprocessingShareStorage *SimplePreprocessingBuilder::BuildPreprocessing(LayeredArithmeticCircuit *lc)
	{
		int length = lc->NumWires;
		std::vector<unsigned char> seed = CryptoUtility::SampleByteArray(32);
		AESRNG *rng = new AESRNG(seed.data());
		
		/// masks contain lx for wire x
// 		std::vector<int64_t> masks(length);
		std::vector<int64_t> masks = rng->GetMaskArray(length); //CryptoUtility::SampleInt64Array(length);
		
		std::vector<std::vector<int64_t> > aliceBeaverShares;
		std::vector<std::vector<int64_t> > bobBeaverShares;
		
		for (int ilayer = 0; ilayer < lc->Depth; ilayer++)
		{
			ArithmeticLayer *bl = lc->operator[](ilayer);

			/// (x + lx) + (y + ly) = (x + y) + (lx + ly)
			for (auto &g : bl->AddGates)
			{
				masks[g->OutputWire] = ArithmeticOperation::add(masks[g->LeftWire], masks[g->RightWire]);
			}
			
			/// (x + lx) + c = (x + c) + lx
			for (auto &g : bl->CAddGates)
			{
				masks[g->OutputWire] = masks[g->InputWire]; 
			}
			
			/// (x + lx) - (y + ly) = (x - y) + (lx - ly)
			for (auto &g : bl->SubGates)
			{
				masks[g->OutputWire] = ArithmeticOperation::sub(masks[g->LeftWire], masks[g->RightWire]);
			}

			/// (x + lx)*c = x*c + lx*c
			for (auto &g : bl->CMulGates)
			{
				masks[g->OutputWire] = ArithmeticOperation::mul(masks[g->InputWire], g->constant);
			}
			
			auto mulGates = bl->MulGates;

			/// (x + lx)(y + ly) --> x*y + lz
			if (mulGates.size() > 0)
			{
				int mulCount = mulGates.size();

				std::vector<int64_t> beaver(mulCount);
								
				// For idx-th gate ( (x, y), z ): beaver[idx] = lx*ly
				for (int idx = 0; idx < mulCount; idx++)
				{
					MultiplicationGate *g = mulGates[idx];
					beaver[idx] = ArithmeticOperation::mul(masks[g->LeftWire], masks[g->RightWire]);
				}

				// For idx-th gate ( (x, y), z ): aliceBeaverShare[idx] = <lx*ly> and bobBeaverShares[idx] = <lx*ly>
				std::vector<int64_t> aliceBeaverShare = CryptoUtility::SampleInt64Array(mulCount);
				
				aliceBeaverShares.push_back(std::move(aliceBeaverShare));
				
				std::vector<int64_t> bobBeaverShare(mulCount);
				for(int idx = 0; idx < mulCount; idx++)
				{
					bobBeaverShare[idx] = ArithmeticOperation::sub(beaver[idx], aliceBeaverShare[idx]); 
				}
				
				bobBeaverShares.push_back(std::move(bobBeaverShare));
				
// 				delete [] beaver;
			}
		}

		/// secret share for wires
		// For wire x: masksShareAlice[x] = <lx> and masksShareBob[x] = <lx>
		std::vector<int64_t> masksShareAlice = CryptoUtility::SampleInt64Array(length);
		
		std::vector<int64_t> masksShareBob(length);
		
		for(int idx = 0; idx < length; idx++)
		{
			masksShareBob[idx] = ArithmeticOperation::sub(masks[idx], masksShareAlice[idx]);
		}
		
		// aliceShare contains <lx> for each wire, and <lx*ly> for each MUL gate
		PreprocessingShare *aliceShare = new PreprocessingShare(std::move(masksShareAlice), std::move(aliceBeaverShares));
		PreprocessingShare *bobShare = new PreprocessingShare(std::move(masksShareBob), std::move(bobBeaverShares));

		return new PreprocessingShareStorage(aliceShare, bobShare, std::move(masks));
	}
	
	PreprocessingShareStorage *SimplePreprocessingBuilder::BuildPreprocessing(LayeredArithmeticCircuit *lc, std::vector<int>& itemsPerUser)
	{
		Timer t;
		
		int length = lc->NumWires;
		std::vector<unsigned char> seed = CryptoUtility::SampleByteArray(32);
		AESRNG *rng = new AESRNG(seed.data());
		
		// Count number of required random values: some wires need 10 masks, some 1
		int numUsers = itemsPerUser.size();
		int sumNumItems = 0;
		for(int idx = 0; idx < itemsPerUser.size(); idx++)
		{
			sumNumItems += itemsPerUser[idx];
		}
		
		int numMasks = 6*DIM*numUsers + (2*DIM + 4)*sumNumItems;
		
		std::vector<std::vector<int64_t> > aliceBeaverShares;
		std::vector<std::vector<int64_t> > bobBeaverShares;
		
		/// Generate all the masks at the same time, then assign values to each wire later
		std::vector<int64_t> masks = rng->GetMaskArray(numMasks);
		t.Tick("AES time");
		
		std::vector<int> maskIndex = CryptoUtility::buildMaskIndex(itemsPerUser);
		
		for (int ilayer = 0; ilayer < lc->Depth; ilayer++)
		{
// 			std::cout << "Layer " << ilayer << std::endl;
			ArithmeticLayer *bl = lc->operator[](ilayer);

			/// (x + lx) + (y + ly) = (x + y) + (lx + ly)
			for (auto &g : bl->AddGates)
			{
				int leftIndex = maskIndex[g->LeftWire];
				int rightIndex = maskIndex[g->RightWire];
				int outIndex = maskIndex[g->OutputWire];
				
				for(int idx = 0; idx < DIM; idx++)
				{
					masks[outIndex] = masks[leftIndex] + masks[rightIndex];
					leftIndex++; rightIndex++; outIndex++;
				}
			}
			
			for (auto &g : bl->NAddGates)
			{
				int outIndex = maskIndex[g->OutputWire];
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					masks[outIndex + kdx] = 0;
				}
				
				for(int idx = 0; idx < g->InputWires.size(); idx++)
				{
					int inputIndex = maskIndex[g->InputWires[idx]];
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						masks[outIndex + kdx] += masks[inputIndex + kdx];
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
					masks[outIndex] = masks[inIndex];
					inIndex++; outIndex++;
				}
	// 				masks[g->OutputWire] = masks[g->InputWire]; 
			}
			
			/// (x + lx) - (y + ly) = (x - y) + (lx - ly)
			for (auto &g : bl->SubGates)
			{
				int leftIndex = maskIndex[g->LeftWire];
				int rightIndex = maskIndex[g->RightWire];
				int outIndex = maskIndex[g->OutputWire];
				
				masks[outIndex] = masks[leftIndex] - masks[rightIndex];
				leftIndex++; rightIndex++; outIndex++;
			}

			/// (x + lx)*c = x*c + lx*c
			for (auto &g : bl->CMulGates)
			{
				int inIndex = maskIndex[g->InputWire];
				int outIndex = maskIndex[g->OutputWire];
				
				for(int idx = 0; idx < DIM; idx++)
				{
					masks[outIndex] = ArithmeticOperation::mul(masks[inIndex], g->constant);
					inIndex++; outIndex++;
				}
	// 			masks[g->OutputWire] = VectorOperation::Mul(masks[g->InputWire], g->constant);
			}
			
			auto mulGates = bl->MulGates;

			/// (x + lx)(y + ly) --> x*y + lz
			if (mulGates.size() > 0)
			{
	// 			std::cout << "Mul Gates" << std::endl;
				// ############################## NOTE!!!##############################//
				// Left wire has only 1 value
				// Right wire has 10 values
				
				int mulCount = mulGates.size();

				// For idx-th gate ( (x, y), z ): aliceBeaverShare[idx] = <lx*ly> and bobBeaverShares[idx] = <lx*ly>
				std::vector<int64_t> aliceBeaverShare = CryptoUtility::SampleInt64Array(DIM*mulCount);			
				std::vector<int64_t> bobBeaverShare(DIM*mulCount);
				
				int count = 0;
				// For idx-th gate ( (x, y), z ): beaver[idx] = lx*ly
				for (int idx = 0; idx < mulCount; idx++)
				{
					MultiplicationGate *g = mulGates[idx];
					
					int leftIndex = maskIndex[g->LeftWire];
					int rightIndex = maskIndex[g->RightWire];
					
					int start = DIM*idx;
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						bobBeaverShare[start + kdx] = ArithmeticOperation::mul(masks[leftIndex], masks[rightIndex + kdx]) - aliceBeaverShare[start + kdx];
					}
				}
				
				aliceBeaverShares.push_back(std::move(aliceBeaverShare));
				bobBeaverShares.push_back(std::move(bobBeaverShare));
			}
			
			auto dotGates = bl->DotGates;
			
			if (dotGates.size() > 0)
			{
	// 			std::cout << "Dot Gates" << std::endl;
				int dotCount = dotGates.size();

				// For idx-th gate ( (x, y), z ): aliceBeaverShare[idx] = <lx*ly> and bobBeaverShares[idx] = <lx*ly>
				std::vector<int64_t> aliceBeaverShare = CryptoUtility::SampleInt64Array(dotCount);
				std::vector<int64_t> bobBeaverShare(dotCount);
								
				// For idx-th gate ( (x, y), z ): beaver[idx] = lx*ly
				for (int idx = 0; idx < dotCount; idx++)
				{
					DotProductGate *g = dotGates[idx];
					
	// 				std::cout << maskIndex[g->LeftWire] << " " << maskIndex[g->RightWire] << std::endl;
					int leftIndex = maskIndex[g->LeftWire];
					int rightIndex = maskIndex[g->RightWire];
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						bobBeaverShare[idx] += masks[leftIndex + kdx]*masks[rightIndex + kdx];
					}
					
					bobBeaverShare[idx] /= (1 << PRECISION_BIT_LENGTH);
					bobBeaverShare[idx] = -aliceBeaverShare[idx];
				}
				
				aliceBeaverShares.push_back(std::move(aliceBeaverShare));
				bobBeaverShares.push_back(std::move(bobBeaverShare));
			}
			
			auto nDotGates = bl->NDotGates;
			int count = nDotGates.size();
			
			if (count != 0)
			{
				std::vector<int64_t> aliceBeaverShare = CryptoUtility::SampleInt64Array(DIM*count);
				std::vector<int64_t> bobBeaverShare(DIM*count);
				
				for (int i = 0; i < count; i++)
				{
					NaryDotGate *gate = nDotGates[i];
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{						
						int step = gate->InputWires.size()/2;
						
						for(int jdx = 0; jdx < step; jdx++)
						{
							bobBeaverShare[DIM*i + kdx] += masks[maskIndex[gate->InputWires[jdx]]]*masks[maskIndex[gate->InputWires[jdx + step]] + kdx];
						}
						
						bobBeaverShare[DIM*i + kdx] /= (1 << PRECISION_BIT_LENGTH);
						bobBeaverShare[DIM*i + kdx] -= aliceBeaverShare[DIM*i + kdx];
					}
				}
				
				aliceBeaverShares.push_back(std::move(aliceBeaverShare));
				bobBeaverShares.push_back(std::move(bobBeaverShare));
			}
			
// 			t.Tick("Time for this layer");
		}
		
		t.Tick("Time to generate masks");
		
		/// secret share for wires
		// For wire x: masksShareAlice[x] = <lx> and masksShareBob[x] = <lx>
		std::vector<int64_t> masksShareAlice = CryptoUtility::SampleInt64Array(numMasks);
		t.Tick("Time to generate random share");
		std::vector<int64_t> masksShareBob   = VectorOperation::Sub(masks, masksShareAlice);
		t.Tick("Time to generate mask share");
		
		// aliceShare contains <lx> for each wire, and <lx*ly> for each MUL gate
		PreprocessingShare *aliceShare = new PreprocessingShare(std::move(masksShareAlice), std::move(aliceBeaverShares));
		PreprocessingShare *bobShare = new PreprocessingShare(std::move(masksShareBob), std::move(bobBeaverShares));
		
		t.Tick("Time to create PreprocessingShare");
		
		return new PreprocessingShareStorage(aliceShare, bobShare, std::move(masks), std::move(maskIndex));
	}
}
