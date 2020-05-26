#include <cassert>
#include <vector>
#include <sstream>
#include <stdint.h>
#include "OutputProducer.h"
#include "CrossCheckException.h"
#include "../Utility/Communicator.h"
#include "../Utility/CryptoUtility.h"
#include "../Utility/Range.h"
#include "../Utility/Maybe.h"
#include "../TwoPartyMaskedEvaluation/PreprocessingShare.h"
#include "../TwoPartyMaskedEvaluation/MaskedEvaluation.h"

using namespace Utility;

namespace CrossCheck
{

	OutputProducer::OutputProducer(Communicator *communicator, std::vector<std::vector<uint64_t> > &maskedEvaluation, std::vector<uint64_t> &mask)
	{
		this->maskedEvaluation = maskedEvaluation;
		this->mask = mask;
		this->communicator = communicator;
	}

	std::vector<std::vector<uint64_t> > OutputProducer::ComputeOutput(Range *range)
	{
		assert(range != nullptr);
		std::vector<uint64_t> outputsMasks(range->Length);
		for(int idx = range->Start; idx < range->Start + range->Length; idx++)
		{
			outputsMasks[idx - range->Start] = mask[idx];
		}
		
		std::vector<unsigned char> encodedOutputMasks = ArrayEncoder::Encode(outputsMasks);
		assert(encodedOutputMasks.size() > 0);
		communicator->SendAlice(encodedOutputMasks);
		communicator->SendBob(encodedOutputMasks);
		std::vector<unsigned char> v1 = communicator->AwaitAlice();
		std::vector<unsigned char> v2 = communicator->AwaitBob();
// 
		if (!(v1 == v2))
		{
			throw InconsistentInputException(std::string("Alice and Bob sent different output masks"));
		}
// 
// 		
// // 		std::stringstream ss;
// // 		ss << "\nMaskedinput: ";
// // 		for(int idx = 0; idx < maskedEvaluation.size(); idx++)
// // 		{
// // 			ss << maskedEvaluation[idx] << " ";
// // 		}
// 		
		std::vector<uint64_t> masks = ArrayEncoder::Decodeuint64_t(v1);
		
		for(int idx = range->Start; idx < range->Start + range->Length; idx++)
		{
			for(int kdx = 0; kdx < maskedEvaluation[idx].size(); kdx++)
			{
				  maskedEvaluation[idx][kdx] -= mask[(*maskIndex)[idx] + kdx];
			}
		}
		
		return maskedEvaluation;
	}
}
