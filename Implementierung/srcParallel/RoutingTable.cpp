#include "RoutingTable.h"

#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/serialization/vector.hpp>
//#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
//#include <boost/test/minimal.hpp>

#include <boost/graph/astar_search.hpp>
#include <boost/graph/adjacency_list.hpp>

typedef std::vector<Vertex*> vertexContainer;
typedef std::vector<Spawner*> spawnerContainer;


using namespace boost;
using boost::graph::distributed::mpi_process_group;


std::queue<int> RoutingTable::reverseQueue(std::queue<int> queue)
{
	std::queue<int> revQueue;

	if (queue.empty()) {
		return queue;
	}

	int vertex = queue.front();
	queue.pop();

	revQueue = reverseQueue(queue);
	revQueue.push(vertex);
	return revQueue;
}

bool RoutingTable::comp(const std::pair<int, float> &a, const std::pair<int, float> &b) {
	return a.second < b.second;
}

template <class Graph, class CostType, class LocMap>
class distance_heuristic : public astar_heuristic<Graph, CostType>
{
public:
	typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
	distance_heuristic(LocMap l, Vertex goal)
		: m_location(l), m_goal(goal) {}
	CostType operator()(Vertex u)
	{
		CostType dx = m_location[m_goal]->getPosition().first - m_location[u]->getPosition().first;
		CostType dy = m_location[m_goal]->getPosition().second - m_location[u]->getPosition().second;
		return ::sqrt(dx * dx + dy * dy);
	}
private:
	LocMap m_location;
	Vertex m_goal;
};

struct found_goal {};// exception for termination

template <class Vertex>
class astar_goal_visitor : public boost::default_astar_visitor
{
public:
	astar_goal_visitor(Vertex goal) : m_goal(goal) {}
	template <class Graph>
	void examine_vertex(Vertex u, Graph& g) {
		if (u == m_goal)
			throw found_goal();
	}
private:
	Vertex m_goal;
};

void RoutingTable::setCost(int originID, int destID, float cost)
{
	costMatrix[originID][destID] = cost;
}

float RoutingTable::getCost(int originID, int destID)
{
	return costMatrix[originID][destID];
}

/*
The Constructor for the Routingtable, that will calculate each Route parallel but the number of Routes sequential
*/
RoutingTable::RoutingTable(Graph* graph, int numberNearestNeighbors) {
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	_graph = graph;
	_numberNearestNeighbors = numberNearestNeighbors;
	std::vector<Spawner*> _spawner = graph->getSpawner();
	calculateRoutesParallel(_spawner);
}

RoutingTable::RoutingTable(Graph* graph, int numberNearestNeighbors, std::map<int, std::map<int, std::queue<int>>> routingMatrixP, std::map<int, std::map<int, float>> costs) {
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	_graph = graph;
	_numberNearestNeighbors = numberNearestNeighbors;
	routingMatrix = routingMatrixP;
	costMatrix = costs;
}


RoutingTable::RoutingTable(Graph* graph, int numberNearestNeighbors, std::vector<int> verticesID) {
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	_graph = graph;
	_numberNearestNeighbors = numberNearestNeighbors;
	std::vector<Spawner*> spawner;
	for (int i = 0; i < verticesID.size(); i++) {
		spawner.push_back(_graph->createSpawnerMap()[verticesID[i]]);
	}
	calculateRoutes(spawner);
}



