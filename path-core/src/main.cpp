#include <irrlicht.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

using std::vector;
using std::string;
using std::cout;
using std::cin;
using std::endl;

#pragma comment(lib, "Irrlicht.lib")

IrrlichtDevice* device = 0;

/**
 * Implementation of A* Algorithm
 * 
 * This algorithm finds the shortest route between two nodes.
 * 
 * You can make a node impassable and the algorithm will not include that node
 * in the solution.
 *
 * If a path can not be found (i.e. all nodes surrounding the destination node are 
 * impassable) then the algorithm will return an empty path.
 *
 * The path (if found) will be output to the console and also displayed on the 
 * graphical display. Start and end nodes will be yellow nodes.
 * The path will be red nodes.
 *
 * This solution uses the irrlicht engine, and was created in the test environment
 * produced after completing the irrlicht tutorials.
 * http://irrlicht.sourceforge.net/docu/example001.html
 * I basically wanted an engine that was ready to go (and worked in Visual Studio)
 * as my engine was not in a working state when I completed this assignment. This
 * allowed visualisation of my algorithm, which helped me debug the algorithm.
 *
 * Controls:
 * Mouse, WSAD and QE to move around.
 *
 * Key:
 * YELLOW: Start and End nodes.
 * RED: Nodes that are part of the path.
 * GREEN: All other nodes.
 *
 * Instructions:
 * As per the assignment spec, enter the start and end node ID's into the console,
 * for example, 1 and 50. 
 * Then enter the numbers of nodes which you would like to be IMPASSABLE. When you 
 * have finished (or if you choose none) type -1.
 * The result will be displayed.
 *
 * Note:
 * git repo has been included which will show how the algorithm was developed!
 * (if at all interesting).
 */


/**
 * Event receiver
 * receives keyboard and mouse events.
 */
class MyEventReceiver : public IEventReceiver
{
public:

	struct SMouseState {
		position2di Position;
		bool LeftButtonDown;
		bool RightButtonDown;
		SMouseState() : LeftButtonDown(false), RightButtonDown(false) {}
	} MouseState;

	// This is the one method that we have to implement
	virtual bool OnEvent(const SEvent& event)
	{
		// Remember the mouse state
		if (event.EventType == irr::EET_MOUSE_INPUT_EVENT)
		{
			switch (event.MouseInput.Event)
			{
			case EMIE_LMOUSE_PRESSED_DOWN:
				MouseState.LeftButtonDown = true;
				break;

			case EMIE_LMOUSE_LEFT_UP:
				MouseState.LeftButtonDown = false;
				break;

			case EMIE_MOUSE_MOVED:
				MouseState.Position.X = event.MouseInput.X;
				MouseState.Position.Y = event.MouseInput.Y;
				break;

			case EMIE_RMOUSE_PRESSED_DOWN:
				MouseState.RightButtonDown = true;
				break;

			case EMIE_RMOUSE_LEFT_UP:
				MouseState.RightButtonDown = false;
				break;

			default:
				// We won't use the wheel
				break;
			}
		}

		// Remember whether each key is down or up
		if (event.EventType == irr::EET_KEY_INPUT_EVENT)
			KeyIsDown[event.KeyInput.Key] = event.KeyInput.PressedDown;
		return false;
	}

	// This is used to check whether a key is being held down
	virtual bool IsKeyDown(EKEY_CODE keyCode) const
	{
		return KeyIsDown[keyCode];
	}

	MyEventReceiver()
	{
		for (u32 i = 0; i<KEY_KEY_CODES_COUNT; ++i)
			KeyIsDown[i] = false;
	}

private:
	// We use this array to store the current state of each key
	bool KeyIsDown[KEY_KEY_CODES_COUNT];
};

/**
 * Shader callback
 */
