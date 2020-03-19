#ifndef PREPROCESSING_BUILDER_H
#define PREPROCESSING_BUILDER_H

#pragma once

#include <vector>
#include <stdint.h>
#include <future>
#include "../Circuit/LayeredArithmeticCircuit.h"
#include "../Utility/ISecureRNG.h"
#include "PreprocessingShare.h"

using namespace Circuit;
using namespace Utility;

namespace TwoPartyMaskedEvaluation
{
	/// <summary>
	/// Builds preprocessing.
	/// </summary>
	class IPreprocessingBuilder
	{
	public:
		virtual std::vector<unsigned char> getSeedAlice() = 0;

		virtual std::vector<unsigned char> getSeedBob() = 0;

		virtual const std::vector<int64_t>& getMasks() = 0;

		virtual std::vector<std::vector<int64_t> > getBobCorrection() = 0;
	};
	
	class MaskShareBuilder
	{
	public:
		static std::vector<int64_t> Build(LayeredArithmeticCircuit *lc, AESRNG *rng, const std::vector<int>& itemsPerUser);
	};

	class PreprocessingBuilder : public IPreprocessingBuilder
	{
	private:
		std::vector<unsigned char> privateSeedAlice;
		std::vector<unsigned char> privateSeedBob;
		std::vector<int64_t> privateMasks;
		std::vector<std::vector<int64_t> > privateBobCorrection;

	public:
		PreprocessingBuilder(){
			privateBobCorrection.resize(0);
		}
		
		std::vector<unsigned char> getSeedAlice();
		
		void setSeedAlice(const std::vector<unsigned char>& value);

		std::vector<unsigned char> getSeedBob();
		
		void setSeedBob(const std::vector<unsigned char>& value);

		const std::vector<int64_t>& getMasks();
		
		int64_t getMasks(int idx);
		
		void setMasks(const std::vector<int64_t>& value);
		void setMasks(std::vector<int64_t>&& value);

		std::vector<std::vector<int64_t> > getBobCorrection();
		
		void addBobCorrection(std::vector<int64_t> value);
		
		void setBobCorrection(const std::vector<std::vector<int64_t> >& value);

		void BuildPreprocessing(std::vector<unsigned char>& seedAlice, std::vector<unsigned char>& seedBob, LayeredArithmeticCircuit *lc, const std::vector<int>& itemsPerUser);
	};

	class AlicePreprocessingBuilder
	{
		/// <summary>
		/// Create preprocessing for Alice from a seed.
		/// </summary>
	public:
		static PreprocessingShare *Build(LayeredArithmeticCircuit *lc, std::vector<unsigned char>& seed, const std::vector<int>& itemsPerUser);
	};

	class BobPreprocessingBuilder
	{
		/// <summary>
		/// Creates a share preprocessing for Bob from a seed and correction
		/// </summary>
	public:
		static PreprocessingShare *Build(LayeredArithmeticCircuit *lc, std::vector<unsigned char>& seed, std::vector<std::vector<int64_t> >&& beaverShare, const std::vector<int>& itemsPerUser);
	};
}

#endif