/*
Calculate the Routingtable sequential(however it is implemented so that seperate calculations and merging of different routes through other
processes is feasible
*/
void RoutingTable::calculateRoutes(std::vector<Spawner*> _spawner) {
	std::vector<Spawner*> spawners = _graph->getSpawner();
	std::vector<Edge*> _edges = _graph->getEdges();
	std::map<int, Vertex*> _vertexMap = _graph->getVertexMap();
	std::vector<Vertex*> _vertices = _graph->getVertices();

	typedef adjacency_list<listS, vecS, directedS, no_property,
		property<edge_weight_t, int> > mygraph_t;
	typedef property_map<mygraph_t, edge_weight_t>::type WeightMap;
	typedef mygraph_t::vertex_descriptor vertex;
	typedef mygraph_t::edge_descriptor edge_descriptor;

	int num_edges = (_edges.size());
	std::vector<int> vertices_ID;
	for (vertexContainer::iterator it = _vertices.begin(); it != _vertices.end(); it++) {
		vertices_ID.push_back((*it)->getID());
	}

	// create graph
	mygraph_t g(vertices_ID.size());
	WeightMap weightmap = get(edge_weight, g);
	for (std::vector<Edge*>::iterator it = _edges.begin(); it != _edges.end(); it++) {
		{
			edge_descriptor e; bool inserted;
			tie(e, inserted) = add_edge((*it)->getVertices().first->getID(),
				(*it)->getVertices().second->getID(), g);
			weightmap[e] = int((*it)->getLength());
		}
	}
	for (spawnerContainer::iterator it2 = _spawner.begin(); it2 != _spawner.end(); it2++) {
		int start = (*it2)->getID();
		std::map<int, float> distances;
		for (spawnerContainer::iterator it = spawners.begin(); it != spawners.end(); it++) {
			int goal = (*it)->getID();

			//Speicher alle Abst�nde von Knoten zum Startknoten
			distances[goal] = _vertexMap[start]->distanceTo(_vertexMap[goal]);
			int sumDistance = 0;
			//Bei Parallelisierung wird diese Zeile nicht beachtet, was in der theorie zu einem zweifachen aufwand f�hrt
			//if (!(getRoute(goal, start).empty()))	continue;

			std::vector<mygraph_t::vertex_descriptor> p(num_vertices(g));
			std::vector<float> d(num_vertices(g));
			try {
				// call astar named parameter interface
				astar_search
					(g, start,
						distance_heuristic<mygraph_t, float, std::map<int, Vertex*>>
						(_vertexMap, goal),
						predecessor_map(&p[0]).distance_map(&d[0]).
						visitor(astar_goal_visitor<vertex>(goal)));


			}
			catch (found_goal fg) { // found a path to the goal
				std::list<vertex> shortest_path;
				for (vertex v = goal;; v = p[v]) {
					shortest_path.push_front(v);
					if (p[v] == v)
						break;
				}
				std::cout << "Shortest path from " << _vertexMap[start]->getID() << " to "
					<< _vertexMap[goal]->getID() << ": ";
				std::list<vertex>::iterator spi = shortest_path.begin();
				std::cout << _vertexMap[start]->getID();
				std::queue<int> route;
				int lastElement;
				for (++spi; spi != shortest_path.end(); ++spi) {
					std::cout << " -> " << _vertexMap[int(*spi)]->getID();
					if (!route.empty()) {
						sumDistance += _vertexMap[(*spi)]->incomingNeighbor(lastElement)->getLength();
						lastElement = (*spi);
					}
					else {
						route.push(_vertexMap[start]->getID());
						lastElement = (*spi);
					}
					route.push(_vertexMap[int(*spi)]->getID());

				}
				insertRoute(start, goal, route);
				setCost(start, goal, sumDistance);
				continue;
			}
			std::cout << "Didn't find a path from " << _vertexMap[start]->getID() << "to"
				<< _vertexMap[goal]->getID() << "!" << std::endl;
		}
		size_t m = _numberNearestNeighbors;
		//Erstelle liste mit abst�nden und mache partial sort
		std::vector<std::pair<int, float>> v{ distances.begin(), distances.end() };
		if (int(v.size()) < _numberNearestNeighbors) {
			m = v.size();
		}
		std::partial_sort(v.begin(), v.begin() + m, v.end(), &comp);
		for (std::vector<std::pair<int, float>> ::iterator it = v.begin(); it != v.begin() + m; it++) {
			k_nn[start].push_back((*it).first);
			std::cout << (*it).first << ">";
		}
		std::cout << std::endl;
	}
}


void RoutingTable::insertRoute(int originID, int destID, std::queue<int> route)
{
	//Pushes originID into first map and int queue pair into the second
	routingMatrix[originID][destID] = route;

	//Pushes queue on symmetrical pair
	//Would cause problems on Parallelism
	//routingMatrix[destID][originID] = reverseQueue(route);

	std::cout << "Added queue from " << originID << " to " << destID << std::endl;
}

void RoutingTable::removeRoute(int originID, int destID)
{
	//Erases queue origin to destination and reverse
	routingMatrix[originID].erase(destID);
	routingMatrix[destID].erase(originID);

	std::cout << "Deleted queue from " << originID << " to " << destID << " and reverse." << std::endl;
}

void RoutingTable::replaceRoute(int originID, int destID, std::queue<int> route)
{
	removeRoute(originID, destID);

	insertRoute(originID, destID, route);

}

