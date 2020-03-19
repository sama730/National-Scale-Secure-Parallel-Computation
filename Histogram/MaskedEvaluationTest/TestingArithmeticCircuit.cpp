#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <random>
#include <chrono>
#include <sstream>
#include <iostream>
#include <functional>
#include "../Circuit/ArithmeticGate.h"
#include "../Circuit/ArithmeticCircuit.h"
#include "../Circuit/ArithmeticCircuitBuilder.h"
#include "../Circuit/LayeredArithmeticCircuit.h"
#include "../Circuit/RandomArithmeticCircuitBuilder.h"
#include "../Circuit/LayeredArithmeticCircuitBuilder.h"
#include "../Circuit/PlainArithmeticCircuitEvaluator.h"
#include "../Circuit/GradientDescendCircuitBuilder.h"
#include "../Utility/CryptoUtility.h"
#include "../Utility/ISecureRNG.h"
#include "../Utility/Timer.h"

#include "TestingArithmeticCircuit.h"

using namespace Circuit;
using namespace Utility;
// using namespace TwoPartyMaskedEvaluation;

void TestingArithmeticCircuit::Test1()
{
	Timer t;
	ArithmeticCircuit *circuit;
	LayeredArithmeticCircuit *lc;
	
	int inputSize;
	int outputSize;
	int circuitSize;

	inputSize = 8;
	outputSize = 4;
	circuitSize = 16;
	
	std::cout << "Generate a random circuit" << std::endl;
	RandomArithmeticCircuitBuilder::Sample(inputSize, outputSize, circuitSize, circuit, lc);
	std::cout << "Circuit params after: " << std::endl;
	std::cout << circuit->InputLength << " " << circuit->OutputLength << " " << circuit->NumGates << " " << circuit->NumWires << std::endl;
	
	if(circuitSize < 20)
	{
		std::cout << lc->ToString() << std::endl;
	}
	
	unsigned seed =  std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	std::uniform_int_distribution<int> distribution(-31, 31);

	std::vector<int64_t> input(circuit->InputLength);
	for(int idx = 0; idx < circuit->InputLength; idx++)
	{
		int64_t rng = ((distribution(generator)) << 17);
		
// 		printf("rng %d = %f\n", idx, rng/1048576.0);
		
		input[idx] = rng;
	}
	
	printf("\n");
	
	std::vector<int64_t> evaluation(circuitSize);
	
	PlainArithmeticCircuitEvaluator pace;
	pace.EvaluateArithmeticCircuit(circuit, lc, input, evaluation);
	
	t.Tick("Elapsed time");
	
	if(circuitSize < 20)
	{
		for(int idx = 0; idx < circuitSize; idx++)
		{
			printf("%d : %f\n", idx, evaluation[idx]/1048576.0);
		}
	}
	printf("\n");
	
	std::cout << "Done and terminate&" << std::endl;
	
	std::cout << "End testing" << std::endl;
}

