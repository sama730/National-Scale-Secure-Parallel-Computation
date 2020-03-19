#include "TestingEvaluation.h"

using namespace Utility;
using namespace Circuit;

namespace MaskedEvaluationTest
{

	void TestingEvaluation::TestLayeredCircuitBuilder()
	{
		for (int i = 0; i < 10; i++)
		{
			int inputSize = 20;
			int outputSize = 21;
			int circuitSize = 42;

//C# TO C++ CONVERTER NOTE: The following 'decomposition declaration' requires C++17:
//ORIGINAL LINE: var(circuit, lc) = RandomCircuitBuilder.Sample(inputSize,outputSize,circuitSize);
			auto [circuit, lc] = RandomCircuitBuilder::Sample(inputSize,outputSize,circuitSize);

			std::function<int(BooleanLayer*)> f = [] (BooleanLayer l)
			{
				return l::AndGates->Count + l::XorGates->Count + l::NotGates->Count;
			};

			Assert::IsTrue(Enumerable::Range(0, lc::Depth)->Select([&] (void *x)
			{
				f(lc[x]);
			}).Sum() == circuit::NumGates);
		}
	}

	void TestingEvaluation::TestEmulatedEvaluation()
	{
		for (int i = 0; i < 10; i++)
		{
			for (int circuitSize : Enumerable::Range(256, 256))
			{
				int inputSize = 128;
				int outputSize = 128;
//C# TO C++ CONVERTER NOTE: The following 'decomposition declaration' requires C++17:
//ORIGINAL LINE: var(circuit, lc) = RandomCircuitBuilder.Sample(inputSize, outputSize, circuitSize);
				auto [circuit, lc] = RandomCircuitBuilder::Sample(inputSize, outputSize, circuitSize);

				auto input = BitArrayUtility::SampleRandomBitArray(inputSize);

				auto pbce = new PlainBooleanCircuitEvaluator();
				auto plainEvaluation = pbce->EvaluateBooleanCircuit(circuit, input);

				auto shareStorage = SimplePreprocessingBuilder::BuildPreprocessing(lc);
				Range tempVar(0, inputSize);
				auto inputMasks = shareStorage->masks.CopySubsequence(&tempVar);
				auto maskedInput = inputMasks->ImmutableXor(input);

				auto eme = new EmulatedMaskedEvaluation(shareStorage->aliceShare, shareStorage->bobShare, lc);
				Range tempVar2(0, inputSize);
				eme->AddInput(maskedInput, &tempVar2);
				eme->EvaluateCircuit();
				Range tempVar3(0, circuitSize);
				auto maskedEvaluationPlain = eme->Decrypt(shareStorage->masks, &tempVar3);

				Assert::IsTrue(plainEvaluation->AreEqual(maskedEvaluationPlain));
			}
		}
	}

//C# TO C++ CONVERTER TODO TASK: There is no equivalent in C++ to the 'async' keyword:
//ORIGINAL LINE: [TestMethod] public async Task TestMaskedEvaluation()
	Task *TestingEvaluation::TestMaskedEvaluation()
	{
		for (int i = 0; i < 10; i++)
		{
			for (int circuitSize : Enumerable::Range(256, 128))
			{
				int inputSize = 128;
				int outputSize = 128;
				auto inputRange = new Range(0, 128);
				auto sizeRange = new Range(0, circuitSize);
				
				auto [circuit, lc] = RandomCircuitBuilder::Sample(inputSize, outputSize, circuitSize);

				auto input = BitArrayUtility::SampleRandomBitArray(inputSize);

				auto pbce = new PlainBooleanCircuitEvaluator();
				auto plainEvaluation = pbce->EvaluateBooleanCircuit(circuit, input);

				auto shareStorage = SimplePreprocessingBuilder::BuildPreprocessing(lc);
				auto inputMasks = shareStorage->masks.CopySubsequence(inputRange);
				auto maskedInput = inputMasks->ImmutableXor(input);

				auto [c1, c2, c3, c4] = CommunicatorBuilder::BuildCommunicator();
				auto aliceEvaluation = new MaskedEvaluation(lc, shareStorage->aliceShare, c1);
				auto bobEvaluation = new MaskedEvaluation(lc, shareStorage->bobShare, c2);
				aliceEvaluation->AddInput(maskedInput, inputRange);
				bobEvaluation->AddInput(maskedInput, inputRange);
				aliceEvaluation->InputAdded();
				bobEvaluation->InputAdded();
				Task *t1 = Task::Run([&] ()
				{
					aliceEvaluation->EvaluateCircuit();
				});
				Task *t2 = Task::Run([&] ()
				{
					bobEvaluation->EvaluateCircuit();
				});
				await *t1;
				await *t2;

				auto maskedEvaluationPlain1 = aliceEvaluation->Decrypt(shareStorage->masks, sizeRange);
				auto maskedEvaluationPlain2 = bobEvaluation->Decrypt(shareStorage->masks, sizeRange);
				Assert::IsTrue(plainEvaluation->AreEqual(maskedEvaluationPlain1));
				Assert::IsTrue(plainEvaluation->AreEqual(maskedEvaluationPlain2));
			}
		}
	}
}