//TODO BIG REFACTORING
int RoutingTable::calculateBestGoal(int startID, int destID, int currentTimeTableIndex)
{
	std::map<int, float> costs;
	for (int goalID : k_nn[destID]) {
		if (goalID == startID || getRoute(startID, goalID).empty()) {
			//std::cout << "Would take same";
			continue;
		}
		//std::cout << "CurrentGoalvertex " << goalID << std::endl;
		int timeTableValue = _graph->getSumWeightFromTimeTables(startID, goalID, currentTimeTableIndex, routingMatrix[startID][goalID]);
		//Berechnet zusatzkosten, daf�r das Zeil weiter weg vom eigentlichen ausgangsziel ist, nutze hierf�r distanzheuristik
		float extraCosts = _graph->distance_heuristicOverID(destID, goalID) / 2;
		costs[goalID] = costMatrix[startID][goalID] + timeTableValue + extraCosts;
		//std::cout << "calculated for one goalVertex" <<std::endl;
	}
	//std::cout << "afterAll ";
	if (costs.empty()) {
		return -1;
	}
	std::vector<std::pair<int, float>> v{ costs.begin(), costs.end() };
	std::partial_sort(v.begin(), v.begin() + 1, v.end(), &comp);
	std::cout << "Best Goal for Car: " << v[0].first << std::endl;
	return v[0].first;
}

void RoutingTable::addCosts(int startID, int destID, int currentTimeTableIndex) {
	std::queue<int> tempqueue = routingMatrix[startID][destID];
	_graph->addWeightToTimeTables(startID, destID, currentTimeTableIndex, tempqueue);
}


//Gibt Route zwischen origin und destination aus
std::queue<int> RoutingTable::getRoute(int originID, int destID) {

	std::queue<int> queue;

	RoutingMatrix::iterator iterOrigin = routingMatrix.find(originID);
	if (iterOrigin != routingMatrix.end()) {
		//Map where destinationID and queues are stored
		std::map<int, std::queue<int>> destinationMap = iterOrigin->second;
		std::map<int, std::queue<int>>::iterator iterDest = destinationMap.find(destID);
		if (iterDest != destinationMap.end()) {
			queue = destinationMap.find(destID)->second;
		}
	}
	else {
		std::cout << "No queue found in Routing Table from" << originID << " to " << destID << std::endl;
		return std::queue<int>();
	}

	return queue;
}

std::vector<std::vector<int>> RoutingTable::getRoutingMatrix() {
	std::vector<std::vector<int>> matrix;
	for (auto const &ent1 : routingMatrix) {
		for (auto const &ent2 : ent1.second) {
			std::vector<int> route;
			std::queue<int> copy = routingMatrix[ent1.first][ent2.first];
			if (copy.size() == 0) {
				continue;
			}
			//std::cout << routingMatrix[ent1.first][ent2.first].size();
			for (int i = 0; i < routingMatrix[ent1.first][ent2.first].size(); i++) {
				//std::cout << ent1.first << " " << ent2.first<<std::endl;
				route.push_back(copy.front());
				copy.pop();
			}
			matrix.push_back(route);
		}
	}
	return matrix;
}

std::vector<std::vector<int>> RoutingTable::getRoutingCosts()
{
	std::vector<std::vector<int>> matrix;
	for (auto const &ent1 : costMatrix) {
		for (auto const &ent2 : ent1.second) {
			std::vector<int> costsOrigin;
			costsOrigin.push_back(ent1.first);
			costsOrigin.push_back(int(ent2.second));
			costsOrigin.push_back(ent2.first);
			matrix.push_back(costsOrigin);
		}
	}
	return matrix;
}

/*
STRUCTURE: IN EACH VECTOR IS THE FIRST VALUE THE ORIGINVERTEX IT BELONGS TO
*/
std::vector<std::vector<int>> RoutingTable::getKNearestMatrix()
{
	std::vector<std::vector<int>> matrix;
	for (auto const &ent1 : k_nn) {
		std::vector<int> k_nnOrigin;
		for (int i = 0; i < k_nn[ent1.first].size(); i++) {
			k_nnOrigin.push_back(ent1.second[i]);
		}
		matrix.push_back(k_nnOrigin);
	}
	return matrix;
}

/*
Parallelism which calculates each Route parallel but the number of Routes sequential
*/
template<typename Vertex, typename Graph>
int get_vertex_name(Vertex v, const Graph& g, std::vector<int> vertex_names)
{
	return vertex_names[g.distribution().global(owner(v), local(v))];
}