class NodeShaderCallBack : public video::IShaderConstantSetCallBack
{
public:
	NodeShaderCallBack(): IShaderConstantSetCallBack() {
		color = SColorf();
	}
	virtual void OnSetConstants(video::IMaterialRendererServices* services, s32 userData)
	{
		video::IVideoDriver* driver = services->getVideoDriver();
			
		// set inverted world matrix
		// if we are using highlevel shaders (the user can select this when
		// starting the program), we must set the constants by name.

		core::matrix4 invWorld = driver->getTransform(video::ETS_WORLD);
		invWorld.makeInverse();
		services->setVertexShaderConstant("mInvWorld", invWorld.pointer(), 16);

		// set clip matrix

		core::matrix4 worldViewProj;
		worldViewProj = driver->getTransform(video::ETS_PROJECTION);
		worldViewProj *= driver->getTransform(video::ETS_VIEW);
		worldViewProj *= driver->getTransform(video::ETS_WORLD);
		services->setVertexShaderConstant("mWorldViewProj", worldViewProj.pointer(), 16);

		// set camera position
		core::vector3df pos = device->getSceneManager()->getActiveCamera()->getAbsolutePosition();
		services->setVertexShaderConstant("mLightPos", reinterpret_cast<f32*>(&pos), 3);

		// set light color

		services->setVertexShaderConstant("mLightColor", reinterpret_cast<f32*>(&color), 4);

		// set transposed world matrix

		core::matrix4 world = driver->getTransform(video::ETS_WORLD);
		world = world.getTransposed();

		services->setVertexShaderConstant("mTransWorld", world.pointer(), 16);

		// set texture, for textures you can use both an int and a float setPixelShaderConstant interfaces (You need it only for an OpenGL driver).
		s32 TextureLayerID = 0;
		services->setPixelShaderConstant("myTexture", &TextureLayerID, 1);
	}
	void setColour(SColorf color) {
		this->color = color;
	}

private:
	SColorf color;
};

struct Node;

/**
 * Represents an edge between two nodes.
 * The weight is the distance between the two nodes.
 */
struct Edge {
	f32 weight;
	Node* from;
	Node* to;
	Edge() : from(nullptr), to(nullptr), weight(0.0f) {}
	Edge(f32 weight, Node* from, Node* to) : from(from), to(to), weight(weight) {}
};

/**
 * Represents a single Node on the Buckminsterfullerene structure.
 */
struct Node {
	std::string name;
	vector3df position;
	SColor color;
	vector<Edge> edges;
	Node* parent;
	bool visited;
	bool passable;
	f32 f;
	f32 g;
	f32 h;
	void reset() { f = g = h = 0; }
	Node(std::string name, vector3df pos, SColor color, bool passable) : f(0), g(0), h(0), position(pos), color(color), visited(false), name(name), parent(nullptr), passable(passable) {};
	friend bool operator<(const Node& l, const Node& r) {
		return l.h < r.h;
	}
	friend bool operator>(const Node& l, const Node& r) {
		return l.h > r.h;
	}
};

/**
 * Creates a vector of nodes represending the Buckminsterfullerene structure in 3d space
 */
