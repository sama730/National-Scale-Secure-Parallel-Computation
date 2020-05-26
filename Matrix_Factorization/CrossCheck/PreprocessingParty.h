#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <future>
#include <thread>
#include <stdint.h>

#include "../Circuit/LayeredArithmeticCircuit.h"
#include "../Circuit/LayeredArithmeticCircuitBuilder.h"

#include "../TwoPartyMaskedEvaluation/PreprocessingShare.h"

#include "../Utility/CryptoUtility.h"
#include "../Utility/Communicator.h"

using namespace Utility;
using namespace Circuit;
using namespace TwoPartyMaskedEvaluation;

namespace CrossCheck
{
	class PreprocessingParty
	{
	private:
		Communicator *communicator;
		bool isAlice = false;
		int player;
	public:
		virtual ~PreprocessingParty()
		{
			delete communicator;
		}

		PreprocessingParty(int player, Communicator *communicator, bool isAlice);

		void RunPreprocessing(LayeredArithmeticCircuit *lc, std::vector<uint64_t>& masks, PreprocessingShare * &share, std::vector<uint64_t>& unTruncatedMasks, const std::vector<int>& itemsPerUser);
		
		void GenerateSeeds(std::vector<unsigned char> &seedAlice, std::vector<unsigned char> &seedBob)
		{
			seedAlice = CryptoUtility::SampleByteArray(32);
			seedBob   = CryptoUtility::SampleByteArray(32);
		}

		void BuildPreprocessing(std::vector<uint64_t>& masks, std::vector<uint64_t>& unTruncatedMasks, LayeredArithmeticCircuit *lc, const std::vector<int>& itemsPerUser, const std::vector<int>& shareIndex);

		PreprocessingShare *ReceivePreprocessing(LayeredArithmeticCircuit *lc, const std::vector<int>& itemsPerUser, const std::vector<int>& shareIndex);
	};
}
