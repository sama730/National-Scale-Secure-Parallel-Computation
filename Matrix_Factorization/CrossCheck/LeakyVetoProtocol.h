#ifndef LEAKY_VETO_PROTOCOL_H__
#define LEAKY_VETO_PROTOCOL_H__

#pragma once

#include <string>
#include <vector>
#include "../Circuit/InvalidOperationException.h"
#include "../Utility/Commitment.h"
#include "../Utility/Communicator.h"

using namespace Utility;

namespace CrossCheck
{
	class LeakyVetoProtocol
	{
	public:
		Communicator *const communicator;
	private:
		HashCommitment *const scheme = new HashCommitment();

	public:
		virtual ~LeakyVetoProtocol()
		{
			delete communicator;
			delete scheme;
		}

		LeakyVetoProtocol(Communicator *communicator);

		bool Run(bool veto);

	private:
		void CreateSharesOfVeto(bool veto, std::vector<unsigned char> &share1, std::vector<unsigned char> &share2, std::vector<unsigned char> &share3);
		void ValidateDecommitment(std::vector<unsigned char> msg, std::vector<unsigned char> seed, std::vector<unsigned char> commitment, const std::string errorMsg);

	};
}

#endif
