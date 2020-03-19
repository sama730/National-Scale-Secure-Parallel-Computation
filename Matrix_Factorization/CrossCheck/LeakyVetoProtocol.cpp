#include "LeakyVetoProtocol.h"
#include "../Utility/CryptoUtility.h"
#include <openssl/sha.h>
#include <iostream>
#include <sstream>

using namespace Utility;

namespace CrossCheck
{

	LeakyVetoProtocol::LeakyVetoProtocol(Communicator *communicator) : communicator(communicator)
	{
	}

	void LeakyVetoProtocol::CreateSharesOfVeto(bool veto, std::vector<unsigned char>& share1, std::vector<unsigned char>& share2, std::vector<unsigned char>& share3)
	{
		std::vector<unsigned char> random = CryptoUtility::SampleByteArray(sizeof(int64_t));
		share1 = CryptoUtility::SampleByteArray(sizeof(int64_t));
		share2 = CryptoUtility::SampleByteArray(sizeof(int64_t));
		
		int64_t val1 = *((int64_t *)share1.data());
		int64_t val2 = *((int64_t *)share2.data());
		
		int64_t *val3 = new int64_t[1];
		*val3 = -(val1 + val2);
		
		unsigned char *temp = (unsigned char *)val3;
		std::vector<unsigned char> val3Vec(temp, temp + sizeof(int64_t));
		
		share3 = veto ? random : val3Vec;
	}

	bool LeakyVetoProtocol::Run(bool veto)
	{
		std::vector<unsigned char> share1, share2, share3;
		
		CreateSharesOfVeto(veto, share1, share2, share3);
		
		assert(share1.size() > 0);
		communicator->SendToPlayers(share1.data(), share2.data(), share3.data(), sizeof(int64_t));
		
		std::vector<unsigned char> s1(sizeof(int64_t)), s2(sizeof(int64_t)), s3(sizeof(int64_t));
		communicator->AwaitFromPlayers(s1.data(), s2.data(), s3.data(), sizeof(int64_t));

		int64_t *val = new int64_t[1];
		*val = *((int64_t *)s1.data()) + *((int64_t *)s2.data()) + *((int64_t *)s3.data());
		
		
		std::stringstream ss;
		
		// ss << "My Shares: " << *val << "\n";
		std::cout << ss.str() << std::endl;
		
		unsigned char *valArray = (unsigned char *)val;
		std::vector<unsigned char> myShare(valArray, valArray + sizeof(int64_t));
		std::vector<unsigned char> seed;
		std::vector<unsigned char> commitment = scheme->Commit(myShare, seed);

		assert(commitment.size() > 0);
		communicator->SendToAll(commitment.data(), commitment.size());
		
		std::vector<unsigned char> c1(commitment.size()), c2(commitment.size()), c3(commitment.size());
		communicator->AwaitFromPlayers(c1.data(), c2.data(), c3.data(), commitment.size());

		assert(myShare.size() > 0);
		communicator->SendToAll(myShare.data(), sizeof(int64_t));
		
		std::vector<unsigned char> m1(sizeof(int64_t)), m2(sizeof(int64_t)), m3(sizeof(int64_t));
		communicator->AwaitFromPlayers(m1.data(), m2.data(), m3.data(), sizeof(int64_t));

		assert(seed.size() > 0);
		communicator->SendToAll(seed.data(), seed.size());
		
		std::vector<unsigned char> seed1(seed.size()), seed2(seed.size()), seed3(seed.size());
		communicator->AwaitFromPlayers(seed1.data(), seed2.data(), seed3.data(), seed.size());

		ValidateDecommitment(m1, seed1, c1, "Validation partner sent an invalid decommitment.");
		ValidateDecommitment(m2, seed2, c2, "Evaluation partner sent an invalid decommitment.");
		ValidateDecommitment(m3, seed3, c3, "Non-partner sent an invalid decommitment.");

		int64_t val_m1 = *((int64_t *)(m1.data()));
		int64_t val_m2 = *((int64_t *)(m2.data()));
		int64_t val_m3 = *((int64_t *)(m3.data()));
		
		auto vetoOutput = *val + val_m1 + val_m2 + val_m3;

		return (vetoOutput != 0);
	}

	void LeakyVetoProtocol::ValidateDecommitment(std::vector<unsigned char> msg, std::vector<unsigned char> seed, std::vector<unsigned char> commitment, const std::string errorMsg)
	{
// 		MyDebug::NonNull(msg, seed, commitment, errorMsg);

		if (!scheme->Verification(commitment, msg, seed))
		{
			throw InvalidDecommitmentException(errorMsg);
		}
	}
}
