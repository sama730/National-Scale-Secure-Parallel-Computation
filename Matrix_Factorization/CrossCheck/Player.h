#ifndef PLAYER_H__
#define PLAYER_H__

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include "../Circuit/InvalidOperationException.h"
#include "../Circuit/LayeredArithmeticCircuit.h"
#include "../Utility/Range.h"
#include "../Utility/Maybe.h"
#include "../Utility/Communicator.h"
#include "../Utility/ISecureRNG.h"
#include "../TwoPartyMaskedEvaluation/PreprocessingShare.h"
#include "../TwoPartyMaskedEvaluation/MaskedEvaluation.h"

using namespace Circuit;
using namespace Utility;
using namespace TwoPartyMaskedEvaluation;

#define ALICE   0
#define BOB     1
#define CHARLIE 2
#define DAVID   3

namespace CrossCheck
{
	class Player
	{
	public:
		Communicator *communicator;
		SPDZ2kMAC *MACGen;
// 	protected:
		IRange* inputRanges;
		
		bool generatedPreprocessing;
		bool inputAdded;
		bool evaluationComplete;
		bool passedCrossChecking;

		MaskedEvaluation *evaluation;
		
		// Hold <lx'>
		PreprocessingShare *share;
		
		// Hold lx
		std::vector<int64_t> masks;


		virtual bool getIsAlice() const = 0;

		virtual bool getIsCrossCheckLeader() const = 0;

		virtual int getMyIndex() const = 0;

		virtual int getPartnerIndex() const = 0;

		virtual int getAliceIndex() const = 0;

		virtual int getBobIndex() const = 0;

	public:
		virtual ~Player()
		{
			delete communicator;
			delete evaluation;
			delete share;
		}

		Player(Communicator *communicator, IRange* inputRanges);
		
		std::vector<std::vector<int64_t> > Run(LayeredArithmeticCircuit *lc, std::vector<std::vector<int64_t> >& input, std::vector<std::vector<uint64_t> >& inputMAC, uint64_t sharedBeta, Range *outputRange, const std::vector<int>& itemsPerUser, AESRNG *rng);
// 		std::vector<std::vector<int64_t> > ProduceOutput(Range *range);
		void LeakInformation();


	private:
		void PrepareComputation(LayeredArithmeticCircuit *lc, const std::vector<int>& itemsPerUser);

		void Evaluation();

		void CrossCheck(int playerID);

// 		std::vector<std::vector<int64_t> >ProduceOutput(Range *range);
	};

	class PlayerOne : public Player
	{
	public:
		PlayerOne(Communicator *communicator, IRange* inputRanges);

	protected:
		bool getIsAlice() const override;
		bool getIsCrossCheckLeader() const override;

		int getMyIndex() const override;
		int getPartnerIndex() const override;
		int getAliceIndex() const override;
		int getBobIndex() const override;
	};

	class PlayerTwo : public Player
	{
	public:
		PlayerTwo(Communicator *communicator, IRange* inputRanges);

	protected:
		bool getIsAlice() const override;
		bool getIsCrossCheckLeader() const override;
		int getMyIndex() const override;
		int getPartnerIndex() const override;
		int getAliceIndex() const override;
		int getBobIndex() const override;
	};

	class PlayerThree : public Player
	{
	public:
		PlayerThree(Communicator *communicator, IRange* inputRanges);

	protected:
		bool getIsAlice() const override;
		bool getIsCrossCheckLeader() const override;

		int getMyIndex() const override;
		int getPartnerIndex() const override;
		int getAliceIndex() const override;
		int getBobIndex() const override;
	};

	class PlayerFour : public Player
	{
	public:
		PlayerFour(Communicator *communicator, IRange* inputRanges);

	protected:
		bool getIsAlice() const override;
		bool getIsCrossCheckLeader() const override;

		int getMyIndex() const override;
		int getPartnerIndex() const override;
		int getAliceIndex() const override;
		int getBobIndex() const override;
	};
}

#endif
