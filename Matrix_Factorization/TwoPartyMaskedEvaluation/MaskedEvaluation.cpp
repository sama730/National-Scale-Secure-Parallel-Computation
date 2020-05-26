#include <stdint.h>
#include <vector>
#include <cassert>
#include <iostream>
#include <sstream>
#include <ctime>
#include <chrono>
#include "MaskedEvaluation.h"
#include "PreprocessingShare.h"
#include "MaskedEvaluationException.h"
#include "../Circuit/LayeredArithmeticCircuit.h"
#include "../Utility/Communicator.h"
#include "../Utility/CryptoUtility.h"

using namespace Circuit;
using namespace Utility;

#define CORRECTION_MOD 15
#define VALID_RANGE 4

namespace TwoPartyMaskedEvaluation
{
 
	MaskedEvaluation::MaskedEvaluation(LayeredArithmeticCircuit *lc, PreprocessingShare *share, Communicator *communicator) : communicator(communicator), lc(lc), share(share)
	{
	}

	MaskedEvaluation::MaskedEvaluation(LayeredArithmeticCircuit *lc, PreprocessingShare *share, std::vector<int> *maskIndex, Communicator *communicator) : communicator(communicator), lc(lc), maskIndex(maskIndex), share(share)
	{
	}
	
	// input range: 0 to range->Length
	// Add to maskedEvaluation from range->Start to range->Start + range->Length
	void MaskedEvaluation::AddInput(const std::vector<std::vector<uint64_t> >& input, Range *range)
	{
		maskedEvaluation = /*std::move*/input;
		maskedEvaluation.resize(lc->NumWires);
		inputAdded = true;
	}

	std::vector<std::vector<uint64_t> > MaskedEvaluation::Decrypt(std::vector<uint64_t> mask, Range *range)
	{
		if (!evaluated)
		{
			throw std::exception();
		}
		
		int count = 0;
		std::vector<std::vector<uint64_t> > output(range->Length);
		
		for(int idx = range->Start; idx < range->Start + range->Length; idx++)
		{
			output[idx - range->Start].resize(maskedEvaluation[idx].size());
			for(int kdx = 0; kdx < maskedEvaluation[idx].size(); kdx++)
			{
				  output[idx - range->Start][kdx] = maskedEvaluation[idx][kdx] - mask[count++];
			}
		}
		
		return output;
	}

	void MaskedEvaluation::EvaluateCircuit()
	{
// 		if(playerID == DAVID) std::cout << "MaskedEvaluation::EvaluateCircuit()" << std::endl;
		
		if (!inputAdded)
		{
			throw NoInputAddedException();
		}

		if (evaluated)
		{
			throw AlreadyEvaluatedException();
		}

		for (int layer = 0; layer < lc->Depth; layer++)
		{
// 			std::cout << playerID << ": layer: " << layer << std::endl;
			EvaluateAddGate(layer);
			EvaluateSubGate(layer);
			EvaluateCMulGate(layer);
			EvaluateMulGate(layer);
			EvaluateDotGate(layer);
			EvaluateNDotGate(layer);
		}

		evaluated = true;
	}

	void MaskedEvaluation::EvaluateAddGate(int layer)
	{
// 		std::cout << playerID << ": MaskedEvaluation::EvaluateAddGate()" << std::endl;
		for (auto &g : lc->operator[](layer)->AddGates)
		{
			std::vector<uint64_t> leftValue  = maskedEvaluation[g->LeftWire];
			std::vector<uint64_t> rightValue = maskedEvaluation[g->RightWire];
			
			maskedEvaluation[g->OutputWire] = VectorOperation::Add(leftValue, rightValue);
		}
	}
	
