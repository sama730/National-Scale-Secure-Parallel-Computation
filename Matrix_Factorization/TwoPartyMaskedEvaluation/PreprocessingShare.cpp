#include <vector>
#include "PreprocessingShare.h"
#include "../Utility/Range.h"

using namespace Utility;

namespace TwoPartyMaskedEvaluation
{

	PreprocessingShare::PreprocessingShare(std::vector<int64_t>&& masks, std::vector<std::vector<int64_t> >&& beaverSharesPerLayer) : maskShare(std::move(masks))
	{
		this->beaverShare = std::move(beaverSharesPerLayer);
	}
	
	PreprocessingShare::PreprocessingShare(std::vector<int64_t>&& masks, std::vector<std::vector<int64_t> >&& beaverSharesPerLayer, const std::vector<int>& maskIndex) : maskShare(std::move(masks))
	{
		this->beaverShare = std::move(beaverSharesPerLayer);
		this->maskIndex = maskIndex;
	}
	
	PreprocessingShare::PreprocessingShare(std::vector<int64_t>&& masks, std::vector<std::vector<int64_t> >&& beaverSharesPerLayer, std::vector<int>&& maskIndex) : maskShare(std::move(masks)), beaverShare(std::move(beaverSharesPerLayer)), maskIndex(std::move(maskIndex))
	{
	}

	int64_t PreprocessingShare::operator[](int wire)
	{
		return maskShare[wire];
	}
	
	int64_t PreprocessingShare::getMask(int index)
	{
		return maskShare[index];
	}

	std::vector<int64_t>  PreprocessingShare::operator[](Range *range)
	{
		std::vector<int64_t> ret(maskShare.begin() + range->Start, maskShare.begin() + range->Start + range->Length);
		
		return ret;
	}

	std::vector<int64_t> PreprocessingShare::GetNextBeaverTriples()
	{
		counter++;
		return beaverShare[counter];
	}
}
