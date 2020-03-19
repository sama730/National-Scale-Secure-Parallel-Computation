#ifndef SIMPLE_PREPROCESSING_BUILDER_H
#define SIMPLE_PREPROCESSING_BUILDER_H

#pragma once

#include "PreprocessingShareStorage.h"
#include "PreprocessingShare.h"
#include "../Circuit/LayeredArithmeticCircuit.h"

using namespace Circuit;

namespace TwoPartyMaskedEvaluation
{
	class SimplePreprocessingBuilder
	{
	public:
		static PreprocessingShareStorage *BuildPreprocessing(LayeredArithmeticCircuit *lc);
		static PreprocessingShareStorage *BuildPreprocessing(LayeredArithmeticCircuit *lc, std::vector<int>& itemsPerUser);
	};
}

#endif
