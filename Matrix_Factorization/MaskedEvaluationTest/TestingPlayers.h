#ifndef TESTING_PLAYER_H__
#define TESTING_PLAYER_H__

#pragma once

namespace MaskedEvaluationTest
{
	class TestPlayers
	{
	public:
		Range *inputRange;
		Range *sizeRange;
		ArithmeticCircuit *circuit;
		LayeredArithmeticCircuit *lc;
		
		void TestEvaluation1();
// 		void TestEvaluation2();
	};
}

#endif
