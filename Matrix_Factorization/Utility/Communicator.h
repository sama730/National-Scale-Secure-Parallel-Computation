#ifndef COMMUNICATOR_H__
#define COMMUNICATOR_H__

#pragma once

#include "../Network.hpp"
#include <vector>

namespace Utility
{
	class Communicator
	{
	public:
		uint64_t sendingSize;
		uint64_t receivingSize;
		
		const bool isAlice;
		uint64_t machineId = -1;

		static constexpr uint64_t PARTNER = 0;
		static constexpr uint64_t ALICE = 1;
		static constexpr uint64_t BOB = 2;
		
		std::vector<Network *> const inputChannels;
		std::vector<Network *> const outputChannels;

		/// <summary>
		/// Order of input channels is Partner, Alice, Bob.
		/// </summary>
		/// <param name="isAlice"></param>
		/// <param name="inputChannels"></param>
		/// <param name="outputChannels"></param>
		
		Communicator(bool isAlice, std::vector<Network *> &inputChannels, std::vector<Network *> &outputChannels);

		void SendMessage(uint64_t player, unsigned char *msg, uint64_t size);

		uint64_t AwaitMessage(uint64_t player, unsigned char *msg, uint64_t bufferSize);

		void SendEvaluationPartner(unsigned char *msg, uint64_t size);
		
		void AwaitEvaluationPartner(unsigned char *msg, uint64_t size);

		void SendAlice(unsigned char *msg, uint64_t size);
		
		void Transfer(unsigned char *msg, uint64_t size);

		uint64_t AwaitAlice(unsigned char *msg, uint64_t size);

		void SendBob(unsigned char *msg, uint64_t size);
		
		uint64_t AwaitBob(unsigned char *msg, uint64_t size);

		void SendVerificationPartner(unsigned char *msg, uint64_t size);
	    
		uint64_t AwaitVerificationPartner(unsigned char *msg, uint64_t size);

		void SendNonPartner(unsigned char *msg, uint64_t size);
		
		uint64_t AwaitNonPartner(unsigned char *msg, uint64_t size);

		void SendToAll(unsigned char *msg, uint64_t size);
		
		void SendToPlayers(unsigned char *m1, unsigned char *m2, unsigned char *m3, uint64_t size)
		{
			SendVerificationPartner(m1, size);
			SendEvaluationPartner(m3, size);
			SendNonPartner(m2, size);
		}
		
		void AwaitFromPlayers(unsigned char *m1, unsigned char *m2, unsigned char *m3, uint64_t size)
		{
			AwaitNonPartner(m1, size);
			AwaitVerificationPartner(m2, size);
			AwaitEvaluationPartner(m3, size);
		}
	};
}

#endif