vector<Node*> GenerateNodes() {
	float o = (1.0f + sqrt(5.0f)) / 2.0f;
	vector<Node*> nodes;
	int i = 0;
	std::string title = "node ";

	// add nodes to vector
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(0, 1, 3 * o)          , SColor(255,255,0,0), true )); // 00
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(0, 1, -3 * o)         , SColor(255,255,0,0), true )); // 01
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(0, -1, 3* o)          , SColor(255,255,0,0), true )); // 02
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(0, -1, -3* o)         , SColor(255,255,0,0), true )); // 03
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(1, 3* o, 0)           , SColor(255,255,0,0), true )); // 04
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(1, -3* o, 0)          , SColor(255,255,0,0), true )); // 05
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-1, 3* o, 0)          , SColor(255,255,0,0), true )); // 06
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-1, -3* o, 0)         , SColor(255,255,0,0), true )); // 07
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(3* o, 0, 1)           , SColor(255,255,0,0), true )); // 08
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(3* o, 0, -1)          , SColor(255,255,0,0), true )); // 09
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-3* o, 0, 1)          , SColor(255,255,0,0), true )); // 10
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-3* o, 0, -1)         , SColor(255,255,0,0), true )); // 11
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2, (1 + 2* o), o)     , SColor(255,255,0,0), true )); // 12
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2, (1 + 2* o), -o)    , SColor(255,255,0,0), true )); // 13
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2, -(1 + 2* o), o)    , SColor(255,255,0,0), true )); // 14
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2, -(1 + 2* o), -o)   , SColor(255,255,0,0), true )); // 15
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2, (1 + 2* o), o)    , SColor(255,255,0,0), true )); // 16
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2, (1 + 2* o), -o)   , SColor(255,255,0,0), true )); // 17
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2, -(1 + 2* o), o)   , SColor(255,255,0,0), true )); // 18
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2, -(1 + 2* o), -o)  , SColor(255,255,0,0), true )); // 19
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((1 + 2* o), o, 2)     , SColor(255,255,0,0), true )); // 20
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((1 + 2* o), o, -2)    , SColor(255,255,0,0), true )); // 21
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((1 + 2* o), -o, 2)    , SColor(255,255,0,0), true )); // 22
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((1 + 2* o), -o, -2)   , SColor(255,255,0,0), true )); // 23
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(1 + 2* o), o, 2)    , SColor(255,255,0,0), true )); // 24
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(1 + 2* o), o, -2)   , SColor(255,255,0,0), true )); // 25
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(1 + 2* o), -o, 2)   , SColor(255,255,0,0), true )); // 26
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(1 + 2* o), -o, -2)  , SColor(255,255,0,0), true )); // 27
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(o, 2, (1 + 2* o))     , SColor(255,255,0,0), true )); // 28
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(o, 2, -(1 + 2* o))    , SColor(255,255,0,0), true )); // 29
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(o, -2, (1 + 2* o))    , SColor(255,255,0,0), true )); // 30
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(o, -2, -(1 + 2* o))   , SColor(255,255,0,0), true )); // 31
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-o, 2, (1 + 2* o))    , SColor(255,255,0,0), true)); // 32
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-o, 2, -(1 + 2* o))   , SColor(255,255,0,0), true )); // 33
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-o, -2, (1 + 2* o))   , SColor(255,255,0,0), true )); // 34
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-o, -2, -(1 + 2* o))  , SColor(255,255,0,0), true )); // 35
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(1, (2 + o), 2* o)     , SColor(255,255,0,0), true )); // 36
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(1, (2 + o), -2* o)    , SColor(255,255,0,0), true )); // 37
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(1, -(2 + o), 2* o)    , SColor(255,255,0,0), true )); // 38
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(1, -(2 + o), -2* o)   , SColor(255,255,0,0), true )); // 39
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-1, (2 + o), 2* o)    , SColor(255,255,0,0), true )); // 40
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-1, (2 + o), -2* o)   , SColor(255,255,0,0), true )); // 41
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-1, -(2 + o), 2* o)   , SColor(255,255,0,0), true )); // 42
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-1, -(2 + o), -2* o)  , SColor(255,255,0,0), true )); // 43
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((2 + o), 2* o, 1)     , SColor(255,255,0,0), true )); // 44
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((2 + o), 2* o, -1)    , SColor(255,255,0,0), true )); // 45
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((2 + o), -2* o, 1)    , SColor(255,255,0,0), true )); // 46
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((2 + o), -2* o, -1)   , SColor(255,255,0,0), true )); // 47
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(2 + o), 2* o, 1)    , SColor(255,255,0,0), true )); // 48
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(2 + o), 2* o, -1)   , SColor(255,255,0,0), true )); // 49
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(2 + o), -2* o, 1)   , SColor(255,255,0,0), true )); // 50
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(2 + o), -2* o, -1)  , SColor(255,255,0,0), true )); // 51
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2* o, 1, (2 + o))     , SColor(255,255,0,0), true )); // 52
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2* o, 1, -(2 + o))    , SColor(255,255,0,0), true )); // 53
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2* o, -1, (2 + o))    , SColor(255,255,0,0), true )); // 54
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2* o, -1, -(2 + o))   , SColor(255,255,0,0), true )); // 55
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2 * o, 1, (2 + o))   , SColor(255,255,0,0), true )); // 56
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2 * o, 1, -(2 + o))  , SColor(255,255,0,0), true )); // 57
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2 * o, -1, (2 + o))  , SColor(255,255,0,0), true )); // 58
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2 * o, -1, -(2 + o)) , SColor(255,255,0,0), true )); // 59

	/**
	 * Create edges between nodes. Each node has 3 shortest edges, so the below
	 * algorithm iterates over the list of nodes and finds the 3 shortest paths.
	 * These 3 nodes are added to the edges list.
	 */
	for (u32 i = 0; i < nodes.size(); ++i) {
		for (u32 j = 0; j < nodes.size(); ++j) {
			if (i != j) {
				f32 distance = nodes[i]->position.getDistanceFrom(nodes[j]->position);
				if (nodes[i]->edges.size() < 3) {
					// add it straight away
					nodes[i]->edges.push_back(Edge(distance, nodes[i], nodes[j]));
				}
				else {
					// check if it is smaller than any of the edges currently stored
					u32 largestIndex = 0;
					f32 largestDistance = nodes[i]->edges[0].weight;

					for (u32 k = 1; k < nodes[i]->edges.size(); ++k) {

						// first we need to find the largest value in the edges array
						if (nodes[i]->edges[k].weight > largestDistance) {
							largestIndex = k;
							largestDistance = nodes[i]->edges[k].weight;
						}
					}

					// if our new edge is smaller than the largest, we replace it
					if (distance < largestDistance) {
						nodes[i]->edges[largestIndex].weight = distance;
						nodes[i]->edges[largestIndex].to = nodes[j];
					}

				}
			}
		}
	}

	return nodes;
}

