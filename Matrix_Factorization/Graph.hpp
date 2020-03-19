#ifndef GRAPH_H__
#define GRAPH_H__

#pragma once

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <math.h>
#include <cstring>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <bitset>
#include <set>

// #ifdef UNIX_PLATFORM

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>

#include "Utility/Range.h"
#include "Utility/Timer.h"
#include "Utility/Communicator.h"
#include "Utility/CommunicatorBuilder.h"
#include "Utility/ISecureRNG.h"
#include "Utility/CryptoUtility.h"

#include "Circuit/ArithmeticGate.h"
#include "Circuit/ArithmeticCircuit.h"
#include "Circuit/ArithmeticCircuitBuilder.h"
#include "Circuit/LayeredArithmeticCircuit.h"
#include "Circuit/RandomArithmeticCircuitBuilder.h"
#include "Circuit/LayeredArithmeticCircuitBuilder.h"
#include "Circuit/PlainArithmeticCircuitEvaluator.h"
#include "Circuit/GradientDescendCircuitBuilder.h"


#include "TwoPartyMaskedEvaluation/PreprocessingBuilder.h"
#include "TwoPartyMaskedEvaluation/SimplePreprocessingBuilder.h"
#include "TwoPartyMaskedEvaluation/MaskedEvaluation.h"

#include "CrossCheck/PreprocessingParty.h"
#include "CrossCheck/Player.h"

#include "Machine.hpp"
#include "GraphData.hpp"

using namespace std;
using namespace std::chrono;
using namespace Circuit;
using namespace Utility;
using namespace CrossCheck;
using namespace TwoPartyMaskedEvaluation;

// const int dimension = 10;

double GAMMA = 0.001;
double LAMBDA = 0.0;
double MU = 0.02;
	
class Graph 
{ 
public:
	int totalMachines;
	int totalParties;
	int machineId;
	int party;
	int partner;
	int totalEdges;
	int totalUsers;
	int totalItems;	
	double epsilon;

	int nUsers, nItems, nNodes, nEdges;
	int firstLightMachineID_User;
	int firstLightMachineID_Item;

	Machine* machine;
	Communicator *com;
	Player *player;
	AESRNG *rng;
	SPDZ2kMAC *MACGen;
	
	Range *inputRange;
	Range *sizeRange;
	ArithmeticCircuit *circuit;
	LayeredArithmeticCircuit *lc;

	uint64_t sharedBeta;
	std::vector<unsigned char> alpha;// = machine->getSharedSeedForPeerAndParty(party, partner, com);
	__int128 alphaVal;


	Graph (int totalMachines, int machineId, int party, int totalEdges, int totalUsers, int totalItems, double epsilon, Machine* machine) {
		this->totalMachines = totalMachines;
		this->totalParties = 4;
		this->machineId = machineId;
		this->party = party;
		
		if (party == Alice) partner = Bob; 
		if (party == Bob) partner = Alice;
		if (party == Charlie) partner = David;
		if (party == David) partner = Charlie;
		
		this->totalEdges = totalEdges;
		this->totalUsers = totalUsers;
		this->totalItems = totalItems;
		this->epsilon = epsilon;
		this->machine = machine;
		
		
		CommunicatorBuilder::BuildCommunicator(party, machine, com);
		com->machineId = machineId;
		cout << "com->machineId: " << com->machineId << " machineId: " << machineId << endl;
		
		// Initialize pseudo random generator 
		std::vector<unsigned char> rand  = machine->getSharedRandomSeed(party, partner, com);
		rng = new AESRNG(rand.data());
		
		// Each group agree on parameters for MAC computation
		alpha = machine->getSharedSeedForPeerAndParty(party, partner, com);
		alphaVal = *((__int128 *)(alpha.data()));
		alphaVal = alphaVal % 0x10000000000;
		
		MACGen = new SPDZ2kMAC((uint64_t)alphaVal, com);
		
		std::vector<uint64_t> randoms = rng->GetUInt64Array(1);
		if(party == Charlie || party == David)
		{
			sharedBeta = MACGen->alpha - randoms[0];
			com->SendAlice((unsigned char *)(&randoms[0]), sizeof(uint64_t));
			com->SendBob((unsigned char *)(&sharedBeta), sizeof(uint64_t));
		}
		else
		{
			uint64_t s1, s2;
			com->AwaitAlice((unsigned char *)(&s1), sizeof(uint64_t));
			com->AwaitBob((unsigned char *)(&s2), sizeof(uint64_t));
			assert(s1 == s2);
			sharedBeta = s1;
		}
	}

	~Graph(){
		delete rng;
		delete MACGen;
		delete machine;
	}

