#include <irrlicht.h>
#include <iostream>
#include <vector>
#include <string>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

using std::vector;
using std::string;
using std::cout;
using std::endl;

#pragma comment(lib, "Irrlicht.lib")

IrrlichtDevice* device = 0;

/*
To receive events like mouse and keyboard input, or GUI events like "the OK
button has been clicked", we need an object which is derived from the
irr::IEventReceiver object. There is only one method to override:
irr::IEventReceiver::OnEvent(). This method will be called by the engine once
when an event happens. What we really want to know is whether a key is being
held down, and so we will remember the current state of each key.
*/
class MyEventReceiver : public IEventReceiver
{
public:
	// This is the one method that we have to implement
	virtual bool OnEvent(const SEvent& event)
	{
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



class MyShaderCallBack : public video::IShaderConstantSetCallBack
{
public:
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

		video::SColorf col(0.0f, 1.0f, 1.0f, 0.0f);
		services->setVertexShaderConstant("mLightColor", reinterpret_cast<f32*>(&col), 4);

		// set transposed world matrix

		core::matrix4 world = driver->getTransform(video::ETS_WORLD);
		world = world.getTransposed();

		services->setVertexShaderConstant("mTransWorld", world.pointer(), 16);

		// set texture, for textures you can use both an int and a float setPixelShaderConstant interfaces (You need it only for an OpenGL driver).
		s32 TextureLayerID = 0;
		services->setPixelShaderConstant("myTexture", &TextureLayerID, 1);
	}
};

struct Node;

struct Edge {
	f32 weight;
	Node* from;
	Node* to;
	Edge() : from(nullptr), to(nullptr), weight(0.0f) {}
	Edge(f32 weight, Node* from, Node* to) : from(from), to(to), weight(weight) {}
};

struct Node {
	std::string name;
	vector3df position;
	SColor color;
	vector<Edge> edges;
	Edge* parentEdge;
	bool visited;
	u32 f;
	u32 g;
	u32 h;
	void reset() { f = g = h = 0; }
	Node(std::string name, vector3df pos, SColor color) : f(0), g(0), h(0), position(pos), color(color), visited(false), name(name), parentEdge(nullptr) {};
};

vector<Node*> GenerateNodes() {
	float o = (1.0f + sqrt(5.0f)) / 2.0f;
	vector<Node*> nodes;
	int i = 0;
	std::string title = "node ";
	// add nodes to vector
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(0, 1, 3 * o)          , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(0, 1, -3 * o)         , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(0, -1, 3* o)          , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(0, -1, -3* o)         , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(1, 3* o, 0)           , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(1, -3* o, 0)          , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-1, 3* o, 0)          , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-1, -3* o, 0)         , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(3* o, 0, 1)           , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(3* o, 0, -1)          , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-3* o, 0, 1)          , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-3* o, 0, -1)         , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2, (1 + 2* o), o)     , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2, (1 + 2* o), -o)    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2, -(1 + 2* o), o)    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2, -(1 + 2* o), -o)   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2, (1 + 2* o), o)    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2, (1 + 2* o), -o)   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2, -(1 + 2* o), o)   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2, -(1 + 2* o), -o)  , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((1 + 2* o), o, 2)     , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((1 + 2* o), o, -2)    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((1 + 2* o), -o, 2)    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((1 + 2* o), -o, -2)   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(1 + 2* o), o, 2)    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(1 + 2* o), o, -2)   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(1 + 2* o), -o, 2)   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(1 + 2* o), -o, -2)  , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(o, 2, (1 + 2* o))     , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(o, 2, -(1 + 2* o))    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(o, -2, (1 + 2* o))    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(o, -2, -(1 + 2* o))   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-o, 2, (1 + 2* o))    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-o, 2, -(1 + 2* o))   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-o, -2, (1 + 2* o))   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-o, -2, -(1 + 2* o))  , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(1, (2 + o), 2* o)     , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(1, (2 + o), -2* o)    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(1, -(2 + o), 2* o)    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(1, -(2 + o), -2* o)   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-1, (2 + o), 2* o)    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-1, (2 + o), -2* o)   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-1, -(2 + o), 2* o)   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-1, -(2 + o), -2* o)  , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((2 + o), 2* o, 1)     , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((2 + o), 2* o, -1)    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((2 + o), -2* o, 1)    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df((2 + o), -2* o, -1)   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(2 + o), 2* o, 1)    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(2 + o), 2* o, -1)   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(2 + o), -2* o, 1)   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-(2 + o), -2* o, -1)  , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2* o, 1, (2 + o))     , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2* o, 1, -(2 + o))    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2* o, -1, (2 + o))    , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(2* o, -1, -(2 + o))   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2 * o, 1, (2 + o))   , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2 * o, 1, -(2 + o))  , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2 * o, -1, (2 + o))  , SColor(255,255,0,0) ));
	nodes.push_back(new Node(title + std::to_string(i++), vector3df(-2 * o, -1, -(2 + o)) , SColor(255,255,0,0) ));

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
			cout << nodes[i]->edges[j].to << ")";
		}
		cout << endl;
	}
}

