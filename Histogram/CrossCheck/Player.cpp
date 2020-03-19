#include <future>
#include <thread>
#include <climits>
#include <sstream>
#include "Player.h"
#include "PreprocessingParty.h"
#include "CrossCheckException.h"
#include "CrossChecker.h"
#include "../Utility/Timer.h"

using namespace std::chrono;
using namespace Circuit;
using namespace Utility;
using namespace TwoPartyMaskedEvaluation;

namespace CrossCheck
{

	Player::Player(Communicator *communicator, IRange* inputRanges)
	{
		this->communicator = communicator;
		this->inputRanges = inputRanges;
		generatedPreprocessing = false;
		inputAdded = false;
		evaluationComplete = false;
		passedCrossChecking = false;
	}
	
	std::vector<std::vector<int64_t> > Player::Run(LayeredArithmeticCircuit *lc, std::vector<std::vector<int64_t> >& input, std::vector<std::vector<uint64_t> >& inputMAC, uint64_t sharedBeta, Range *outputRange, const std::vector<int>& itemsPerUser, AESRNG *rng)
	{
		// Timer t;
		PrepareComputation(lc, itemsPerUser);
		// t.Tick("PrepareComputation() elapsed time: ");
		auto startConversion = high_resolution_clock::now();

		// Prepare <x + lx>
		int count = 0;
		for(int idx = 0; idx < input.size(); idx++)
		{
			for(int kdx = 0; kdx < input[idx].size(); kdx++)
			{
				if(getMyIndex() == ALICE || getMyIndex() == BOB)
				{
					inputMAC[idx][kdx] += sharedBeta*masks[count];
				}
				
				input[idx][kdx] += share->maskShare[count]; count++;
			}
		}
		
		std::vector<unsigned char> ourShare = ArrayEncoder::EncodeInt64Array(input);
		std::vector<unsigned char> theirShare(ourShare.size());
		

		if (getIsAlice())
		{
			communicator->SendEvaluationPartner(ourShare.data(), ourShare.size());
			communicator->AwaitEvaluationPartner(theirShare.data(), theirShare.size());
		}
		else
		{
			communicator->AwaitEvaluationPartner(theirShare.data(), theirShare.size());
			communicator->SendEvaluationPartner(ourShare.data(), ourShare.size());
		}
		
		
		std::vector<std::vector<int64_t> > theirSharedMaskedInput = ArrayEncoder::DecodeInt64Array(theirShare);
		
		// Compute (x + lx)
		for(int idx = 0; idx < input.size(); idx++)
		{
			for(int kdx = 0; kdx < input[idx].size(); kdx++)
			{
				input[idx][kdx] += theirSharedMaskedInput[idx][kdx];
			}
		}

		// Verify data MAC (x + lx')
		if(getMyIndex() == ALICE || getMyIndex() == BOB)
		{
			std::vector<unsigned char> flatMAC = ArrayEncoder::EncodeUInt64Array(inputMAC);
			uint64_t size = flatMAC.size();
			communicator->SendVerificationPartner((unsigned char *)(&size), sizeof(uint64_t));
			communicator->SendVerificationPartner(flatMAC.data(), flatMAC.size());
		}
		else
		{
			uint64_t size;
			communicator->AwaitVerificationPartner((unsigned char *)(&size), sizeof(uint64_t));
			
			std::vector<unsigned char> flatMAC(size);
			communicator->AwaitVerificationPartner(flatMAC.data(), size);
			inputMAC = ArrayEncoder::DecodeUInt64Array(flatMAC);
			
			std::vector<unsigned char> theirFlatMAC(size);
			
			if(getMyIndex() == CHARLIE)
			{
				communicator->SendEvaluationPartner(flatMAC.data(), size);
				communicator->AwaitEvaluationPartner(theirFlatMAC.data(), size);
			}
			else
			{
				communicator->AwaitEvaluationPartner(theirFlatMAC.data(), size);
				communicator->SendEvaluationPartner(flatMAC.data(), size);
			}
			
			std::vector<std::vector<uint64_t> > theirInputMAC = ArrayEncoder::DecodeUInt64Array(theirFlatMAC);
			
			uint64_t val;
			for(int idx = 0; idx < inputMAC.size(); idx++)
			{
				for(int kdx = 0; kdx < inputMAC[idx].size(); kdx++)
				{
					val = inputMAC[idx][kdx] + theirInputMAC[idx][kdx] - MACGen->alpha*input[idx][kdx];
					if(val != 0) std::cout << idx << " " << kdx << std::endl;
					assert(val == 0);
				}
			}
		}



		auto ShareConversionDone = high_resolution_clock::now();
		auto ShareConversionDoneDuration = duration_cast<milliseconds>(ShareConversionDone - startConversion);
		cout << "ShareConversion " << ShareConversionDoneDuration.count() << endl;



		
		evaluation->AddInput(input, (Range *)inputRanges);
		inputAdded = true;
		
// 		t.Tick("AddInput() elapsed time: ");
		Evaluation();
		
// 		t.Tick("Evaluation() elapsed time: ");
		
		CrossCheck(getMyIndex());
		
// 		t.Tick("CrossCheck() elapsed time: ");
		
		int start = evaluation->share->maskIndex[outputRange->Start];
		
		std::vector<int64_t> outputsMasks(masks.size() - start);
		for(int idx = start; idx < masks.size(); idx++)
		{
			outputsMasks[idx - start] = masks[idx];
		}
		
		// Convert masked output to secret share
		
		std::vector<std::vector<int64_t> > sharedOutput(outputRange->Length);
		
		std::vector<int64_t> share1 = rng->GetMaskArray(dimension*outputRange->Length);
		std::vector<int64_t> share2(share1.size());
		
		for(int idx = 0; idx < dimension*outputRange->Length; idx++)
		{
			share2[idx] = -share1[idx] - outputsMasks[idx];
		}

		std::vector<int64_t> theirShare1(share1.size()), theirShare2(share1.size());
		if(getMyIndex() == ALICE || getMyIndex() == BOB){
			communicator->SendAlice((unsigned char *)(share1.data()), sizeof(int64_t)*share1.size());
			communicator->SendBob((unsigned char *)(share2.data()), sizeof(int64_t)*share2.size());
						
			communicator->AwaitAlice((unsigned char *)(theirShare1.data()), sizeof(int64_t)*share1.size());
			communicator->AwaitBob((unsigned char *)(theirShare2.data()), sizeof(int64_t)*share2.size());
		}
		else
		{			
			communicator->AwaitAlice((unsigned char *)(theirShare1.data()), sizeof(int64_t)*share1.size());
			communicator->AwaitBob((unsigned char *)(theirShare2.data()), sizeof(int64_t)*share2.size());

			communicator->SendAlice((unsigned char *)(share1.data()), sizeof(int64_t)*share1.size());
			communicator->SendBob((unsigned char *)(share2.data()), sizeof(int64_t)*share2.size());
		}
		
		assert(theirShare1 == theirShare2);
		
		count = 0;
		
		for(int idx = outputRange->Start; idx < outputRange->Start + outputRange->Length; idx++)
		{
			sharedOutput[idx - outputRange->Start].resize(dimension);
			for(int kdx = 0; kdx < dimension; kdx++)
			{
				sharedOutput[idx - outputRange->Start][kdx] = evaluation->maskedEvaluation[idx][kdx] + theirShare1[count];
				count++;
			}
		}
		
// 		TestUtility::PrintVector(sharedOutput, "Shared output", (1<<20));
		
		// Decode output
		std::vector<unsigned char> encodedOutputMasks = ArrayEncoder::Encode(outputsMasks);
		assert(encodedOutputMasks.size() > 0);

		std::vector<unsigned char> v1(encodedOutputMasks.size()), v2(encodedOutputMasks.size());

		if(getMyIndex() == ALICE || getMyIndex() == BOB){
			communicator->SendAlice(encodedOutputMasks.data(), encodedOutputMasks.size());
			communicator->SendBob(encodedOutputMasks.data(), encodedOutputMasks.size());
			
			communicator->AwaitAlice(v1.data(), v1.size());
			communicator->AwaitBob(v2.data(), v2.size());
		}
		else
		{	
			communicator->AwaitAlice(v1.data(), v1.size());
			communicator->AwaitBob(v2.data(), v2.size());

			communicator->SendAlice(encodedOutputMasks.data(), encodedOutputMasks.size());
			communicator->SendBob(encodedOutputMasks.data(), encodedOutputMasks.size());
		}
		
		if (!(v1 == v2))
		{
			throw InconsistentInputException(std::string("Alice and Bob sent different output masks"));
		}
		
		std::vector<int64_t> theirMask = ArrayEncoder::Decodeint64_t(v1);
		
		return evaluation->Decrypt(theirMask, outputRange);
	}