	void GraphComputation() {

		int alpha = 1;  /* modify alpha */		
		// double p = 1 - (1/exp(epsilon));  // p
		// int biasedCoinBits = (int) ceil(log2(1/p)); // k
		// int alpha = (int) ceil(-39 * log(2) / log(1 - p) - (log(totalItems) / log(1 - p)));  // d or alpha

		int nUsers, nItems;
		int firstLightMachineID_User = totalUsers % totalMachines;
		int firstLightMachineID_Item = totalItems % totalMachines;

		/* Compute nUsers: number of Users in each machine */
		if (machineId < firstLightMachineID_User)
		{
			// no. of users in heavy load machine;
			nUsers = (totalUsers + totalMachines - 1)/totalMachines; 
		}
		else
		{
			// no of users in light load machine;
			nUsers = totalUsers/totalMachines; 
		}
		
		/* Compute nItems: number of Items in each machine */
		if (machineId < firstLightMachineID_Item)
		{
			nItems = (totalItems + totalMachines - 1)/totalMachines;
		}
		else
		{
			nItems = totalItems/totalMachines;
		}
		
		/* Compute nNodes & nEdges in each machine */
		int nNodes = nUsers + nItems;
		int nEdges = totalEdges / totalMachines;

		cout << "************ Input ************" << endl;
		cout << "nEdges: " << nEdges << " , nNodes:" << nNodes << " , nUsers:" << nUsers << " , nItems:" << nItems << endl;
		
		Secret_Node * secret_nodes = new Secret_Node[nNodes];
		Secret_Edge * secret_edges = new Secret_Edge[nEdges];

		SecretEdgeMAC *MACedEdges;

		if ((secret_nodes == nullptr) || (secret_edges  == nullptr))
		{
			cout << "Error: memory could not be allocated";
		}

    	/* Alice & Bob generate data & compute its aMac, then send them to Charlie & David */
		if (party == Alice || party == Bob)
		{
			SecretSharing(party, partner, secret_edges, nEdges, secret_nodes, nUsers, nItems, alpha);
		}

		std::cout << "--------------- Original Nodes -----------------" << std::endl;
		// printNodes(secret_nodes, nNodes);
		std::cout << "--------------- Original Edges -----------------" << std::endl;
		// printEdges(secret_edges, nEdges);


    // **************************  Shuffle **********************************************************************************************************************************
		auto start = high_resolution_clock::now();
		Timer t;

		std::cout << "---------------" << machineId << " Start MACs -----------------" << std::endl;
		if (party == Alice || party == Bob)
		{
			MACedEdges = computeMAC(party, secret_edges, nEdges);
			com->SendVerificationPartner((unsigned char *)secret_edges, nEdges*sizeof(Secret_Edge));
			com->SendVerificationPartner((unsigned char *)MACedEdges, nEdges*sizeof(SecretEdgeMAC));
		}
		else
		{
			MACedEdges = new SecretEdgeMAC[nEdges];
			com->AwaitVerificationPartner((unsigned char *)secret_edges, nEdges*sizeof(Secret_Edge));
			com->AwaitVerificationPartner((unsigned char *)MACedEdges, nEdges*sizeof(SecretEdgeMAC));
		}

		std::cout << machineId << ": finished MAC....." << std::endl;


   		/* Charlie & David shuffle edges and Mac, re-randomize them and send them back to Alice & Bob */
		Secret_Edge   *shuffledEdges;
		SecretEdgeMAC *shuffledMACs;
		SecretEdgeMAC *bMACs;
		
		if (party == Charlie || party == David)
		{
			/* Generate & Distribuite permutation seeds to the other party */
			std::cout << machineId << ": start seed " << endl;
			// std::vector<unsigned char> seed = machine->getSharedSeedForPeerAndParty(party, partner, com);
			// __int128 permSeed = *((__int128 *)(alpha.data()));
			std::cout << machineId << " end seed" << endl;
			
			std::vector<int> shuffledArray(totalEdges);
			std::iota (std::begin(shuffledArray), std::end(shuffledArray), 0);  //Fills the range [first, last) with sequentially increasing values, starting with value 0.

			std::mt19937 g(alphaVal);  // permutation function
			std::shuffle(shuffledArray.begin(), shuffledArray.end(), g);  // permute an integer string
			
			std::cout << machineId << ": Shuffling edges....." << std::endl;
			shuffledEdges = Shuffle<Secret_Edge>(party, partner, shuffledArray, secret_edges, nEdges);
			
			std::cout << machineId << ": Shuffling macs....." << std::endl;
			shuffledMACs = Shuffle<SecretEdgeMAC>(party, partner, shuffledArray, MACedEdges, nEdges);
			
			std::cout << machineId << ": Re-randomize shares....." << std::endl;
			rerandomize(party, shuffledEdges, nEdges);
			rerandomize(party, shuffledMACs, nEdges);
			
			// Send to verification partner
			com->SendVerificationPartner((unsigned char *)(shuffledEdges), nEdges*sizeof(Secret_Edge));
			com->SendVerificationPartner((unsigned char *)(shuffledMACs), nEdges*sizeof(SecretEdgeMAC));
		}
		else
		{ 
			shuffledEdges = new Secret_Edge[nEdges];
			shuffledMACs  = new SecretEdgeMAC[nEdges];
			
			com->AwaitVerificationPartner((unsigned char *)(shuffledEdges), nEdges*sizeof(Secret_Edge));
			com->AwaitVerificationPartner((unsigned char *)(shuffledMACs), nEdges*sizeof(SecretEdgeMAC));

			cout << machineId << ": ------- Verify MAC shuffling -------" << endl;
			verifyMAC(party, shuffledMACs, shuffledEdges, nEdges);
		}

		delete[] secret_edges;
		delete[] MACedEdges;

		std::cout << machineId << ": --------------- Shuffled Edges -----------------" << std::endl;
		// printEdges(shuffledEdges, nEdges);
		std::cout << machineId << ": --------------- Shuffled MACs -----------------" << std::endl;



		auto ShuffleDone = high_resolution_clock::now(); 
		auto ShuffleDuration = duration_cast<milliseconds>(ShuffleDone - start);


   // **************************  Gather **********************************************************************************************************************************

		// generate macs and send them to Alice and Bob
		if (party == Charlie || party == David)
		{
			// std::cout << machineId << ": Mac the id_u, id_v, and data" << std::endl;		
			bMACs = computeMAC(party, shuffledEdges, nEdges);
			com->SendVerificationPartner((unsigned char *)bMACs, nEdges*sizeof(SecretEdgeMAC));
		}
		else
		{ 
			bMACs = new SecretEdgeMAC[nEdges];
			com->AwaitVerificationPartner((unsigned char *)bMACs, nEdges*sizeof(SecretEdgeMAC));
		}


		bool direction = true;
		std::vector<int> elementsPerNode;
		std::vector<std::vector<int64_t> > input;
		std::vector<uint64_t> OpenedVertexIds(nEdges);
		std::vector<uint64_t> MACId(nEdges);
		std::vector<std::vector<uint64_t> > inputMAC;


		/* Alice and Bob verify the suffling, compute and store new macs on the rerandomized shares */
		/* Alice and Bob Gather & send data to Charlie & David */	
		if(party == Alice || party == Bob)
		{
			// Alice & Bob do gathering and send data to Charlie & David
			cout << machineId << ": ------- Gathered Edges -------" << endl;
			OpenedVertexIds = Gather(party, partner, shuffledEdges, bMACs, nEdges, secret_nodes, MACId, nUsers, nItems, direction);
			
			// Send re-randomized OpenedId & MACId to Charlie & David so that they can verify the MAC
			// Alice & Bob get the same PRNG and re-rand Ids & MACId
			std::vector<uint64_t> Id1(nEdges), Id2(nEdges);
			std::vector<unsigned char> seed = machine->getSharedRandomSeed(party, partner, com);
			AESRNG *rng = new AESRNG(seed.data());
			
			std::vector<uint64_t> randUInt64 = rng->GetUInt64Array(nEdges);
			
			for(int idx = 0; idx < nEdges; idx++)
			{
				Id1[idx] = randUInt64[idx];
				Id2[idx] = OpenedVertexIds[idx] - Id1[idx];
			}
			
			randUInt64 = rng->GetUInt64Array(nEdges);
			
			for(int idx = 0; idx < nEdges; idx++)
			{
				if(party % 2 == 0)
				{
					MACId[idx] += randUInt64[idx];
				}
				else
				{
					MACId[idx] -= randUInt64[idx];
				}
			}
			
			// Send Id1 to Charlie, Id2 to David, MACId to Verification partner
			if(party == ALICE)
			{
				com->SendVerificationPartner((unsigned char *)(Id1.data()), nEdges*sizeof(uint64_t));
				std::vector<unsigned char> hashedId2 = ArrayEncoder::Hash(Id2);
				com->SendNonPartner(hashedId2.data(), hashedId2.size());
			}
			else
			{
				// Send the hash of the vector
				com->SendVerificationPartner((unsigned char *)(Id2.data()), nEdges*sizeof(uint64_t));
				std::vector<unsigned char> hashedId1 = ArrayEncoder::Hash(Id1);
				com->SendNonPartner(hashedId1.data(), hashedId1.size());
			}
			
			com->SendVerificationPartner((unsigned char *)(MACId.data()), nEdges*sizeof(uint64_t));
			
			std::cout << machineId << ": --------- Generate circuit metadata -------------" << std::endl;
			for(int idx = 0; idx < nNodes; idx++)
			{
				if(secret_nodes[idx].halfEdge.size() > 0)
				{
					elementsPerNode.push_back(secret_nodes[idx].halfEdge.size());
					// std::cout << idx << " " << secret_nodes[idx].halfEdge.size() << " " << secret_nodes[idx].vertexID << endl;
				}
			}
			
			int size = elementsPerNode.size();
			com->SendVerificationPartner((unsigned char *)(&size), sizeof(int));
			com->SendVerificationPartner((unsigned char *)elementsPerNode.data(), size*sizeof(int));
		}
		else
		{
			// Receive Ids, hashed ids, and MACId
			std::vector<uint64_t> Id(nEdges);
			std::vector<unsigned char> hashedId(32);
			
			com->AwaitVerificationPartner((unsigned char *)(Id.data()), nEdges*sizeof(uint64_t));
			com->AwaitNonPartner(hashedId.data(), 32);
			
			com->AwaitVerificationPartner((unsigned char *)(MACId.data()), nEdges*sizeof(uint64_t));
			
			std::vector<unsigned char> hashedId2 = ArrayEncoder::Hash(Id);
			assert(hashedId == hashedId2);
			verifyMAC(party, MACId, Id);

			int size;
			com->AwaitVerificationPartner((unsigned char *)(&size), sizeof(int));
			elementsPerNode.resize(size);
			com->AwaitVerificationPartner((unsigned char *)elementsPerNode.data(), size*sizeof(int));
		}


		auto GatherDone = high_resolution_clock::now();
		auto GatherDuration = duration_cast<milliseconds>(GatherDone - ShuffleDone);


   // **************************  Apply **********************************************************************************************************************************

		

		std::cout << "------------------- Running 4 Party Protocol --------------------" << std::endl;
		MACId.resize(0);
		int numUsers = elementsPerNode.size();
		int sumNumItems = 0;
		for(int idx = 0; idx < elementsPerNode.size(); idx++)
		{
			sumNumItems += elementsPerNode[idx];
		}
		
		int numMasks = 6*DIM*numUsers + (2*DIM + 4)*sumNumItems;
		int numWires = 6*numUsers + 6*sumNumItems;
		
		std::stringstream ss;
		ss << "circuit"; ss << party; ss << machineId; ss << ".txt";
		
		ArithmeticCircuitBuilder circuitBuilder;
		LayeredArithmeticCircuitBuilder layerBuilder;
		
		circuit = GradientDescendCircuitBuilder::BuildMultipleUsersByLayers(elementsPerNode, GAMMA, MU);
		lc = layerBuilder.CreateLayeredCircuit(circuit);
		
		// std::cout << lc->ToString() << std::endl;
		
		int inputSize = circuit->InputLength;
		int outputSize = circuit->OutputLength;;
		int circuitSize = circuit->NumWires;
		
		inputRange = new Range(0, inputSize);
		Range *outputRange = new Range(numWires-numUsers, numUsers);


		
		if(Alice == party || Bob == party)
		{
		    // input = TestUtility::GenerateInput(elementsPerNode, inputSize);
			// std::cout << "Get shared masked input: " << inputSize << std::endl;


			input = getSharedMaskedInput(secret_nodes, nNodes, nUsers, inputSize, direction);


			inputMAC = getBetaMAC(secret_nodes, nNodes, nUsers, inputSize, direction);
			
			std::vector<unsigned char> flatInput = ArrayEncoder::EncodeInt64Array(input);
			uint64_t size = flatInput.size();
			std::cout << "Flat input size: " << size << std::endl;
			com->SendVerificationPartner((unsigned char *)(&size), sizeof(uint64_t));
			com->SendVerificationPartner(flatInput.data(), size);
		}
		else
		{
			uint64_t size;
			std::cout << "Flat input size: " << size << std::endl;
			com->AwaitVerificationPartner((unsigned char *)(&size), sizeof(uint64_t));
			std::vector<unsigned char> flatInput(size);
			com->AwaitVerificationPartner(flatInput.data(), size);
			
			input = ArrayEncoder::DecodeInt64Array(flatInput);
		}
		
		if (party == Alice)   player = new PlayerOne(com, inputRange);
		if (party == Bob)     player = new PlayerTwo(com, inputRange);
		if (party == Charlie) player = new PlayerThree(com, inputRange);
		if (party == David)   player = new PlayerFour(com, inputRange);

		player->MACGen = MACGen;

		std::vector<std::vector<int64_t> > output = player->Run(lc, input, inputMAC, sharedBeta, outputRange, elementsPerNode, rng);


		auto ApplyDone = high_resolution_clock::now();
		auto ApplyDuration = duration_cast<milliseconds>(ApplyDone - GatherDone);


   // **************************  Scatter **********************************************************************************************************************************
		cout << "------- Scattered Edges -------" << endl;
		Secret_Edge * ScatteredEdges = new Secret_Edge[nEdges];
		if (party == Alice || party == Bob)
		{
			ScatteredEdges = Scatter(party, partner, OpenedVertexIds, shuffledEdges, nEdges, secret_nodes, nUsers, nItems, direction);
		}
		// cout << "------- Scattered Edges -------" << endl;
		// printEdges(ScatteredEdges, nEdges, party, Alice, Bob);

		auto ScatterDone = high_resolution_clock::now();
		auto ScatterDuration = duration_cast<milliseconds>(ScatterDone - ApplyDone);
		
		delete[] secret_nodes;
		// delete[] secret_edges;


   // **************************  Timing **********************************************************************************************************************************
		
		// std::cout << "Total data sent:      " << com->sendingSize << " bytes" << std::endl;
		// std::cout << "Total data received : " << com->receivingSize << " bytes" << std::endl;

		std::cout << "dataSentReceived" << " " << com->sendingSize << " " << com->receivingSize << std::endl;
		cout << "phase" << " " << ShuffleDuration.count() << " " << GatherDuration.count() << " " << ApplyDuration.count() <<" " << ScatterDuration.count() << endl;
		t.Tick("GraphComputation() ");
	}
	

