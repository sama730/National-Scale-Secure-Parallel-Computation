#include <stdint.h>
#include <iostream>
#include "EmulatedMaskedEvaluation.h"
#include "PreprocessingShare.h"
#include "../Utility/CryptoUtility.h"

using namespace Circuit;
using namespace Utility;

namespace TwoPartyMaskedEvaluation
{

	EmulatedMaskedEvaluation::EmulatedMaskedEvaluation(PreprocessingShare *aliceShare, PreprocessingShare *bobShare, LayeredArithmeticCircuit *lc) : aliceShare(aliceShare), bobShare(bobShare), lc(lc){}
	
	EmulatedMaskedEvaluation::EmulatedMaskedEvaluation(PreprocessingShare *aliceShare, PreprocessingShare *bobShare, std::vector<int> *maskIndex, LayeredArithmeticCircuit *lc) : aliceShare(aliceShare), bobShare(bobShare), maskIndex(maskIndex), lc(lc){}

// 	void EmulatedMaskedEvaluation::AddInput(std::vector<uint64_t> input, Range *range)
// 	{
// 		maskedEvaluation = std::move(input);
// 		maskedEvaluation.resize(lc->NumWires);
// 		
// // 		for(int idx = range->Start; idx < range->Start + range->Length; idx++)
// // 		{
// // 			maskedEvaluation[idx] = input[idx];
// // 		}
// 		
// 		inputAdded = true;
// 	}
	
	void EmulatedMaskedEvaluation::AddInput(std::vector<std::vector<uint64_t> >&& input, Range *range)
	{
		maskedEvaluation = std::move(input);
		maskedEvaluation.resize(lc->NumWires);
		
		inputAdded = true;
	}

// 	std::vector<uint64_t> EmulatedMaskedEvaluation::Decrypt(std::vector<uint64_t> mask, Range *range)
// 	{
// // 		assert(mask->Length == range->Length);
// 
// 		if (!evaluated)
// 		{
// 			throw std::exception();
// 		}
// // 		std::cout << "Masks: " << std::endl;
// 		for(int idx = range->Start; idx < range->Start + range->Length; idx++)
// 		{
// // 			std::cout << idx << "\t" << mask[idx] << std::endl;
// 			maskedEvaluation[idx] = ArithmeticOperation::sub(maskedEvaluation[idx], mask[idx]);
// 		}
// 		
// 		return maskedEvaluation;
// 	}

	std::vector<std::vector<uint64_t> > EmulatedMaskedEvaluation::Decrypt(std::vector<uint64_t> mask, Range *range)
	{
// 		assert(mask->Length == range->Length);

		if (!evaluated)
		{
			throw std::exception();
		}
// 		std::cout << "Masks: " << std::endl;
		for(int idx = range->Start; idx < range->Start + range->Length; idx++)
		{
			for(int kdx = 0; kdx < maskedEvaluation[idx].size(); kdx++)
			{
				  maskedEvaluation[idx][kdx] -= mask[(*maskIndex)[idx] + kdx];
			}
		}
		
		return maskedEvaluation;
	}

	
	void EmulatedMaskedEvaluation::EvaluateCircuit()
	{
		if (!inputAdded)
		{
			throw std::exception();
		}

// 		uint64_t *aliceBeaver = aliceShare->GetNextBeaverTriples();
// 		uint64_t *bobBeaver = bobShare->GetNextBeaverTriples();

		for (int i = 0; i < lc->Depth; i++)
		{
			EvaluateAddGate(i);
			EvaluateSubGate(i);
			EvaluateCAddGate(i);
			EvaluateCMulGate(i);
			EvaluateNAddGate(i);
			EvaluateMulGate(i, aliceShare, bobShare);
			EvaluateDotGate(i, aliceShare, bobShare);
		}
		
		evaluated = true;
	}