void TestingArithmeticCircuit::Test2()
{
	std::vector<int> itemsPerUser; 
	for(int idx = 0; idx < 1; idx++)
	{
		itemsPerUser.push_back(2);
	}
	
	GradientDescendCircuitBuilder::BuildMultipleUsersByLayers(itemsPerUser, 0.25, 0.1, "../MaskedEvaluationTest/Circuits/MultipleUsersByLayers.txt");
	
	Timer t;
	
	ArithmeticCircuitBuilder circuitBuilder;
	LayeredArithmeticCircuitBuilder layerBuilder;
	
	ArithmeticCircuit *circuit = circuitBuilder.CreateCircuit("./Circuits/MultipleUsersByLayers.txt");
	LayeredArithmeticCircuit *lc = layerBuilder.CreateLayeredCircuit(circuit);
	
	t.Tick("Time to create circuit");
	
	// ####################################################################################################
	
	int length = lc->NumWires;
	std::vector<unsigned char> seed = CryptoUtility::SampleByteArray(32);
	ISecureRNG *rng = new AESRNG(seed.data());
	
	// Count number of required random values: some wires need 10 masks, some 1
	int numUsers = itemsPerUser.size();
	int sumNumItems = 0;
	int count = 0;
	
	for(int idx = 0; idx < itemsPerUser.size(); idx++)
	{
		sumNumItems += itemsPerUser[idx];
	}
	
	int numMasks = 6*DIM*numUsers + (2*DIM + 4)*sumNumItems;
	int numWires = 6*numUsers + 6*sumNumItems;
	/// masks contain lx for wire x
// 	std::vector<int64_t> masks(numMasks);
// 	std::vector<int64_t> masks = rng->GetMaskArray(numMasks); //CryptoUtility::SampleInt64Array(length);
	
	
	std::vector<std::vector<int64_t> > aliceBeaverShares;
	std::vector<std::vector<int64_t> > bobBeaverShares;
	
	
	
	/// Generate all the masks at the same time, then assign values to each wire later
	std::vector<int64_t> masks = rng->GetMaskArray(numMasks);
	
	t.Tick("Time to compute AES:");
	
// 	std::vector<int> maskIndex = SimpleProcessingBuilder::buildMaskIndex(itemsPerUser);
	std::vector<int> maskIndex = CryptoUtility::buildMaskIndex(itemsPerUser);
	t.Tick("Time to index masks: ");
	
	for (int ilayer = 0; ilayer < lc->Depth; ilayer++)
	{
		std::cout << "Layer " << ilayer << std::endl;
		ArithmeticLayer *bl = lc->operator[](ilayer);

		/// (x + lx) + (y + ly) = (x + y) + (lx + ly)
		for (auto &g : bl->AddGates)
		{
// 			std::cout << "Add Gate Index: " << g->LeftWire << " " << g->RightWire << " " << g->OutputWire << std::endl;
			int leftIndex = maskIndex[g->LeftWire];
			int rightIndex = maskIndex[g->RightWire];
			int outIndex = maskIndex[g->OutputWire];
			
			for(int idx = 0; idx < DIM; idx++)
			{
// 				std::cout << "Index: " << outIndex << " " << leftIndex << " " << rightIndex << std::endl;
				masks[outIndex] = masks[leftIndex] + masks[rightIndex];
				leftIndex++; rightIndex++; outIndex++;
			}
// 				masks[g->OutputWire] = VectorOperation::Add(masks[g->LeftWire], masks[g->RightWire]);
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
// 					bobBeaverShare[start + kdx] = ArithmeticOperation::mul(masks[leftIndex], masks[rightIndex]) - aliceBeaverShare[start + kdx];
					bobBeaverShare[count] = ArithmeticOperation::mul(masks[leftIndex], masks[rightIndex]) - aliceBeaverShare[count];
					leftIndex++; rightIndex++; count++;
				}
			}
			
			aliceBeaverShares.push_back(aliceBeaverShare);
			bobBeaverShares.push_back(bobBeaverShare);
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
				
				bobBeaverShare[idx] = -aliceBeaverShare[idx];
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					bobBeaverShare[idx] += ArithmeticOperation::mul(masks[leftIndex], masks[rightIndex]);
					leftIndex++; rightIndex++;
				}
			}
			
			aliceBeaverShares.push_back(aliceBeaverShare);
			bobBeaverShares.push_back(bobBeaverShare);
		}
		
		auto nDotGates = bl->NDotGates;
		
		if (nDotGates.size() > 0)
		{
// 			std::cout << "NDot Gates" << std::endl;
			int count = nDotGates.size();
			
// 			std::cout << "Num NDOT gates: " << count << std::endl;
			
			// For idx-th gate ( (x, y), z ): aliceBeaverShare[idx] = <lx*ly> and bobBeaverShares[idx] = <lx*ly>
			std::vector<int64_t> aliceBeaverShare = CryptoUtility::SampleInt64Array(DIM*count);
			std::vector<int64_t> bobBeaverShare(DIM*count);
							
			// For idx-th gate ( (x, y), z ): beaver[idx] = lx*ly
			for (int idx = 0; idx < count; idx++)
			{
				NaryDotGate *g = nDotGates[idx];
				
// 				std::cout << maskIndex[g->LeftWire] << " " << maskIndex[g->RightWire] << std::endl;
				int step = g->InputWires.size()/2;
				
				for(int jdx = 0; jdx < step; jdx++)
				{
					int leftIndex = maskIndex[g->InputWires[jdx]];
					int rightIndex = maskIndex[g->InputWires[jdx + step]];
					
					for(int kdx = 0; kdx < DIM; kdx++)
					{
						bobBeaverShare[DIM*idx + kdx] += ArithmeticOperation::mul(masks[leftIndex + kdx], masks[rightIndex + kdx]);
					}
				}
				
				for(int kdx = 0; kdx < DIM; kdx++)
				{
					bobBeaverShare[DIM*idx + kdx] = -aliceBeaverShare[DIM*idx + kdx];
				}
			}
			
			aliceBeaverShares.push_back(aliceBeaverShare);
			bobBeaverShares.push_back(bobBeaverShare);
		}
		
		t.Tick("Time for this layer");
	}

	/// secret share for wires
	// For wire x: masksShareAlice[x] = <lx> and masksShareBob[x] = <lx>
	std::vector<int64_t> masksShareAlice = CryptoUtility::SampleInt64Array(numMasks);
	std::vector<int64_t> masksShareBob   = VectorOperation::Sub(masks, masksShareAlice);
	
	t.Tick("Time to prepare shares:");
	
	int inputSize = circuit->InputLength;
	int outputSize = circuit->OutputLength;;
	int circuitSize = circuit->NumWires;
	
	if(circuitSize < 50)
	{
		std::cout << lc->ToString() << std::endl;
	}
	
	unsigned seed2 =  std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed2);
	std::uniform_int_distribution<int> distribution(-31, 31);

	std::uniform_int_distribution<int> distribution2(1, 5);
	
	// input for 2*(4 - <u,v>) + 2u
	// u, v, 4
	std::vector<std::vector<int64_t> > input(circuit->InputLength);
	count = 0;
	for(int udx = 0; udx < itemsPerUser.size(); udx++)
	{
		// Input for u
		for(int kdx = 0; kdx < 10; kdx++)
		{
		      int64_t rng = ((distribution(generator)) << 17);
		      
		      input[count].push_back(rng);
		}
		count++;
		// Input for items
		for(int idx = 0; idx < itemsPerUser[udx]; idx++)
		{
			// Input for v
			for(int kdx = 0; kdx < 10; kdx++)
			{
			      int64_t rng = ((distribution(generator)) << 17);
			      
			      input[count].push_back(rng);
			}
			count++;
			input[count].push_back(distribution2(generator)<<20);
			count++;
			input[count].push_back(1<<20);
			count++;
		}
	}
	printf("\n");
	
	std::vector<std::vector<int64_t> >evaluation(circuitSize);
	
	PlainArithmeticCircuitEvaluator pace;
	pace.EvaluateArithmeticCircuit(circuit, lc, input, itemsPerUser, evaluation);
	
	t.Tick("Time to evaluate the circuit:");
	
	if(circuitSize < 50)
	{
		for(int idx = 0; idx < circuitSize; idx++)
		{
			printf("%d : ", idx);
			for(int kdx = 0; kdx < evaluation[idx].size(); kdx++)
			{
				printf("%f \n", evaluation[idx][kdx]/1048576.0);
			}
			printf("\n\n");
		}
	}
	printf("\n");
	
	std::cout << "Done and terminate&" << std::endl;
	
	std::cout << "End testing" << std::endl;
}