vector<Edge> AStarPathAlgorithm(vector<Node*> nodes, Node* start, Node* end) {
	// G = Distance from Start
	// H = Distance to End
	// F = G + H
	
	// 0. Reset all algorithm info on nodes
	for (u16 i = 0; i < nodes.size(); ++i) {
		nodes[i]->visited = false;
		nodes[i]->f = 0.0f;
		nodes[i]->g = 0.0f;
		nodes[i]->h = 0.0f;
		nodes[i]->parentEdge = nullptr;
	}

	vector<Edge> path;
	Node* currentNode = start;
	bool finished = false;

	while (!finished) {
		// 1. Mark a block off list
		currentNode->visited = true;

		Node* lowestFNode = nullptr;

		// 2. Analyse adjacent blocks
		for (u8 i = 0; i < currentNode->edges.size(); ++i) {
			// if we have not visited the connected node ...
			if (!currentNode->edges[i].to->visited) {
				
				Node* connectedNode = currentNode->edges[i].to;

				// check if we've reached the target
				if (connectedNode != end) {
					// set parent to be the current node
					connectedNode->parentEdge = &currentNode->edges[i];

					// calculate g
					connectedNode->g = currentNode->g + currentNode->edges[i].weight;
					// calculate h
					connectedNode->h = currentNode->position.getDistanceFrom(connectedNode->position);
					// calculate f
					connectedNode->f = connectedNode->g + connectedNode->h;

					// update lowest f node pointer
					if (!lowestFNode || connectedNode->f < lowestFNode->f)
						lowestFNode = connectedNode;
				}
				else {
					// SUCCESS! Build path and return it
					connectedNode->parentEdge = &currentNode->edges[i];
					currentNode = connectedNode;
					while (currentNode != start) {
						path.push_back(Edge(*(currentNode->parentEdge)));
						currentNode = currentNode->parentEdge->from;
					}
					return path;
				}
			}
		}

		// 3. Pick block with lowest F
		currentNode = lowestFNode;
	}
}