	void GenerateNodes(Secret_Node * nodes, int nUsers, int nItems) 
	{
		// std::cout << "Generate nodes:" << std::endl;
		/* vertex populating */
		// User nodes
		int firstLightMachineID_User = totalUsers % totalMachines;
		for (int i = 0; i < nUsers; i++)
		{
			if (machineId < firstLightMachineID_User)
			{
				nodes[i].vertexID = machineId*nUsers + i + 1;
			}
			else
			{
				// Light machine has 1 less user: offset = machineId*nUsers + firstLightMachineID_User
				nodes[i].vertexID = (machineId*nUsers + firstLightMachineID_User) + i + 1;
			}
			
			// std::cout << "idx: " << i << "\t" << nodes[i].vertexID << std::endl;
		}

		// Item nodes
		int firstLightMachineID_Item = totalItems % totalMachines;
		for (int i = 0; i < nItems; i++)
		{
			if (machineId < firstLightMachineID_Item)
			{
				nodes[i + nUsers].vertexID = totalUsers + machineId*nItems + i + 1;
			}
			else
			{
				nodes[i + nUsers].vertexID = totalUsers + (machineId*nItems + firstLightMachineID_Item) + i + 1;
			}
			
			// std::cout << "idx: " << i << "\t" << nodes[i + nUsers].vertexID << std::endl;
		}
	}
	
	void GenerateEdges(Secret_Edge* edges, int nEdges, int nItems, int alpha) 
	{

		unsigned seed =  std::chrono::system_clock::now().time_since_epoch().count();
		default_random_engine  generator_int(seed);
		uniform_int_distribution<uint64_t>  randUserId(1, totalUsers);
		uniform_int_distribution<uint64_t>  randItemId(totalUsers+1, totalUsers+totalItems);
		uniform_int_distribution<uint64_t>  randRate(1, 5);

		/* edge populating */
		int q = 0;  // index of all edges, first dummies then reals
		for (int i = 0; i < nItems; i++) // fill the first 2alpha edges with dummy ones
		{	
			for (int j = 0; j < (2*alpha); j++)
			{
				edges[q].id_u = randUserId(generator_int);
				edges[q].id_v = randItemId(generator_int);//nodeID[i + nUsers];  // we could generate them as random as well, but to avoid redundancy we do this
				edges[q].rating = randRate(generator_int);
				edges[q].isReal = 0;   // false ~ dummy edges
				for(int idx = 0; idx < dimension; idx++)
				{
					edges[q].profile_u[idx] = (1<<15);
					edges[q].profile_v[idx] = (1<<15);
				}
				q++;
			}
		}

		for (int k = q; k < nEdges; k++) // fill the rest of the edges with real ones
		{
			edges[k].id_u = randUserId(generator_int);
			edges[k].id_v = randItemId(generator_int);
			edges[k].rating = randRate(generator_int);
			edges[k].isReal = (1/* << 20*/);   // true ~ real edges
			for(int idx = 0; idx < dimension; idx++)
			{
				edges[k].profile_u[idx] = (1<<15);
				edges[k].profile_v[idx] = (1<<15);
			}
		}
	}

	void rerandomize(int party, Secret_Edge *secret_edges, int nEdges)
	{
		std::vector<unsigned char> seed = machine->getSharedRandomSeed(party, partner, com);
		AESRNG *rng = new AESRNG(seed.data());
		
		std::vector<uint64_t> randUInt16 = rng->GetUInt64Array(2*nEdges);
		std::vector<uint64_t> randUInt64 = rng->GetUInt64Array((2*dimension + 2)*nEdges);
		
		int counter16 = 0;
		int counter64 = 0;
		
		if (party % 2 == 0)
		{
			for (int edx = 0; edx < nEdges; edx++) 
			{
				secret_edges[edx].id_u += randUInt16[counter16]; counter16++;
				secret_edges[edx].id_v += randUInt16[counter16]; counter16++;
				
				for(int kdx = 0; kdx < dimension; kdx++)
				{
					secret_edges[edx].profile_u[kdx] += randUInt64[counter64]; counter64++;
					secret_edges[edx].profile_v[kdx] += randUInt64[counter64]; counter64++;
				}
				secret_edges[edx].rating += randUInt64[counter64]; counter64++;
				secret_edges[edx].isReal += randUInt64[counter64]; counter64++;
			}
		}
		else
		{
			for (int edx = 0; edx < nEdges; edx++) 
			{
				secret_edges[edx].id_u -= randUInt16[counter16]; counter16++;
				secret_edges[edx].id_v -= randUInt16[counter16]; counter16++;
				
				for(int kdx = 0; kdx < dimension; kdx++)
				{
					secret_edges[edx].profile_u[kdx] -= randUInt64[counter64]; counter64++;
					secret_edges[edx].profile_v[kdx] -= randUInt64[counter64]; counter64++;
				}
				secret_edges[edx].rating -= randUInt64[counter64]; counter64++;
				secret_edges[edx].isReal -= randUInt64[counter64]; counter64++;
			}
		}
	}
	
	void rerandomize(int party, std::vector<uint64_t>& MACedEdges, int nEdges)
	{
		std::vector<unsigned char> seed = machine->getSharedRandomSeed(party, partner, com);
		AESRNG *rng = new AESRNG(seed.data());
		
		std::vector<uint64_t> randUInt64 = rng->GetUInt64Array(nEdges);
		if (party % 2 == 0)
		{
			for (int edx = 0; edx < nEdges; edx++) 
			{
				MACedEdges[edx] = (MACedEdges[edx] + randUInt64[edx])/* % (0x10000000000 - 293)*/;
			}
		}
		else
		{
			for (int edx = 0; edx < nEdges; edx++) 
			{
				MACedEdges[edx] = (MACedEdges[edx] - randUInt64[edx])/* % (0x10000000000 - 293)*/;
			}
		}
	}
	
