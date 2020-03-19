#ifndef PREPROCESSING_SHARE_H
#define PREPROCESSING_SHARE_H

#pragma once
#include <stdint.h>
#include "PreprocessingShare.h"

namespace TwoPartyMaskedEvaluation
{
	class PreprocessingShareStorage
	{
	public:
		// aliceShare contains <lx> for each wire, and <lx*ly> for each MUL gate (similar for bobShare)
		// <lx> = aliceShare->maskShare[x]
		// <lx*ly> = aliceShare->beaverShare[idx]
		PreprocessingShare *aliceShare;
		PreprocessingShare *bobShare;
		
		// Each wire may need 1 or 10 masks.
		// masks[x] = lx
		std::vector<int64_t> masks;
		std::vector<int> maskIndex;
		
		virtual ~PreprocessingShareStorage()
		{
			delete [] aliceShare;
			delete [] bobShare;
		}

		PreprocessingShareStorage(PreprocessingShare *aliceShare, PreprocessingShare *bobShare, std::vector<int64_t>&& masks);
		PreprocessingShareStorage(PreprocessingShare *aliceShare, PreprocessingShare *bobShare, std::vector<int64_t>&& masks, std::vector<int> maskIndex);
	};
}

#endif