int main(void) {

	// create device
	MyEventReceiver receiver;

	device = createDevice(video::EDT_OPENGL, core::dimension2d<u32>(640, 480), 32, false, false, false, &receiver);
	if (!device) return 1;

	device->setWindowCaption(L"Path Planning");

	IVideoDriver* driver = device->getVideoDriver();
	ISceneManager* smgr = device->getSceneManager();
	IGUIEnvironment* gui = device->getGUIEnvironment();

	io::path vsFileName; // filename for the vertex shader
	io::path psFileName; // filename for the pixel shader

	psFileName = "./res/opengl.frag";
	vsFileName = "./res/opengl.vert";
		
	// create materials

	video::IGPUProgrammingServices* gpu = driver->getGPUProgrammingServices();
	s32 newMaterialType1 = 0;
	s32 newMaterialType2 = 0;

	MyShaderCallBack* mc = new MyShaderCallBack();

	// create the shaders depending on if the user wanted high level
	// or low level shaders:

	// Choose the desired shader type. Default is the native
	// shader type for the driver, for Cg pass the special
	// enum value EGSL_CG
	const video::E_GPU_SHADING_LANGUAGE shadingLanguage = video::EGSL_DEFAULT;

	// create material from high level shaders (hlsl, glsl or cg)

	newMaterialType1 = gpu->addHighLevelShaderMaterialFromFiles(
		vsFileName, "vertexMain", video::EVST_VS_1_1,
		psFileName, "pixelMain", video::EPST_PS_1_1,
		mc, video::EMT_SOLID, 0, shadingLanguage);

	newMaterialType2 = gpu->addHighLevelShaderMaterialFromFiles(
		vsFileName, "vertexMain", video::EVST_VS_1_1,
		psFileName, "pixelMain", video::EPST_PS_1_1,
		mc, video::EMT_TRANSPARENT_ADD_COLOR, 0, shadingLanguage);
		
	mc->drop();


	vector<Node*> nodes = GenerateNodes();
	PrintNodes(nodes);


	/**
	* PATH
	* FINDING
	* ALGORITHM
	* HERE
	*/
	vector<Edge> path = AStarPathAlgorithm(nodes, nodes[0], nodes[1]);
	
	cout << "Path Found: Size = " << path.size() << endl;
	for (int i = 0; i < path.size(); ++i) {
		cout << "from: " << path[i].from->name << " to: " << path[i].to->name << " weight: " << path[i].weight << endl;
	}

	
	// buffers for lines between nodes
	//S3DVertex pVertexBuffer[2];
	//pVertexBuffer[0] = S3DVertex(vector3df(), vector3df(), SColor(), vector2df());
	//pVertexBuffer[1] = S3DVertex(vector3df(0, 100, 0), vector3df(), SColor(), vector2df());
	//
	//u16 pIndexBuffer[2] = { 0,1 };

	IVertexBuffer* lineVertexBuffer = new CVertexBuffer(EVT_STANDARD);
	IIndexBuffer* lineIndexBuffer = new CIndexBuffer(EIT_16BIT);
	SMaterial lineMaterial = SMaterial();
	lineMaterial.Thickness = 1;

	for (u32 i = 0; i < nodes.size(); ++i) {
		std::string name();
		scene::ISceneNode* node = smgr->addCubeSceneNode(0.01f);
		node->setPosition(nodes[i]->position);
		node->setMaterialTexture(0, driver->getTexture("./res/wall.bmp"));
		node->setMaterialFlag(video::EMF_LIGHTING, false);
		node->setMaterialType((video::E_MATERIAL_TYPE)newMaterialType1);

		smgr->addTextSceneNode(gui->getBuiltInFont(),
			std::to_wstring(i).c_str(),
			video::SColor(255, 18, 48, 12), node);
		
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

	scene::ICameraSceneNode* cam = smgr->addCameraSceneNodeFPS();
	cam->setPosition(core::vector3df(2, 5, 10));
	cam->setTarget(core::vector3df(0, 0, 0));
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

			smgr->drawAll();

			driver->setMaterial(lineMaterial);

			driver->setTransform(ETS_WORLD, matrix4());
			// draw lines
			smgr->getVideoDriver()->drawVertexPrimitiveList(
				lineVertexBuffer->pointer(), lineVertexBuffer->size(),
				lineIndexBuffer->pointer(), lineVertexBuffer->size() / 2,
				EVT_STANDARD,
				EPT_LINES,
				EIT_16BIT);

			driver->endScene();

			// Work out a frame delta time.
			const u32 now = device->getTimer()->getTime();
			const f32 frameDeltaTime = (f32)(now - then) / 1000.f; // Time in seconds
			then = now;

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

			cam->setPosition(camPosition);

			int fps = driver->getFPS();

			if (lastFPS != fps)
			{
				core::stringw str = L"Irrlicht Engine - Vertex and pixel shader example [";
				str += driver->getName();
				str += "] FPS:";
				str += fps;

				device->setWindowCaption(str.c_str());
				lastFPS = fps;
			}
		}

	device->drop();

	//vector<Node> nodes = GenerateNodes();
	//
	//while (device->run()) {
	//	driver->beginScene(true, true, SColor(255, 100, 101, 140));
	//
	//	driver->draw2DPolygon(vector2d<s32>(50,50), 50.0f, SColor(255, 255, 255, 255), 10);
	//
	//	driver->draw3DTriangle(triangle3df(vector3df(-1, -1, 0), vector3df(0, 1, 0), vector3df(1, -1, 0)));
	//
	//	//for (int i = 0; i < nodes.size(); i++) {
	//	//	//driver->draw3DLine(nodes[i].position, nodes[(i + 1) % (nodes.size()-1)].position, SColor(255, 255, 255, 255));
	//	//	driver->draw3DBox()
	//	//}
	//
	//	//smgr->drawAll();
	//	//guienv->drawAll();
	//
	//	driver->endScene();
	//}

	return 0;
}