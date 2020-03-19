#include "TestingEmulatedMaskedEvaluation.h"
#include "../Utility/Timer.h"
#include "../Circuit/GradientDescendCircuitBuilder.h"
/*
void TestingEmulatedMaskedEvaluation::Test1()
{
	ArithmeticCircuit *circuit;
	LayeredArithmeticCircuit *lc;
	
	int inputSize;
	int outputSize;
	int circuitSize;

	inputSize = 500;
	outputSize = 100;
	circuitSize = 1000000;
	
	std::cout << "Generate a random circuit" << std::endl;
	RandomArithmeticCircuitBuilder::Sample(inputSize, outputSize, circuitSize, circuit, lc);
	std::cout << "Circuit params after: " << std::endl;
	std::cout << circuit->InputLength << " " << circuit->OutputLength << " " << circuit->NumGates << " " << circuit->NumWires << std::endl;
	
// 	std::cout << circuit->ToString() << std::endl;
	
// 	std::cout << lc->ToString() << std::endl;
	
	unsigned seed =  std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	std::uniform_int_distribution<int> distribution(-1, 1);

	std::vector<int64_t> input;
	for(int idx = 0; idx < circuit->InputLength; idx++)
	{
		int64_t rng = ((distribution(generator)) << 17);
		
// 		printf("rng %d = %f\n", idx, rng/1048576.0);
		
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
	
	for(int idx = 0; idx < circuitSize; idx++)
	{
// 		printf("%d : %f\n", idx, evaluation[idx]/1048576.0);
	}
	
	printf("\n");
	
	auto shareStorage = SimplePreprocessingBuilder::BuildPreprocessing(lc);
	
// 	Range tempVar(0, inputSize);
	
	
	std::vector<int64_t> maskedInput(inputSize);
// 	std::cout << "Masks before evaluation:" << std::endl;
	for(int idx = 0; idx < inputSize; idx++)
	{
// 		std::cout << idx << "\t" << shareStorage->masks[idx] << std::endl;
		maskedInput[idx] = (shareStorage->masks[idx] + input[idx]);
	}
	
// 	auto inputMasks = shareStorage->masks.CopySubsequence(&tempVar);
// 	auto maskedInput = inputMasks->ImmutableXor(input);

	auto eme = new EmulatedMaskedEvaluation(shareStorage->aliceShare, shareStorage->bobShare, lc);
	
	Range tempVar2(0, inputSize);
	eme->AddInput(maskedInput, &tempVar2);
	
	clock_t begin = clock();
	
	eme->EvaluateCircuit();
	
	Range tempVar3(0, circuitSize);
	auto maskedEvaluationPlain = eme->Decrypt(shareStorage->masks, &tempVar3);

	clock_t end = clock();
	
	std::cout << "Elapsed time: " << (double)(end - begin)/CLOCKS_PER_SEC << std::endl;
	
// 	Assert::IsTrue(plainEvaluation->AreEqual(maskedEvaluationPlain));
	
// 	for(int idx = 0; idx < circuitSize; idx++)
// 	{
// // 		printf("%d %f\n", idx, maskedEvaluationPlain[idx]/1048576.0);
// 	}
	
	std::set<int> diff;	
	std::set<int>::iterator it = diff.begin();
	
	for(int idx = 0; idx < circuitSize; idx++)
	{
// 		printf("%d %f %f %f\n", idx, maskedEvaluationPlain[idx]/1048576.0, evaluation[idx]/1048576.0, (maskedEvaluationPlain[idx] - evaluation[idx])/1048576.0);
// 		printf("%d ", (int)((maskedEvaluationPlain[idx] - evaluation[idx])/1048576.0));
		diff.insert(it, (int)((maskedEvaluationPlain[idx] - evaluation[idx])/1048576.0));
	}
	
	std::cout << "Comparing output: ";
	for(it = diff.begin(); it != diff.end(); ++it)
	{
		if(circuitSize < 20) std::cout << *it << " ";
	}
	
	std::cout << std::endl;
	
	printf("\n");
	
	std::cout << "Done and terminate&" << std::endl;
	
	std::cout << "End testing" << std::endl;
}*/

void TestingEmulatedMaskedEvaluation::Test2()
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
// 	auto inputMasks = shareStorage->masks.CopySubsequence(&tempVar);
// 	auto maskedInput = inputMasks->ImmutableXor(input);

	std::cout << "Evaluate Circuit" << std::endl;
	auto eme = new EmulatedMaskedEvaluation(shareStorage->aliceShare, shareStorage->bobShare, &(shareStorage->maskIndex), lc);
	
	Range tempVar2(0, inputSize);
	eme->AddInput(std::move(maskedInput), &tempVar2);
	
	t.Tick("After adding input");
	
	eme->EvaluateCircuit();
	
	std::cout << "Decryption....." << std::endl;
	
	Range tempVar3(0, circuitSize);
	std::vector<std::vector<int64_t> > maskedEvaluationPlain = eme->Decrypt(shareStorage->masks, &tempVar3);

// 	PrintUtility::PrintVector(maskedEvaluationPlain, "maskedEvaluationPlain", 1048576.0);
	
	t.Tick("Evaluation elapsed time");
	
// 	Assert::IsTrue(plainEvaluation->AreEqual(maskedEvaluationPlain));
	
// 	for(int idx = 0; idx < circuitSize; idx++)
// 	{
// // 		printf("%d %f\n", idx, maskedEvaluationPlain[idx]/1048576.0);
// 	}
	
	std::set<int> diff;	
	std::set<int>::iterator it = diff.begin();
	
	for(int idx = 0; idx < circuitSize; idx++)
	{
		for(int kdx = 0; kdx < evaluation[idx].size(); kdx++)
		{
			diff.insert(it, (int)((maskedEvaluationPlain[idx][kdx] - evaluation[idx][kdx])/1048576.0));
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
	
	std::cout << "End testing" << std::endl;
}
