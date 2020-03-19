#include <cassert>
#include <stdint.h>
#include <sstream>
#include "InputProducer.h"
#include "CrossCheckException.h"
#include "../Utility/Communicator.h"
#include "../Utility/CryptoUtility.h"
#include "../Utility/Range.h"
#include "../Utility/Maybe.h"

using namespace TwoPartyMaskedEvaluation;
using namespace Utility;

namespace CrossCheck
{

	InputProducer::InputProducer(Communicator *communicator, std::vector<int64_t> masks, MaskedEvaluation *eval, PreprocessingShare *share) : eval(eval), masks(masks), communicator(communicator), share(share)
	{
// 		MyDebug::NonNull(communicator, masks, eval,share);
	}

	void InputProducer::AddInput(IRange *myRange, IRange *evaluationPartnerRange, IRange *aliceRange, IRange *bobRange, IMaybe<std::vector<std::vector<int64_t> > > *myInput)
	{
// 		MyDebug::NonNull(myRange, evaluationPartnerRange, aliceRange, bobRange, myInput);
		assert((dynamic_cast<Just<std::vector<std::vector<int64_t> > *>(myInput) != nullptr) || (dynamic_cast<EmptyRange *>(myRange) != nullptr));
		ss << "AddInput\n";
		AddEvaluationInput(myRange, evaluationPartnerRange, myInput);
		TransferInput(myRange, aliceRange, bobRange, myInput);
		eval->InputAdded();
	}

	std::vector<int64_t> InputProducer::GetMasks(Range *range)
	{
// 		MyDebug::NonNull(range);
		std::vector<int64_t> ret(range->Length);
		for(int idx = range->Start; idx < range->Start + range->Length; idx++)
		{
			ret[idx - range->Start] = masks[idx];
		}
		return ret;
	}

	void InputProducer::AddMyInput(IRange *myInputRange, IMaybe<std::vector<std::vector<int64_t> > > *myInput)
	{
		if (dynamic_cast<Range*>(myInputRange) != nullptr && dynamic_cast<Just<std::vector<std::vector<int64_t> > *>(myInput) != nullptr)
		{
			ss << "AddMyInput\n";
			Range *range = (Range *)myInputRange;
			std::vector<unsigned char> temp = communicator->AwaitEvaluationPartner();
			auto partnerMaskShare = ArrayEncoder::Decodeint64_t(temp);

			std::vector<std::vector<int64_t> > myMaskedInput(range->Length);
			std::vector<int64_t> myShare = share->operator[](range);
			
			for(int idx = 0; idx < range->Length; idx++)
			{
				myMaskedInput[idx] = partnerMaskShare[idx] +  myShare[idx] + myInput->getValue()[idx];
			}

			eval->AddInput(myMaskedInput, range);
			
			std::vector<unsigned char> myMaskedInputArray = ArrayEncoder::Encode(myMaskedInput);
			communicator->SendEvaluationPartner(myMaskedInputArray);
		}
		ss << "Done add my input\n";
// 		std::cout << ss.str() << std::endl;
	}

	void InputProducer::AddEvaluationInput(IRange *myInputRange, IRange *partnerInputRange, IMaybe<std::vector<std::vector<int64_t> > > *myInput)
	{
// 		MyDebug::NonNull(myInputRange, partnerInputRange, myInput);
// 		assert(dynamic_cast<Just<std::vector<int64_t> >*>(myInput) != nullptr ^ dynamic_cast<EmptyRange*>(myInputRange) != nullptr);
		
		if (dynamic_cast<Range*>(partnerInputRange) != nullptr)
		{
			ss << "dynamic_cast<Range*>(partnerInputRange) != nullptr" << std::endl;
			Range *range = (Range *)partnerInputRange;
			std::vector<int64_t> mask = share->operator[](range);
			std::vector<unsigned char> maskArray = ArrayEncoder::Encode(mask);
			communicator->SendEvaluationPartner(maskArray);
			
			AddMyInput(myInputRange, myInput);
			
			std::vector<unsigned char> temp2 = communicator->AwaitEvaluationPartner();
			auto hisMaskedInput = ArrayEncoder::Decodeint64_t(temp2);
			
			eval->AddInput(hisMaskedInput, (Range *)partnerInputRange);
		}
		else
		{
			ss << "else of .... dynamic_cast<Range*>(partnerInputRange) != nullptr" << std::endl;
			AddMyInput(myInputRange, myInput);
		}
	}

