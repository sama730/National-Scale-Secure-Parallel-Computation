#ifndef TESTING_PREPROCESSING_BUILDER_H__
#define TESTING_PREPROCESSING_BUILDER_H__

#pragma once

#include <vector>

#include "../Circuit/ArithmeticCircuit.h"
#include "../Circuit/LayeredArithmeticCircuit.h"
#include "../Circuit/RandomArithmeticCircuitBuilder.h"
#include "../Utility/Range.h"
#include "../Utility/Timer.h"
#include "../Utility/Communicator.h"
#include "../TwoPartyMaskedEvaluation/PreprocessingShare.h"

using namespace Circuit;
using namespace Utility;
using namespace TwoPartyMaskedEvaluation;

namespace MaskedEvaluationTest
{
	class TestingPreprocessingBuilder
	{
	public:
		Range *inputRange;
		Range *sizeRange;
		ArithmeticCircuit *circuit;
		LayeredArithmeticCircuit *lc;
		
		void TestPreprocessingBuilder();

		void TestDistributedPreprocessing();
		
		void TestPreprocessing(PreprocessingShare *aliceShare, PreprocessingShare *bobShare, Communicator *c1, Communicator *c2, const std::vector<int>& itemsPerUser);
	};
}

#endif
