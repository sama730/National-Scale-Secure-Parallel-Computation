#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>
#include <ctime>
#include <set>
#include <random>
#include <chrono>
#include <iostream>
#include <functional>
#include <future>
#include <thread>

#include "../Circuit/ArithmeticGate.h"
#include "../Circuit/ArithmeticCircuit.h"
#include "../Circuit/ArithmeticCircuitBuilder.h"
#include "../Circuit/LayeredArithmeticCircuit.h"
#include "../Circuit/RandomArithmeticCircuitBuilder.h"
#include "../Circuit/LayeredArithmeticCircuitBuilder.h"
#include "../Circuit/PlainArithmeticCircuitEvaluator.h"

#include "../TwoPartyMaskedEvaluation/PreprocessingBuilder.h"
#include "../TwoPartyMaskedEvaluation/SimplePreprocessingBuilder.h"
#include "../TwoPartyMaskedEvaluation/MaskedEvaluation.h"

#include "../Utility/CryptoUtility.h"
#include "../Utility/Range.h"
#include "../Utility/Channels.h"
#include "../Utility/Communicator.h"
#include "../Utility/CommunicatorBuilder.h"

#include "../CrossCheck/PreprocessingParty.h"

#include "../Circuit/GradientDescendCircuitBuilder.h"

#include "TestingPreprocessingBuilder.h"

using namespace Utility;
using namespace Circuit;
using namespace CrossCheck;
using namespace TwoPartyMaskedEvaluation;

namespace MaskedEvaluationTest
{
	void TestingPreprocessingBuilder::TestPreprocessingBuilder()
	{
		Timer t;
		
		std::vector<int> itemsPerUser; 
		for(int idx = 0; idx < 1; idx++)
		{
			itemsPerUser.push_back(2);
		}
		
		GradientDescendCircuitBuilder::BuildMultipleUsersByLayers(itemsPerUser, 0.25, 0.5, "../MaskedEvaluationTest/Circuits/MultipleUsersByLayers.txt");
		
		ArithmeticCircuitBuilder circuitBuilder;
		LayeredArithmeticCircuitBuilder layerBuilder;
		
		circuit = circuitBuilder.CreateCircuit("./Circuits/MultipleUsersByLayers.txt");
		lc = layerBuilder.CreateLayeredCircuit(circuit);
		
		int inputSize = circuit->InputLength;
		int outputSize = circuit->OutputLength;;
		int circuitSize = circuit->NumWires;
		
		inputRange = new Range(0, inputSize);
		
		if(circuitSize < 50)
		{
			std::cout << lc->ToString() << std::endl;
		}
		
		t.Tick("Time to create circuit");
		
		
		PreprocessingBuilder *builder = new PreprocessingBuilder();
		
		std::vector<unsigned char> seedAlice = CryptoUtility::SampleByteArray(32);
		std::vector<unsigned char> seedBob   = CryptoUtility::SampleByteArray(32);
		
		std::cout << "Building circuit..." << std::endl;
		builder->BuildPreprocessing(seedAlice, seedBob, lc, itemsPerUser);
		
		std::cout << "Getting Alice's share..." << std::endl;
		
		auto aliceShare = AlicePreprocessingBuilder::Build(lc, seedAlice, itemsPerUser);
		
		std::cout << "Getting Bob's share..." << std::endl;
		auto bobShare = BobPreprocessingBuilder::Build(lc, seedBob, builder->getBobCorrection(), itemsPerUser);
		
		Communicator *c1, *c2, *c3, *c4;
		CommunicatorBuilder::BuildCommunicator(c1, c2, c3, c4);
		
		if(circuitSize < 50)
		{
			TestUtility::PrintVector(aliceShare->beaverShare, "Alice beaver share", 1.0);
			TestUtility::PrintVector(bobShare->beaverShare, "Bob beaver share", 1.0);
		}
		
		std::cout << "TestPreprocessing..." << std::endl;
		TestPreprocessing(aliceShare, bobShare, c1, c2, itemsPerUser);
		t.Tick("Total test time");
	}

