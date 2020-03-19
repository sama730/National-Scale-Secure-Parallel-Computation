#ifndef IPLAYER_H__
#define IPLAYER_H__

#pragma once
#include "../Circuit/LayeredArithmeticCircuit.h"
#include "../Utility/Range.h"
#include "../Utility/Maybe.h"

using namespace Circuit;
using namespace Utility;

namespace CrossCheck
{
	class IPlayer
	{
	public:
		virtual void PrepareComputation(LayeredArithmeticCircuit *lc) = 0;
		virtual void AddInput(IMaybe<int64_t *> *input) = 0;
		virtual void Evaluation() = 0;
		virtual void CrossCheck() = 0;
		virtual int64_t * ProduceOutput(Range *range) = 0;
	};
}

#endif