/**
 * Prints out the list of nodes to std out.
 * Used for debug purposes.
 */
void PrintNodes(vector<Node*> nodes) {
	for (u32 i = 0; i < nodes.size(); ++i) {
		cout << i << " " << nodes[i] << ": e(" << nodes[i]->edges.size() << "):";
		// for each edge
		for (u32 j = 0; j < nodes[i]->edges.size(); ++j) {
			// iterate over the nodes and locate the index of which node it points to

			cout << "(edge " << j << ": ";
			//for (u32 k = 0; k < nodes.size(); ++k) {
			//	if (nodes[i].edges[j].to == &nodes[k]) cout << k;
			//}
			cout << nodes[i]->edges[j].to->name << ")";
		}
		cout << endl;
	}
}

/**
 * Removes the node from the list
 */
void listRemove(vector<Node*> &nodes, Node* value) {
	nodes.erase(std::remove(nodes.begin(), nodes.end(), value), nodes.end());
}

/**
 * Adds the node to the list
 */
void listAdd(vector<Node*> &nodes, Node* value) {
	nodes.push_back(value);
}

/**
 * Sorts the list
 */
void listSort(vector<Node*> &nodes) {
	struct PointerCompare {
		bool operator()(const Node* l, const Node* r) {
			return (*l).h < (*r).h;
		}
	};
	std::sort(nodes.begin(), nodes.end(), PointerCompare());
}

/**
 * Finds the index of the item in the list. If it is not found, return -1
 */
int listFind(vector<Node*> &nodes, Node* value) {
	vector<Node*>::iterator it = std::find(nodes.begin(), nodes.end(), value);
	// return -1 if not found
	if (it == nodes.end()) return -1;
	// return the index of the element
	return (it - nodes.begin());
}

/**
 * A path finding algorithm based on the A star algorithm.
 * resources used:
 * http://www.policyalmanac.org/games/aStarTutorial.htm
 * (the explanation of the algorithm, not the C++ code)
 * https://www.youtube.com/watch?v=C0qCR18gXdU
 * (watched to gain a high level understanding of how the algorithm works)
 */