	void TestingPreprocessingBuilder::TestDistributedPreprocessing()
	{
		Communicator *c1, *c2, *c3, *c4;
		CommunicatorBuilder::BuildCommunicator(c1, c2, c3, c4);
		
		PreprocessingParty *p1 = new PreprocessingParty(c1, true);
		PreprocessingParty *p2 = new PreprocessingParty(c2, false);
		PreprocessingParty *p3 = new PreprocessingParty(c3, true);
		PreprocessingParty *p4 = new PreprocessingParty(c4, false);

		Timer t;
		
		std::vector<int> itemsPerUser; 
		for(int idx = 0; idx < 6000; idx++)
		{
			itemsPerUser.push_back(200);
		}
		
		GradientDescendCircuitBuilder::BuildMultipleUsersByLayers(itemsPerUser, 0.25, 0.5, "../MaskedEvaluationTest/Circuits/MultipleUsersByLayers.txt");
		
		ArithmeticCircuitBuilder circuitBuilder;
		LayeredArithmeticCircuitBuilder layerBuilder;
		
		circuit = circuitBuilder.CreateCircuit("./Circuits/MultipleUsersByLayers.txt");
		lc = layerBuilder.CreateLayeredCircuit(circuit);
		
		int inputSize = circuit->InputLength;
		int outputSize = circuit->OutputLength;;
		int circuitSize = circuit->NumWires;
		
		inputRange = new Range(0, inputSize);
		
		if(circuitSize < 50)
		{
			std::cout << lc->ToString() << std::endl;
		}
		
		t.Tick("Time to create circuit");
		
		std::cout << "p1 and p2 build preprocessing..." << std::endl;
		std::future<std::vector<int64_t> > t1 = std::async(std::launch::async, [&]{return p1->BuildPreprocessing(lc, itemsPerUser);});
		std::future<std::vector<int64_t> > t2 = std::async(std::launch::async, [&]{return p2->BuildPreprocessing(lc, itemsPerUser);});
		
		
		std::cout << "p3 and p4 receive preprocessing..." << std::endl;
		
		std::future<PreprocessingShare *> t3 = std::async(std::launch::async, [&]{return p3->ReceivePreprocessing(lc, itemsPerUser);});
		std::future<PreprocessingShare *> t4 = std::async(std::launch::async, [&]{return p4->ReceivePreprocessing(lc, itemsPerUser);});
		
		t1.wait();
		t2.wait();
		t3.wait();
		t4.wait();
		
		auto p1Mask = t1.get(); 
		auto p2Mask = t2.get();
		auto aliceShare = t3.get();
		auto bobShare = t4.get();
		
		
// 		t.Tick("Time to build masks and share");
		
		std::cout << "Runging test preprocessing..." << std::endl;
		TestPreprocessing(aliceShare, bobShare, c3, c4, itemsPerUser);
		
		t.Tick("Processing time");
	}
	
