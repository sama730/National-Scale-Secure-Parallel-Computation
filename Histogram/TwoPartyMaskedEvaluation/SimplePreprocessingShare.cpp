#include <cassert>
#include "SimplePreprocessingShare.h"

using namespace Utility;

namespace TwoPartyMaskedEvaluation
{

	SimplePreprocessingShare::SimplePreprocessingShare(std::vector<int64_t> masks, std::vector<int64_t> beaverShare) : MaskShare(masks), BeaverShare(beaverShare)
	{
		assert(masks != nullptr);
		assert(beaverShare != nullptr);
	}

	std::vector<int64_t> SimplePreprocessingShare::GetMaskShare(Range *range)
	{
		assert(range != nullptr);
		return (MaskShare + range->Start);
	}

	int64_t SimplePreprocessingShare::operator [](int wire)
	{
		return MaskShare[wire];
	}

	int64_t SimplePreprocessingShare::GetBeaverShare(int wire)
	{
		return BeaverShare[wire];
	}
}
