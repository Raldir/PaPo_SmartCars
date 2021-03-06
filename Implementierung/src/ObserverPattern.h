#pragma once

#include <iostream>
#include <vector>
#include "main.h"

class Edge;
class Car;
class Vertex;

class SubjectEdge {

protected:
	Vertex* endVertex;
	Vertex* startVertex;
	int _ID;

public:
	virtual void registerObserver(Vertex * obs, std::string indicator) {};
	virtual void removeObserver(std::string indicator) {};

	//Notifies attached Vertex that car has reached position 0
	virtual void notifyVertex() = 0;

	virtual std::pair<Vertex*, Vertex*> getVertices() = 0;

	virtual int getID() = 0;
};
