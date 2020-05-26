#include <cassert>
#include "SimplePreprocessingShare.h"

using namespace Utility;

namespace TwoPartyMaskedEvaluation
{

	SimplePreprocessingShare::SimplePreprocessingShare(std::vector<uint64_t> masks, std::vector<uint64_t> beaverShare) : MaskShare(masks), BeaverShare(beaverShare)
	{
		assert(masks != nullptr);
		assert(beaverShare != nullptr);
	}

	std::vector<uint64_t> SimplePreprocessingShare::GetMaskShare(Range *range)
	{
		assert(range != nullptr);
		return (MaskShare + range->Start);
	}

	uint64_t SimplePreprocessingShare::operator [](int wire)
	{
		return MaskShare[wire];
	}

	uint64_t SimplePreprocessingShare::GetBeaverShare(int wire)
	{
		return BeaverShare[wire];
	}
}
