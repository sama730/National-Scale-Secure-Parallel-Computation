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
		std::vector<uint64_t> maskShare;
		
		// Each wire is a vector of values
		// The maskIndex maps the wire ID to the maskArray
		// For example, wire i contains the vector [i_{0}, ..., i_{DIM-1}]
		// maskShare[maskIndex[i] + j] <--> mask of i_{j}
		std::vector<int> maskIndex;
		
		std::vector<std::vector<uint64_t> > beaverShare; // [lz + lx*ly]
	private:
		/// To access the beaver share
		int counter = -1; 
		
		
		/// <summary>
		/// Instantiate preprocessing.
		/// </summary>
	public:
		virtual ~PreprocessingShare(){}

		PreprocessingShare(std::vector<uint64_t>&& masks, std::vector<std::vector<uint64_t> >&& beaverSharesPerLayer);
		PreprocessingShare(std::vector<uint64_t>&& masks, std::vector<std::vector<uint64_t> >&& beaverSharesPerLayer, const std::vector<int>& maskIndex);
		PreprocessingShare(std::vector<uint64_t>&& masks, std::vector<std::vector<uint64_t> >&& beaverSharesPerLayer, std::vector<int>&& maskIndex);

		/// <summary>
		/// Returns the share of a mask for a given wire.
		/// </summary>
		uint64_t operator[](int wire);

		uint64_t getMask(int index);
		
		std::vector<uint64_t>  operator[](Range *range);

		/// <summary>
		/// Returns the next beaver triple
		/// </summary>
		std::vector<uint64_t> GetNextBeaverTriples();
		
		void resetCounter(){counter = -1;}
	};
}

#endif

