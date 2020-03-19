#pragma once

//========================================================================
// This conversion was produced by the Free Edition of
// C# to C++ Converter courtesy of Tangible Software Solutions.
// Order the Premium Edition at https://www.tangiblesoftwaresolutions.com
//========================================================================

#include <vector>

using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace CrossCheck;
using namespace Utility;
using namespace TwoPartyMaskedEvaluation;
using namespace Circuit;

namespace MaskedEvaluationTest
{
//C# TO C++ CONVERTER NOTE: The following .NET attribute has no direct equivalent in native C++:
//ORIGINAL LINE: [TestClass] public class CrossCheckTester
	class CrossCheckTester
	{
	private:
		std::vector<CrossChecker*> GenerateCrossCheckers();

	public:
//C# TO C++ CONVERTER TODO TASK: There is no equivalent in C++ to the 'async' keyword:
//ORIGINAL LINE: [TestMethod] public async Task TestCrossCheckerValidation()
//C# TO C++ CONVERTER NOTE: The following .NET attribute has no direct equivalent in native C++:
//ORIGINAL LINE: [TestMethod] public async Task TestCrossCheckerValidation()
		Task *TestCrossCheckerValidation();

//C# TO C++ CONVERTER TODO TASK: There is no equivalent in C++ to the 'async' keyword:
//ORIGINAL LINE: [TestMethod] public async Task TestCrossCheckerInvalid()
//C# TO C++ CONVERTER NOTE: The following .NET attribute has no direct equivalent in native C++:
//ORIGINAL LINE: [TestMethod] public async Task TestCrossCheckerInvalid()
		Task *TestCrossCheckerInvalid();
	};
}
