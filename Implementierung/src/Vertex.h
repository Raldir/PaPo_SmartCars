#pragma once

#include <iostream>
#include <utility>
#include <map>
#include "main.h"
#include "Edge.h"
#include "TrafficLight.h"

class Edge;
class Car;
class TrafficLight;

class Vertex {

public:
	Vertex() {};

	Vertex(int id, float x, float y);
	Vertex(int id, float x, float y, TrafficLight tL);

	void setTrafficLight(TrafficLight tL);
	TrafficLight* getTrafficLight();

	float distanceTo(Vertex* v);

	void Update();

	//TrafficLight* trafficLight;

	/*
		Makes the actual transition of the car
		ONLY use when certain that transtition will be possible
	*/
	void transferCar(int incomingEdgeID);

	//Hilfefunktion
	Car* takeCar(int incomingEdgeID);
	void giveCar(Edge* edge, Car* car);

	void destroyCar(Car* car);

	//Checks wheter TrafficLight allow transit and wheter or not the next edge is full
	bool canTransit(int incomingEdgeID, int outgoingEdgeID);

	void addIncomingEdges(Edge *edge);
	void addOutgoingEdges(Edge *edge);

	std::vector<Edge*> getIncomingEdges();
	std::vector<Edge*> getOutgoingEdges();

	std::vector<Edge*> getEdges();

	Edge* outgoingNeighbor(int destID);

	Edge* incomingNeighbor(int destID);

	Edge* getEdgeFromID(int edgeID);

	void setIsEdgeFull(int, bool);

	void printEdges();

	float getX();
	float getY();

	int getID();

	std::pair<float, float> getPosition();

private:

	TrafficLight trafficLight;

	/*
		First int -> ID of edge
		Second bool -> is full or not
	*/
	std::map<int, bool> isEdgeFullMap;

	/*
		int -> ID of edge
		Edge* -> pointer to the actual edge
	*/
	std::map<int, Edge*> incomingEdges;
	std::map<int, Edge*> outgoingEdges;

	float _X;
	float _Y;
	int _ID;
};