//========================================================================
// This conversion was produced by the Free Edition of
// C# to C++ Converter courtesy of Tangible Software Solutions.
// Order the Premium Edition at https://www.tangiblesoftwaresolutions.com
//========================================================================

#include "CrossCheckTester.h"

using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace CrossCheck;
using namespace Utility;
using namespace TwoPartyMaskedEvaluation;
using namespace Circuit;

namespace MaskedEvaluationTest
{

	std::vector<CrossChecker*> CrossCheckTester::GenerateCrossCheckers()
	{
//C# TO C++ CONVERTER NOTE: The following 'decomposition declaration' requires C++17:
//ORIGINAL LINE: var(c1, c2, c3, c4) = CommunicatorBuilder.BuildCommunicator();
		auto [c1, c2, c3, c4] = CommunicatorBuilder::BuildCommunicator();

		return std::vector<CrossChecker*>
		{
			new CrossChecker(c1, true),
			new CrossChecker(c2, true),
			new CrossChecker(c3, false),
			new CrossChecker(c4, false)
		};
	}

//C# TO C++ CONVERTER TODO TASK: There is no equivalent in C++ to the 'async' keyword:
//ORIGINAL LINE: [TestMethod] public async Task TestCrossCheckerValidation()
	Task *CrossCheckTester::TestCrossCheckerValidation()
	{
		for (int i = 0; i < 20; i++)
		{
			std::vector<CrossChecker*> checkers = GenerateCrossCheckers();
			BitArray *array_Renamed = BitArrayUtility::SampleRandomBitArray(128);
			auto tasks = checkers.Select([&] (void *x)
			{
				Task::Run([&] ()
				{
					x::CrossCheck(array_Renamed);
				});
			})->ToArray();
			auto task = tasks.First();
//C# TO C++ CONVERTER TODO TASK: There is no equivalent to 'await' in C++:
			bool vetoed = await task;
			Assert::IsFalse(vetoed);
		}
	}

//C# TO C++ CONVERTER TODO TASK: There is no equivalent in C++ to the 'async' keyword:
//ORIGINAL LINE: [TestMethod] public async Task TestCrossCheckerInvalid()
	Task *CrossCheckTester::TestCrossCheckerInvalid()
	{
		for (int i = 0; i < 20; i++)
		{
			std::vector<CrossChecker*> checkers = GenerateCrossCheckers();
			BitArray *array_Renamed = BitArrayUtility::SampleRandomBitArray(128);
			BitArray *array2 = BitArrayUtility::SampleRandomBitArray(128);


			auto tasks = std::vector<Task<bool>*>(4) { Task::Run([&] ()
			{
				checkers[0]->CrossCheck(array2);
//C# TO C++ CONVERTER TODO TASK: The following lambda expression could not be converted:
			}), Task::Run(() => checkers[1]->CrossCheck(array_Renamed)), Task::Run(() => checkers[2]->CrossCheck(array2)), Task::Run(() => checkers[3]->CrossCheck(array2)) };

//C# TO C++ CONVERTER TODO TASK: There is no equivalent to 'await' in C++:
			bool veto = await tasks[0];
			Assert::IsTrue(veto);
		}
	}
}
