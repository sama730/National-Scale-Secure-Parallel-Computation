#ifndef SIMPLE_PREPROCESSING_SHARE_H
#define SIMPLE_PREPROCESSING_SHARE_H

#pragma once

#include <stdexcept>

using namespace Utility;

namespace TwoPartyMaskedEvaluation
{
	class NoTripleAtLayerException : public std::exception
	{
	};

	class SimplePreprocessingShare
	{
	private:
		std::vector<int64_t> MaskShare;
		std::vector<int64_t> BeaverShare;

	public:
		virtual ~SimplePreprocessingShare()
		{
			delete [] MaskShare;
			delete [] BeaverShare;
		}

		SimplePreprocessingShare(std::vector<int64_t> masks, std::vector<int64_t> beaverShare);

		std::vector<int64_t> GetMaskShare(Range *range);

		int64_t operator [](int wire);

		int64_t GetBeaverShare(int wire);
	};
}

#endif
