
#include "PreprocessingShareStorage.h"
#include "PreprocessingShare.h"

using namespace Utility;

namespace TwoPartyMaskedEvaluation
{

	PreprocessingShareStorage::PreprocessingShareStorage(PreprocessingShare *aliceShare, PreprocessingShare *bobShare, std::vector<int64_t>&& masks) : aliceShare(aliceShare), bobShare(bobShare), masks(std::move(masks))
	{
	}
	
	PreprocessingShareStorage::PreprocessingShareStorage(PreprocessingShare *aliceShare, PreprocessingShare *bobShare, std::vector<int64_t>&& masks, std::vector<int> maskIndex) : aliceShare(aliceShare), bobShare(bobShare), masks(std::move(masks)), maskIndex(std::move(maskIndex))
	{
	}
}