void RoutingTable::calculateRoutesParallel(std::vector<Spawner*> _spawner) {
	std::vector<Spawner*> spawners = _graph->getSpawner();
	std::vector<Edge*> _edges = _graph->getEdges();
	std::map<int, int> vertices_ID;
	std::vector<Vertex*> _vertices = _graph->getVertices();
	std::vector<int> vertex_names;
	typedef adjacency_list<listS, distributedS<mpi_process_group, vecS>, directedS,
		no_property,                 // Vertex properties
		property<edge_weight_t, int> // Edge properties
	> graph_t;
	typedef graph_traits < graph_t >::vertex_descriptor vertex_descriptor;
	typedef graph_traits < graph_t >::edge_descriptor edge_descriptor;

	int count = 0;
	for (vertexContainer::iterator it = _vertices.begin(); it != _vertices.end(); it++) {
		vertices_ID[(*it)->getID()] = count;
		vertex_names.push_back((*it)->getID());
		count++;
	}
	graph_t g(_graph->getNumberVertices());

	for (std::vector<Edge*>::iterator it = _edges.begin(); it != _edges.end(); it++) {
		add_edge(vertex((*it)->getVertices().first->getID(), g), vertex((*it)->getVertices().second->getID(), g), int((*it)->getLength()) + 1, g);
		std::cout << int((*it)->getLength());
	}
	synchronize(g.process_group());

	for (spawnerContainer::iterator it2 = _spawner.begin(); it2 != _spawner.end(); it2++) {
		vertex_descriptor s = vertex((*it2)->getID(), g);

		std::vector<vertex_descriptor> p(num_vertices(g));
		std::vector<int> d(num_vertices(g));
		auto parents =
			make_iterator_property_map(p.begin(), get(vertex_index, g));

		//*************NUR AUF UNIXSYSTEMEN AUSKOMMENTIEREN; FUNKTIONIERT AUF WINDOWS NICHT!*******************************
		//dijkstra_shortest_paths
		//	(g, s,
		//		predecessor_map(parents).
		//		distance_map(
		//			make_iterator_property_map(d.begin(), get(vertex_index, g)))
		//		);
		synchronize(g.process_group());

		for (spawnerContainer::iterator it = _spawner.begin(); it != _spawner.end(); it++) {
			vertex_descriptor v = vertex((*it)->getID(), g);
			std::vector<int> route;
			request(parents, v);
			synchronize(parents);
			vertex_descriptor current = get(parents, v);
			route.push_back(get_vertex_name(v, g, vertex_names));
			route.push_back(get_vertex_name(current, g, vertex_names));
			if (process_id(g.process_group()) == 0) {
				std::cout << "pathOf(" << get_vertex_name(v, g, vertex_names) << ") = " << get_vertex_name(current, g, vertex_names) << " ";
			}
			while (current != s) {
				current = get(parents, current);
				if (std::find(route.begin(), route.end(), get_vertex_name(current, g, vertex_names)) != route.end()) {
					break;
				}
				route.push_back(get_vertex_name(current, g, vertex_names));
				if (process_id(g.process_group()) == 0) {
					std::cout << get_vertex_name(current, g, vertex_names) << " ";
				}
			}
			if (process_id(g.process_group()) == 0) {
				std::cout << std::endl;
			}

			std::reverse(route.begin(), route.end());
			std::queue<int> routeQ;
			for (size_t i = 0; i < route.size(); i++) {
				routeQ.push(route[i]);
			}
			insertRoute(get_vertex_name(s, g, vertex_names), get_vertex_name(v, g, vertex_names), routeQ);
			//Kosten m�ssten noch berechent werden, soll aber f�r weiteres nicht wichtig sein.....
		}
	}
}

void RoutingTable::calculateKNearest() {
	std::vector<Spawner*> spawners = _graph->getSpawner();
	//VERWENDE EVENTUELL DIE LUFTDISTANZ?? BESSERE HEURISTIK?
	for (spawnerContainer::iterator it2 = spawners.begin(); it2 != spawners.end(); it2++) {
		std::vector<std::pair<int, float>>  v;
		int start = (*it2)->getID();
		for (spawnerContainer::iterator it = spawners.begin(); it != spawners.end(); it++) {
			int goal = (*it)->getID();
			v.push_back(std::pair<int, float>(goal, costMatrix[start][goal]));
		}
		int m = _numberNearestNeighbors;
		if (int(v.size()) < _numberNearestNeighbors) {
			m = v.size();
		}
		std::partial_sort(v.begin(), v.begin() + m, v.end(), &comp);
		for (std::vector<std::pair<int, float>> ::iterator it = v.begin(); it != v.begin() + m; it++) {
			k_nn[start].push_back((*it).first);
			std::cout << (*it).first << ">";
		}
		std::cout << std::endl;
	}
}