vector<Node*> AStarPathAlgorithm(vector<Node*> nodes, Node* start, Node* end) {
	// 0. Reset all algorithm info on nodes
	for (u16 i = 0; i < nodes.size(); ++i) {
		nodes[i]->visited = false;
		nodes[i]->f = 0.0f;
		nodes[i]->g = 0.0f;
		nodes[i]->h = 0.0f;
		nodes[i]->parent = nullptr;
	}

	Node* current = start;
	std::vector<Node*> openList;
	std::vector<Node*> closedList;

	bool pathFound = false;
	vector<Node*> path;

	// add CURRENT to open list
	listAdd(openList, current);

	while (!pathFound) {
		if (openList.size() > 0) {

			// sort the lists
			listSort(openList);
			listSort(closedList); // TODO: do we need to sort the closed list?

			// set current node to be smallest in openlist
			current = openList[0];
			cout << current->name << endl;
			// drop current node from open list and add to closed list
			listRemove(openList, current);
			listAdd(closedList, current);

			// check if we have found our destination
			if (current == end) {
				// target found!
				break;
			}

			// iterate over connected nodes
			for (u8 i = 0; i < current->edges.size(); ++i) {
				Node* connected = current->edges[i].to;

				// ignore flag: set ignore to true if the node is impassible
				// or the node is in the closed list
				bool ignore = (!connected->passable || listFind(closedList, connected) != -1);

				if (!ignore) {
					// add connected node to open list
					if (listFind(openList, connected) == -1) {
						// not in list so add it
						listAdd(openList, connected);
						// set all connected nodes parent to be THIS node
						connected->parent = current;
						// calculate G
						connected->g = current->g + current->edges[i].weight;
						// calculate H
						connected->h = connected->position.getDistanceFrom(end->position);
						// calculate F
						connected->f = connected->g + connected->h;
					} else {
						// is already in list, so need to possibly update it
						f32 newG = current->g + current->edges[i].weight;
						bool betterPathExists = newG < connected->g;
						if (betterPathExists) {
							connected->parent = current;
							connected->g = current->g + current->edges[i].weight;
							connected->f = connected->g + connected->h;
							listSort(openList); // resort list
						}
					}
				}
			}
		}
		else {
			cout << "OPENLIST EMPTY" << endl;
			break;
		}
	}

	if (current == end) {
		// PATH FOUND
		while (current != start) {
			path.push_back(current);
			current = current->parent;
		}
		path.push_back(current); // push the final (end) node into the path
	}
	else {
		// NO PATH FOUND
		cout << "Really, no path found." << endl;
	}
	return path;
}

enum
{
	// flag = node is not pickable
	ID_IsNotPickable = 0,

	// flag = node is pickable by ray selection
	IDFlag_IsPickable = 1 << 0
};

