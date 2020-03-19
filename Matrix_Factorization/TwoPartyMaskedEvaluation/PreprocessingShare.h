#ifndef PREPROCESSHING_SHARE_H
#define PREPROCESSHING_SHARE_H

#pragma once
#include <stdint.h>
#include <vector>
#include <map>
#include "../Utility/Range.h"

using namespace Utility;

namespace TwoPartyMaskedEvaluation
{
	class PreprocessingShare
	{
	public:
		// share <lx>
		std::vector<int64_t> maskShare;
		std::vector<int> maskIndex;
		std::vector<std::vector<int64_t> > beaverShare;
	private:
		/// To access the beaver share
		int counter = -1; 
		
		
		/// <summary>
		/// Instantiate preprocessing.
		/// </summary>
	public:
		virtual ~PreprocessingShare(){}

		PreprocessingShare(std::vector<int64_t>&& masks, std::vector<std::vector<int64_t> >&& beaverSharesPerLayer);
		PreprocessingShare(std::vector<int64_t>&& masks, std::vector<std::vector<int64_t> >&& beaverSharesPerLayer, const std::vector<int>& maskIndex);
		PreprocessingShare(std::vector<int64_t>&& masks, std::vector<std::vector<int64_t> >&& beaverSharesPerLayer, std::vector<int>&& maskIndex);

		/// <summary>
		/// Returns the share of a mask for a given wire.
		/// </summary>
		int64_t operator[](int wire);

		int64_t getMask(int index);
		
		std::vector<int64_t>  operator[](Range *range);

		/// <summary>
		/// Returns the next beaver triple
		/// </summary>
		std::vector<int64_t> GetNextBeaverTriples();
		
		void resetCounter(){counter = -1;}
	};
}

#endif