	void Player::LeakInformation()
	{
// 		int64_t *doublyMasked = new int64_t[];
// 		
// 		masks->ImmutableXor(evaluation->maskedEvaluation);
// 
// 		std::string result = std::string("---------------------------------\n")
// 					  + this + "\n"
// 					  + "myMaskedEvaluation: " + evaluation->maskedEvaluation.ConvertToBitString() + "\n"
// 					  + "masks: " + masks->ConvertToBitString() + "\n"
// 					  + "doubly masked: " + doublyMasked->ConvertToBitString();

		//Console.WriteLine(result);
	}
	
	void Player::PrepareComputation(LayeredArithmeticCircuit *lc, const std::vector<int>& itemsPerUser)
	{
// 		MyDebug::NonNull(lc);
		auto party = new PreprocessingParty(communicator, getIsAlice());
		std::future<void> f = std::async(std::launch::async, [&]{ party->RunPreprocessing(lc, masks, share, itemsPerUser); } );
		f.wait(); f.get();		
		evaluation = new MaskedEvaluation(lc, share, &(share->maskIndex), communicator);

		evaluation->masks = masks;
		evaluation->playerID = getMyIndex();
		evaluation->maskIndex = &(share->maskIndex);
		generatedPreprocessing = true;
	}
	
	void Player::Evaluation()
	{
		if (inputAdded == false)
		{
			throw NoInputAddedException();
		}

		evaluation->EvaluateCircuit();
		evaluationComplete = true;
	}

