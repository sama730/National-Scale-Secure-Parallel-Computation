#include "TestingMaskedEvaluation.h"
#include "../Circuit/GradientDescendCircuitBuilder.h"

using namespace Circuit;
using namespace Utility;
using namespace TwoPartyMaskedEvaluation;

void TestingMaskedEvaluation::Test2()
{
	std::vector<int> itemsPerUser; 
	for(int idx = 0; idx < 1; idx++)
	{
		itemsPerUser.push_back(2);
	}
	
	GradientDescendCircuitBuilder::BuildMultipleUsersByLayers(itemsPerUser, 0.25, 0.5, "../MaskedEvaluationTest/Circuits/MultipleUsersByLayers.txt");
	
	Timer t;
	
	ArithmeticCircuitBuilder circuitBuilder;
	LayeredArithmeticCircuitBuilder layerBuilder;
	
	ArithmeticCircuit *circuit = circuitBuilder.CreateCircuit("./Circuits/MultipleUsersByLayers.txt");
	LayeredArithmeticCircuit *lc = layerBuilder.CreateLayeredCircuit(circuit);
	
	int inputSize = circuit->InputLength;
	int outputSize = circuit->OutputLength;;
	int circuitSize = circuit->NumWires;
	
	if(circuitSize < 50)
	{
		std::cout << lc->ToString() << std::endl;
	}
	
	t.Tick("Time to create circuit");
	
	int count = 0;
	
	std::vector<std::vector<int64_t> > input = TestUtility::GenerateInput(itemsPerUser, circuit->InputLength);
	std::vector<std::vector<int64_t> > evaluation(circuitSize);
	
	PlainArithmeticCircuitEvaluator pace;
	pace.EvaluateArithmeticCircuit(circuit, lc, input, itemsPerUser, evaluation);
	
	t.Tick("Plain evaluation");
	
	if(circuitSize < 50)
	{
		TestUtility::PrintVector(evaluation, "Evaluation", 1048576.0);
	}
	
	PreprocessingShareStorage *shareStorage = SimplePreprocessingBuilder::BuildPreprocessing(lc, itemsPerUser);	
	
	t.Tick("PreprocessingShareStorage");
	
	std::vector<std::vector<int64_t> > maskedInput(inputSize);
// 	std::cout << "Masks before evaluation:" << std::endl;
	for(int idx = 0; idx < inputSize; idx++)
	{
		maskedInput[idx].resize(input[idx].size());
		
		for(int kdx = 0; kdx < input[idx].size(); kdx++)
		{
			maskedInput[idx][kdx] = (shareStorage->masks[shareStorage->maskIndex[idx] + kdx] + input[idx][kdx]);
		}
	}
	
	t.Tick("Compute masked input");
	
	Communicator *c1, *c2, *c3, *c4;
	CommunicatorBuilder::BuildCommunicator(c1, c2, c3, c4);
	
	MaskedEvaluation *aliceEvaluation = new MaskedEvaluation(lc, shareStorage->aliceShare, &(shareStorage->maskIndex), c1);
	MaskedEvaluation *bobEvaluation = new MaskedEvaluation(lc, shareStorage->bobShare, &(shareStorage->maskIndex), c2);
	
	auto inputRange = new Range(0, inputSize);
	auto sizeRange = new Range(0, circuitSize);
			      
	aliceEvaluation->AddInput(maskedInput, inputRange);
	bobEvaluation->AddInput(maskedInput, inputRange);
	
	t.Tick("Add input time");
	
	std::cout << "Alice Evaluation ..." << std::endl;
	std::future<void> f1 = std::async(std::launch::async, [&aliceEvaluation]{ aliceEvaluation->EvaluateCircuit(); });
	
	std::cout << "Bob Evaluation ..." << std::endl;
	std::future<void> f2 = std::async(std::launch::async, [&bobEvaluation]{ bobEvaluation->EvaluateCircuit(); });
	
	f1.wait();
	f2.wait();
	
	f1.get(); f2.get();
	
	t.Tick("Evaluation time");

	std::cout << "Decrypting ..." << std::endl;
	auto maskedEvaluationPlain1 = aliceEvaluation->Decrypt(shareStorage->masks, sizeRange);
	auto maskedEvaluationPlain2 = bobEvaluation->Decrypt(shareStorage->masks, sizeRange);
	
	t.Tick("Decrypt time");
	
	if(circuitSize < 50)
	{
		TestUtility::PrintVector(aliceEvaluation->maskedEvaluation, "Alice Evaluation", 1048576.0);
		TestUtility::PrintVector(bobEvaluation->maskedEvaluation, "Bob Evaluation", 1048576.0);
	}
	
	std::cout << "maskedEvaluationPlain1: " << std::endl;
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
	
	std::cout << std::endl;
	
	printf("\n");
	
	std::cout << "Done and terminate&" << std::endl;
	
}

