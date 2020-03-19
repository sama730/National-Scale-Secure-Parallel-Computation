
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "Machine.hpp"
#include "Graph.hpp"
#include "Utility/Timer.h"

using namespace std;

int main(int argc, char** argv) 
{
	if (argc < 10)
		printf("Error: More arguments are needed. \n");
	int totalMachines = atoi(argv[1]);
	int machineId = atoi(argv[2]);
	int peerBasePort = atoi(argv[3]);
	int party = atoi(argv[4]);
	int partyBasePort = atoi(argv[5]);
	int edgeLength = atoi(argv[6]);
	int userLength = atoi(argv[7]);
	int itemLength = atoi(argv[8]);
	double epsilon = atof(argv[9]);
	
	int ITERATIONS = 1;
	double GAMMA = 0.001;
	double LAMBDA = 0.0;
	double MU = 0.02;


	Machine * machine = new Machine(totalMachines, machineId, party);
	machine->connecting(peerBasePort, partyBasePort);
	cout << "Networking Done." << endl;
	usleep(5000);

	Graph * graph = new Graph(totalMachines, machineId, party, edgeLength, userLength, itemLength, epsilon, machine);

	Timer t;	
	graph->GraphComputation();
	// t.Tick("GraphComputation()");

	delete machine;
}