	void SecretSharing(int party, int partner, Secret_Edge* secret_edges, int nEdges, Secret_Node* secret_nodes, int nUsers, int nItems, int alpha)
	{

		// cout << "secret_edges: " << secret_edges[0].profile_u[0] << endl;
		
		// 		std::vector<unsigned char> seed = machine->getSharedRandomSeed(party, partner);
		
		// 		AESRNG *rng = new AESRNG(seed.data());
		
		std::vector<uint64_t> randUInt16 = rng->GetUInt64Array(nUsers + nItems + 2*nEdges);
		std::vector<uint64_t> randUInt64 = rng->GetUInt64Array(2*dimension*(nUsers + nItems) + (2*dimension + 2)*nEdges);
		
		// 		TestUtility::PrintVector(randUInt64, "rand 64");
		
		if ((party == Alice) || (party == Charlie))
		{
			
			int counter16 = 0;
			int counter64 = 0;
			
			for (int i = 0; i < (nUsers+nItems); i++)
			{
				secret_nodes[i].vertexID = randUInt16[counter16]; counter16++;
				
				for(int kdx = 0; kdx < dimension; kdx++)
				{
					secret_nodes[i].Profile[kdx] = randUInt64[counter64]; counter64++;
					secret_nodes[i].newProfile[kdx] = randUInt64[counter64]; counter64++;
				}
			}
			
			for (int i = 0; i < nEdges; i++) 
			{
				secret_edges[i].id_u = randUInt16[counter16]; counter16++;
				secret_edges[i].id_v = randUInt16[counter16]; counter16++;
				
				for(int kdx = 0; kdx < dimension; kdx++)
				{
					secret_edges[i].profile_u[kdx] = randUInt64[counter64]; counter64++;
					secret_edges[i].profile_v[kdx] = randUInt64[counter64]; counter64++;
				}
				secret_edges[i].rating = randUInt64[counter64]; counter64++;
				secret_edges[i].isReal = randUInt64[counter64]; counter64++;
			}
		}
		else if ((party == Bob) || (party == David))
		{
			GenerateNodes(secret_nodes, nUsers, nItems);
			
			int counter16 = 0;
			int counter64 = 0;
			
			for (int i = 0; i < (nUsers+nItems); i++)
			{
				secret_nodes[i].vertexID = (secret_nodes[i].vertexID -  randUInt16[counter16]); counter16++;

				for (int j = 0; j < dimension; j++)
				{
					secret_nodes[i].Profile[j] = (secret_nodes[i].Profile[j] - randUInt64[counter64]); counter64++;
					secret_nodes[i].newProfile[j] = (secret_nodes[i].newProfile[j] - randUInt64[counter64]); counter64++;
				}
			}

			GenerateEdges(secret_edges, nEdges, nItems, alpha);
			
			for (int i = 0; i < nEdges; i++) 
			{
				secret_edges[i].id_u = (secret_edges[i].id_u - randUInt16[counter16]); counter16++;
				secret_edges[i].id_v = (secret_edges[i].id_v - randUInt16[counter16]); counter16++;
				
				for (int j = 0; j < dimension; j++)
				{
					secret_edges[i].profile_u[j] = (secret_edges[i].profile_u[j] - randUInt64[counter64]); counter64++;
					secret_edges[i].profile_v[j] = (secret_edges[i].profile_v[j] - randUInt64[counter64]); counter64++;
				}
				
				secret_edges[i].rating = (secret_edges[i].rating - randUInt64[counter64]); counter64++;
				secret_edges[i].isReal = (secret_edges[i].isReal - randUInt64[counter64]); counter64++;
			}
		}
	}

