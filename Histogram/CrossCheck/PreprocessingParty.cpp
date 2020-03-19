#include <openssl/sha.h>
#include "PreprocessingParty.h"
#include "CrossCheckException.h"
#include "../TwoPartyMaskedEvaluation/PreprocessingBuilder.h"
#include "../Utility/Timer.h"

#include <string>
#include <sstream>
#include <iostream>

using namespace Utility;
using namespace TwoPartyMaskedEvaluation;
using namespace Circuit;

namespace CrossCheck
{

	PreprocessingParty::PreprocessingParty(Communicator *communicator, bool isAlice)
	{
		this->communicator = communicator;
		this->isAlice = isAlice;
	}

	void PreprocessingParty::RunPreprocessing(LayeredArithmeticCircuit *lc, std::vector<int64_t>& masks, PreprocessingShare * &share, const std::vector<int>& itemsPerUser)
	{
		std::future<std::vector<int64_t> > f1 = std::async(std::launch::async, [&]{return BuildPreprocessing(lc, itemsPerUser);} );
		std::future<PreprocessingShare *> f2 = std::async(std::launch::async, [&]{return ReceivePreprocessing(lc, itemsPerUser);} );
		
		f1.wait();
		f2.wait();
		
		masks = f1.get();
		share = f2.get();
	}
	
	std::vector<int64_t> PreprocessingParty::BuildPreprocessing(LayeredArithmeticCircuit *lc, const std::vector<int>& itemsPerUser)
	{
		std::vector<unsigned char> seedAlice, seedBob;
		
		int stop;
		
		if (isAlice)
		{
			GenerateSeeds(seedAlice, seedBob);
			
			communicator->SendEvaluationPartner(seedAlice.data(), seedAlice.size());
			communicator->SendEvaluationPartner(seedBob.data(), seedBob.size());
		}
		else
		{
			seedAlice.resize(32);
			seedBob.resize(32);
			communicator->AwaitEvaluationPartner(seedAlice.data(), seedAlice.size());
			communicator->AwaitEvaluationPartner(seedBob.data(), seedBob.size());
		}
		
		// TestUtility::PrintByteArray(seedAlice, "Seed Alice");
		// TestUtility::PrintByteArray(seedBob, "Seed Bob");
		
		communicator->SendAlice(seedAlice.data(), seedAlice.size());
		communicator->SendBob(seedBob.data(), seedBob.size());
		
		PreprocessingBuilder *builder = new PreprocessingBuilder();
		// Timer t;
		builder->BuildPreprocessing(seedAlice, seedBob, lc, itemsPerUser);
		// t.Tick("BuildPreprocessing -- builder->BuildPreprocessing(seedAlice, seedBob, lc)");
		std::vector<unsigned char> beaverCorrection = ArrayEncoder::EncodeInt64Array(builder->getBobCorrection());
		
		if(isAlice)
		{
			// std::cout << "Alice sending bob beaver correction: " << beaverCorrection.size() << std::endl;
			uint64_t size = beaverCorrection.size();
			communicator->SendBob((unsigned char *)(&size), sizeof(uint64_t));
			communicator->SendBob(beaverCorrection.data(), beaverCorrection.size());
			// t.Tick("SendBob beaverCorrection");
		}
		else{
			// std::cout << "Bob sending bob hash: " << beaverCorrection.size() << std::endl;
			std::vector<unsigned char> hash = CryptoUtility::ComputeHash(beaverCorrection);
			uint64_t size = hash.size();
			communicator->SendBob((unsigned char *)(&size), sizeof(uint64_t));
			communicator->SendBob(hash.data(), hash.size());
			// t.Tick("SendBob hash");
		}
		
		return builder->getMasks();
	}
	
	PreprocessingShare *PreprocessingParty::ReceivePreprocessing(LayeredArithmeticCircuit *lc, const std::vector<int>& itemsPerUser)
	{
		// Timer t;
		
		std::vector<unsigned char> mySeed1(32), mySeed2(32);
		communicator->AwaitAlice(mySeed1.data(), mySeed1.size());
		communicator->AwaitBob(mySeed2.data(), mySeed2.size());
		
		PreprocessingShare *share;
		
		// TestUtility::PrintByteArray(mySeed1, "Received Seed From Alice");
		// TestUtility::PrintByteArray(mySeed2, "Received Seed From Bob");
		
		// std::cout << "Received seeds..." << std::endl;
		if (!(mySeed1 == mySeed2))
		{
			throw InconsistentSeedException("Alice and Bob sent different seeds.");
		}

		if (isAlice)
		{
			// std::cout << "AlicePreprocessingBuilder::Build(lc, mySeed1.data());" << std::endl;
			share = AlicePreprocessingBuilder::Build(lc, mySeed1, itemsPerUser);
		}
		else
		{
			uint64_t size;
			communicator->AwaitAlice((unsigned char *)(&size), sizeof(uint64_t));
			std::vector<unsigned char> beaverCorrectionEncoding(size);
			communicator->AwaitAlice(beaverCorrectionEncoding.data(), beaverCorrectionEncoding.size());
			communicator->AwaitBob((unsigned char *)(&size), sizeof(uint64_t));
			std::vector<unsigned char> hash(size);
			communicator->AwaitBob(hash.data(), hash.size());
			std::vector<unsigned char> myHash = CryptoUtility::ComputeHash(beaverCorrectionEncoding);

			if (!(hash == myHash))
			{
				throw InconsistentHashException("Preprocessing share production inconsistent.");
			}

			std::vector<std::vector<int64_t> > beaverCorreaction = ArrayEncoder::DecodeInt64Array(beaverCorrectionEncoding);
			share = BobPreprocessingBuilder::Build(lc, mySeed1, std::move(beaverCorreaction), itemsPerUser);
		}
		
		// t.Tick("ReceivePreprocessing");
		return share;
	}
}
