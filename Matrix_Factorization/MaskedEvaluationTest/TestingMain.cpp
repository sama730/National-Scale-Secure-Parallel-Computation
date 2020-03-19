#include "TestingMaskedEvaluation.h"
#include "TestingEmulatedMaskedEvaluation.h"
#include "TestingArithmeticCircuit.h"
#include "TestingPreprocessingBuilder.h"
#include "TestingPlayers.h"

using namespace MaskedEvaluationTest;

int main()
{
	Timer t;
// 	TestingMaskedEvaluation::Test2();
// 	TestingEmulatedMaskedEvaluation::Test2();

// 	TestingArithmeticCircuit test;
// 	test.Test2();
// 	TestingPreprocessingBuilder *t1 = new TestingPreprocessingBuilder();
// 	t1->TestPreprocessingBuilder();
// 	t1->TestDistributedPreprocessing();
// // 	TestingPreprocessingBuilder *t2 = new TestingPreprocessingBuilder();
// 	t2->TestDistributedPreprocessing();
	
// 	t->TestDistributedPreprocessing();
	
	TestPlayers *t2 = new TestPlayers();
	t2->TestEvaluation1();
// 	t2->TestEvaluation2();
	
	t.Tick("Total test time");
	return 0;
}
