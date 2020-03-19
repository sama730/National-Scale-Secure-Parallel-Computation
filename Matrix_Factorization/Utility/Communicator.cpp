#include <cassert>
#include "Communicator.h"
#define MAX_PACKAGE_SIZE 1073741824

namespace Utility
{
	Communicator::Communicator(bool isAlice, std::vector<Network *> &inputChannels, std::vector<Network *> &outputChannels) : isAlice(isAlice), inputChannels(inputChannels), outputChannels(outputChannels){
		sendingSize = 0;
		receivingSize = 0;
	}

	void Communicator::SendMessage(uint64_t player, unsigned char *msg, uint64_t size)
	{
		sendingSize += size;
		
		if(size > 1024*1024) std::cout << machineId << ": sending " << size << " bytes" << std::endl;
		
		uint64_t count = 0;
		uint64_t bufferSize;
		
		while(count < size)
		{
			if (count + MAX_PACKAGE_SIZE < size)
			{
				bufferSize = MAX_PACKAGE_SIZE;
			}
			else
			{
				bufferSize = size - count;
			}
			
			uint64_t bytes_sent = 0;

			while(bytes_sent < bufferSize)
			{
				uint64_t bs = send(inputChannels[player]->sock_fd, msg + count, bufferSize - bytes_sent, 0);

				if(bs < 0)
				{
					perror("error: send");
					exit(1);
				}

				bytes_sent += bs;
				count += bs;
			}
		}
		
		assert(count == size);
	}

	uint64_t Communicator::AwaitMessage(uint64_t player, unsigned char *msg, uint64_t size)
	{
		receivingSize += size;
		if(size > 1024*1024) std::cout << machineId << ": awaiting " << size << " bytes" << std::endl;
		
		
		uint64_t sd = outputChannels[player]->sock_fd;
		fd_set reading_set;
		
		while (true)
		{
			FD_ZERO(&reading_set); //clear the socket set 
			FD_SET(sd, &reading_set); //add master socket to set
			uint64_t activity = select(sd+1, &reading_set , NULL , NULL , NULL);   //&timeout
			if ((activity < 0) && (errno!=EINTR))  { perror("select failed"); exit(1); }
			if (activity > 0)
			{
				if (FD_ISSET(sd, &reading_set)){
					uint64_t count = 0;
					uint64_t bufferSize;
					uint64_t bytes_received = 0;
					
					while(count < size)
					{
						if (count + MAX_PACKAGE_SIZE < size)
						{
							bufferSize = MAX_PACKAGE_SIZE;
						}
						else
						{
							bufferSize = size - count;
						}
						
						uint64_t arrived = 0;

						while (arrived < bufferSize){
							uint64_t br = recv(sd, msg + count, bufferSize - arrived, 0);
							arrived += br;
							count += br;
						}

						bytes_received += arrived;

						// count += MAX_PACKAGE_SIZE;
					}
					assert(bytes_received == size);
					return bytes_received;
				}
				
			}
			else if (activity == 0){
				return 0;
			}
		}
	}

	void Communicator::SendEvaluationPartner(unsigned char *msg, uint64_t size)
	{
		SendMessage(PARTNER, msg, size);
	}
	
	void Communicator::AwaitEvaluationPartner(unsigned char *msg, uint64_t size)
	{
		AwaitMessage(PARTNER, msg, size);
	}

	void Communicator::SendAlice(unsigned char *msg, uint64_t size)
	{
		SendMessage(ALICE, msg, size);
	}
	
	void Communicator::Transfer(unsigned char *msg, uint64_t size)
	{
		SendAlice(msg, size);
		SendBob(msg, size);
	}

	uint64_t Communicator::AwaitAlice(unsigned char *msg, uint64_t size)
	{
		return AwaitMessage(ALICE, msg, size);
	}

	void Communicator::SendBob(unsigned char *msg, uint64_t size)
	{
		SendMessage(BOB, msg, size);
	}
	
	uint64_t Communicator::AwaitBob(unsigned char *msg, uint64_t size)
	{
		return AwaitMessage(BOB, msg, size);
	}

	void Communicator::SendVerificationPartner(unsigned char *msg, uint64_t size)
	{
		if (isAlice)
		{
			SendAlice(msg, size);
		}
		else
		{
			SendBob(msg, size);
		}
	}
     
	uint64_t Communicator::AwaitVerificationPartner(unsigned char *msg, uint64_t size)
	{
		if (isAlice)
		{
			return AwaitAlice(msg, size);
		}
		else
		{
			return AwaitBob(msg, size);
		}
	}

	void Communicator::SendNonPartner(unsigned char *msg, uint64_t size)
	{
		if (isAlice)
		{
			SendBob(msg, size);
		}
		else
		{
			SendAlice(msg, size);
		}
	}
	
	uint64_t Communicator::AwaitNonPartner(unsigned char *msg, uint64_t size)
	{
		if (isAlice)
		{
			return AwaitBob(msg, size);
		}
		else
		{
			return AwaitAlice(msg, size);
		}
	}

	void Communicator::SendToAll(unsigned char *msg, uint64_t size)
	{
		SendEvaluationPartner(msg, size);
		SendAlice(msg, size);
		SendBob(msg, size);
	}
}