int main(void) {

	s32 input_start = -1;
	s32 input_end = -1;

	while (input_start < 0 || input_start > 59 || input_end < 0 || input_end > 59) {
		cout << "Please enter the start node (0-59): ";
		cin >> input_start;
		cout << endl << "Please enter the end node (0-59): ";
		cin >> input_end;
	}

	// generate the nodes
	vector<Node*> nodes = GenerateNodes();

	s32 input_impassable = 0;
	while (input_impassable != -1) {
		cout << "All nodes are passable by default. Enter a node ID to make it impassible. -1 to continue: ";
		cin >> input_impassable;
		if (input_impassable >= 0 && input_impassable < 60) {
			nodes[input_impassable]->passable = false;
		}
	}

	// instance of event receiver
	MyEventReceiver receiver;

	device = createDevice(video::EDT_OPENGL, core::dimension2d<u32>(640, 480), 32, false, false, false, &receiver);
	if (!device) return 1;

	// set window title
	device->setWindowCaption(L"Path Planning");

	IVideoDriver* driver = device->getVideoDriver();
	ISceneManager* smgr = device->getSceneManager();
	IGUIEnvironment* gui = device->getGUIEnvironment();
	
	io::path vsFileName; // filename for the vertex shader
	io::path psFileName; // filename for the pixel shader
	psFileName = "./res/shader/opengl.frag";
	vsFileName = "./res/shader/opengl.vert";

	// create materials
	video::IGPUProgrammingServices* gpu = driver->getGPUProgrammingServices();
	s32 redMaterialType = 0;
	s32 greenMaterialType = 0;
	s32 yellowMaterialType = 0;

	// instance of shader callback
	NodeShaderCallBack* redShader = new NodeShaderCallBack();
	redShader->setColour(SColorf(1.0f, 0.0f, 0.0f, 0.5f));
	NodeShaderCallBack* yellowShader = new NodeShaderCallBack();
	yellowShader->setColour(SColorf(1.0f, 1.0f, 0.0f, 0.5f));
	NodeShaderCallBack* greenShader = new NodeShaderCallBack();
	greenShader->setColour(SColorf(0.0f, 1.0f, 0.0f, 0.5f));

	// select shading language as GLSLs
	const video::E_GPU_SHADING_LANGUAGE shadingLanguage = video::EGSL_DEFAULT;

	// create material from high level shaders (hlsl, glsl or cg)

	redMaterialType = gpu->addHighLevelShaderMaterialFromFiles(
		vsFileName, "vertexMain", video::EVST_VS_1_1,
		psFileName, "pixelMain", video::EPST_PS_1_1,
		redShader, video::EMT_TRANSPARENT_ADD_COLOR, 0, shadingLanguage);


	greenMaterialType = gpu->addHighLevelShaderMaterialFromFiles(
		vsFileName, "vertexMain", video::EVST_VS_1_1,
		psFileName, "pixelMain", video::EPST_PS_1_1,
		greenShader, video::EMT_TRANSPARENT_ADD_COLOR, 0, shadingLanguage);


	yellowMaterialType = gpu->addHighLevelShaderMaterialFromFiles(
		vsFileName, "vertexMain", video::EVST_VS_1_1,
		psFileName, "pixelMain", video::EPST_PS_1_1,
		yellowShader, video::EMT_TRANSPARENT_ADD_COLOR, 0, shadingLanguage);

	redShader->drop();
	yellowShader->drop();
	greenShader->drop();

	// print node info
	PrintNodes(nodes);

	/**
	* PATH
	* FINDING
	* ALGORITHM
	* HERE
	*/
	vector<Node*> path = AStarPathAlgorithm(nodes, nodes[input_start], nodes[input_end]);

	// print path
	cout << "PATH LENGTH = " << path.size() << endl;
	for (int i = path.size() - 1; i >= 0; --i) {
		cout << path[i]->name << "->";
	}
	cout << "END." << endl;

	IVertexBuffer* lineVertexBuffer = new CVertexBuffer(EVT_STANDARD);
	IIndexBuffer* lineIndexBuffer = new CIndexBuffer(EIT_16BIT);
	SMaterial lineMaterial = SMaterial();
	lineMaterial.Thickness = 3;
	lineMaterial.MaterialType = EMT_TRANSPARENT_ALPHA_CHANNEL;
	lineMaterial.AmbientColor = SColor(255,255,255,255);

	for (u32 i = 0; i < nodes.size(); ++i) {
		std::string name();
		scene::ISceneNode* node = smgr->addSphereSceneNode(0.2f, 512,0, IDFlag_IsPickable);
		node->setMaterialTexture(0, driver->getTexture("./res/portal7.bmp"));
		node->setMaterialFlag(EMF_BLEND_OPERATION, true);
		node->setPosition(nodes[i]->position);
		node->setName(nodes[i]->name.c_str());
		node->setID(IDFlag_IsPickable);
		if (i == input_start) {
			// start node
			node->setDebugDataVisible(true);
			node->setMaterialType((E_MATERIAL_TYPE)yellowMaterialType);
		}
		else if (i == input_end) {
			// end node
			node->setDebugDataVisible(true);
			node->setMaterialType((E_MATERIAL_TYPE)yellowMaterialType);
		} else if (listFind(path, nodes[i]) != -1) {
			node->setMaterialType((E_MATERIAL_TYPE)redMaterialType);
		}
		else {
			node->setMaterialType((E_MATERIAL_TYPE)greenMaterialType);
			node->setMaterialTexture(0, driver->getTexture("./res/wall.bmp"));
		}

		smgr->addTextSceneNode(gui->getBuiltInFont(),
			std::to_wstring(i).c_str(),
			video::SColor(255, 255, 255, 255), node, vector3df(0,0,0.05f), ID_IsNotPickable);
		
		lineVertexBuffer->push_back(S3DVertex(nodes[i]->position, vector3df(), SColor(255,0, 255,0), vector2df()));
		lineVertexBuffer->push_back(S3DVertex(nodes[i]->edges[0].to->position, vector3df(), nodes[i]->edges[0].to->color, vector2df()));
		lineVertexBuffer->push_back(S3DVertex(nodes[i]->position, vector3df(), nodes[i]->color, vector2df()));
		lineVertexBuffer->push_back(S3DVertex(nodes[i]->edges[1].to->position, vector3df(), nodes[i]->edges[1].to->color, vector2df()));
		lineVertexBuffer->push_back(S3DVertex(nodes[i]->position, vector3df(), nodes[i]->color, vector2df()));
		lineVertexBuffer->push_back(S3DVertex(nodes[i]->edges[2].to->position, vector3df(), nodes[i]->edges[2].to->color, vector2df()));
		lineIndexBuffer->push_back((i * 6) + 0);
		lineIndexBuffer->push_back((i * 6) + 1);
		lineIndexBuffer->push_back((i * 6) + 2);
		lineIndexBuffer->push_back((i * 6) + 3);
		lineIndexBuffer->push_back((i * 6) + 4);
		lineIndexBuffer->push_back((i * 6) + 5);
	}

	// add a nice skybox

	driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);
	smgr->addSkyBoxSceneNode(
		driver->getTexture("./res/irrlicht2_up.jpg"),
		driver->getTexture("./res/irrlicht2_dn.jpg"),
		driver->getTexture("./res/irrlicht2_lf.jpg"),
		driver->getTexture("./res/irrlicht2_rt.jpg"),
		driver->getTexture("./res/irrlicht2_ft.jpg"),
		driver->getTexture("./res/irrlicht2_bk.jpg"));
	driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, true);

	// add a camera and disable the mouse cursor

	scene::ICameraSceneNode* cam = smgr->addCameraSceneNodeFPS(0, 100.0f, 0.3f, ID_IsNotPickable, 0, 0, false);
	cam->setPosition(core::vector3df(2, 5, 10));
	cam->setTarget(core::vector3df(0, 0, 0));
	cam->setName("cam");
	device->getCursorControl()->setVisible(false);

	int lastFPS = -1;

	// In order to do framerate independent movement, we have to know
	// how long it was since the last frame
	u32 then = device->getTimer()->getTime();

	// This is the movemen speed in units per second.
	const f32 MOVEMENT_SPEED = 5.f;

	while (device->run())
		if (device->isWindowActive())
		{
			driver->beginScene(true, true, video::SColor(255, 0, 0, 0));

			// Work out a frame delta time.
			const u32 now = device->getTimer()->getTime();
			const f32 frameDeltaTime = (f32)(now - then) / 1000.f; // Time in seconds
			then = now;

			smgr->drawAll();

			// sine function for displaying the path
			static float s = 0.0f;
			s += 0.1f;

			// iterate over scene nodes that are part of the path and make the do something
			for (u32 i = 0; i < path.size(); ++i) {
				ISceneNode* n = smgr->getSceneNodeFromName(path[i]->name.c_str());
				n->setScale(vector3df((sin(s) * 0.1f)  + 1.0f));
			}

			driver->setMaterial(lineMaterial);

			driver->setTransform(ETS_WORLD, matrix4());

			// draw lines
			smgr->getVideoDriver()->drawVertexPrimitiveList(
				lineVertexBuffer->pointer(), lineVertexBuffer->size(),
				lineIndexBuffer->pointer(), lineVertexBuffer->size() / 2,
				EVT_STANDARD,
				EPT_LINES,
				EIT_16BIT);

			/* Check if keys W, S, A or D are being held down, and move the
			sphere node around respectively. */
			vector3df camPosition = cam->getPosition();

			if (receiver.IsKeyDown(KEY_KEY_W))
				camPosition.Y += MOVEMENT_SPEED * frameDeltaTime;
			else if (receiver.IsKeyDown(KEY_KEY_S))
				camPosition.Y -= MOVEMENT_SPEED * frameDeltaTime;

			if (receiver.IsKeyDown(KEY_KEY_A))
				camPosition.X += MOVEMENT_SPEED * frameDeltaTime;
			else if (receiver.IsKeyDown(KEY_KEY_D))
				camPosition.X -= MOVEMENT_SPEED * frameDeltaTime;

			if (receiver.IsKeyDown(KEY_KEY_E))
				camPosition.Z -= MOVEMENT_SPEED * frameDeltaTime;
			else if (receiver.IsKeyDown(KEY_KEY_Q))
				camPosition.Z += MOVEMENT_SPEED * frameDeltaTime;
			
			driver->endScene();

			cam->setPosition(camPosition);

			int fps = driver->getFPS();

			if (lastFPS != fps)
			{
				core::stringw str = L"Buckminsterfullerene A* Algorithm [";
				str += driver->getName();
				str += "] FPS:";
				str += fps;

				device->setWindowCaption(str.c_str());
				lastFPS = fps;
			}
		}

	device->drop();

	return 0;
}