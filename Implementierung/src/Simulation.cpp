#include "Simulation.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <string>

typedef std::vector<Edge*> edgeContainer;
typedef std::vector<Spawner*> spawnerContainer;
typedef std::vector<Vertex*> vertexContainer;
typedef std::chrono::high_resolution_clock chrono_t;

Simulation::Simulation()
{
	//Time at the beginning
	chrono_t::time_point t1 = chrono_t::now();
	chrono_t::time_point tC;

	std::vector<chrono_t::time_point> timePointBeginning;
	std::vector<chrono_t::time_point> timePointEnding;

	timePointBeginning.resize(_SIMULATION_TICKS);
	timePointEnding.resize(_SIMULATION_TICKS);

	_currentTick = 0;

	clock_t begin = clock();
	_graph = new Graph();

	_routingTable = new RoutingTable(_graph, 3);
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	std::cout << "Gesamte Zeit: " << elapsed_secs << std::endl;
	initSpawner();
	std::cout << "Init completed";

	InitVectors();

	for (int i = 0; i < _SIMULATION_TICKS; i++) {
		timePointBeginning[i] = chrono_t::now();
		nextTick();
		timePointEnding[i] = chrono_t::now();
		writeResultsCurrentTick();

		std::cout << "#####################################Time after tick " << _currentTick << ": " << std::chrono::duration_cast<std::chrono::duration<double>>(timePointEnding[i] - t1).count() << std::endl;

	}

	//Time at the end
	chrono_t::time_point t2 = chrono_t::now();

	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
	std::cout << "TIME PASSED SINCE BEGINNING: " << time_span.count() << " SECONDS" << std::endl;

	double tickSpan = 0;

	for (int t = 0; t < _SIMULATION_TICKS; t++) {
		tickSpan += std::chrono::duration_cast<std::chrono::duration<double>>(timePointEnding[t] - timePointBeginning[t]).count();
	}

	std::cout << "Durchschnittliche Zeit pro Tick: " << (double)tickSpan / _SIMULATION_TICKS << std::endl;
}


Simulation::~Simulation()
{
}

void Simulation::writeResultsCurrentTick()
{
	std::ofstream results;
	edgeContainer edges = _graph->getEdges();
	results.open("../../Output/" + std::to_string(_currentTick) + ".txt");
	for (edgeContainer::iterator it2 = edges.begin(); it2 != edges.end(); it2++) {
		results << (*it2)->getID() << " " << (*it2)->numberOfCars() << " " << (*it2)->getEdgeCapacity() << '\n';
	}
	results.close();
}


void Simulation::nextTick()
{
	_currentTick++;
	int timeStamp = _currentTick %_TIMETABLE_SPAN;
	std::cout << "begin update" << '\n';
	//vertexContainer vertices = _graph->getVertices();
	//for(vertexContainer::iterator it2 = _vertices.begin(); it2 != _vertices.end(); it2++){
	for (Vertex* &v : _vertices) {
		v->Update();
	}
	std::cout << "Vertex update completed" << '\n';
	//for (Edge* ed : _graph->getEdges()) {
	for (Edge* &ed : _edges) {
		ed->Update(_currentTick);
	}
	std::cout << "Edge update Phase 1 completed" << '\n';
	//std::vector<Edge*> remainingEdges = _graph->getEdges();
	std::vector<Edge*> remainingEdges = _edges;
	//for (int i = 0; i < _graph->getEdges().size(); i++) {
	for (edgeContainer::iterator it2 = remainingEdges.begin(); it2 != remainingEdges.end();) {
		(*it2)->UpdateOverflow();
		if (!(*it2)->hasOverflow()) {
			it2 = remainingEdges.erase(it2);
		}
		else it2++;
	}
	//}
	std::cout << "Edge update Phase 2 completed" << '\n';
	//spawnerContainer spawners = _graph->getSpawner();
	//for (spawnerContainer::iterator it2 = spawners.begin(); it2 != spawners.end(); it2++) {
	for (Spawner* &s : _spawners) {
		s->Update(timeStamp);
	}
	std::cout << "Tick " << _currentTick << "finished" << '\n';
}

void Simulation::initSpawner() {
	std::vector<Spawner*> spawner = _graph->getSpawner();
	std::vector<std::pair<Spawner*, int>> vertexPriorities;
	for (std::vector<Spawner*>::iterator it2 = spawner.begin(); it2 != spawner.end(); it2++) {
		vertexPriorities.push_back(std::pair<Spawner*, int>((*it2), rand() % VERTEX_PRIORITY_DIVERGENCE));
	}
	for (std::vector<Spawner*>::iterator it2 = spawner.begin(); it2 != spawner.end(); it2++) {
		(*it2)->linkRoutingTable(_routingTable);
		(*it2)->linkVertexPriorities(vertexPriorities);
	}
}

void Simulation::InitVectors()
{
	_vertexMap = _graph->getVertexMap();

	_edges = _graph->getEdges();
	_vertices = _graph->getVertices();
	_spawners = _graph->getSpawner();

	std::cout << "Init completed" << std::endl;
}

