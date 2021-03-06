#ifndef GRAPH_DATA_H__
#define GRAPH_DATA_H__

#include <vector>

#define dimension 1

struct Secret_Edge
{
	uint64_t id_u;  	// 64 bit, [1..2^64]  ~ 4 byte
	uint64_t id_v;  	// 64 bit, [1..2^64]  ~ 4 byte
	uint64_t isReal; 	// 8 bit, [true,false]  ~ 1 byte
};


typedef Secret_Edge SecretEdgeMAC;

// secret shared values
struct Half_Secret_Edge
{
	uint64_t isReal;
	SecretEdgeMAC bMACs;
};

struct Secret_Node
{
	uint64_t vertexID;  // 16 bit, [1..2^16-1]
	std::vector<Half_Secret_Edge> halfEdge;
};

#endif