	template <typename T>
	T * Shuffle(int party, int partner, const std::vector<int>& shuffledArray, T* data, int size)
	{
		/* Shuffling starts */
		T * shuffledData = new T[size];
		int srcMachine = 0, srcIndex = 0, dstMachine = 0, dstIndex = 0;
		
		std::vector<std::vector<int> > recvIndex(totalMachines);
		std::vector<std::vector<T> > sendBuffer(totalMachines);
		std::vector<std::vector<T> > recvBuffer(totalMachines);


		cout << machineId << " inside shuffle " << endl;


		for (int i = 0; i < totalEdges; i++) 
		{
			srcMachine = i / size;
			srcIndex   = i % size;
			dstMachine = (shuffledArray[i] / size);  // find where is the destination machine
			dstIndex   = (shuffledArray[i] % size);  // find where is the local index in destination machine


			// cout << machineId << " srcMachine " << srcMachine << " srcIndex " << srcIndex << " dstMachine " << dstMachine << " dstIndex " << dstIndex << " size " << size << endl;			
			
			if (machineId == srcMachine && machineId == dstMachine)
			{
				shuffledData[dstIndex] = data[srcIndex];
			}
			else {
				if (machineId == srcMachine)
				{
					sendBuffer[dstMachine].push_back(data[srcIndex]);
				}
				else if (machineId == dstMachine)
				{
					recvIndex[srcMachine].push_back(dstIndex);
				}
			}
		}

		/* Sending to Lower Machine & Receiving from Ups to avoid DOSing the machines */
		for (int m = 0; m < machineId; m++)
		{
			// cout << "Sending halfedge info to Upper Machines... " << counter[m] << endl;
			if(sendBuffer[m].size() > 0)
			{
				cout << machineId << " ----> " << m << endl;
				machine->sendToPeer(machineId, m, (unsigned char *)(sendBuffer[m].data()), sendBuffer[m].size()*sizeof(T));
			}
		}



		for (int n = (machineId+1); n < totalMachines; n++) 
		{
			// cout << "Receiving halfedge info from Lower Machines..." << endl;
			// if (machineId==0) cout << "recvIndex[n].size()..." << recvIndex[n].size() << endl;
			if (recvIndex[n].size() > 0)
			{	
				std::vector<T> recvData(recvIndex[n].size());
				cout << machineId << " <---- " << n << endl;
				machine->receiveFromPeer(machineId, n, (unsigned char *)(recvData.data()), recvIndex[n].size()*sizeof(T));
				
				

				for(int cdx = 0; cdx < recvIndex[n].size(); cdx++)
				{
					// if(recvIndex[n][cdx] >= size) std::cout << machineId << ": recvIndex[n] = " << recvIndex[n][cdx] << "***********************" << endl;
					shuffledData[recvIndex[n][cdx]] = recvData[cdx];
				}
			}
		}

		for (int q = 0; q < machineId; q++) 
		{
			// cout << "Receiving halfedge info from Lower Machines..." << endl;
			if (recvIndex[q].size() > 0)
			{	
				std::vector<T> recvData(recvIndex[q].size());
				cout << machineId << " <---- " << q << endl;
				machine->receiveFromPeer(machineId, q, (unsigned char *)(recvData.data()), recvIndex[q].size()*sizeof(T));
				
				for(int cdx = 0; cdx < recvIndex[q].size(); cdx++)
				{
					// if(recvIndex[q][cdx] >= size) std::cout << machineId << ": recvIndex[q] = " << recvIndex[q][cdx] << "***********************" << endl;
					shuffledData[recvIndex[q][cdx]] = recvData[cdx];
				}
			}
		}

		/* Sending to Upper Machine & Receiving from Downs to avoid DOSing the machines */
		for (int p = (machineId+1); p < totalMachines; p++)
		{
			// cout << "Sending halfedge info to Upper Machines... " << counter[p] << endl;
			// cout << "Sending halfedge info to Upper Machines... " << counter[m] << endl;
			if(sendBuffer[p].size() > 0)
			{
				cout << machineId << " ----> " << p << endl;
				machine->sendToPeer(machineId, p, (unsigned char *)(sendBuffer[p].data()), sendBuffer[p].size()*sizeof(T));
			}
		}

		return shuffledData;


		// for (int i = 0; i < totalEdges; i++) 
		// {
		// 	srcMachine = i / size;
		// 	srcIndex   = i % size;
		// 	dstMachine = (shuffledArray[i] / size);  // find where is the destination machine
		// 	dstIndex   = (shuffledArray[i] % size);  // find where is the local index in destination machine
			
		// 	if (machineId == srcMachine && machineId == dstMachine)
		// 	{
		// 		shuffledData[dstIndex] = data[srcIndex];
		// 	}
		// 	else {
		// 		if (machineId == srcMachine)
		// 		{
		// 			machine->sendToPeer(srcMachine, dstMachine, (unsigned char *)(&data[srcIndex]), sizeof(T));
		// 		}
		// 		else if (machineId == dstMachine)
		// 		{
		// 			int size = machine->receiveFromPeer(dstMachine, srcMachine, (unsigned char *)(&shuffledData[dstIndex]), sizeof(T));
		// 			assert(size == sizeof(T));
		// 		}
		// 	}
		// }
		
		// return shuffledData;
	}
	
	
	std::vector<uint64_t> Gather(int party, int partner, Secret_Edge* edges, SecretEdgeMAC *bMACs, int nEdges, Secret_Node* nodes, std::vector<uint64_t>& MACId, int nUsers, int nItems, bool isEdgeIncoming)
	{
		int firstLightMachineID_Item = nItems % totalMachines;
		int firstLightMachineID_User = nUsers % totalMachines;
		
		int nHeavy_Item = (int)(ceil ( (double)totalItems / (double)totalMachines)); // no of types in heavy load machine;
		int nLight_Item = (int)(floor( (double)totalItems / (double)totalMachines)); // no of types in light load machine;
		
		int nHeavy_User = (int)(ceil ( (double)totalUsers / (double)totalMachines)); // no of types in heavy load machine;
		int nLight_User = (int)(floor( (double)totalUsers / (double)totalMachines)); // no of types in light load machine;

		/// Store the gather data
		Half_Secret_Edge edgeData;
		
		uint64_t vertexId = -1; //, receiverMachine = -1, receiverLocalIndex = -1;

		uint64_t counter[totalMachines] = {0};
		vector<vector<uint64_t>> edgeTracker (totalMachines);
		vector<vector<uint64_t>> indexTracker (totalMachines);

		vector<uint64_t> OpenedVertexIds(nEdges);
		MACId.resize(nEdges);
		vector<uint64_t> receiverLocalIndex(nEdges), receiverMachine(nEdges);
		
		for(int idx = 0; idx < nEdges; idx++)
		{
			receiverLocalIndex[idx] = -1;
			receiverMachine[idx] = -1;
		}

		if(isEdgeIncoming)
		{
			std::vector<uint64_t> edgesIdv(nEdges), theirIdv(nEdges);
			
			for(int idx = 0; idx < nEdges; idx++)
			{
				edgesIdv[idx] = edges[idx].id_v;
				MACId[idx] = bMACs[idx].id_v;
			}
			
			if(party % 2 == 0)
			{
				com->SendEvaluationPartner((unsigned char *)(edgesIdv.data()), nEdges*sizeof(uint64_t));
				com->AwaitEvaluationPartner((unsigned char *)(theirIdv.data()), nEdges*sizeof(uint64_t));
			}
			else
			{
				com->AwaitEvaluationPartner((unsigned char *)(theirIdv.data()), nEdges*sizeof(uint64_t));
				com->SendEvaluationPartner((unsigned char *)(edgesIdv.data()), nEdges*sizeof(uint64_t));
			}
			

			// if (machineId == 16) 
				// cout << machineId << ": A" << endl;

			// Reconstructed item Vertex Ids
			for(int idx = 0; idx < nEdges; idx++)
			{
				OpenedVertexIds[idx] = edgesIdv[idx] + theirIdv[idx];

				uint64_t vId = OpenedVertexIds[idx] - totalUsers;
				// if (machineId == 16) 
					// cout << machineId << ": vId =" << vId<< " , OpenedVertexIds = " << OpenedVertexIds[idx] << endl;
				
				if (vId > firstLightMachineID_Item*nHeavy_Item)
				{
					vId = vId - (firstLightMachineID_Item*nHeavy_Item);
					
					for (int i = 0; i < (totalMachines-firstLightMachineID_Item); i++) {
						if (vId <= nLight_Item) 
						{
							receiverMachine[idx] = i+firstLightMachineID_Item;
							if (receiverMachine[idx] < firstLightMachineID_User)
								receiverLocalIndex[idx] = nHeavy_User + vId - 1;
							else
								receiverLocalIndex[idx] = nLight_User + vId - 1;
							break;
						}
						else vId -= nLight_Item;
					}
					// if (machineId == 16) 
				 // 		cout << machineId << ": if" << endl;
				}
				else 
				{ 
					for (int j = 0; j < firstLightMachineID_Item; j++) {
						if (vId <= nHeavy_Item) {
							receiverMachine[idx] = j;
							if (receiverMachine[idx] < firstLightMachineID_User)
								receiverLocalIndex[idx] = nHeavy_User + vId - 1;
							else
								receiverLocalIndex[idx] = nLight_User + vId - 1;
							break;
						}
						else vId -= nHeavy_Item;
					}
					// if (machineId == 16) 
					// 	cout << machineId << ": else" << endl;
				}

				// if (machineId == 16) 
				// 	cout << machineId << ": idx= " << idx << endl;
			}

		}
		else if(!isEdgeIncoming)
		{
			std::vector<uint64_t> edgesIdu(nEdges), theirIdu(nEdges);
			
			for(int idx = 0; idx < nEdges; idx++)
			{
				edgesIdu[idx] = edges[idx].id_u;
				MACId[idx] = bMACs[idx].id_u;
			}
			
			if(party % 2 == 0)
			{
				com->SendEvaluationPartner((unsigned char *)(edgesIdu.data()), nEdges*sizeof(uint64_t));
				com->AwaitEvaluationPartner((unsigned char *)(theirIdu.data()), nEdges*sizeof(uint64_t));
			}
			else
			{
				com->AwaitEvaluationPartner((unsigned char *)(theirIdu.data()), nEdges*sizeof(uint64_t));
				com->SendEvaluationPartner((unsigned char *)(edgesIdu.data()), nEdges*sizeof(uint64_t));
			}
			
			// Reconstructed user Vertex Ids
			for(int idx = 0; idx < nEdges; idx++)
			{
				OpenedVertexIds[idx] = edgesIdu[idx] + theirIdu[idx];

				uint64_t uId = OpenedVertexIds[idx];
				
				if (uId > firstLightMachineID_User*nHeavy_User)
				{
					uId = uId - (firstLightMachineID_User*nHeavy_User);
					receiverMachine[idx] = firstLightMachineID_User + (uId-1)/nLight_User;
					receiverLocalIndex[idx] = (uId-1) % nLight_User;

					// for (int i = 0; i < (totalMachines-firstLightMachineID_User); i++) {
					// 	if (uId <= nLight_User) {
					// 		receiverMachine[idx] = i+firstLightMachineID_User;
					// 		receiverLocalIndex[idx] = uId - 1;
					// 		break;
					// 	}
					// 	else uId -= nLight_User;
					// }
				}
				else 
				{ 
					receiverMachine[idx] =  (uId-1)/nHeavy_User;
					receiverLocalIndex[idx] = (uId-1) % nHeavy_User;

					// for (int j = 0; j < firstLightMachineID_User; j++) {
					// 	if (uId <= nHeavy_User) {
					// 		receiverMachine[idx] = j;
					// 		receiverLocalIndex[idx] = uId - 1;
					// 		break;
					// 	}
					// 	else uId -= nHeavy_Item;
					// }
				}
			}
		}

		
		

		for(int idx = 0; idx < nEdges; idx++)
		{
			if (machineId == receiverMachine[idx])
			{
				for (int i = 0; i < dimension; i++)
				{
					edgeData.profile_u[i] = edges[idx].profile_u[i];
					edgeData.profile_v[i] = edges[idx].profile_v[i];
				}
				if (receiverLocalIndex[idx]== -1) cout << machineId << ": receiverLocalIndex[idx]= " << receiverLocalIndex[idx] << endl;
				edgeData.rating = edges[idx].rating;
				edgeData.isReal = edges[idx].isReal;
				edgeData.bMACs  = bMACs[idx];
				nodes[receiverLocalIndex[idx]].halfEdge.push_back(edgeData);
			}
			else
			{	
				if (receiverMachine[idx]== -1) cout << machineId << ": receiverMachine[idx]= " << receiverMachine[idx] << endl;
				if (receiverLocalIndex[idx]== -1) cout << machineId << ": receiverLocalIndex[idx]= " << receiverLocalIndex[idx] << endl;
				++counter[receiverMachine[idx]];
				edgeTracker[receiverMachine[idx]].push_back(idx);
				indexTracker[receiverMachine[idx]].push_back(receiverLocalIndex[idx]);
			}
		}

		// Allocate buffers for each receiver machine
		std::vector<Half_Secret_Edge *> buffers(totalMachines);
		for(int idx = 0; idx < totalMachines; idx++)
		{
			if(machineId != idx)
			{
				buffers[idx] = new Half_Secret_Edge[counter[idx]];
			
				for(int kdx = 0; kdx < counter[idx]; kdx++)
				{
					int index = edgeTracker[idx][kdx];
					
					for(int i = 0; i < dimension; i++)
					{
						edgeData.profile_u[i] = edges[index].profile_u[i];
						edgeData.profile_v[i] = edges[index].profile_v[i];
					}
					edgeData.rating = edges[index].rating;
					edgeData.isReal = edges[index].isReal;
					edgeData.bMACs  = bMACs[index];
					
					buffers[idx][kdx] = edgeData;
				}
			}
		}


		
		/* Sending to Upper Machine to avoid DOSing the machines */
		for (uint64_t m = 0; m < machineId; m++)
		{
			cout << machineId << " -----> " << m << endl;
			machine->sendToPeer(machineId, m, (unsigned char *)(&counter[m]), sizeof(uint64_t));
			if(counter[m] > 0)
			{
				machine->sendToPeer(machineId, m, (unsigned char *)(indexTracker[m].data()), indexTracker[m].size()*sizeof(uint64_t));
				machine->sendToPeer(machineId, m, (unsigned char *)(buffers[m]), counter[m]*sizeof(Half_Secret_Edge));
			}
		}

		/* Receiving from Lowers  */
		for (uint64_t n = (machineId+1); n < totalMachines; n++) 
		{
			cout << machineId << " <----- " << n << endl;
			uint64_t count;// = -1;
			machine->receiveFromPeer(machineId, n, (unsigned char *)(&count), sizeof(uint64_t));
			if (count > 0)
			{	
				std::vector<uint64_t> localIndex(count);
				Half_Secret_Edge *hse = new Half_Secret_Edge[count];
				
				machine->receiveFromPeer(machineId, n, (unsigned char *)(localIndex.data()), count*sizeof(uint64_t));
				machine->receiveFromPeer(machineId, n, (unsigned char *)(hse), count*sizeof(Half_Secret_Edge));
				
				for(uint64_t cdx = 0; cdx < count; cdx++)
				{
					nodes[localIndex[cdx]].halfEdge.push_back(hse[cdx]);
				}
				delete[] hse;
			}
		}

		/* Receiving from Upper */
		for (uint64_t q = 0; q < machineId; q++) 
		{
			cout << machineId << " <----- " << q << endl;
			uint64_t count; // = -1;
			machine->receiveFromPeer(machineId, q, (unsigned char *)(&count), sizeof(uint64_t));
			if (count > 0)
			{	
				std::vector<uint64_t> localIndex(count);
				Half_Secret_Edge *hse = new Half_Secret_Edge[count];
				
				machine->receiveFromPeer(machineId, q, (unsigned char *)(localIndex.data()), count*sizeof(uint64_t));
				machine->receiveFromPeer(machineId, q, (unsigned char *)(hse), count*sizeof(Half_Secret_Edge));
				
				for(int cdx = 0; cdx < count; cdx++)
				{
					nodes[localIndex[cdx]].halfEdge.push_back(hse[cdx]);
				}
				
				delete[] hse;
			}
		}

		/* Sending to Lower Machine to avoid DOSing the machines */
		for (uint64_t p = (machineId+1); p < totalMachines; p++)
		{
			cout << machineId << " -----> " << p << endl;
			machine->sendToPeer(machineId, p, (unsigned char *)(&counter[p]), sizeof(uint64_t));
			if(counter[p] > 0)
			{
				machine->sendToPeer(machineId, p, (unsigned char *)(indexTracker[p].data()), indexTracker[p].size()*sizeof(uint64_t));
				machine->sendToPeer(machineId, p, (unsigned char *)(buffers[p]), counter[p]*sizeof(Half_Secret_Edge));
			}
		}



		return OpenedVertexIds;
		
		// **************************************************************************************************//
	}

