#ifndef CROSSCHECKER_H__
#define CROSSCHECKER_H__

#pragma once
#include <vector>
#include "../Utility/Communicator.h"
#include "../Utility/Commitment.h"

using namespace Utility;

namespace CrossCheck
{
	class CrossChecker
	{
	private:
		HashCommitment *const scheme = new HashCommitment();
		Communicator *const communicator;
		const bool isCrossCheckLeader;
		static constexpr bool LEADER = true;
		static constexpr bool FOLLOWER = false;

	public:
		virtual ~CrossChecker()
		{
			delete scheme;
			delete communicator;
		}

		CrossChecker(Communicator *communicator, bool isCrossCheckLeader);

		bool CrossCheck(std::vector<std::vector<int64_t> >  doublyMaskedEvaluation, int playerID);

	private:
		std::vector<unsigned char> CreateMask(bool isCrossCheckLeader);
	};
}

#endif
