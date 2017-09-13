#pragma once
#include <queue>
#include <map>
#include <algorithm>
#include <boost/graph/astar_search.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/random.hpp>
#include <boost/random.hpp>
#include <boost/graph/graphviz.hpp>
#include <time.h>
#include <list>
#include <fstream>
#include <math.h>  
#include "Graph.h"
#include <boost/lambda/lambda.hpp>  // _1
#include <boost/lambda/bind.hpp>    // bind()
#include <boost/tuple/tuple_io.hpp>

class Edge;
class Vertex;

typedef std::map<int, std::map<int, std::queue<int>>> RoutingMatrix;
typedef std::map<int, std::map<int, std::queue<int>>>::iterator RoutingMatrixIt;

typedef std::map<int, std::map<int, float>> CostMatrix;
typedef std::map<int, std::vector<int>> KNearestNeighborMatrix;


class RoutingTable {

public:
	//TODO Mulitple Routenfindungsalgorithmen implementieren

	RoutingTable(Graph* g, int numberNearestNeighbors);
	RoutingTable(Graph* g, int numberNearestNeighbors, std::vector<int> spawner);
	RoutingTable::RoutingTable(Graph* graph, int numberNearestNeighbors, std::map<int, std::map<int, std::queue<int>>> routingMatrix);

	void insertRoute(int originID, int destID, std::queue<int> route);
	void removeRoute(int originID, int destID);
	
	void replaceRoute(int originID, int destID, std::queue<int> route);

	int calculateBestGoal(int startID, int destID, int currentTimeTableIndex);
	void calculateRoutes(std::vector<Spawner*> _spawner);
	//ToDO Add  a method that only adds on a specific range
	void addCosts(int startID, int destID, int currentTimeTableIndex);

	//Get Route from origin to destination
	std::queue<int> getRoute(int originID, int destID);
	std::vector<std::vector<int>> RoutingTable::getRoutingMatrix();
	void setCost(int originID, int destID, float cost);
	float getCost(int originID, int destID);

	std::pair<std::map<int, std::vector<int>>, std::map<int, std::vector<int>>> getProcessConnectionVectors();
	
	//Returns incomingConnection and outgoingConnection for every connected process
	void insertProcessRoutes(std::pair<int, std::vector<std::pair<int, int>>>);

private:
	std::queue<int> reverseQueue(std::queue<int> queue);

	static bool comp(const std::pair<int, float> &a, const std::pair<int, float> &b);
	int dimension;
	int _numberNearestNeighbors;

	//Routing Table "Matrix", die die Routen enth�lt
	/*
		First int --> ID of origin vertex
		Second int --> ID of destination vertex
	*/
	RoutingMatrix routingMatrix;
	CostMatrix costMatrix;
	KNearestNeighborMatrix k_nn;
	Graph* _graph;

	//Contains every route inside this process
	std::vector<std::vector<int>> processRoutingMatrix;

	std::map<int, std::vector<std::pair<int, int>>> processRoutesMap;
};