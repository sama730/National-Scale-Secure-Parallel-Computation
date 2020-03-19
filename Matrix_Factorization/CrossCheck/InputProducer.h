#ifndef INPUT_PRODUCER_H__
#define INPUT_PRODUCER_H__

#pragma once

#include <vector>
#include <sstream>
#include <functional>
#include "../Utility/Communicator.h"
#include "../Utility/Range.h"
#include "../Utility/Maybe.h"
#include "../TwoPartyMaskedEvaluation/PreprocessingShare.h"
#include "../TwoPartyMaskedEvaluation/MaskedEvaluation.h"

using namespace TwoPartyMaskedEvaluation;
using namespace Utility;

namespace CrossCheck
{
	class InputProducer
	{
	public:
		std::stringstream ss;
		
		MaskedEvaluation *eval;
		Communicator *communicator;
		PreprocessingShare *share;
		std::vector<__int128> masks;

		virtual ~InputProducer()
		{
			delete eval;
			delete communicator;
			delete share;
		}

		InputProducer(Communicator *communicator, std::vector<__int128> masks, MaskedEvaluation *eval, PreprocessingShare *share);

		void AddInput(IRange *myRange, IRange *evaluationPartnerRange, IRange *aliceRange, IRange *bobRange, IMaybe<std::vector<std::vector<__int128> > > *myInput);

	private:
		std::vector<__int128> GetMasks(Range *range);

		void AddMyInput(IRange *myInputRange, IMaybe<std::vector<std::vector<__int128> > > *myInput);

		void AddEvaluationInput(IRange *myInputRange, IRange *partnerInputRange, IMaybe<std::vector<std::vector<__int128> > > *myInput);

		void TransferInput(IRange *myInputRange, IRange *aliceRange, IRange *bobRange, IMaybe<std::vector<std::vector<__int128> > > *myInput);

		std::vector<unsigned char> ReceiveTransferedInput(IRange *iRange, std::vector<unsigned char> maskedInput);

		void VerifyTransfer(std::vector<unsigned char> aliceMaskedInput, std::vector<unsigned char> bobMaskedInput);
	};
}

#endif