	void Player::CrossCheck(int playerID)
	{
		// std::cout << "CrossCheck()" << std::endl;
		if (!evaluationComplete)
		{
			throw NotEvaluatedException();
		}

		CrossChecker *checker = new CrossChecker(communicator, getIsCrossCheckLeader());
		std::vector<std::vector<int64_t> > doublyMaskedOutput = evaluation->maskedEvaluation;
		
		
// 		std::stringstream ss;
// 		ss << "doublyMaskedOutput: ";
		
		int count = 0;
		for(int idx = 0; idx < evaluation->maskedEvaluation.size(); idx++)
		{
			for(int kdx = 0; kdx < evaluation->maskedEvaluation[idx].size(); kdx++)
			{
				doublyMaskedOutput[idx][kdx] += masks[count]; count++;
			}
// 			ss << doublyMaskedOutput[idx] << " ";
		}
		
// 		std::cout << ss.str() << std::endl;
		
		bool vetoed = checker->CrossCheck(doublyMaskedOutput, playerID);

// 		if (vetoed)
// 		{
			std::cout << "Veto: " << vetoed << std::endl;
// // 			throw VetoException();
// 		}

		passedCrossChecking = true;
	}

// 	std::vector<std::vector<int64_t> > Player::ProduceOutput(Range *range)
// 	{
// 		if (!passedCrossChecking)
// 		{
// 			throw NotCrossCheckedException();
// 		}
// 
// 		OutputProducer *producer = new OutputProducer(communicator, evaluation->maskedEvaluation, masks);
// 		return producer->ComputeOutput(range);
// 	}

	PlayerOne::PlayerOne(Communicator *communicator, IRange* inputRanges) : Player(communicator, inputRanges)
	{
		generatedPreprocessing = false;
		inputAdded = false;
		evaluationComplete = false;
		passedCrossChecking = false;
	}

	bool PlayerOne::getIsAlice() const
	{
		return true;
	}

	bool PlayerOne::getIsCrossCheckLeader() const
	{
		return true;
	}

	int PlayerOne::getMyIndex() const
	{
		return ALICE;
	}

	int PlayerOne::getPartnerIndex() const
	{
		return BOB;
	}

	int PlayerOne::getAliceIndex() const
	{
		return CHARLIE;
	}

	int PlayerOne::getBobIndex() const
	{
		return DAVID;
	}

	PlayerTwo::PlayerTwo(Communicator *communicator, IRange* inputRanges) : Player(communicator, inputRanges)
	{
		generatedPreprocessing = false;
		inputAdded = false;
		evaluationComplete = false;
		passedCrossChecking = false;
	}

	bool PlayerTwo::getIsAlice() const
	{
		return false;
	}

	bool PlayerTwo::getIsCrossCheckLeader() const
	{
		return true;
	}

	int PlayerTwo::getMyIndex() const
	{
		return BOB;
	}

	int PlayerTwo::getPartnerIndex() const
	{
		return ALICE;
	}

	int PlayerTwo::getAliceIndex() const
	{
		return CHARLIE;
	}

	int PlayerTwo::getBobIndex() const
	{
		return DAVID;
	}

	PlayerThree::PlayerThree(Communicator *communicator, IRange* inputRanges) : Player(communicator, inputRanges)
	{
		generatedPreprocessing = false;
		inputAdded = false;
		evaluationComplete = false;
		passedCrossChecking = false;
	}

	bool PlayerThree::getIsAlice() const
	{
		return true;
	}

	bool PlayerThree::getIsCrossCheckLeader() const
	{
		return false;
	}

	int PlayerThree::getMyIndex() const
	{
		return CHARLIE;
	}

	int PlayerThree::getPartnerIndex() const
	{
		return DAVID;
	}

	int PlayerThree::getAliceIndex() const
	{
		return ALICE;
	}

	int PlayerThree::getBobIndex() const
	{
		return BOB;
	}

	PlayerFour::PlayerFour(Communicator *communicator, IRange* inputRanges) : Player(communicator, inputRanges)
	{
		generatedPreprocessing = false;
		inputAdded = false;
		evaluationComplete = false;
		passedCrossChecking = false;
	}

	bool PlayerFour::getIsAlice() const
	{
		return false;
	}

	bool PlayerFour::getIsCrossCheckLeader() const
	{
		return false;
	}

	int PlayerFour::getMyIndex() const
	{
		return DAVID;
	}

	int PlayerFour::getPartnerIndex() const
	{
		return CHARLIE;
	}

	int PlayerFour::getAliceIndex() const
	{
		return ALICE;
	}

	int PlayerFour::getBobIndex() const
	{
		return BOB;
	}
}