/*
void Test1()
{
	clock_t begin = clock();
	ArithmeticCircuit *circuit;
	LayeredArithmeticCircuit *lc;
	
	int inputSize;
	int outputSize;
	int circuitSize;

	inputSize = 8;
	outputSize = 4;
	circuitSize = 18;
	
	std::cout << "Generate a random circuit" << std::endl;
	RandomArithmeticCircuitBuilder::Sample(inputSize, outputSize, circuitSize, circuit, lc);
	std::cout << "Circuit params after: " << std::endl;
	std::cout << circuit->InputLength << " " << circuit->OutputLength << " " << circuit->NumGates << " " << circuit->NumWires << std::endl;
	
	if(circuitSize < 20) std::cout << lc->ToString() << std::endl;
	
	unsigned seed =  std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	std::uniform_int_distribution<int> distribution(-3, 3);

	std::vector<int64_t> input;
	for(int idx = 0; idx < circuit->InputLength; idx++)
	{
		int64_t rng = ((distribution(generator)) << 17);
		
		input.push_back(rng);
	}
	
	printf("\n");
	
	std::vector<int64_t> evaluation(circuitSize);
	for(int idx = 0; idx < circuitSize; idx++)
	{
		evaluation[idx] = 0;
	}
	
	PlainArithmeticCircuitEvaluator pace;
	pace.EvaluateArithmeticCircuit(circuit, lc, input, evaluation);
	
	std::cout << "Plain evaluation..." << std::endl;
	for(int idx = 0; idx < inputSize + 10; idx++)
	{
		printf("%d : %f\n", idx, evaluation[idx]/1048576.0);
	}
	
	printf("\n");
	
	auto shareStorage = SimplePreprocessingBuilder::BuildPreprocessing(lc);
	
	std::vector<int64_t> maskedInput(inputSize);
// 	std::cout << "Masks before evaluation:" << std::endl;
	for(int idx = 0; idx < inputSize; idx++)
	{
// 		std::cout << idx << "\t" << shareStorage->masks[idx] << std::endl;
		maskedInput[idx] = (shareStorage->masks[idx] + input[idx]);
	}
	
	Communicator *c1, *c2, *c3, *c4;
	CommunicatorBuilder::BuildCommunicator(c1, c2, c3, c4);
	
	MaskedEvaluation *aliceEvaluation = new MaskedEvaluation(lc, shareStorage->aliceShare, c1);
	MaskedEvaluation *bobEvaluation = new MaskedEvaluation(lc, shareStorage->bobShare, c2);
	
	auto inputRange = new Range(0, inputSize);
	auto sizeRange = new Range(0, circuitSize);
			      
	aliceEvaluation->AddInput(maskedInput, inputRange);
	bobEvaluation->AddInput(maskedInput, inputRange);
	
	aliceEvaluation->InputAdded();
	bobEvaluation->InputAdded();
	
	std::cout << "Alice Evaluation ..." << std::endl;
	std::future<void> f1 = std::async(std::launch::async, [&aliceEvaluation]{ aliceEvaluation->EvaluateCircuit(); });
	
	std::cout << "Bob Evaluation ..." << std::endl;
	std::future<void> f2 = std::async(std::launch::async, [&bobEvaluation]{ bobEvaluation->EvaluateCircuit(); });
		
	f1.wait();
	f2.wait();
	
	f1.get(); f2.get();
		
	std::cout << "Decrypting ..." << std::endl;
	auto maskedEvaluationPlain1 = aliceEvaluation->Decrypt(shareStorage->masks, sizeRange);
	auto maskedEvaluationPlain2 = bobEvaluation->Decrypt(shareStorage->masks, sizeRange);
	
	std::cout << "Alice Result ..." << std::endl;
	for(int idx = 0; idx < inputSize + 10; idx++)
	{
		printf("%d %f\n", idx, aliceEvaluation->maskedEvaluation[idx]/1048576.0);
	}
	
	clock_t end = clock();
	
	std::cout << "Elapsed time: " << (double)(end - begin)/CLOCKS_PER_SEC << std::endl;
	
	std::cout << "maskedEvaluationPlain1: " << std::endl;
	std::set<int> diff;	
	std::set<int>::iterator it = diff.begin();
	
	for(int idx = 0; idx < circuitSize; idx++)
	{
// 		printf("%d %f %f %f\n", idx, maskedEvaluationPlain[idx]/1048576.0, evaluation[idx]/1048576.0, (maskedEvaluationPlain[idx] - evaluation[idx])/1048576.0);
// 		printf("%d ", (int)((maskedEvaluationPlain[idx] - evaluation[idx])/1048576.0));
		diff.insert(it, (int)((maskedEvaluationPlain1[idx] - evaluation[idx])));
		diff.insert(it, (int)((maskedEvaluationPlain2[idx] - evaluation[idx])/1048576.0));
	}
	
	std::cout << "Comparing output: ";
	for(it = diff.begin(); it != diff.end(); ++it)
	{
		if(circuitSize < 20) std::cout << *it << " ";
		if (abs(*it) > 0) std::cout << *it << " ";
	}
	
	std::cout << std::endl;
	
	printf("\n");
	
	std::cout << "Done and terminate&" << std::endl;
	
	std::cout << "End testing" << std::endl;
}*/