	std::vector<std::vector<int64_t> > getSharedMaskedInput(Secret_Node *nodes, int nNodes, int nUsers, int inputSize, bool isEdgeIncoming)
	{
		std::vector<std::vector<int64_t> > input(inputSize);
		
		if(isEdgeIncoming)
		{
			uint64_t count = 0;
			
			for(uint64_t idx = nUsers; idx < nNodes; idx++)
			{
				// Get v (v is the same for all edges)
				input[count].resize(dimension);
				for(uint64_t kdx = 0; kdx < dimension; kdx++)
				{
					input[count][kdx] = (int64_t)(nodes[idx].halfEdge[0].profile_v[kdx]);
				}
				count++;
				
				// Get u, rating, isReal on each edge
				for(uint64_t cdx = 0; cdx < nodes[idx].halfEdge.size(); cdx++)
				{
					input[count].resize(dimension);
					for(int kdx = 0; kdx < dimension; kdx++)
					{
						input[count][kdx] = nodes[idx].halfEdge[cdx].profile_u[kdx];
					}	
					count++;
					
					input[count].resize(1);
					input[count][0] = nodes[idx].halfEdge[cdx].rating;
					count++;
					
					input[count].resize(1);
					input[count][0] = nodes[idx].halfEdge[cdx].isReal;
					count++;
				}
			}
		}
		else
		{
			int count = 0;
			
			for(int idx = 0; idx < nUsers; idx++)
			{
				// Get u (u is the same for all edges)
				input[count].resize(dimension);
				for(int kdx = 0; kdx < dimension; kdx++)
				{
					input[count][kdx] = nodes[idx].halfEdge[0].profile_u[kdx];
				}
				count++;
				
				// Get v, rating, isReal on each edge
				for(int cdx = 0; cdx < nodes[idx].halfEdge.size(); cdx++)
				{
					input[count].resize(dimension);
					for(int kdx = 0; kdx < dimension; kdx++)
					{
						input[count][kdx] = nodes[idx].halfEdge[cdx].profile_v[kdx];
					}	
					count++;
					
					input[count].resize(1);
					input[count][0] = nodes[idx].halfEdge[cdx].rating;
					count++;
					
					input[count].resize(1);
					input[count][0] = nodes[idx].halfEdge[cdx].isReal;
					count++;
				}
			}
		}
		
		return input;
	}

	std::vector<std::vector<uint64_t> > getBetaMAC(Secret_Node *nodes, int nNodes, int nUsers, int inputSize, bool isEdgeIncoming)
	{
		std::vector<std::vector<uint64_t> > input(inputSize);
		
		if(isEdgeIncoming)
		{
			int count = 0;
			
			for(int idx = nUsers; idx < nNodes; idx++)
			{
				// Get bMACs for profile_v (v is the same for all edges)
				input[count].resize(dimension);
				for(int kdx = 0; kdx < dimension; kdx++)
				{
					input[count][kdx] = (nodes[idx].halfEdge[0].bMACs.profile_v[kdx]);
				}
				count++;
				
				// Get bMACs for u, rating, isReal on each edge
				for(int cdx = 0; cdx < nodes[idx].halfEdge.size(); cdx++)
				{
					input[count].resize(dimension);
					for(int kdx = 0; kdx < dimension; kdx++)
					{
						input[count][kdx] = nodes[idx].halfEdge[cdx].bMACs.profile_u[kdx];
					}	
					count++;
					
					input[count].resize(1);
					input[count][0] = nodes[idx].halfEdge[cdx].bMACs.rating;
					count++;
					
					input[count].resize(1);
					input[count][0] = nodes[idx].halfEdge[cdx].bMACs.isReal;
					count++;
				}
			}
		}
		else
		{
			int count = 0;
			
			for(int idx = 0; idx < nUsers; idx++)
			{
				// Get bMACs for u (u is the same for all edges)
				input[count].resize(dimension);
				for(int kdx = 0; kdx < dimension; kdx++)
				{
					input[count][kdx] = nodes[idx].halfEdge[0].bMACs.profile_u[kdx];
				}
				count++;
				
				// Get v, rating, isReal on each edge
				for(int cdx = 0; cdx < nodes[idx].halfEdge.size(); cdx++)
				{
					input[count].resize(dimension);
					for(int kdx = 0; kdx < dimension; kdx++)
					{
						input[count][kdx] = nodes[idx].halfEdge[cdx].bMACs.profile_v[kdx];
					}	
					count++;
					
					input[count].resize(1);
					input[count][0] = nodes[idx].halfEdge[cdx].bMACs.rating;
					count++;
					
					input[count].resize(1);
					input[count][0] = nodes[idx].halfEdge[cdx].bMACs.isReal;
					count++;
				}
			}
		}
		
		return input;
	}
	