	void TestingPreprocessingBuilder::TestPreprocessing(PreprocessingShare *aliceShare, PreprocessingShare *bobShare, Communicator *c1, Communicator *c2, const std::vector<int>& itemsPerUser)
	{
		Timer t;
		
		int inputSize = circuit->InputLength;
		int outputSize = circuit->OutputLength;;
		int circuitSize = circuit->NumWires;
		
		int numUsers = itemsPerUser.size();
		int sumNumItems = 0;
		for(int idx = 0; idx < itemsPerUser.size(); idx++)
		{
			sumNumItems += itemsPerUser[idx];
		}
		
		int numMasks = 6*DIM*numUsers + (2*DIM + 4)*sumNumItems;
		int numWires = 6*numUsers + 6*sumNumItems;
		
		std::cout << "Num masks: " << numMasks << std::endl;
		
		std::cout << "Get masks..." << std::endl;
		std::vector<int64_t> masks(numMasks);
		for(int idx = 0; idx < numMasks; idx++)
		{
			masks[idx] = aliceShare->maskShare[idx] + bobShare->maskShare[idx];
		}
		
		sizeRange = new Range(0, numWires);
		
		std::vector<std::vector<int64_t> > input = TestUtility::GenerateInput(itemsPerUser, circuit->InputLength);
		std::vector<std::vector<int64_t> > evaluation(circuitSize);
		
		PlainArithmeticCircuitEvaluator pace;
		pace.EvaluateArithmeticCircuit(circuit, lc, input, itemsPerUser, evaluation);
		
		t.Tick("Plain evaluation");
		
		if(circuitSize < 50)
		{
			TestUtility::PrintVector(evaluation, "Evaluation", 1048576.0);
		}
		
		std::vector<int> maskIndex = CryptoUtility::buildMaskIndex(itemsPerUser);
		
		std::vector<std::vector<int64_t> > maskedInput(inputSize);
// 		std::cout << "Masks before evaluation:" << std::endl;
		for(int idx = 0; idx < inputSize; idx++)
		{
			maskedInput[idx].resize(input[idx].size());
			
			for(int kdx = 0; kdx < input[idx].size(); kdx++)
			{
				maskedInput[idx][kdx] = (masks[maskIndex[idx] + kdx] + input[idx][kdx]);
			}
		}
		
		t.Tick("Compute masked input");
	  
// 		std::cout << "aliceEvaluation..." << std::endl;
		auto aliceEvaluation = new MaskedEvaluation(lc, aliceShare, &maskIndex, c1);
		aliceEvaluation->AddInput(maskedInput, inputRange);
		
// 		std::cout << "bobEvaluation..." << std::endl;
		auto bobEvaluation = new MaskedEvaluation(lc, bobShare, &maskIndex, c2);
		bobEvaluation->AddInput(maskedInput, inputRange);

		std::cout << "Alice Evaluation ..." << std::endl;
		std::future<void> t5 = std::async(std::launch::async, [&aliceEvaluation]{ aliceEvaluation->EvaluateCircuit(); });
		
		std::cout << "Bob Evaluation ..." << std::endl;
		std::future<void> t6 = std::async(std::launch::async, [&bobEvaluation]{ bobEvaluation->EvaluateCircuit(); });
			
		t5.wait();
		t6.wait();
		
		t5.get(); t6.get();

		t.Tick("Evaluate circuit");
		
		std::cout << "Alice Decrypts ..." << std::endl;
		auto maskedEvaluationPlain1 = aliceEvaluation->Decrypt(masks, sizeRange);
		
		std::cout << "Bob Decrypts ..." << std::endl;
		auto maskedEvaluationPlain2 = bobEvaluation->Decrypt(masks, sizeRange);
		
		t.Tick("Decrypt output");
		
		if(circuitSize < 50)
		{
			TestUtility::PrintVector(maskedEvaluationPlain1, "Alice Evaluation", 1048576.0);
			TestUtility::PrintVector(maskedEvaluationPlain2, "Bob Evaluation", 1048576.0);
		}
		else
		{
			TestUtility::PrintVector(evaluation[circuitSize-1], "Plain Evaluation");
			TestUtility::PrintVector(maskedEvaluationPlain1[circuitSize-1], "Alice Evaluation");
			TestUtility::PrintVector(maskedEvaluationPlain2[circuitSize-1], "Bob Evaluation");
		}
		
		std::set<int> diff;	
		std::set<int>::iterator it = diff.begin();
		
		for(int idx = 0; idx < circuitSize; idx++)
		{
			for(int kdx = 0; kdx < evaluation[idx].size(); kdx++)
			{
				diff.insert(it, (int)((maskedEvaluationPlain1[idx][kdx] - evaluation[idx][kdx])/1048576.0));
				diff.insert(it, (int)((maskedEvaluationPlain2[idx][kdx] - evaluation[idx][kdx])/1048576.0));
			}
		}
		
		std::cout << "Comparing output: ";
		for(it = diff.begin(); it != diff.end(); ++it)
		{
			if(circuitSize < 50) std::cout << *it << " ";
			else std::cout << diff.size() << std::endl;
		}
		
// 		std::stringstream ss;
// 		ss << "Layer difference: ";
// 		std::vector<int64_t> layer_diff(0);
// 		for(int ldx = 0; ldx < lc->Depth; ldx++)
// 		{
// 			int64_t maxdiff = 0;
// 			
// 			std::vector<IGate *> lgates;
// 			
// 			/// Get all the gates of the ldx-th layer
// 			for (auto &g : (lc->operator[](ldx))->AddGates) {lgates.push_back(g);}
// 			for (auto &g : (lc->operator[](ldx))->SubGates) {lgates.push_back(g);}
// 			for (auto &g : (lc->operator[](ldx))->MulGates) {lgates.push_back(g);}
// 			for (auto &g : (lc->operator[](ldx))->DivGates) {lgates.push_back(g);}
// 			for (auto &g : (lc->operator[](ldx))->CAddGates) {lgates.push_back(g);}
// 			for (auto &g : (lc->operator[](ldx))->CMulGates) {lgates.push_back(g);}
// 			
// 			/// Evaluate each gate
// 			for (int i = 0; i < lgates.size();i++)
// 			{
// 				IGate *g = lgates[i];
// 				
// 				if (dynamic_cast<BinaryGate*>(g) != nullptr)
// 				{
// 					BinaryGate *bg = (BinaryGate *)g;
// 					int outputWire = bg->OutputWire;
// 					int64_t temp = abs(maskedEvaluationPlain1[outputWire] - maskedEvaluationPlain2[outputWire]);
// 					maxdiff = maxdiff >= temp ? maxdiff : temp;
// 				}
// 				else if (dynamic_cast<ConstantGate *>(g) != nullptr)
// 				{
// 					ConstantGate *ug = (ConstantGate *)g;
// 					int outputWire = ug->OutputWire;
// 					int64_t temp = abs(maskedEvaluationPlain1[outputWire] - maskedEvaluationPlain2[outputWire]);
// 					maxdiff = maxdiff >= temp ? maxdiff : temp;
// 				}
// 			}
// 			
// 			ss << maxdiff << " ";
// 			layer_diff.push_back(maxdiff);
// 		}
		printf("\n");
// 		std::cout << ss.str() << std::endl;
		std::cout << "Done and terminate&" << std::endl;
		
		std::cout << "End testing" << std::endl;
	}
	
	
	/*void TestingPreprocessingBuilder::TestPreprocessingBuilder()
	{
		int circuitSize = 15;
		int inputSize, outputSize;
		Range *inputRange, *sizeRange;
		ArithmeticCircuit *circuit;
		LayeredArithmeticCircuit *lc;
		
		CreateInputOutput(circuitSize, inputSize, outputSize, inputRange, sizeRange, circuit, lc);

		unsigned seed =  std::chrono::system_clock::now().time_since_epoch().count();
		
		std::cout << lc->ToString() << std::endl;
		
		std::default_random_engine generator(seed);
		std::uniform_int_distribution<int> distribution(-3, 3);

		std::vector<int64_t> input;
		for(int idx = 0; idx < circuit->InputLength; idx++)
		{
			int64_t rng = ((distribution(generator)) << 17);
			
	// 		printf("rng %d = %f\n", idx, rng/1048576.0);
			
			input.push_back(rng);
		}
		
// 		printf("\n");
		
		std::vector<int64_t> evaluation(circuitSize);
		for(int idx = 0; idx < circuitSize; idx++)
		{
			evaluation[idx] = 0;
		}
		
		PlainArithmeticCircuitEvaluator pace;
		pace.EvaluateArithmeticCircuit(circuit, lc, input, evaluation);
		
		std::cout << "Finished evaluating plain circuit..." << std::endl;
		
		PreprocessingBuilder *builder = new PreprocessingBuilder();
		
		std::vector<unsigned char> seedAlice = CryptoUtility::SampleByteArray(32);
		std::vector<unsigned char> seedBob   = CryptoUtility::SampleByteArray(32);
		
		std::cout << "Building circuit..." << std::endl;
		builder->BuildPreprocessing(seedAlice, seedBob, lc);
		
		std::cout << "Getting Alice's share..." << std::endl;
		
		auto aliceShare = AlicePreprocessingBuilder::Build(lc, seedAlice);
// 		std::cout << "Alice mask: ";
// 		for(int idx = 0; idx < lc->NumWires; idx++)
// 		{
// 			std::cout << aliceShare->maskShare[idx] << " ";
// 		}
// 		std::cout << std::endl;
		
		std::cout << "Getting Bob's share..." << std::endl;
		auto bobShare = BobPreprocessingBuilder::Build(lc, seedBob, builder->getBobCorrection());
		
// 		std::cout << "Bob mask: ";
// 		for(int idx = 0; idx < lc->NumWires; idx++)
// 		{
// 			std::cout << bobShare->maskShare[idx] << " ";
// 		}
// 		std::cout << std::endl;
		
		Communicator *c1, *c2, *c3, *c4;
		CommunicatorBuilder::BuildCommunicator(c1, c2, c3, c4);
		
		
		std::cout << "TestPreprocessing..." << std::endl;
		TestPreprocessing(aliceShare, bobShare, c1, c2, inputRange, circuit, lc);
	}

	void TestingPreprocessingBuilder::TestDistributedPreprocessing()
	{
		clock_t begin = clock();
		Communicator *c1, *c2, *c3, *c4;
		CommunicatorBuilder::BuildCommunicator(c1, c2, c3, c4);
		
		PreprocessingParty *p1 = new PreprocessingParty(c1, true);
		PreprocessingParty *p2 = new PreprocessingParty(c2, false);
		PreprocessingParty *p3 = new PreprocessingParty(c3, true);
		PreprocessingParty *p4 = new PreprocessingParty(c4, false);

		int circuitSize = 100000;
		int inputSize, outputSize;
		Range *inputRange, *sizeRange;
		ArithmeticCircuit *circuit;
		LayeredArithmeticCircuit *lc;
		
		CreateInputOutput(circuitSize, inputSize, outputSize, inputRange, sizeRange, circuit, lc);
		
		if(circuitSize < 20) std::cout << lc->ToString() << std::endl;
		
		std::cout << "p1 and p2 build preprocessing..." << std::endl;
		std::future<std::vector<int64_t> > t1 = std::async(std::launch::async, [&]{return p1->BuildPreprocessing(lc);});
		std::future<std::vector<int64_t> > t2 = std::async(std::launch::async, [&]{return p2->BuildPreprocessing(lc);});
		
		
		std::cout << "p3 and p4 receive preprocessing..." << std::endl;
		
		std::future<PreprocessingShare *> t3 = std::async(std::launch::async, [&]{return p3->ReceivePreprocessing(lc);});
		std::future<PreprocessingShare *> t4 = std::async(std::launch::async, [&]{return p4->ReceivePreprocessing(lc);});
		
		t1.wait();
		t2.wait();
		t3.wait();
		t4.wait();
		
		auto p1Mask = t1.get(); 
		auto p2Mask = t2.get();
		auto aliceShare = t3.get();
		auto bobShare = t4.get();
		
		clock_t end = clock();
		std::stringstream ss;
		ss << "BuildPreprocessing() elapsed time: " << (double)(end - begin)/CLOCKS_PER_SEC << "\n";
		std::cout << ss.str() << std::endl;
		
		begin = clock();
		
		std::cout << "Runging test preprocessing..." << std::endl;
		TestPreprocessing(aliceShare, bobShare, c3, c4, inputRange, circuit, lc);
		end = clock();
		
		ss << "Testing() elapsed time: " << (double)(end - begin)/CLOCKS_PER_SEC << "\n";
		std::cout << ss.str() << std::endl;
	}
	
	void TestingPreprocessingBuilder::TestPreprocessing(PreprocessingShare *aliceShare, PreprocessingShare *bobShare, Communicator *c1, Communicator *c2, Range *inputRange, ArithmeticCircuit *circuit, LayeredArithmeticCircuit *lc)
	{
		int circuitSize = lc->NumWires;
		
		Range *sizeRange = new Range(0, circuitSize);
		
		std::cout << "Get masks..." << std::endl;
		std::vector<int64_t> masks(circuitSize);
		for(int idx = 0; idx < circuitSize; idx++)
		{
			masks[idx] = aliceShare->maskShare[idx] + bobShare->maskShare[idx];
		}
		
		std::cout << "Get inputMasks..." << std::endl;
		std::vector<int64_t> inputMasks(circuit->InputLength);
		for(int idx = 0; idx < circuit->InputLength; idx++)
		{
			inputMasks[idx] = masks[idx];
		}
		
		unsigned seed =  std::chrono::system_clock::now().time_since_epoch().count();
		std::default_random_engine generator(seed);
		std::uniform_int_distribution<int> distribution(-3, 3);

		std::vector<int64_t> input;
		for(int idx = 0; idx < circuit->InputLength; idx++)
		{
			int64_t rng = ((distribution(generator)) << 17);
			input.push_back(rng);
		}
		
		std::vector<int64_t> maskedInput(circuit->InputLength);
		for(int idx = 0; idx < circuit->InputLength; idx++)
		{
			maskedInput[idx] = inputMasks[idx] + input[idx];
		}

		std::cout << "Plain evaluation..." << std::endl;
		std::vector<int64_t> evaluation(circuitSize);
		for(int idx = 0; idx < circuitSize; idx++)
		{
			evaluation[idx] = 0;
		}
		
		PlainArithmeticCircuitEvaluator pace;
		pace.EvaluateArithmeticCircuit(circuit, lc, input, evaluation);
		
// 		std::cout << "aliceEvaluation..." << std::endl;
		auto aliceEvaluation = new MaskedEvaluation(lc, aliceShare, c1);
		aliceEvaluation->AddInput(maskedInput, inputRange);
		aliceEvaluation->InputAdded();
		
// 		std::cout << "bobEvaluation..." << std::endl;
		auto bobEvaluation = new MaskedEvaluation(lc, bobShare, c2);		
		bobEvaluation->AddInput(maskedInput, inputRange);
		bobEvaluation->InputAdded();

		clock_t begin = clock();
		std::cout << "Alice Evaluation ..." << std::endl;
		std::future<void> t5 = std::async(std::launch::async, [&aliceEvaluation]{ aliceEvaluation->EvaluateCircuit(); });
		
		std::cout << "Bob Evaluation ..." << std::endl;
		std::future<void> t6 = std::async(std::launch::async, [&bobEvaluation]{ bobEvaluation->EvaluateCircuit(); });
			
		t5.wait();
		t6.wait();
		
		t5.get(); t6.get();

		std::cout << "Alice Decrypts ..." << std::endl;
		auto maskedEvaluationPlain1 = aliceEvaluation->Decrypt(masks, sizeRange);
		
		std::cout << "Bob Decryptsn ..." << std::endl;
		auto maskedEvaluationPlain2 = bobEvaluation->Decrypt(masks, sizeRange);
		
		clock_t end = clock();
	
		std::cout << "Elapsed time: " << (double)(end - begin)/CLOCKS_PER_SEC << std::endl;
		
		if(circuitSize < 20)
		{
			std::cout << "Plain evaluation: " << std::endl;
			for(int idx = 0; idx < circuitSize; idx++)
			{
				printf("%d %f\n", idx, evaluation[idx]/1048576.0);
			}		
			std::cout << "maskedEvaluationPlain1: " << std::endl;
			for(int idx = 0; idx < circuitSize; idx++)
			{
				printf("%d %f\n", idx, maskedEvaluationPlain1[idx]/1048576.0);
			}
			std::cout << "maskedEvaluationPlain2: " << std::endl;
			for(int idx = 0; idx < circuitSize; idx++)
			{
				printf("%d %f\n", idx, maskedEvaluationPlain2[idx]/1048576.0);
			}
		}
		std::set<int> diff;	
		std::set<int>::iterator it = diff.begin();
		
		for(int idx = 0; idx < circuitSize; idx++)
		{
	// 		printf("%d %f %f %f\n", idx, maskedEvaluationPlain[idx]/1048576.0, evaluation[idx]/1048576.0, (maskedEvaluationPlain[idx] - evaluation[idx])/1048576.0);
	// 		printf("%d ", (int)((maskedEvaluationPlain[idx] - evaluation[idx])/1048576.0));
			diff.insert(it, (int)((maskedEvaluationPlain1[idx] - evaluation[idx])/1048576.0));
			diff.insert(it, (int)((maskedEvaluationPlain2[idx] - evaluation[idx])/1048576.0));
		}
		
		std::cout << "Comparing output: " << diff.size();
		for(it = diff.begin(); it != diff.end(); ++it)
		{
			if(circuitSize < 20) std::cout << *it << " ";
		}
		
		std::cout << std::endl;
		
		std::stringstream ss;
		ss << "Layer difference: ";
		std::vector<int64_t> layer_diff(0);
		for(int ldx = 0; ldx < lc->Depth; ldx++)
		{
			int64_t maxdiff = 0;
			
			std::vector<IGate *> lgates;
			
			/// Get all the gates of the ldx-th layer
			for (auto &g : (lc->operator[](ldx))->AddGates) {lgates.push_back(g);}
			for (auto &g : (lc->operator[](ldx))->SubGates) {lgates.push_back(g);}
			for (auto &g : (lc->operator[](ldx))->MulGates) {lgates.push_back(g);}
			for (auto &g : (lc->operator[](ldx))->DivGates) {lgates.push_back(g);}
			for (auto &g : (lc->operator[](ldx))->CAddGates) {lgates.push_back(g);}
			for (auto &g : (lc->operator[](ldx))->CMulGates) {lgates.push_back(g);}
			
			/// Evaluate each gate
			for (int i = 0; i < lgates.size();i++)
			{
				IGate *g = lgates[i];
				
				if (dynamic_cast<BinaryGate*>(g) != nullptr)
				{
					BinaryGate *bg = (BinaryGate *)g;
					int outputWire = bg->OutputWire;
					int64_t temp = abs(maskedEvaluationPlain1[outputWire] - maskedEvaluationPlain2[outputWire]);
					maxdiff = maxdiff >= temp ? maxdiff : temp;
				}
				else if (dynamic_cast<ConstantGate *>(g) != nullptr)
				{
					ConstantGate *ug = (ConstantGate *)g;
					int outputWire = ug->OutputWire;
					int64_t temp = abs(maskedEvaluationPlain1[outputWire] - maskedEvaluationPlain2[outputWire]);
					maxdiff = maxdiff >= temp ? maxdiff : temp;
				}
			}
			
			ss << maxdiff << " ";
			layer_diff.push_back(maxdiff);
		}
// 		printf("\n");
		std::cout << ss.str() << std::endl;
		std::cout << "Done and terminate&" << std::endl;
		
		std::cout << "End testing" << std::endl;
	}*/
}
