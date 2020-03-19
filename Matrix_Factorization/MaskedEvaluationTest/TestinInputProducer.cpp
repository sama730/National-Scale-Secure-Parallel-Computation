//========================================================================
// This conversion was produced by the Free Edition of
// C# to C++ Converter courtesy of Tangible Software Solutions.
// Order the Premium Edition at https://www.tangiblesoftwaresolutions.com
//========================================================================

#include "TestinInputProducer.h"

using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace Circuit;
using namespace Utility;
using namespace TwoPartyMaskedEvaluation;
using namespace CrossCheck;

namespace MaskedEvaluationTest
{

//C# TO C++ CONVERTER TODO TASK: There is no equivalent in C++ to the 'async' keyword:
//ORIGINAL LINE: [TestMethod] public async Task TestProductionWithIRange()
	Task *TestingInputProducer::TestProductionWithIRange()
	{
		int inputSize = 128;
		int outputSize = 128;
		int circuitSize = 40000;
		
//C# TO C++ CONVERTER NOTE: The following 'decomposition declaration' requires C++17:
//ORIGINAL LINE: var(circuit, lc) = RandomCircuitBuilder.Sample(inputSize, outputSize, circuitSize);
		auto [circuit, lc] = RandomCircuitBuilder::Sample(inputSize, outputSize, circuitSize);
//C# TO C++ CONVERTER NOTE: The following 'decomposition declaration' requires C++17:
//ORIGINAL LINE: var(c1, c2, c3, c4) = CommunicatorBuilder.BuildCommunicator();
		auto [c1, c2, c3, c4] = CommunicatorBuilder::BuildCommunicator();

		int inputPerPlayer = inputSize / 2;
		std::function<Range*(int)> f = [] (int x)
		{
			return new Range(x, inputPerPlayer);
		};
		std::vector<IRange*> ranges = {f(0), f(inputPerPlayer), new EmptyRange(), new EmptyRange()};

		BitArray *input = BitArrayUtility::SampleRandomBitArray(inputSize);

		std::vector<IMaybe<BitArray*>*> dividedInputs = std::vector<IMaybe<BitArray*>*>()
		{
			new Just<BitArray*>(input->CopySubsequence(dynamic_cast<Range*>(ranges[0]))),
			new Just<BitArray*>(input->CopySubsequence(dynamic_cast<Range*>(ranges[1]))),
			new Nothing<BitArray*>(),
			new Nothing<BitArray*>()
		};

		auto shareStorage1 = SimplePreprocessingBuilder::BuildPreprocessing(lc);
		auto shareStorage2 = SimplePreprocessingBuilder::BuildPreprocessing(lc);

		std::function<MaskedEvaluation*(PreprocessingShare*, Communicator*)> g;
		g = [] (share, communicator)
		{
			return new MaskedEvaluation(lc, share, communicator);
		};

		auto evaluations = std::vector<MaskedEvaluation*> {g(shareStorage2->aliceShare, c1), g(shareStorage2->bobShare, c2), g(shareStorage1->aliceShare, c3), g(shareStorage1->bobShare, c4)};

		std::function<InputProducer*(Communicator*, BitArray*, MaskedEvaluation*, PreprocessingShare*)> h;
		h = [] (c, mask, eval, share)
		{
			return new InputProducer(c, mask, eval, share);
		};

		auto producers = std::vector<InputProducer*> {h(c1, shareStorage1->masks, evaluations[0], shareStorage2->aliceShare), h(c2, shareStorage1->masks, evaluations[1], shareStorage2->bobShare), h(c3, shareStorage2->masks, evaluations[2], shareStorage1->aliceShare), h(c4, shareStorage2->masks, evaluations[3], shareStorage1->bobShare)};

		std::function<Task*(int, int, int, int)> addingInput;
		addingInput = [] (a, b, c, d)
		{
			return Task::Run([&] ()
			{
				producers[a]->AddInput(ranges[a], ranges[b], ranges[c], ranges[d], dividedInputs[a]);
			});
		};


		std::vector<Task*> tasks = {addingInput(0, 1, 2, 3), addingInput(1, 0, 2, 3), addingInput(2, 3, 0, 1), addingInput(3, 2, 0, 1)};

//C# TO C++ CONVERTER TODO TASK: There is no equivalent to 'await' in C++:
		await Task::WhenAll(tasks);
		std::for_each(evaluations.begin(), evaluations.end(), [&] (void *x)
		{
			x::InputAdded();
		});

		Range *circuitRange = new Range(0, circuitSize);

		auto evaluationTasks = evaluations.Select([&] (void *eval)
		{
			Task::Run([&] ()
			{
				eval::EvaluateCircuit();
			});
		});
//C# TO C++ CONVERTER TODO TASK: There is no equivalent to 'await' in C++:
		await Task::WhenAll(evaluationTasks->ToArray());

		auto evaluator = new PlainBooleanCircuitEvaluator();
		auto plainEvaluation = evaluator->EvaluateBooleanCircuit(circuit, input);

		auto output1 = evaluations[0]->Decrypt(shareStorage2->masks, circuitRange);
		auto output2 = evaluations[1]->Decrypt(shareStorage2->masks, circuitRange);
		auto output3 = evaluations[2]->Decrypt(shareStorage1->masks, circuitRange);
		auto output4 = evaluations[3]->Decrypt(shareStorage1->masks, circuitRange);

		Assert::IsTrue(output1->AreEqual(output2));
		Assert::IsTrue(output2->AreEqual(output3));
		Assert::IsTrue(output3->AreEqual(output4));
		Assert::IsTrue(output1->AreEqual(plainEvaluation));
	}

//C# TO C++ CONVERTER TODO TASK: There is no equivalent in C++ to the 'async' keyword:
//ORIGINAL LINE: [TestMethod] public async Task TestProduction()
	Task *TestingInputProducer::TestProduction()
	{
		int inputSize = 128;
		int outputSize = 128;
		int circuitSize = 40000;

//C# TO C++ CONVERTER NOTE: The following 'decomposition declaration' requires C++17:
//ORIGINAL LINE: var(circuit, lc) = RandomCircuitBuilder.Sample(inputSize, outputSize, circuitSize);
		auto [circuit, lc] = RandomCircuitBuilder::Sample(inputSize, outputSize, circuitSize);
//C# TO C++ CONVERTER NOTE: The following 'decomposition declaration' requires C++17:
//ORIGINAL LINE: var(c1, c2, c3, c4) = CommunicatorBuilder.BuildCommunicator();
		auto [c1, c2, c3, c4] = CommunicatorBuilder::BuildCommunicator();

		int inputPerPlayer = inputSize / 4;
		std::function<Range*(int)> f = [] (int x)
		{
			return new Range(x, inputPerPlayer);
		};
		std::vector<Range*> ranges = {f(0), f(inputPerPlayer), f(inputPerPlayer*2), f(inputPerPlayer*3)};

		BitArray *input = BitArrayUtility::SampleRandomBitArray(inputSize);

		std::vector<BitArray*> dividedInputs = ranges.Select([&] (void *range)
		{
			input->CopySubsequence(range);
		}).ToList();

		auto shareStorage1 = SimplePreprocessingBuilder::BuildPreprocessing(lc);
		auto shareStorage2 = SimplePreprocessingBuilder::BuildPreprocessing(lc);

		std::function<MaskedEvaluation*(PreprocessingShare*, Communicator*)> g;
		g = [] (share, communicator)
		{
			return new MaskedEvaluation(lc, share, communicator);
		};

		auto evaluations = std::vector<MaskedEvaluation*> {g(shareStorage2->aliceShare, c1), g(shareStorage2->bobShare, c2), g(shareStorage1->aliceShare, c3), g(shareStorage1->bobShare, c4)};

		std::function<InputProducer*(Communicator*, BitArray*, MaskedEvaluation*, PreprocessingShare*)> h;
		h = [] (c, mask, eval, share)
		{
			return new InputProducer(c, mask, eval, share);
		};

		auto producers = std::vector<InputProducer*> {h(c1, shareStorage1->masks, evaluations[0], shareStorage2->aliceShare), h(c2, shareStorage1->masks, evaluations[1], shareStorage2->bobShare), h(c3, shareStorage2->masks, evaluations[2], shareStorage1->aliceShare), h(c4, shareStorage2->masks, evaluations[3], shareStorage1->bobShare)};

		std::function<Task*(int, int, int, int)> addingInput;
		addingInput = [] (a, b, c, d)
		{
			return Task::Run([&] ()
			{
				Just<BitArray*> tempVar(dividedInputs[a]);
				producers[a]->AddInput(ranges[a], ranges[b], ranges[c], ranges[d], &tempVar);
			});
		};


		std::vector<Task*> tasks = {addingInput(0, 1, 2, 3), addingInput(1, 0, 2, 3), addingInput(2, 3, 0, 1), addingInput(3, 2, 0, 1)};

//C# TO C++ CONVERTER TODO TASK: There is no equivalent to 'await' in C++:
		await Task::WhenAll(tasks);
		std::for_each(evaluations.begin(), evaluations.end(), [&] (void *x)
		{
			x::InputAdded();
		});

		Range *circuitRange = new Range(0, circuitSize);

		auto evaluationTasks = evaluations.Select([&] (void *eval)
		{
			Task::Run([&] ()
			{
				eval::EvaluateCircuit();
			});
		});
//C# TO C++ CONVERTER TODO TASK: There is no equivalent to 'await' in C++:
		await Task::WhenAll(evaluationTasks->ToArray());

		auto evaluator = new PlainBooleanCircuitEvaluator();
		auto plainEvaluation = evaluator->EvaluateBooleanCircuit(circuit, input);

		auto output1 = evaluations[0]->Decrypt(shareStorage2->masks, circuitRange);
		auto output2 = evaluations[1]->Decrypt(shareStorage2->masks, circuitRange);
		auto output3 = evaluations[2]->Decrypt(shareStorage1->masks, circuitRange);
		auto output4 = evaluations[3]->Decrypt(shareStorage1->masks, circuitRange);

		Assert::IsTrue(output1->AreEqual(output2));
		Assert::IsTrue(output2->AreEqual(output3));
		Assert::IsTrue(output3->AreEqual(output4));
		Assert::IsTrue(output1->AreEqual(plainEvaluation));

	}
}