	void EmulatedMaskedEvaluation::EvaluateAddGate(int layer)
	{
		for (auto &g : lc->operator[](layer)->AddGates)
		{
			std::vector<uint64_t> leftValue  = maskedEvaluation[g->LeftWire];
			std::vector<uint64_t> rightValue = maskedEvaluation[g->RightWire];
			
			maskedEvaluation[g->OutputWire].resize(leftValue.size());
			
			for(int idx = 0; idx < leftValue.size(); idx++)
			{
				maskedEvaluation[g->OutputWire][idx] = ArithmeticOperation::add(leftValue[idx], rightValue[idx]);
			}
// 			maskedEvaluation[outputWire] = ArithmeticOperation::add(maskedEvaluation[leftWire], maskedEvaluation[rightWire]);
		}
	}
	
	void EmulatedMaskedEvaluation::EvaluateNAddGate(int layer)
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
	
	void EmulatedMaskedEvaluation::EvaluateSubGate(int layer)
	{
		for (auto &g : lc->operator[](layer)->SubGates)
		{
			std::vector<uint64_t> leftValue  = maskedEvaluation[g->LeftWire];
			std::vector<uint64_t> rightValue = maskedEvaluation[g->RightWire];
			
			maskedEvaluation[g->OutputWire].resize(leftValue.size());
			
			for(int idx = 0; idx < leftValue.size(); idx++)
			{
				maskedEvaluation[g->OutputWire][idx] = ArithmeticOperation::sub(leftValue[idx], rightValue[idx]);
			}
					
// 			maskedEvaluation[outputWire] = ArithmeticOperation::sub(maskedEvaluation[leftWire], maskedEvaluation[rightWire]);
		}
	}
	
	void EmulatedMaskedEvaluation::EvaluateCAddGate(int layer)
	{
		for (auto &g : lc->operator[](layer)->CAddGates)
		{
			std::vector<uint64_t> inputValue = maskedEvaluation[g->InputWire];
			uint64_t constant = g->constant;
			maskedEvaluation[g->OutputWire].resize(inputValue.size());
			for(int idx = 0; idx < inputValue.size(); idx++)
			{
				maskedEvaluation[g->OutputWire][idx] = ArithmeticOperation::add(constant, inputValue[idx]);
			}
			
// 			maskedEvaluation[outputWire] = ArithmeticOperation::add(maskedEvaluation[inputWire], g->constant);
		}
	}
	
	void EmulatedMaskedEvaluation::EvaluateCMulGate(int layer)
	{
		for (auto &g : lc->operator[](layer)->CMulGates)
		{
			std::vector<uint64_t> inputValue = maskedEvaluation[g->InputWire];
			uint64_t constant = g->constant;
			maskedEvaluation[g->OutputWire].resize(inputValue.size());
			
			for(int idx = 0; idx < inputValue.size(); idx++)
			{
				maskedEvaluation[g->OutputWire][idx] = ArithmeticOperation::mul(constant, inputValue[idx]);
			}
		}
	}

