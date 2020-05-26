#include "CrossChecker.h"
#include <cassert>
#include <openssl/sha.h>
#include "LeakyVetoProtocol.h"
#include "../Utility/CryptoUtility.h"

using namespace Utility;

#define ALICE   0
#define BOB     1
#define CHARLIE 2
#define DAVID   3

namespace CrossCheck
{

	CrossChecker::CrossChecker(Communicator *communicator, bool isCrossCheckLeader) : communicator(communicator), isCrossCheckLeader(isCrossCheckLeader)
	{
		assert(communicator != nullptr);

	}

	bool CrossChecker::CrossCheck(std::vector<std::vector<uint64_t> > doublyMaskedEvaluation, int playerID)
	{
		std::vector<unsigned char> mask = CreateMask(isCrossCheckLeader);
		
		// unroll output
		std::vector<uint64_t> temp;
		for(int idx = 0; idx < doublyMaskedEvaluation.size(); idx++)
		{
			temp.insert(temp.end(), doublyMaskedEvaluation[idx].begin(), doublyMaskedEvaluation[idx].end());
		}
		
		std::vector<unsigned char> hashDoubleMaskedEvaluation = ArrayEncoder::Hash(temp);

		std::vector<unsigned char> myMaskedHash = CryptoUtility::ComputeHash(hashDoubleMaskedEvaluation);
		
		assert(myMaskedHash.size() > 0);
		
		
		std::vector<unsigned char> maskedHash1(myMaskedHash.size()), maskedHash2(myMaskedHash.size());
		
		if(playerID == ALICE || playerID == CHARLIE)
		{
			communicator->SendEvaluationPartner(myMaskedHash.data(), myMaskedHash.size());
			communicator->SendNonPartner(myMaskedHash.data(), myMaskedHash.size());
			
			communicator->AwaitEvaluationPartner(maskedHash1.data(), maskedHash1.size());
			communicator->AwaitNonPartner(maskedHash2.data(), maskedHash2.size());
		}
		else
		{
			communicator->AwaitEvaluationPartner(maskedHash1.data(), maskedHash1.size());
			communicator->AwaitNonPartner(maskedHash2.data(), maskedHash2.size());
			
			communicator->SendEvaluationPartner(myMaskedHash.data(), myMaskedHash.size());
			communicator->SendNonPartner(myMaskedHash.data(), myMaskedHash.size());
		}
		
		bool veto = !(maskedHash1 == maskedHash2);
		
		LeakyVetoProtocol *protocol = new LeakyVetoProtocol(communicator);
		
		return protocol->Run(veto);
	}

	std::vector<unsigned char> CrossChecker::CreateMask(bool isCrossCheckLeader)
	{
		std::vector<unsigned char> mask;

		if (isCrossCheckLeader == LEADER)
		{
			mask = CryptoUtility::SampleByteArray(sizeof(uint64_t));
			assert(mask.size() > 0);
			communicator->SendVerificationPartner(mask.data(), mask.size());
		}
		else
		{
			mask.resize(sizeof(uint64_t));
			communicator->AwaitVerificationPartner(mask.data(), mask.size());
		}

		return mask;
	}
}