	void InputProducer::TransferInput(IRange *myInputRange, IRange *aliceRange, IRange *bobRange, IMaybe<std::vector<std::vector<int64_t> > > *myInput)
	{
// 		MyDebug::NonNull(myInputRange, aliceRange, bobRange, myInput);
// 		assert(dynamic_cast<EmptyRange*>(myInputRange) != nullptr ^ dynamic_cast<Just<std::vector<int64_t> >*>(myInput) != nullptr);
		ss << "TransferInput: ";
		if (dynamic_cast<Range*>(myInputRange) != nullptr)
		{
			ss << "dynamic_cast<Range*>(myInputRange) != nullptr" << std::endl;
			Range *range = (Range *)myInputRange;
			std::vector<std::vector<int64_t> >  maskedInput(range->Length);
			for(int idx = 0; idx < range->Length; idx++)
			{
				maskedInput[idx] = masks[idx + range->Start] + myInput->getValue()[idx];
			}
			
			std::vector<unsigned char> myMaskedInputEncoded = ArrayEncoder::Encode(maskedInput);
			
			assert(myMaskedInputEncoded.size() > 0);
			communicator->SendAlice(myMaskedInputEncoded);
			communicator->SendBob(myMaskedInputEncoded);
		}

		std::vector<unsigned char> aliceMaskedInput, bobMaskedInput;
		if (dynamic_cast<Range *>(aliceRange) != nullptr)
		{
			aliceMaskedInput = ReceiveTransferedInput(aliceRange, communicator->AwaitAlice());
		}
		
		if (dynamic_cast<Range *>(bobRange) != nullptr)
		{
			auto bobMaskedInput = ReceiveTransferedInput(bobRange, communicator->AwaitBob());
		}
		
		if(aliceMaskedInput.size() + bobMaskedInput.size() > 0)
		{
			VerifyTransfer(aliceMaskedInput, bobMaskedInput);
		}
		
		ss << "Done transfering\n";
		std::cout << ss.str() << std::endl;
	}

	std::vector<unsigned char> InputProducer::ReceiveTransferedInput(IRange *iRange, std::vector<unsigned char> maskedInput)
	{
		if (dynamic_cast<Range*>(iRange) != nullptr)
		{
// 			std::cout << "dynamic_cast<Range*>(iRange) != nullptr" << std::endl;
			Range *range = (Range *)iRange;

			eval->AddInput(ArrayEncoder::Decodeint64_t(maskedInput), range);
			return maskedInput;
		}
		else
		{
			return std::vector<unsigned char>(0);
		}
	}

	void InputProducer::VerifyTransfer(std::vector<unsigned char> aliceMaskedInput, std::vector<unsigned char> bobMaskedInput)
	{
		aliceMaskedInput.insert(aliceMaskedInput.end(), bobMaskedInput.begin(), bobMaskedInput.end());
		
		if (aliceMaskedInput.size() > 0)
		{
// 			std::cout << "aliceMaskedInput.size() > 0" << std::endl;
			communicator->SendEvaluationPartner(aliceMaskedInput);
			std::vector<unsigned char> partnerReceived = communicator->AwaitEvaluationPartner();

			if (!(aliceMaskedInput == partnerReceived))
			{
				throw InconsistentInputException(std::string("Received different input from partner"));
			}
		}
	}
}