	void EmulatedMaskedEvaluation::EvaluateMulGate(int layer, PreprocessingShare *aliceBeaver, PreprocessingShare *bobBeaver)
	{
		auto mulGates = lc->operator[](layer)->MulGates;
		
		if (mulGates.size() > 0)
		{
			std::vector<uint64_t> aliceBeaverShare = aliceBeaver->GetNextBeaverTriples();
			std::vector<uint64_t> bobBeaverShare   = bobBeaver->GetNextBeaverTriples();
			
			assert(aliceBeaverShare.size() == bobBeaverShare.size());
			assert(aliceBeaverShare.size() == (DIM*mulGates.size()));
			
			for (int i = 0; i < mulGates.size(); i++)
			{
				MultiplicationGate *g = mulGates[i];
				
				maskedEvaluation[g->OutputWire].resize(DIM);
				
				uint64_t mx = maskedEvaluation[g->LeftWire][0];  // x + lx
				std::vector<uint64_t> my = maskedEvaluation[g->RightWire]; // y + ly

				int leftWire = g->LeftWire;
				int rightWire = g->RightWire;
				int outputWire = g->OutputWire;
				
				uint64_t lx1 = aliceShare->operator[]((*maskIndex)[leftWire]);
				uint64_t lx2 = bobShare->operator[]((*maskIndex)[leftWire]);
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					uint64_t ly1 = aliceShare->operator[]((*maskIndex)[rightWire] + kdx);
					uint64_t lz1 = aliceShare->operator[]((*maskIndex)[outputWire] + kdx);
					uint64_t aliceBeaverValue = -ArithmeticOperation::mul(lx1, my[kdx]) - ArithmeticOperation::mul(ly1, mx) + lz1 + aliceBeaverShare[i*DIM + kdx];
					
					uint64_t ly2 = bobShare->operator[]((*maskIndex)[rightWire] + kdx);
					uint64_t lz2 = bobShare->operator[]((*maskIndex)[outputWire] + kdx);
					
					uint64_t bobBeaverValue = -ArithmeticOperation::mul(lx2, my[kdx]) - ArithmeticOperation::mul(ly2, mx) + lz2 + bobBeaverShare[i*DIM + kdx];
					
					maskedEvaluation[g->OutputWire][kdx] = ArithmeticOperation::mul(mx, my[kdx]) + aliceBeaverValue + bobBeaverValue;
				}
			}
		}
	}
	
	void EmulatedMaskedEvaluation::EvaluateDotGate(int layer, PreprocessingShare *aliceBeaver, PreprocessingShare *bobBeaver){
		auto dotGates = lc->operator[](layer)->DotGates;
		
		if(dotGates.size() > 0)
		{
			std::vector<uint64_t> aliceBeaverShare = aliceBeaver->GetNextBeaverTriples();
			std::vector<uint64_t> bobBeaverShare   = bobBeaver->GetNextBeaverTriples();
			
			assert(aliceBeaverShare.size() == bobBeaverShare.size());
			assert(aliceBeaverShare.size() == dotGates.size());
			
			for (int i = 0; i < dotGates.size(); i++)
			{
				DotProductGate *g = dotGates[i];
				
				maskedEvaluation[g->OutputWire].resize(1);
				maskedEvaluation[g->OutputWire][0] = 0;
				
				std::vector<uint64_t> mx = maskedEvaluation[g->LeftWire];  // x + lx
				std::vector<uint64_t> my = maskedEvaluation[g->RightWire]; // y + ly

				int leftWire = g->LeftWire;
				int rightWire = g->RightWire;
				int outputWire = g->OutputWire;
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					uint64_t lx1 = aliceShare->operator[]((*maskIndex)[leftWire] + kdx);
					uint64_t ly1 = aliceShare->operator[]((*maskIndex)[rightWire] + kdx);
					
					uint64_t aliceBeaverValue = -ArithmeticOperation::mul(lx1, my[kdx]) - ArithmeticOperation::mul(ly1, mx[kdx]);
					
					uint64_t lx2 = bobShare->operator[]((*maskIndex)[leftWire] + kdx);
					uint64_t ly2 = bobShare->operator[]((*maskIndex)[rightWire] + kdx);
					
					uint64_t bobBeaverValue = -ArithmeticOperation::mul(lx2, my[kdx]) - ArithmeticOperation::mul(ly2, mx[kdx]);
					
					maskedEvaluation[g->OutputWire][0] += ArithmeticOperation::mul(mx[kdx], my[kdx]) + aliceBeaverValue + bobBeaverValue;
				}
				
				uint64_t lz1 = aliceShare->operator[]((*maskIndex)[outputWire]);
				uint64_t lz2 = bobShare->operator[]((*maskIndex)[outputWire]);
				maskedEvaluation[g->OutputWire][0] =  maskedEvaluation[g->OutputWire][0] + aliceBeaverShare[i] + bobBeaverShare[i] + lz1 + lz2;
			}
		}
	}

	uint64_t EmulatedMaskedEvaluation::BeaverEvaluation(MultiplicationGate *g, uint64_t beaver, uint64_t mx, uint64_t my, PreprocessingShare *share)
	{
		int leftWire = g->LeftWire;
		int rightWire = g->RightWire;
		int outputWire = g->OutputWire;

		uint64_t lx = share->operator[](leftWire);
		uint64_t ly = share->operator[](rightWire);
		uint64_t lz = share->operator[](outputWire);

		return -ArithmeticOperation::mul(lx, my) - ArithmeticOperation::mul(ly, mx) + lz + beaver;
	}
}
