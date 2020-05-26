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
		std::vector<uint64_t> MaskShare;
		std::vector<uint64_t> BeaverShare;

	public:
		virtual ~SimplePreprocessingShare()
		{
			delete [] MaskShare;
			delete [] BeaverShare;
		}

		SimplePreprocessingShare(std::vector<uint64_t> masks, std::vector<uint64_t> beaverShare);

		std::vector<uint64_t> GetMaskShare(Range *range);

		uint64_t operator [](int wire);

		uint64_t GetBeaverShare(int wire);
	};
}

#endif
