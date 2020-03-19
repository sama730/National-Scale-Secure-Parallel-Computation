#include <stdio.h>
#include <string>
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
#include "../Circuit/GradientDescendCircuitBuilder.h"

#include "../TwoPartyMaskedEvaluation/PreprocessingBuilder.h"
#include "../TwoPartyMaskedEvaluation/SimplePreprocessingBuilder.h"
#include "../TwoPartyMaskedEvaluation/MaskedEvaluation.h"

#include "../Utility/CryptoUtility.h"
#include "../Utility/Maybe.h"
#include "../Utility/Range.h"
#include "../Utility/Timer.h"
#include "../Utility/Channels.h"
#include "../Utility/Communicator.h"
#include "../Utility/CommunicatorBuilder.h"

#include "../CrossCheck/PreprocessingParty.h"
#include "../CrossCheck/Player.h"


#include "TestingPlayers.h"
#include "TestingPreprocessingBuilder.h"

using namespace Circuit;
using namespace Utility;
using namespace CrossCheck;
using namespace TwoPartyMaskedEvaluation;

namespace MaskedEvaluationTest
{
	void TestPlayers::TestEvaluation1()
	{
		Timer t;
		
		std::vector<int> itemsPerUser; 
		for(int idx = 0; idx < 1; idx++)
		{
			itemsPerUser.push_back(1);
		}
		
		int numUsers = itemsPerUser.size();
		int sumNumItems = 0;
		for(int idx = 0; idx < itemsPerUser.size(); idx++)
		{
			sumNumItems += itemsPerUser[idx];
		}
		
		int numMasks = 6*DIM*numUsers + (2*DIM + 4)*sumNumItems;
		int numWires = 6*numUsers + 6*sumNumItems;
		
		std::vector<int> subsetItemsPerUser; 
		subsetItemsPerUser.insert(subsetItemsPerUser.end(), itemsPerUser.begin(), itemsPerUser.begin() + itemsPerUser.size()/4);
		
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

// 		if(circuitSize < 50) std::cout << lc->ToString() << std::endl;
		
		Range *outputRange = new Range(0, numWires);
		
		Communicator *c1, *c2, *c3, *c4;
		CommunicatorBuilder::BuildCommunicator(c1, c2, c3, c4);
		
		std::vector<std::vector<int64_t> > inputs = TestUtility::GenerateInput(itemsPerUser, inputSize);
		
		inputRange = new Range(0, inputSize);
		
		std::vector<Player*> players;
		Player *p1 = new PlayerOne(c1, inputRange);
		Player *p2 = new PlayerTwo(c2, inputRange);
		Player *p3 = new PlayerThree(c3, inputRange);
		Player *p4 = new PlayerFour(c4, inputRange);
		
		players.push_back(p1);
		players.push_back(p2);
		players.push_back(p3);
		players.push_back(p4);

		if(circuitSize < 50) 
		{
		}

		std::vector<std::vector<int64_t> > evaluation(circuitSize);
		
		PlainArithmeticCircuitEvaluator pace;
		pace.EvaluateArithmeticCircuit(circuit, lc, inputs, itemsPerUser, evaluation);
		
		t.Tick("Plain evaluation");
		
		if(circuitSize < 50)
		{
			TestUtility::PrintVector(evaluation, "Evaluation", 1048576.0);
		}
		
		std::future<std::vector<std::vector<int64_t> > > t0 = std::async(std::launch::async, [&]{return players[0]->Run(lc, inputs, outputRange, itemsPerUser);});
		std::future<std::vector<std::vector<int64_t> > > t1 = std::async(std::launch::async, [&]{return players[1]->Run(lc, inputs, outputRange, itemsPerUser);});
		std::future<std::vector<std::vector<int64_t> > > t2 = std::async(std::launch::async, [&]{return players[2]->Run(lc, inputs, outputRange, itemsPerUser);});
		std::future<std::vector<std::vector<int64_t> > > t3 = std::async(std::launch::async, [&]{return players[3]->Run(lc, inputs, outputRange, itemsPerUser);});
		
		
		t0.wait(); t1.wait(); t2.wait(); t3.wait();
		
		std::vector<std::vector<int64_t> >  out0 = t0.get();
		std::vector<std::vector<int64_t> >  out1 = t1.get();
		std::vector<std::vector<int64_t> >  out2 = t2.get();
		std::vector<std::vector<int64_t> >  out3 = t3.get();
		
		std::vector<std::vector<std::vector<int64_t> > > outputs;
		outputs.push_back(out0);
		outputs.push_back(out1);
		outputs.push_back(out2);
		outputs.push_back(out3);
		
		TestUtility::PrintVector(out0, "output 0", (1<<20));
// 		std::vector<std::vector<int64_t> > diff(out0.size());
// 		for(int idx = 0; idx < out0.size(); idx++)
// 		{
// 			diff[idx].resize(out0[idx].size());
// 			for(int kdx = 0; kdx < out0[idx].size(); kdx++)
// 			{
// 				diff[idx][kdx] = out0[idx][kdx] - out2[idx][kdx];
// 			}
// 		}
				
// 		TestUtility::PrintVector(diff, "Output difference between parties", 1);
	}

// 	void TestPlayers::TestEvaluation2()
// 	{
// 		ArithmeticCircuit *circuit;
// 		LayeredArithmeticCircuit *lc;
// 		int inputSize;
// 		int outputSize;
// 		int circuitSize;
// 
// 		inputSize = 8;
// 		outputSize = 1;
// 		circuitSize = 16;
// 		
// 		int increment = inputSize/2;
// 		
// 		RandomArithmeticCircuitBuilder::Sample(inputSize, outputSize, circuitSize, circuit, lc);
// 
// 		if(circuitSize < 20) std::cout << lc->ToString() << std::endl;
// 		
// 		Range *outputRange = new Range(0, circuitSize);
// 		
// 		Communicator *c1, *c2, *c3, *c4;
// 		CommunicatorBuilder::BuildCommunicator(c1, c2, c3, c4);
// 
// 		auto inputRanges = std::vector<IRange *>();
// 		IRange *r1 = new Range(0, inputSize/2);
// 		IRange *r2 = new Range(inputSize/2, inputSize/2);
// 		IRange *r3 = new EmptyRange();
// 		IRange *r4 = new EmptyRange();
// 		
// 		inputRanges.push_back(r1); 
// 		inputRanges.push_back(r2); 
// 		inputRanges.push_back(r3); 
// 		inputRanges.push_back(r4); 
// 
// 		std::vector<Player*> players;
// 		Player *p1 = new PlayerOne(c1, inputRanges);
// 		Player *p2 = new PlayerTwo(c2, inputRanges);
// 		Player *p3 = new PlayerThree(c3, inputRanges);
// 		Player *p4 = new PlayerFour(c4, inputRanges);
// 		
// 		players.push_back(p1);
// 		players.push_back(p2);
// 		players.push_back(p3);
// 		players.push_back(p4);
// 
// 
// 		auto inputs = std::vector<IMaybe<std::vector<int64_t> > *>();
// 		IMaybe<std::vector<int64_t> > *input1 = new Just<std::vector<int64_t> >(CryptoUtility::SampleInt64Array(inputSize / 2));
// 		IMaybe<std::vector<int64_t> > *input2 = new Just<std::vector<int64_t> >(CryptoUtility::SampleInt64Array(inputSize / 2));
// 		IMaybe<std::vector<int64_t> > *input3 = new Nothing<std::vector<int64_t> >();
// 		IMaybe<std::vector<int64_t> > *input4 = new Nothing<std::vector<int64_t> >();
// 		
// 		if(circuitSize < 20) 
// 		{
// 			std::cout << "input1: "; input1->print();
// 			std::cout << "input2: "; input2->print();
// 		}
// 		
// 		inputs.push_back(input1);
// 		inputs.push_back(input2);
// 		inputs.push_back(input3);
// 		inputs.push_back(input4);
// 
// 		std::vector<int64_t> plainInput(inputSize);
// 		std::vector<int64_t> temp0 = inputs[0]->getValue();
// 		std::vector<int64_t> temp1 = inputs[1]->getValue();
// 		
// 		for(int idx = 0; idx < inputSize/2; idx++)
// 		{
// 			plainInput[idx] = temp0[idx];
// 			plainInput[inputSize/2 + idx] = temp1[idx];
// 		}
// 
// 		std::vector<int64_t> plainOutput(circuitSize);
// 		PlainArithmeticCircuitEvaluator plainEval;
// 		plainEval.EvaluateArithmeticCircuit(circuit, lc, plainInput, plainOutput);
// 		
// 		if(circuitSize < 20)
// 		{
// 			std::stringstream plain;
// 			plain << "PlainOutput: ";
// 			for(int idx = 0; idx < plainOutput.size(); idx++)
// 			{
// 				plain << plainOutput[idx]/1048576.0 << " ";
// 			}
// 			
// 			std::cout << plain.str() << std::endl;
// 		}
// 		std::future<std::vector<int64_t> > t0 = std::async(std::launch::async, [&]{return players[0]->Run(lc, inputs[0], outputRange);});
// 		std::future<std::vector<int64_t> > t1 = std::async(std::launch::async, [&]{return players[1]->Run(lc, inputs[1], outputRange);});
// 		std::future<std::vector<int64_t> > t2 = std::async(std::launch::async, [&]{return players[2]->Run(lc, inputs[2], outputRange);});
// 		std::future<std::vector<int64_t> > t3 = std::async(std::launch::async, [&]{return players[3]->Run(lc, inputs[3], outputRange);});
// 		
// 		
// 		t0.wait(); t1.wait(); t2.wait(); t3.wait();
// 		
// 		std::vector<int64_t> out0 = t0.get();
// 		std::vector<int64_t> out1 = t1.get();
// 		std::vector<int64_t> out2 = t2.get();
// 		std::vector<int64_t> out3 = t3.get();
// 		
// 		if(circuitSize < 20) 
// 		{
// 			std::cout << "out0: ";
// 			for(int idx = 0; idx < out0.size(); idx++)
// 			{
// 				std::cout << out0[idx]/1048576.0 << " ";
// 			}
// 			std::cout << std::endl;
// 			
// 			std::cout << "out1: ";
// 			for(int idx = 0; idx < out1.size(); idx++)
// 			{
// 				std::cout << out1[idx]/1048576.0 << " ";
// 			}
// 			std::cout << std::endl;
// 			
// 			
// 			std::cout << "out2: ";
// 			for(int idx = 0; idx < out2.size(); idx++)
// 			{
// 				std::cout << out2[idx]/1048576.0 << " ";
// 			}
// 			std::cout << std::endl;
// 			
// 			
// 			std::cout << "out3: ";
// 			for(int idx = 0; idx < out3.size(); idx++)
// 			{
// 				std::cout << out3[idx]/1048576.0 << " ";
// 			}
// 			std::cout << std::endl;
// 		}
// 		std::vector<std::vector<int64_t> > outputs;
// 		outputs.push_back(out0);
// 		outputs.push_back(out1);
// 		outputs.push_back(out2);
// 		outputs.push_back(out3);
// 		
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
// 					int64_t temp = abs(out0[outputWire] - out2[outputWire]);
// 					maxdiff = maxdiff >= temp ? maxdiff : out0[outputWire] - out2[outputWire];
// 				}
// 				else if (dynamic_cast<ConstantGate *>(g) != nullptr)
// 				{
// 					ConstantGate *ug = (ConstantGate *)g;
// 					int outputWire = ug->OutputWire;
// 					int64_t temp = abs(out0[outputWire] - out2[outputWire]);
// 					maxdiff = maxdiff >= temp ? maxdiff : out0[outputWire] - out2[outputWire];
// 				}
// 			}
// 			
// 			ss << maxdiff << " ";
// 			layer_diff.push_back(maxdiff);
// 		}
// // 		printf("\n");
// 		std::cout << ss.str() << std::endl;
// 	}
}