	void MaskedEvaluation::EvaluateNAddGate(int layer)
	{
		for (auto &g : lc->operator[](layer)->NAddGates)
		{
			maskedEvaluation[g->OutputWire].resize(DIM);
			
			for(int idx = 0; idx < g->InputWires.size(); idx++)
			{
				std::vector<uint64_t> wireValue  = maskedEvaluation[g->InputWires[idx]];
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					maskedEvaluation[g->OutputWire][kdx] += wireValue[kdx];
				}
			}
		}
	}
	
	void MaskedEvaluation::EvaluateSubGate(int layer)
	{
// 		std::cout << playerID << ": MaskedEvaluation::EvaluateSubGate()" << std::endl;
		for (auto &g : lc->operator[](layer)->SubGates)
		{
			std::vector<uint64_t> leftValue  = maskedEvaluation[g->LeftWire];
			std::vector<uint64_t> rightValue = maskedEvaluation[g->RightWire];
			
			maskedEvaluation[g->OutputWire] = VectorOperation::Sub(leftValue, rightValue);
		}
	}
	
	void MaskedEvaluation::EvaluateCAddGate(int layer)
	{
		for (auto &g : lc->operator[](layer)->CAddGates)
		{
			std::vector<uint64_t> inputValue = maskedEvaluation[g->InputWire];
			uint64_t constant = g->constant;
			maskedEvaluation[g->OutputWire].resize(inputValue.size());
			for(int idx = 0; idx < inputValue.size(); idx++)
			{
				maskedEvaluation[g->OutputWire][idx] = constant + inputValue[idx];
			}
		}
	}
	
	void MaskedEvaluation::EvaluateCMulGate(int layer)
	{
// 		std::cout << playerID << ": MaskedEvaluation::EvaluateCMulGate()" << std::endl;
		auto cMulGates = lc->operator[](layer)->CMulGates;
		auto count = cMulGates.size();
		
		if(count > 0)
		{
			std::vector<uint64_t> beaverShare = share->GetNextBeaverTriples();	// <lz>
			std::vector<uint64_t> myBeaverValuesShare(DIM*count);			// <c*y + lz>
			
			for (int idx = 0; idx < count; idx++)
			{
				ConstantMultiplicationGate *g = cMulGates[idx];
				
				uint64_t mx        = g->constant;		           // c
				std::vector<uint64_t> my = maskedEvaluation[g->InputWire]; // y + ly
				
				int inputWire   = g->InputWire;
				int outputWire  = g->OutputWire;
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					uint64_t ly = share->operator[]((*maskIndex)[inputWire] + kdx);
					uint64_t lz = share->operator[]((*maskIndex)[outputWire] + kdx);
					
					if (playerID == ALICE || playerID == CHARLIE){
						myBeaverValuesShare[DIM*idx + kdx] = -mx*ly + beaverShare[idx*DIM + kdx];
					}
					else {
						myBeaverValuesShare[DIM*idx + kdx] = mx*my[kdx] - ly*mx + beaverShare[idx*DIM + kdx];
					}
				}
			}
			
			// Second, reconstruct mz = xy + lz and compute the truncation
			std::vector<unsigned char> temp = ArrayEncoder::Encode(myBeaverValuesShare);
			assert(temp.size() > 0);

			std::vector<unsigned char> recv(DIM*count*sizeof(uint64_t));
			if (playerID == ALICE || playerID == CHARLIE)
			{
				communicator->SendEvaluationPartner(temp.data(), temp.size());
				communicator->AwaitEvaluationPartner(recv.data(), recv.size());
			}
			else
			{
				communicator->AwaitEvaluationPartner(recv.data(), recv.size());
				communicator->SendEvaluationPartner(temp.data(), temp.size());
			}
			
			std::vector<uint64_t> theirBeaverValues = ArrayEncoder::UCharVec2UInt64tVec(recv);
			
			for (int idx = 0; idx < count; idx++)
			{
				ConstantMultiplicationGate *g = cMulGates[idx];
				maskedEvaluation[g->OutputWire].resize(DIM);
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					__uint128_t temp = 0;
					maskedEvaluation[g->OutputWire][kdx] = myBeaverValuesShare[DIM*idx + kdx] + theirBeaverValues[DIM*idx + kdx];
					temp += maskedEvaluation[g->OutputWire][kdx];
					temp += unTruncatedMasks[(*maskIndex)[g->OutputWire] + kdx];
					temp = (temp >> PRECISION_BIT_LENGTH) - (masks[(*maskIndex)[g->OutputWire] + kdx]);
					maskedEvaluation[g->OutputWire][kdx] = (uint64_t)temp;
				}
			}
		}
	}
	
	void MaskedEvaluation::EvaluateMulGate(int layer)
	{
// 		std::cout << playerID << ": MaskedEvaluation::EvaluateMulGate()" << std::endl;
// 		MyDebug::NonNull(beaver);
// 		assert(nullptr != beaver);
		
		auto mulGates = lc->operator[](layer)->MulGates;
		auto count = mulGates.size();
		// (x + lx)*(y+ ly) -<lx*my> - <ly*mx> + <lz + lx*ly>
		if (count > 0)
		{
			std::vector<uint64_t> beaverShare = share->GetNextBeaverTriples();	// <lx*ly>
			std::vector<uint64_t> myBeaverValuesShare(DIM*count);			// -<lx*my> - <ly*mx> + <lz> + <lx*ly>
			
			assert(DIM*count == beaverShare.size());
			
			// First, we compute <xy + lz>
			for (int i = 0; i < mulGates.size(); i++)
			{
				MultiplicationGate *g = mulGates[i];
				// Left wire is a scalar, Right wire is a vector
				uint64_t mx              = maskedEvaluation[g->LeftWire][0];  // x + lx
				std::vector<uint64_t> my = maskedEvaluation[g->RightWire]; // y + ly
				
				int leftWire   = g->LeftWire;
				int rightWire  = g->RightWire;
				int outputWire = g->OutputWire;
				
				uint64_t lx = share->operator[]((*maskIndex)[leftWire]);
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					uint64_t ly = share->operator[]((*maskIndex)[rightWire] + kdx);
					uint64_t lz = share->operator[]((*maskIndex)[outputWire] + kdx);
					
					if (playerID == ALICE || playerID == CHARLIE){
						myBeaverValuesShare[DIM*i + kdx] = -lx*my[kdx] - ly*mx + beaverShare[i*DIM + kdx];
					}
					else {
						myBeaverValuesShare[DIM*i + kdx] = mx*my[kdx] -lx*my[kdx] - ly*mx + beaverShare[i*DIM + kdx];
					}
				}
			}
			
			// Second, reconstruct mz = xy + lz and compute the truncation
			std::vector<unsigned char> temp = ArrayEncoder::Encode(myBeaverValuesShare);
			assert(temp.size() > 0);

			std::vector<unsigned char> recv(DIM*count*sizeof(uint64_t));
			if (playerID == ALICE || playerID == CHARLIE)
			{
				communicator->SendEvaluationPartner(temp.data(), temp.size());
				communicator->AwaitEvaluationPartner(recv.data(), recv.size());
			}
			else
			{
				communicator->AwaitEvaluationPartner(recv.data(), recv.size());
				communicator->SendEvaluationPartner(temp.data(), temp.size());
			}
			
			std::vector<uint64_t> theirBeaverValues = ArrayEncoder::UCharVec2UInt64tVec(recv);
			
			for (int idx = 0; idx < count; idx++)
			{
				MultiplicationGate *g = mulGates[idx];
				maskedEvaluation[g->OutputWire].resize(DIM);
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					__uint128_t temp = 0;
					maskedEvaluation[g->OutputWire][kdx] = myBeaverValuesShare[DIM*idx + kdx] + theirBeaverValues[DIM*idx + kdx];
					temp += maskedEvaluation[g->OutputWire][kdx];
					temp += unTruncatedMasks[(*maskIndex)[g->OutputWire] + kdx];
					temp = (temp >> PRECISION_BIT_LENGTH) - (masks[(*maskIndex)[g->OutputWire] + kdx]);
					maskedEvaluation[g->OutputWire][kdx] = (uint64_t)temp;
				}
			}
		}
	}
	
	void MaskedEvaluation::EvaluateDotGate(int layer)
	{
// 		std::cout << playerID << ": MaskedEvaluation::EvaluateDotGate()" << std::endl;
		auto dotGates = lc->operator[](layer)->DotGates;
		int count = dotGates.size();
		
		if(count > 0)
		{
			std::vector<uint64_t> beaverShare = share->GetNextBeaverTriples();	// <lx*ly>
			std::vector<uint64_t> myBeaverValuesShare(count);			// -<lx*my> - <ly*mx> + <lz> + <lx*ly>
			
			assert(beaverShare.size() == count);
			
			for (int i = 0; i < count; i++)
			{
				DotProductGate *g = dotGates[i];
				
				std::vector<uint64_t> mx = maskedEvaluation[g->LeftWire];  // x + lx
				std::vector<uint64_t> my = maskedEvaluation[g->RightWire]; // y + ly
				
				int leftWire = g->LeftWire;
				int rightWire = g->RightWire;
				int outputWire = g->OutputWire;
				
				myBeaverValuesShare[i] = 0;
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					uint64_t lx = share->operator[]((*maskIndex)[leftWire] + kdx);
					uint64_t ly = share->operator[]((*maskIndex)[rightWire] + kdx);
					
					if (playerID == ALICE || playerID == CHARLIE){
						myBeaverValuesShare[i] += (-lx*my[kdx] - ly*mx[kdx]);
					}
					else{
						myBeaverValuesShare[i] += (mx[kdx]*my[kdx] -lx*my[kdx] - ly*mx[kdx]);
					}
				}
				
				myBeaverValuesShare[i] = myBeaverValuesShare[i] + beaverShare[i];
			}
			
			std::vector<unsigned char> temp = ArrayEncoder::Encode(myBeaverValuesShare);
			assert(temp.size() > 0);

			std::vector<unsigned char> recv(count*sizeof(uint64_t));
			if (playerID == ALICE || playerID == CHARLIE)
			{
				communicator->SendEvaluationPartner(temp.data(), temp.size());
				communicator->AwaitEvaluationPartner(recv.data(), recv.size());
			}
			else
			{
				communicator->AwaitEvaluationPartner(recv.data(), recv.size());
				communicator->SendEvaluationPartner(temp.data(), temp.size());
			}
			
			std::vector<uint64_t> theirBeaverValues = ArrayEncoder::UCharVec2UInt64tVec(recv);
			
			for(int idx = 0; idx < count; idx++)
			{
				DotProductGate *g = dotGates[idx];
				
				maskedEvaluation[g->OutputWire].resize(1);
				
				__uint128_t temp = 0;
				maskedEvaluation[g->OutputWire][0] = myBeaverValuesShare[idx] + theirBeaverValues[idx];
				temp += maskedEvaluation[g->OutputWire][0];
				temp += unTruncatedMasks[(*maskIndex)[g->OutputWire]];
				temp = (temp >> PRECISION_BIT_LENGTH) - (masks[(*maskIndex)[g->OutputWire]]);
				maskedEvaluation[g->OutputWire][0] = (uint64_t)temp;
			}
		}
	}
	
	void MaskedEvaluation::EvaluateNDotGate(int layer)
	{
// 		if(playerID == DAVID) std::cout << "MaskedEvaluation::EvaluateNDotGate()" << std::endl;
		auto nDotGates = lc->operator[](layer)->NDotGates;
		
		int count = nDotGates.size();
		
		std::stringstream ss;
		if(count > 0)
		{
			std::vector<uint64_t> beaverShare = share->GetNextBeaverTriples();	// <lx*ly>
			std::vector<uint64_t> myBeaverValuesShare(DIM*count);			// -<lx*my> - <ly*mx> + <lz> + <lx*ly>
			
			assert(beaverShare.size() == DIM*count);
			
			for (int i = 0; i < count; i++)
			{
				NaryDotGate *g = nDotGates[i];
				
				int step = g->InputWires.size()/2;
				for(int idx = 0; idx < step; idx++)
				{
					uint64_t mx              = maskedEvaluation[g->InputWires[idx]][0];  // r_idx + lr_idx
					std::vector<uint64_t> my = maskedEvaluation[g->InputWires[idx + step]]; // y_idx + ly_idx
					
					int leftWire = g->InputWires[idx];
					int rightWire = g->InputWires[idx + step];
					
					uint64_t lx = share->operator[]((*maskIndex)[leftWire]); 	// lx = <lr>
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						uint64_t ly = share->operator[]((*maskIndex)[rightWire] + kdx);// ly = <ly[kdx]>
						
						if (playerID == ALICE || playerID == CHARLIE){
							myBeaverValuesShare[DIM*i + kdx] += (-lx*my[kdx] - ly*mx);
						}
						else {
							myBeaverValuesShare[DIM*i + kdx] += (mx*my[kdx] -lx*my[kdx] - ly*mx);
						}
					}
				}
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					myBeaverValuesShare[DIM*i + kdx] += beaverShare[DIM*i + kdx];
				}
			}
			
			std::vector<unsigned char> temp = ArrayEncoder::Encode(myBeaverValuesShare);
			assert(temp.size() > 0);

			std::vector<unsigned char> recv(DIM*count*sizeof(uint64_t));
			if (playerID == ALICE || playerID == CHARLIE)
			{
				communicator->SendEvaluationPartner(temp.data(), temp.size());
				communicator->AwaitEvaluationPartner(recv.data(), recv.size());
			}
			else
			{
				communicator->AwaitEvaluationPartner(recv.data(), recv.size());
				communicator->SendEvaluationPartner(temp.data(), temp.size());
			}
			
			std::vector<uint64_t> theirBeaverValues = ArrayEncoder::UCharVec2UInt64tVec(recv);
			
				
			for(int idx = 0; idx < count; idx++)
			{
				NaryDotGate *g = nDotGates[idx];
				
				maskedEvaluation[g->OutputWire].resize(DIM);
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					maskedEvaluation[g->OutputWire][kdx] = myBeaverValuesShare[DIM*idx + kdx] + theirBeaverValues[DIM*idx + kdx];
				}
			}
		}
	}
}
