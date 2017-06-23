#pragma once

#include <iostream>
#include <queue>
#include <utility>
#include "main.h"

class Edge {

	private:

		//Kapazi�t bei leerer Star�e
		int baseCapacity;

		//Kapazit�t der Stra�e
		int currentCapacity;

		//Queue in der die Autos gespeichert werden.
		std::queue<Car*> carQueue;

		//L�nge der Stra�e
		const float _LENGTH;

		//Timetable in dem die Gewichte f�r jeden Zeitabschnitt gespeichert werden.
		int* timetable[_LENGTH * _CAR_SPEED]; 
		
		//Kreuzungen an denen die Stra�e liegt.
		std::pair <Intersection, Intersection> nodes;

	public:
		//Constructor
		Edge(float length, );

		//Observer Pattern
		void notify();

		void addCar();

		void removeCar();

};