	Secret_Edge* Scatter(int party, int partner, vector<uint64_t> OpenedVertexIds, Secret_Edge* edges, int nEdges, Secret_Node* nodes, int nUsers, int nItems, bool isEdgeIncoming)
	{
		int firstLightMachineID_Item = nItems % totalMachines;
		int firstLightMachineID_User = nUsers % totalMachines;
		
		int nHeavy_Item = (int)(ceil ( (double)totalItems / (double)totalMachines)); // no of types in heavy load machine;
		int nLight_Item = (int)(floor( (double)totalItems / (double)totalMachines)); // no of types in light load machine;
		
		int nHeavy_User = (int)(ceil ( (double)totalUsers / (double)totalMachines)); // no of types in heavy load machine;
		int nLight_User = (int)(floor( (double)totalUsers / (double)totalMachines)); // no of types in light load machine;

		vector<uint64_t> receiverLocalIndex(nEdges), receiverMachine(nEdges);
		
		for(int idx = 0; idx < nEdges; idx++)
		{
			receiverLocalIndex[idx] = -1;
			receiverMachine[idx] = -1;
		}

		
		uint64_t index = -1;

		std::vector<UpdateNode *> updateVecs(totalMachines);

		
		std::vector <std::vector <uint64_t>> allProfiles;

		if(isEdgeIncoming)
		{
			for(int idx = 0; idx < totalMachines; idx++)
			{
				updateVecs[idx] = new UpdateNode[nItems];
			}

			// Write updated profiles for nodes on our machine
			for(int idx = 0; idx < nItems; idx++)
			{
				for(int d = 0; d < dimension; d++)
				{
					updateVecs[machineId][idx].newProfile[d] = nodes[nUsers + 1 + idx].newProfile[d];
				}
			}

			// Communicate to send our update profiles to others and to receiver from others
			/* Sending to Upper Machine & Receiving from Downs to avoid DOSing the machines */
			for (int m = 0; m < machineId; m++)
			{
				cout << machineId << " ---> " << m << endl;
				// cout << machineId << " send " << sizeof(uint64_t) << "bytes" << endl;

				// machine->sendToPeer(machineId, m, (unsigned char *)(&tempId), sizeof(uint64_t));
				// for (int d = 0; d < dimension; d++)
				// {
					// cout << machineId << " send " << sizeof(uint64_t) << "bytes" << endl;
					machine->sendToPeer(machineId, m, (unsigned char *)(updateVecs[machineId]), nItems*sizeof(UpdateNode));
				// }
			}

			// cout << machineId << ": test1" << endl;

			for (int n = (machineId+1); n < totalMachines; n++) 
			{
				cout << machineId << " <--- " << n << endl;
				// machine->receiveFromPeer(machineId, n, (unsigned char *)(&index), sizeof(uint64_t));
				// for (int d = 0; d < dimension; d++)
					machine->receiveFromPeer(machineId, n, (unsigned char *)(updateVecs[n]), nItems*sizeof(UpdateNode));
			}

			// cout << machineId << ": test2" << endl;

			for (int p = 0; p < machineId; p++) 
			{
				// cout << "Receiving vertexID from Upper Machines..." << endl;
				cout << machineId << " <--- " << p << endl;
				// machine->receiveFromPeer(machineId, p, (unsigned char *)(&index), sizeof(uint64_t));
				// for (int d = 0; d < dimension; d++)
					machine->receiveFromPeer(machineId, p, (unsigned char *)(updateVecs[p]), nItems*sizeof(UpdateNode));
			}

			/* Sending to Lower Machine & Receiving from Uppers to avoid DOSing the machines */
			for (int q = (machineId+1); q < totalMachines; q++)
			{
				cout << machineId << " ---> " << q << endl;
				// machine->sendToPeer(machineId, p, (unsigned char *)(&tempId), sizeof(uint64_t));
				// for (int d = 0; d < dimension; d++)
					machine->sendToPeer(machineId, q, (unsigned char *)(updateVecs[machineId]), nItems*sizeof(UpdateNode));
			}
		}

		
			// Reconstructed Vertex Ids
			for(int idx = 0; idx < nEdges; idx++)
			{
				// OpenedVertexIds[idx] = edgesIdv[idx] + theirIdv[idx];

				uint64_t vId = OpenedVertexIds[idx] - totalUsers;
				// if (machineId == 16) 
					// cout << machineId << ": vId =" << vId<< " , OpenedVertexIds = " << OpenedVertexIds[idx] << endl;
				
				if (vId > firstLightMachineID_Item*nHeavy_Item)
				{
					vId = vId - (firstLightMachineID_Item*nHeavy_Item);
					
					for (int i = 0; i < (totalMachines-firstLightMachineID_Item); i++) {
						if (vId <= nLight_Item) 
						{
							receiverMachine[idx] = i+firstLightMachineID_Item;
							if (receiverMachine[idx] < firstLightMachineID_User)
								receiverLocalIndex[idx] = nHeavy_User + vId - 1;
							else
								receiverLocalIndex[idx] = nLight_User + vId - 1;
							break;
						}
						else vId -= nLight_Item;
					}
					// if (machineId == 16) 
				 // 		cout << machineId << ": if" << endl;
				}
				else 
				{ 
					for (int j = 0; j < firstLightMachineID_Item; j++) {
						if (vId <= nHeavy_Item) {
							receiverMachine[idx] = j;
							if (receiverMachine[idx] < firstLightMachineID_User)
								receiverLocalIndex[idx] = nHeavy_User + vId - 1;
							else
								receiverLocalIndex[idx] = nLight_User + vId - 1;
							break;
						}
						else vId -= nHeavy_Item;
					}
					// if (machineId == 16) 
					// 	cout << machineId << ": else" << endl;
				}

				
				// if (machineId == 16) 
				// 	cout << machineId << ": idx= " << idx << endl;
			}

			if(isEdgeIncoming)
			{
				for(int idx = 0; idx < nEdges; idx++)
				{
					for (int d = 0; d < dimension; d++)
					{
						edges[idx].profile_v[d] = updateVecs[receiverMachine[idx]][receiverLocalIndex[idx]-nUsers-1].newProfile[d];
					}
				}
			}
			cout << "Scatter Done!" << endl;

		// if (isEdgeIncoming) 
		// {
		// 	// std::vector <std::vector <float>> allProfiles(totalItems, vector< float> (dimension));
		// 	allProfiles.resize(totalItems); //, std::vector<uint64_t>(dimension));
		// 	for(int idx = 0; idx < allProfiles.size(); idx++) allProfiles[idx].resize(dimension);


		// 	for(int t = nUsers; t < (nUsers+nItems); t++)  // [nUsers+0 .. nUsers+nItems] is the range for item indices
		// 	{	
		// 		for (int d = 0; d < dimension; d++)
		// 		{
		// 			allProfiles[OpenedVertexIds[t]-totalUsers-1][d] = nodes[t].newProfile[d];
		// 		}
		// 		uint64_t tempId = OpenedVertexIds[t]-totalUsers-1;
			
		// 		uint64_t temp;
		// 		// cout << "totalItems: " << totalItems << endl;

		// 		/* Sending to Upper Machine & Receiving from Downs to avoid DOSing the machines */
		// 		for (int m = 0; m < machineId; m++)
		// 		{
		// 			cout << machineId << " ---> " << m << endl;
		// 			// cout << machineId << " send " << sizeof(uint64_t) << "bytes" << endl;

		// 			machine->sendToPeer(machineId, m, (unsigned char *)(&tempId), sizeof(uint64_t));
		// 			for (int d = 0; d < dimension; d++)
		// 			{
		// 				// cout << machineId << " send " << sizeof(uint64_t) << "bytes" << endl;
		// 				machine->sendToPeer(machineId, m, (unsigned char *)(&nodes[t].newProfile[d]), sizeof(uint64_t));
		// 			}
		// 		}

		// 		// cout << machineId << ": test1" << endl;

		// 		for (int n = (machineId+1); n < totalMachines; n++) 
		// 		{
		// 			cout << machineId << " <--- " << n << endl;
		// 			machine->receiveFromPeer(machineId, n, (unsigned char *)(&index), sizeof(uint64_t));
		// 			for (int d = 0; d < dimension; d++)
		// 				machine->receiveFromPeer(machineId, n, (unsigned char *)(&allProfiles[index][d]), sizeof(uint64_t));
		// 		}

		// 		// cout << machineId << ": test2" << endl;

		// 		for (int p = 0; p < machineId; p++) 
		// 		{
		// 			// cout << "Receiving vertexID from Upper Machines..." << endl;
		// 			cout << machineId << " <--- " << p << endl;
		// 			machine->receiveFromPeer(machineId, p, (unsigned char *)(&index), sizeof(uint64_t));
		// 			for (int d = 0; d < dimension; d++)
		// 				machine->receiveFromPeer(machineId, p, (unsigned char *)(&allProfiles[index][d]), sizeof(uint64_t));
		// 		}

		// 		/* Sending to Lower Machine & Receiving from Uppers to avoid DOSing the machines */
		// 		for (int p = (machineId+1); p < totalMachines; p++)
		// 		{
		// 			cout << machineId << " ---> " << p << endl;
		// 			machine->sendToPeer(machineId, p, (unsigned char *)(&tempId), sizeof(uint64_t));
		// 			for (int d = 0; d < dimension; d++)
		// 				machine->sendToPeer(machineId, p, (unsigned char *)(&nodes[t].newProfile[d]), sizeof(uint64_t));
		// 		}
		// 		// cout << machineId << ": test4" << endl;
		// 	}
		// }

		// else if (!isEdgeIncoming) 
		// {
		// 	// std::vector <std::vector <float>> allProfiles(totalItems, vector< float> (dimension));
		// 	allProfiles.resize(totalUsers, std::vector<uint64_t>(dimension));

		// 	for(int t = 0; t < nUsers; t++)  // [nUsers+0 .. nUsers+nItems] is the range for item indices
		// 	{	
		// 		for (int d = 0; d < dimension; d++)
		// 			allProfiles[nodes[t].vertexID-1][d] = nodes[t].newProfile[d];

		// 		/* Sending to Upper Machine & Receiving from Downs to avoid DOSing the machines */
		// 		for (int m = 0; m < machineId; m++)
		// 		{
		// 			// cout << "Sending vertexID to Upper Machines... " << endl;
		// 			machine->sendToPeer(machineId, m, (unsigned char *)(&nodes[t].vertexID), sizeof(uint64_t));
		// 			for (int d = 0; d < dimension; d++)
		// 				machine->sendToPeer(machineId, m, (unsigned char *)(&nodes[t].newProfile[d]), sizeof(nodes[t].newProfile[d]));
		// 		}

		// 		for (int m = (machineId+1); m < totalMachines; m++) 
		// 		{
		// 			// cout << "Receiving vertexID from Lower Machines..." << endl;
		// 			int size = machine->receiveFromPeer(machineId, m, (unsigned char *)(&index), sizeof(uint64_t));
		// 			for (int d = 0; d < dimension; d++)
		// 				int size = machine->receiveFromPeer(machineId, m, (unsigned char *)(&allProfiles[index-1][d]), sizeof(uint64_t));
		// 		}

		// 		for (int p = 0; p < machineId; p++) 
		// 		{
		// 			// cout << "Receiving vertexID from Upper Machines..." << endl;
		// 			int size = machine->receiveFromPeer(machineId, p, (unsigned char *)(&index), sizeof(uint64_t));
		// 			for (int d = 0; d < dimension; d++)
		// 				int size = machine->receiveFromPeer(machineId, p, (unsigned char *)(&allProfiles[index-1][d]), sizeof(uint64_t));
		// 		}

		// 		/* Sending to Lower Machine & Receiving from Uppers to avoid DOSing the machines */
		// 		for (int p = (machineId+1); p < totalMachines; p++)
		// 		{
		// 			// cout << "Sending vertexID to Lower Machines... " << endl;
		// 			machine->sendToPeer(machineId, p, (unsigned char *)(&nodes[t].vertexID), sizeof(uint64_t));
		// 			for (int d = 0; d < dimension; d++)
		// 				machine->sendToPeer(machineId, p, (unsigned char *)(&nodes[t].newProfile[d]), sizeof(nodes[t].newProfile[d]));
		// 		}
		// 	}
		// }
		
		// for (int e = 0; e < nEdges; e++) 
		// 	if (isEdgeIncoming)
		// 		for (int i = 0; i < dimension; i++)
		// 			edges[e].profile_v[i] = allProfiles[OpenedVertexIds[e]-totalUsers-1][i];

		// 	else if (!isEdgeIncoming)
		// 		for (int i = 0; i < dimension; i++)
		// 			edges[e].profile_u[i] = allProfiles[OpenedVertexIds[e]-1][i];
		return edges;
	}

