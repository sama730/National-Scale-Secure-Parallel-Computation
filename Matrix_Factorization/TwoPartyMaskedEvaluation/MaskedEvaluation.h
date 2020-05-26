#ifndef MASKED_EVALUATION_H
#define MASKED_EVALUATION_H

#pragma once

#include <vector>
#include <stdint.h>
#include <stdexcept>
#include "MaskedEvaluation.h"
#include "PreprocessingShare.h"
#include "MaskedEvaluationException.h"

#include "../Circuit/LayeredArithmeticCircuit.h"
#include "../Circuit/ArithmeticGate.h"
#include "../Utility/Communicator.h"

using namespace Circuit;
using namespace Utility;

#define ALICE   0
#define BOB     1
#define CHARLIE 2
#define DAVID   3

namespace TwoPartyMaskedEvaluation
{
	class MaskedEvaluation
	{
	public:
		Communicator *communicator;
		LayeredArithmeticCircuit *lc;
		
		// Which player is running the evaluation
		int playerID;
		
		// share holds <lx> <ly> <lx*ly>
		PreprocessingShare *share;
		
		// masks hold lx'
		std::vector<uint64_t> masks; 
		std::vector<uint64_t> unTruncatedMasks;
		
		std::vector<std::vector<uint64_t> > maskedEvaluation;
		
		std::vector<int> *maskIndex;
		
		bool inputAdded = false;
		bool evaluated = false;


		virtual ~MaskedEvaluation()
		{
			delete communicator;
			delete lc;
			delete share;
// 			delete maskedEvaluation;
		}

		MaskedEvaluation(LayeredArithmeticCircuit *lc, PreprocessingShare *share, Communicator *communicator);
		MaskedEvaluation(LayeredArithmeticCircuit *lc, PreprocessingShare *share, std::vector<int> *maskIndex, Communicator *communicator);

		void AddInput(const std::vector<std::vector<uint64_t> >& input, Range *range);

		std::vector<std::vector<uint64_t> > Decrypt(std::vector<uint64_t> mask, Range *range);

		/// <summary>
		/// Computes the masked evaluation of a circuit on a layer per layer basis. 
		/// </summary>
		void EvaluateCircuit();

	private:
		void EvaluateAddGate(int layer);

		void EvaluateSubGate(int layer);
		
		void EvaluateCAddGate(int layer);
		
		void EvaluateCMulGate(int layer);
		
		void EvaluateNAddGate(int layer);

		void EvaluateMulGate(int layer);
		
		void EvaluateDotGate(int layer);
		
		void EvaluateNDotGate(int layer);

	private:
// 		uint64_t BeaverEvaluation(MultiplicationGate *ag, uint64_t beaver, PreprocessingShare *share);

	};
}

#endif
