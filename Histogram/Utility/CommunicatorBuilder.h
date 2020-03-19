#ifndef COMMUNICATOR_BUILDER_H__
#define COMMUNICATOR_BUILDER_H__

#pragma once

#include "../Network.hpp"
#include "Communicator.h"

namespace Utility
{
	class CommunicatorBuilder
	{
	public:
		static void BuildCommunicator(int party, Machine *machine, Communicator * &c)
		{
			Network *cPartner, *cAlice, *cBob;
			
			bool isAlice = true;
			
			if(Alice == party)
			{
				cPartner = machine->partiesDown[Bob - Alice - 1];
				cAlice   = machine->partiesDown[Charlie - Alice - 1];
				cBob     = machine->partiesDown[David - Alice - 1];
			  
				std::vector<Network *> input = {cPartner, cAlice, cBob};
				std::vector<Network *> output = {cPartner, cAlice, cBob};
				
				c = new Communicator(isAlice, input, output);
			}
			else if(Bob == party)
			{
				cPartner = machine->partiesUp[Alice];
				cAlice   = machine->partiesDown[Charlie - Bob - 1];
				cBob     = machine->partiesDown[David - Bob - 1];
			  
				std::vector<Network *> input = {cPartner, cAlice, cBob};
				std::vector<Network *> output = {cPartner, cAlice, cBob};
				
				c = new Communicator(!isAlice, input, output);
			}
			else if(Charlie == party)
			{
				cPartner = machine->partiesDown[David - Charlie - 1];
				cAlice   = machine->partiesUp[Alice];
				cBob     = machine->partiesUp[Bob];
			  
				std::vector<Network *> input = {cPartner, cAlice, cBob};
				std::vector<Network *> output = {cPartner, cAlice, cBob};
				
				c = new Communicator(isAlice, input, output);
			}
			else if(David == party)
			{
				cPartner = machine->partiesUp[Charlie];
				cAlice   = machine->partiesUp[Alice];
				cBob     = machine->partiesUp[Bob];
			  
				std::vector<Network *> input = {cPartner, cAlice, cBob};
				std::vector<Network *> output = {cPartner, cAlice, cBob};
				
				c = new Communicator(!isAlice, input, output);
			}
		}
	};
}

#endif

