#ifndef TESTING_MASKED_EVALUATION_H__
#define TESTING_MASKED_EVALUATION_H__

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

#include "../TwoPartyMaskedEvaluation/PreprocessingBuilder.h"
#include "../TwoPartyMaskedEvaluation/SimplePreprocessingBuilder.h"
#include "../TwoPartyMaskedEvaluation/MaskedEvaluation.h"

#include "../Utility/CryptoUtility.h"
#include "../Utility/Range.h"
#include "../Utility/Timer.h"
#include "../Utility/Channels.h"
#include "../Utility/Communicator.h"
#include "../Utility/CommunicatorBuilder.h"

class TestingMaskedEvaluation 
{
public:
// 	static void Test1();
	static void Test2();
};

#endif
