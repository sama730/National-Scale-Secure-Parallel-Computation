#ifndef TESTING_EMULATED_MASKED_EVALUATION_H__
#define TESTING_EMULATED_MASKED_EVALUATION_H__

#include <stdio.h>
#include <string>
#include <vector>
#include <ctime>
#include <set>
#include <random>
#include <chrono>
#include <iostream>
#include <functional>
#include "../Circuit/ArithmeticGate.h"
#include "../Circuit/ArithmeticCircuit.h"
#include "../Circuit/ArithmeticCircuitBuilder.h"
#include "../Circuit/LayeredArithmeticCircuit.h"
#include "../Circuit/RandomArithmeticCircuitBuilder.h"
#include "../Circuit/LayeredArithmeticCircuitBuilder.h"
#include "../Circuit/PlainArithmeticCircuitEvaluator.h"
#include "../TwoPartyMaskedEvaluation/SimplePreprocessingBuilder.h"
#include "../TwoPartyMaskedEvaluation/EmulatedMaskedEvaluation.h"

#include "../Utility/CryptoUtility.h"
#include "../Utility/Range.h"

using namespace Circuit;
using namespace Utility;
using namespace TwoPartyMaskedEvaluation;

class TestingEmulatedMaskedEvaluation 
{
public:
// 	static void Test1();
	static void Test2();
};

#endif