	SecretEdgeMAC * computeMAC(int party, Secret_Edge* secret_edges, int nEdges)
	{
		std::vector<unsigned char> seed = machine->getSharedRandomSeed(party, partner, com);
		AESRNG *rng = new AESRNG(seed.data());
		
		// Each entry of an edge needs a random element
		std::vector<uint64_t> randoms = rng->GetUInt64Array(nEdges*24);

		SecretEdgeMAC *MACedEdges = new SecretEdgeMAC[nEdges];

		int count = 0;
		for (int e = 0; e < nEdges; e++){
			MACedEdges[e].id_u = MACGen->computeMAC(party, secret_edges[e].id_u, randoms[count]); count++;
			MACedEdges[e].id_v = MACGen->computeMAC(party, secret_edges[e].id_v, randoms[count]); count++;
			for(int idx = 0; idx < dimension; idx++)
			{
				MACedEdges[e].profile_u[idx] = MACGen->computeMAC(party, secret_edges[e].profile_u[idx], randoms[count]); count++;
				MACedEdges[e].profile_v[idx] = MACGen->computeMAC(party, secret_edges[e].profile_v[idx], randoms[count]); count++;
			}
			MACedEdges[e].rating = MACGen->computeMAC(party, secret_edges[e].rating, randoms[count]); count++;
			MACedEdges[e].isReal = MACGen->computeMAC(party, secret_edges[e].isReal, randoms[count]); count++;
		}
		
		return MACedEdges;
	}

	void verifyMAC(int party, SecretEdgeMAC *MACedEdges, Secret_Edge *secretEdge, int nEdges)
	{
		std::cout << "--------------- verifyMAC -----------------" << std::endl;
		std::vector<uint64_t> myDifference(nEdges*24), theirDiffference(nEdges*24);
		
		int count = 0;
		for (int e = 0; e < nEdges; e++){
			myDifference[count] = MACedEdges[e].id_u - MACGen->alpha*secretEdge[e].id_u; count++;
			myDifference[count] = MACedEdges[e].id_v - MACGen->alpha*secretEdge[e].id_v; count++;
			for(int idx = 0; idx < dimension; idx++)
			{
				myDifference[count] = MACedEdges[e].profile_u[idx] - MACGen->alpha*secretEdge[e].profile_u[idx]; count++;
				myDifference[count] = MACedEdges[e].profile_v[idx] - MACGen->alpha*secretEdge[e].profile_v[idx]; count++;
			}
			myDifference[count] = MACedEdges[e].rating - MACGen->alpha*secretEdge[e].rating; count++;
			myDifference[count] = MACedEdges[e].isReal - MACGen->alpha*secretEdge[e].isReal; count++;
		}
		
		// Verify that all of them are reconstructed to zero
		if(party % 2 == 0)
		{
			com->SendEvaluationPartner((unsigned char *)myDifference.data(), myDifference.size()*sizeof(uint64_t));
			com->AwaitEvaluationPartner((unsigned char *)theirDiffference.data(), myDifference.size()*sizeof(uint64_t));
		}
		else
		{
			com->AwaitEvaluationPartner((unsigned char *)theirDiffference.data(), myDifference.size()*sizeof(uint64_t));
			com->SendEvaluationPartner((unsigned char *)myDifference.data(), myDifference.size()*sizeof(uint64_t));
		}
		
		for(int idx = 0; idx < myDifference.size(); idx++)
		{
			assert(myDifference[idx] + theirDiffference[idx] == 0);
		}
	}
	
	void verifyMAC(int party, std::vector<uint64_t> MACs, std::vector<uint64_t> shares)
	{
		std::cout << "--------------- verifyMAC -----------------" << std::endl;
		assert(MACs.size() == shares.size());
		int nEdges = MACs.size();
		
		std::vector<uint64_t> myDifference(nEdges), theirDiffference(nEdges);
		
		for (int idx = 0; idx < nEdges; idx++){
			myDifference[idx] = MACs[idx] - MACGen->alpha*shares[idx];
		}
		
		// Verify that all of them are reconstructed to zero
		if(party % 2 == 0)
		{
			com->SendEvaluationPartner((unsigned char *)myDifference.data(), myDifference.size()*sizeof(uint64_t));
			com->AwaitEvaluationPartner((unsigned char *)theirDiffference.data(), myDifference.size()*sizeof(uint64_t));
		}
		else
		{
			com->AwaitEvaluationPartner((unsigned char *)theirDiffference.data(), myDifference.size()*sizeof(uint64_t));
			com->SendEvaluationPartner((unsigned char *)myDifference.data(), myDifference.size()*sizeof(uint64_t));
		}
		
		for(int idx = 0; idx < myDifference.size(); idx++)
		{
			assert(myDifference[idx] + theirDiffference[idx] == 0);
		}
	}	

	void printNodes(Secret_Node* nodes, int nNodes)
	{
		cout << "----Nodes----" << endl;
		cout << "machine: id Profile newProfile" << endl;
		for (int i = 0; i < nNodes; i++)
		{
			cout << machineId << ": " << nodes[i].vertexID << "\t" << nodes[i].Profile[0] << " " << nodes[i].newProfile[0] <<  " " << nodes[i].halfEdge.size() << endl;
		}
	}

	void printEdges(Secret_Edge* edges, int nEdges)
	{
		cout << "----Edges----" << endl;
		cout << "machine: u v   uProfile   vProfile   rating   isReal" << endl;
		for (int i = 0; i < nEdges; i++)
		{
			cout << machineId << ": " << edges[i].id_u << " " << edges[i].id_v << " " << edges[i].profile_u[0] << " " << edges[i].profile_v[0] << " " << edges[i].rating << " " << edges[i].isReal << endl;
		}
	}
};


#endif



	





