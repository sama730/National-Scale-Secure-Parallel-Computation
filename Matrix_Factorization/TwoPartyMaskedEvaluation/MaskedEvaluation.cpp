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
	void MaskedEvaluation::AddInput(const std::vector<std::vector<int64_t> >& input, Range *range)
	{
		maskedEvaluation = /*std::move*/input;
		maskedEvaluation.resize(lc->NumWires);
		inputAdded = true;
	}

	std::vector<std::vector<int64_t> > MaskedEvaluation::Decrypt(std::vector<int64_t> mask, Range *range)
	{
		if (!evaluated)
		{
			throw std::exception();
		}
		
		int count = 0;
		std::vector<std::vector<int64_t> > output(range->Length);
		
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
// 			EvaluateNAddGate(layer);
			EvaluateMulGate(layer, share);
			EvaluateDotGate(layer, share);
			EvaluateNDotGate(layer, share);
		}

		evaluated = true;
	}

	void MaskedEvaluation::EvaluateAddGate(int layer)
	{
// 		std::cout << playerID << ": MaskedEvaluation::EvaluateAddGate()" << std::endl;
		for (auto &g : lc->operator[](layer)->AddGates)
		{
			std::vector<int64_t> leftValue  = maskedEvaluation[g->LeftWire];
			std::vector<int64_t> rightValue = maskedEvaluation[g->RightWire];
			
			maskedEvaluation[g->OutputWire].resize(leftValue.size());
			
			for(int idx = 0; idx < leftValue.size(); idx++)
			{
				maskedEvaluation[g->OutputWire][idx] = ArithmeticOperation::add(leftValue[idx], rightValue[idx]);
			}
		}
	}
	
	void MaskedEvaluation::EvaluateNAddGate(int layer)
	{
		for (auto &g : lc->operator[](layer)->NAddGates)
		{
			maskedEvaluation[g->OutputWire].resize(DIM);
			
			for(int idx = 0; idx < g->InputWires.size(); idx++)
			{
				std::vector<int64_t> wireValue  = maskedEvaluation[g->InputWires[idx]];
				
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
			std::vector<int64_t> leftValue  = maskedEvaluation[g->LeftWire];
			std::vector<int64_t> rightValue = maskedEvaluation[g->RightWire];
			
			maskedEvaluation[g->OutputWire].resize(leftValue.size());
			
			for(int idx = 0; idx < leftValue.size(); idx++)
			{
				maskedEvaluation[g->OutputWire][idx] = ArithmeticOperation::sub(leftValue[idx], rightValue[idx]);
			}
		}
	}
	
	void MaskedEvaluation::EvaluateCAddGate(int layer)
	{
		for (auto &g : lc->operator[](layer)->CAddGates)
		{
			std::vector<int64_t> inputValue = maskedEvaluation[g->InputWire];
			int64_t constant = g->constant;
			maskedEvaluation[g->OutputWire].resize(inputValue.size());
			for(int idx = 0; idx < inputValue.size(); idx++)
			{
				maskedEvaluation[g->OutputWire][idx] = ArithmeticOperation::add(constant, inputValue[idx]);
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
			std::vector<int8_t> corrections(DIM*count);
			std::vector<int8_t> theirCorrections(DIM*count);
			
			for (int idx = 0; idx < count; idx++)
			{
				ConstantMultiplicationGate *g = cMulGates[idx];
				
				std::vector<int64_t> inputValue = maskedEvaluation[g->InputWire];
				int64_t constant = g->constant;
				maskedEvaluation[g->OutputWire].resize(inputValue.size());
				
				
				for(int kdx = 0; kdx < inputValue.size(); kdx++)
				{
					maskedEvaluation[g->OutputWire][kdx] = (constant*inputValue[kdx]) >> PRECISION_BIT_LENGTH;
					corrections[DIM*idx + kdx] = ((maskedEvaluation[g->OutputWire][kdx] + masks[(*maskIndex)[g->OutputWire] + kdx]) % CORRECTION_MOD);
				}
			}
				
			if (playerID == ALICE || playerID == BOB)
			{
// 				std::vector<unsigned char> temp = ArrayEncoder::Encode(corrections);
				
				communicator->SendVerificationPartner((unsigned char *)corrections.data(), corrections.size());
// 				if(playerID == ALICE) communicator->SendAlice(temp);
// 				if(playerID == BOB)   communicator->SendBob(temp);
			}
			else if(playerID == CHARLIE || playerID == DAVID)
			{
				communicator->AwaitVerificationPartner((unsigned char *)theirCorrections.data(), theirCorrections.size());
// 				if(playerID == CHARLIE) recv = communicator->AwaitAlice();
// 				if(playerID == DAVID)   recv = communicator->AwaitBob();
// 				std::vector<int64_t> theirCorrections = ArrayEncoder::UCharVec2Int64tVec(recv);
				
				// Check the corrections
				for(int idx = 0; idx < cMulGates.size(); idx++)
				{
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						auto g = cMulGates[idx];
						int diff = theirCorrections[DIM*idx + kdx] - corrections[DIM*idx + kdx];
						if (diff < -VALID_RANGE) diff += CORRECTION_MOD;
						if (diff > VALID_RANGE)  diff -= CORRECTION_MOD;
// 						if(playerID == DAVID) std::cout << "cmul diff = " << diff << std::endl;
						if (diff < -VALID_RANGE || diff > VALID_RANGE)
						{
							std::cout << "cmul different is too large: " << diff << std::endl;
						}
						maskedEvaluation[g->OutputWire][kdx] += diff;
					}
				}
			}
		}
	}
	
	void MaskedEvaluation::EvaluateMulGate(int layer, PreprocessingShare *beaver)
	{
// 		std::cout << playerID << ": MaskedEvaluation::EvaluateMulGate()" << std::endl;
// 		MyDebug::NonNull(beaver);
		assert(nullptr != beaver);
		
		auto mulGates = lc->operator[](layer)->MulGates;
		auto count = mulGates.size();
		// (x + lx)*(y+ ly) -<lx*my> - <ly*mx> + <lz> + <lx*ly>
		if (count > 0)
		{
			std::vector<int64_t> beaverShare = beaver->GetNextBeaverTriples();	// <lx*ly>
			std::vector<int64_t> myBeaverValuesShare(DIM*count);			// -<lx*my> - <ly*mx> + <lz> + <lx*ly>
			std::vector<int64_t> maskProduct(DIM*count);				// mx*my
			std::vector<int8_t> corrections(DIM*count);
			std::vector<int8_t> theirCorrections(DIM*count);
			
			assert(DIM*count == beaverShare.size());
			
			for (int i = 0; i < mulGates.size(); i++)
			{
				MultiplicationGate *g = mulGates[i];
				// Left wire is a scalar, Right wire is a vector
				int64_t mx              = maskedEvaluation[g->LeftWire][0];  // x + lx
				std::vector<int64_t> my = maskedEvaluation[g->RightWire]; // y + ly
				
				int leftWire = g->LeftWire;
				int rightWire = g->RightWire;
				int outputWire = g->OutputWire;
				
				int64_t lx = beaver->operator[]((*maskIndex)[leftWire]);
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					int64_t ly = beaver->operator[]((*maskIndex)[rightWire] + kdx);
					int64_t lz = beaver->operator[]((*maskIndex)[outputWire] + kdx);
					
					if (playerID == ALICE || playerID == CHARLIE){
						// myBeaverValuesShare[DIM*i + kdx] = ((-lx*my[kdx] - ly*mx + beaverShare[i*DIM + kdx]) >> PRECISION_BIT_LENGTH ) + (lz);
						myBeaverValuesShare[DIM*i + kdx] = (-lx*my[kdx] - ly*mx + beaverShare[i*DIM + kdx]) + (lz);
					}
					else {
						// myBeaverValuesShare[DIM*i + kdx] = ((mx*my[kdx] -lx*my[kdx] - ly*mx + beaverShare[i*DIM + kdx]) >> PRECISION_BIT_LENGTH ) + (lz);
						myBeaverValuesShare[DIM*i + kdx] = (mx*my[kdx] -lx*my[kdx] - ly*mx + beaverShare[i*DIM + kdx]) + (lz);
					}
				}
			}
			
			std::vector<unsigned char> temp = ArrayEncoder::Encode(myBeaverValuesShare);
			assert(temp.size() > 0);

			///TODO: Fix this

			std::vector<unsigned char> recv(DIM*count*sizeof(int64_t));
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
			
			std::vector<int64_t> theirBeaverValues = ArrayEncoder::UCharVec2Int64tVec(recv);
			
			for (int i = 0; i < count; i++)
			{
				MultiplicationGate *g = mulGates[i];
				maskedEvaluation[g->OutputWire].resize(DIM);
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					maskedEvaluation[g->OutputWire][kdx] = (myBeaverValuesShare[DIM*i + kdx] + theirBeaverValues[DIM*i + kdx]/* + maskProduct[DIM*i + kdx]*/);
					corrections[DIM*i + kdx] = ((maskedEvaluation[g->OutputWire][kdx] + masks[(*maskIndex)[g->OutputWire] + kdx]) % CORRECTION_MOD);
				}
			}
			
			if (playerID == ALICE || playerID == BOB)
			{
				communicator->SendVerificationPartner((unsigned char *)corrections.data(), sizeof(int8_t)*corrections.size());
			}
			else if(playerID == CHARLIE || playerID == DAVID)
			{
				communicator->AwaitVerificationPartner((unsigned char *)theirCorrections.data(), sizeof(int8_t)*theirCorrections.size());
				
				// Check the corrections
				for(int idx = 0; idx < mulGates.size(); idx++)
				{
					auto g = mulGates[idx];
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						int diff = theirCorrections[DIM*idx + kdx] - corrections[DIM*idx + kdx];
						if (diff < -VALID_RANGE) diff += CORRECTION_MOD;
						if (diff > VALID_RANGE)  diff -= CORRECTION_MOD;
						
						if (diff < -VALID_RANGE || diff > VALID_RANGE)
						{
							throw ErrorExceedsBound();
						}
						maskedEvaluation[g->OutputWire][kdx] += diff;
					}
				}
			}
		}
	}
	
	void MaskedEvaluation::EvaluateDotGate(int layer, PreprocessingShare *beaver)
	{
// 		std::cout << playerID << ": MaskedEvaluation::EvaluateDotGate()" << std::endl;
		auto dotGates = lc->operator[](layer)->DotGates;
		int count = dotGates.size();
		
		if(count > 0)
		{
			std::vector<int64_t> beaverShare = beaver->GetNextBeaverTriples();	// <lx*ly>
			std::vector<int64_t> myBeaverValuesShare(count);			// -<lx*my> - <ly*mx> + <lz> + <lx*ly>
			std::vector<int64_t> maskProduct(count);				// mx*my
			
			std::vector<int8_t> corrections(count);
			std::vector<int8_t> theirCorrections(count);
			
			assert(beaverShare.size() == count);
			
			for (int i = 0; i < count; i++)
			{
				DotProductGate *g = dotGates[i];
				
				std::vector<int64_t> mx = maskedEvaluation[g->LeftWire];  // x + lx
				std::vector<int64_t> my = maskedEvaluation[g->RightWire]; // y + ly
				
				int leftWire = g->LeftWire;
				int rightWire = g->RightWire;
				int outputWire = g->OutputWire;
				
				myBeaverValuesShare[i] = 0;
				maskProduct[i] = 0;
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					int64_t lx = beaver->operator[]((*maskIndex)[leftWire] + kdx);
					int64_t ly = beaver->operator[]((*maskIndex)[rightWire] + kdx);
					
					if (playerID == ALICE || playerID == CHARLIE){
						myBeaverValuesShare[i] += (-lx*my[kdx] - ly*mx[kdx]);
					}
					else{
						myBeaverValuesShare[i] += (mx[kdx]*my[kdx] -lx*my[kdx] - ly*mx[kdx]);
					}
					// at most +/- 1 off
					maskProduct[i] += mx[kdx]*my[kdx];
				}
				
				myBeaverValuesShare[i] = ((myBeaverValuesShare[i] + beaverShare[i]) >> PRECISION_BIT_LENGTH) + beaver->operator[]((*maskIndex)[outputWire]);
			}
			
			std::vector<unsigned char> temp = ArrayEncoder::Encode(myBeaverValuesShare);
			assert(temp.size() > 0);

			std::vector<unsigned char> recv(count*sizeof(int64_t));
			if (playerID == ALICE || playerID == CHARLIE)
			{
				communicator->SendEvaluationPartner(temp.data(), temp.size());
	// 			std::vector<unsigned char> recv = communicator->AwaitEvaluationPartner();
				communicator->AwaitEvaluationPartner(recv.data(), recv.size());
			}
			else
			{
	// 			std::vector<unsigned char> recv = communicator->AwaitEvaluationPartner();
				communicator->AwaitEvaluationPartner(recv.data(), recv.size());
				communicator->SendEvaluationPartner(temp.data(), temp.size());
			}
			
			std::vector<int64_t> theirBeaverValues = ArrayEncoder::UCharVec2Int64tVec(recv);
			
			for(int idx = 0; idx < count; idx++)
			{
				DotProductGate *g = dotGates[idx];
				
				maskedEvaluation[g->OutputWire].resize(1);
				
				// at most +/- 2 off
				maskedEvaluation[g->OutputWire][0] = (myBeaverValuesShare[idx] + theirBeaverValues[idx]);
				corrections[idx] = ((maskedEvaluation[g->OutputWire][0] + masks[(*maskIndex)[g->OutputWire]]) % CORRECTION_MOD);
			}
			
			if (playerID == ALICE || playerID == BOB)
			{
				communicator->SendVerificationPartner((unsigned char *)corrections.data(), sizeof(int8_t)*corrections.size());
			}
			else if(playerID == CHARLIE || playerID == DAVID)
			{
				communicator->AwaitVerificationPartner((unsigned char *)theirCorrections.data(), sizeof(int8_t)*theirCorrections.size());
				
				// Check the corrections
				for(int idx = 0; idx < count; idx++)
				{
					DotProductGate *g = dotGates[idx];
					
					int diff = theirCorrections[idx] - corrections[idx];
					if (diff < -VALID_RANGE) diff += CORRECTION_MOD;
					if (diff > VALID_RANGE)  diff -= CORRECTION_MOD;
					
					if (diff < -VALID_RANGE || diff > VALID_RANGE)
					{
						std::cout << "dot different is too large: " << diff << std::endl;
					}
					maskedEvaluation[g->OutputWire][0] += diff;
				}
			}
		}
	}
	
	void MaskedEvaluation::EvaluateNDotGate(int layer, PreprocessingShare *beaver)
	{
// 		if(playerID == DAVID) std::cout << "MaskedEvaluation::EvaluateNDotGate()" << std::endl;
		auto nDotGates = lc->operator[](layer)->NDotGates;
		
		int count = nDotGates.size();
		
		std::stringstream ss;
		if(count > 0)
		{
			std::vector<int64_t> beaverShare = beaver->GetNextBeaverTriples();	// <lx*ly>
			std::vector<int64_t> myBeaverValuesShare(DIM*count);			// -<lx*my> - <ly*mx> + <lz> + <lx*ly>
			std::vector<int64_t> maskProduct(DIM*count);				// mx*my
			std::vector<int8_t>  corrections(DIM*count);
			std::vector<int8_t>  theirCorrections(DIM*count);
			
			assert(beaverShare.size() == DIM*count);
			
			for (int i = 0; i < count; i++)
			{
				NaryDotGate *g = nDotGates[i];
				
				int step = g->InputWires.size()/2;
				for(int idx = 0; idx < step; idx++)
				{
					int64_t mx = maskedEvaluation[g->InputWires[idx]][0];  // r_idx + lr_idx
					std::vector<int64_t> my = maskedEvaluation[g->InputWires[idx + step]]; // y_idx + ly_idx
					
					// leftWire: r_idx
					// rightWire: y_idx
					int leftWire = g->InputWires[idx];
					int rightWire = g->InputWires[idx + step];
					
					int64_t lx = beaver->operator[]((*maskIndex)[leftWire]); 	// lx = <lr>
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						int64_t ly = beaver->operator[]((*maskIndex)[rightWire] + kdx);// ly = <ly[kdx]>
						
						if (playerID == ALICE || playerID == CHARLIE){
							myBeaverValuesShare[DIM*i + kdx] += (-lx*my[kdx] - ly*mx);
						}
						else {
							myBeaverValuesShare[DIM*i + kdx] += (mx*my[kdx] -lx*my[kdx] - ly*mx);
						}
							// at most +/- 1 off
						maskProduct[DIM*i + kdx] += mx*my[kdx];
					}
				}
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					myBeaverValuesShare[DIM*i + kdx] = ((myBeaverValuesShare[DIM*i + kdx] + beaverShare[DIM*i + kdx])/* >> PRECISION_BIT_LENGTH*/);
					myBeaverValuesShare[DIM*i + kdx] += beaver->operator[]((*maskIndex)[g->OutputWire] + kdx);
				}
			}
			
			std::vector<unsigned char> temp = ArrayEncoder::Encode(myBeaverValuesShare);
			assert(temp.size() > 0);

			std::vector<unsigned char> recv(DIM*count*sizeof(int64_t));
			if (playerID == ALICE || playerID == CHARLIE)
			{
				communicator->SendEvaluationPartner(temp.data(), temp.size());
// 			std::vector<unsigned char> recv = communicator->AwaitEvaluationPartner();
				communicator->AwaitEvaluationPartner(recv.data(), recv.size());
			}
			else
			{
				communicator->AwaitEvaluationPartner(recv.data(), recv.size());
				communicator->SendEvaluationPartner(temp.data(), temp.size());
// 			std::vector<unsigned char> recv = communicator->AwaitEvaluationPartner();
			}
			
			std::vector<int64_t> theirBeaverValues = ArrayEncoder::UCharVec2Int64tVec(recv);
			
				
			for(int idx = 0; idx < count; idx++)
			{
				NaryDotGate *g = nDotGates[idx];
				
				maskedEvaluation[g->OutputWire].resize(DIM);
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					// at most +/- 2 off
					maskedEvaluation[g->OutputWire][kdx] = (myBeaverValuesShare[DIM*idx + kdx] + theirBeaverValues[DIM*idx + kdx]);
					corrections[DIM*idx + kdx] = ((maskedEvaluation[g->OutputWire][kdx] + masks[(*maskIndex)[g->OutputWire] + kdx]) % CORRECTION_MOD);
				}
			}
			
			if (playerID == ALICE || playerID == BOB)
			{
				communicator->SendVerificationPartner((unsigned char *)corrections.data(), sizeof(int8_t)*corrections.size());
			}
			else if(playerID == CHARLIE || playerID == DAVID)
			{
				communicator->AwaitVerificationPartner((unsigned char *)theirCorrections.data(), sizeof(int8_t)*theirCorrections.size());
				
				// Check the corrections
				for(int idx = 0; idx < nDotGates.size(); idx++)
				{
					NaryDotGate *g = nDotGates[idx];
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						int diff = theirCorrections[DIM*idx + kdx] - corrections[DIM*idx + kdx];
						if (diff < -VALID_RANGE) diff += CORRECTION_MOD;
						if (diff > VALID_RANGE)  diff -= CORRECTION_MOD;
						
						if (diff < -VALID_RANGE || diff > VALID_RANGE)
						{
							std::cout << "ndot different is too large: " << diff << std::endl;
						}
						maskedEvaluation[g->OutputWire][kdx] += diff;
					}
				}
			}
		}
	}
}
