#pragma once


int current_timeTable_tick = 0;
const float _CAR_SPEED = 10;
//IRRELEVANT const float _CAR_LENGTH = 10;
const float _CAR_MINIMUM_GAP = 10;

//TRAFFIC LIGHT
/*
#define _TRAFFIC_LIGHT_PERIOD^^^^
#define _TRAFFIC_LIGHT_DURATION
*/

const int _SIMULATION_TICKS = 6000;
const int _SIMULATION_SECONDS_PER_TICK = 5;

const float _CAR_SPEED_PER_TICK = _SIMULATION_SECONDS_PER_TICK * _CAR_SPEED;

const int BASE_SPAWN_RATE = 10;

const int VERTEX_PRIORITY_DIVERGENCE = 5;

//TODO �ndere zu: Average_EDGELENGTH / VALUE
const int _CAR_RELEVANCE = 10;
