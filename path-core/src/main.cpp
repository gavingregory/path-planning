#include <irrlicht.h>
#include <iostream>
#include <vector>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

using std::vector;

#pragma comment(lib, "Irrlicht.lib")

struct Node {
	vector3df position;
};

vector<Node> GenerateNodes() {
	float o = (1.0f + sqrt(5.0f)) / 2.0f;
	vector<Node> nodes;

	nodes.push_back(Node{ vector3df(0, 1, -3 * o) });
	nodes.push_back(Node{vector3df(0, -1, 3* o) });
	nodes.push_back(Node{vector3df(0, -1, -3* o) });
	nodes.push_back(Node{vector3df(1, 3* o, 0) });
	nodes.push_back(Node{vector3df(1, -3* o, 0) });
	nodes.push_back(Node{vector3df(-1, 3* o, 0) });
	nodes.push_back(Node{vector3df(-1, -3* o, 0) });
	nodes.push_back(Node{vector3df(3* o, 0, 1) });
	nodes.push_back(Node{vector3df(3* o, 0, -1) });
	nodes.push_back(Node{vector3df(-3* o, 0, 1) });
	nodes.push_back(Node{vector3df(-3* o, 0, -1) });
	nodes.push_back(Node{vector3df(2, (1 + 2* o), o) });
	nodes.push_back(Node{vector3df(2, (1 + 2* o), -o) });
	nodes.push_back(Node{vector3df(2, -(1 + 2* o), o) });
	nodes.push_back(Node{vector3df(2, -(1 + 2* o), -o) });
	nodes.push_back(Node{vector3df(-2, (1 + 2* o), o) });
	nodes.push_back(Node{vector3df(-2, (1 + 2* o), -o) });
	nodes.push_back(Node{vector3df(-2, -(1 + 2* o), o) });
	nodes.push_back(Node{vector3df(-2, -(1 + 2* o), -o) });
	nodes.push_back(Node{vector3df((1 + 2* o), o, 2) });
	nodes.push_back(Node{vector3df((1 + 2* o), o, -2) });
	nodes.push_back(Node{vector3df((1 + 2* o), -o, 2) });
	nodes.push_back(Node{vector3df((1 + 2* o), -o, -2) });
	nodes.push_back(Node{vector3df(-(1 + 2* o), o, 2) });
	nodes.push_back(Node{vector3df(-(1 + 2* o), o, -2) });
	nodes.push_back(Node{vector3df(-(1 + 2* o), -o, 2) });
	nodes.push_back(Node{vector3df(-(1 + 2* o), -o, -2) });
	nodes.push_back(Node{vector3df(o, 2, (1 + 2* o)) });
	nodes.push_back(Node{vector3df(o, 2, -(1 + 2* o)) });
	nodes.push_back(Node{vector3df(o, -2, (1 + 2* o)) });
	nodes.push_back(Node{vector3df(o, -2, -(1 + 2* o)) });
	nodes.push_back(Node{vector3df(-o, 2, (1 + 2* o)) });
	nodes.push_back(Node{vector3df(-o, 2, -(1 + 2* o)) });
	nodes.push_back(Node{vector3df(-o, -2, (1 + 2* o)) });
	nodes.push_back(Node{vector3df(-o, -2, -(1 + 2* o)) });
	nodes.push_back(Node{vector3df(1, (2 + o), 2* o) });
	nodes.push_back(Node{vector3df(1, (2 + o), -2* o) });
	nodes.push_back(Node{vector3df(1, -(2 + o), 2* o) });
	nodes.push_back(Node{vector3df(1, -(2 + o), -2* o) });
	nodes.push_back(Node{vector3df(-1, (2 + o), 2* o) });
	nodes.push_back(Node{vector3df(-1, (2 + o), -2* o) });
	nodes.push_back(Node{vector3df(-1, -(2 + o), 2* o) });
	nodes.push_back(Node{vector3df(-1, -(2 + o), -2* o) });
	nodes.push_back(Node{vector3df((2 + o), 2* o, 1) });
	nodes.push_back(Node{vector3df((2 + o), 2* o, -1) });
	nodes.push_back(Node{vector3df((2 + o), -2* o, 1) });
	nodes.push_back(Node{vector3df((2 + o), -2* o, -1) });
	nodes.push_back(Node{vector3df(-(2 + o), 2* o, 1) });
	nodes.push_back(Node{vector3df(-(2 + o), 2* o, -1) });
	nodes.push_back(Node{vector3df(-(2 + o), -2* o, 1) });
	nodes.push_back(Node{vector3df(-(2 + o), -2* o, -1) });
	nodes.push_back(Node{vector3df(2* o, 1, (2 + o)) });
	nodes.push_back(Node{vector3df(2* o, 1, -(2 + o)) });
	nodes.push_back(Node{vector3df(2* o, -1, (2 + o)) });
	nodes.push_back(Node{vector3df(2* o, -1, -(2 + o)) });
	nodes.push_back(Node{ vector3df(-2 * o, 1, (2 + o)) });
	nodes.push_back(Node{ vector3df(-2 * o, 1, -(2 + o)) });
	nodes.push_back(Node{ vector3df(-2 * o, -1, (2 + o)) });
	nodes.push_back(Node{ vector3df(-2 * o, -1, -(2 + o)) });

	return nodes;
}

int main(void) {
	
	IrrlichtDevice* device = createDevice(video::EDT_OPENGL, core::dimension2d<u32>(640, 480), 32, false, false, false, 0);
	if (!device) return 1;

	device->setWindowCaption(L"Path Planning");

	IVideoDriver* driver = device->getVideoDriver();
	ISceneManager* smgr = device->getSceneManager();
	IGUIEnvironment* guienv = device->getGUIEnvironment();

	guienv->addStaticText(L"Hello", rect<s32>(10, 10, 260, 22), true);

	IAnimatedMesh* mesh = smgr->getMesh("./res/sydney.md2");
	if (!mesh) {
		device->drop();
		return 1;
	}
	IAnimatedMeshSceneNode* node = smgr->addAnimatedMeshSceneNode(mesh);
	
	if (node) {
		node->setMaterialFlag(EMF_LIGHTING, false);
		node->setMD2Animation(EMAT_STAND);
		node->setMaterialTexture(0, driver->getTexture("./res/sydney.bmp"));
	}

	smgr->addCameraSceneNode(0, vector3df(0, 0, -1000), vector3df(0, 0, 0));

	vector<Node> nodes = GenerateNodes();

	while (device->run()) {
		driver->beginScene(true, true, SColor(255, 100, 101, 140));

		driver->draw2DPolygon(vector2d<s32>(50,50), 50.0f, SColor(255, 255, 255, 255), 10);

		driver->draw3DTriangle(triangle3df(vector3df(-1, -1, 0), vector3df(0, 1, 0), vector3df(1, -1, 0)));

		//for (int i = 0; i < nodes.size(); i++) {
		//	//driver->draw3DLine(nodes[i].position, nodes[(i + 1) % (nodes.size()-1)].position, SColor(255, 255, 255, 255));
		//	driver->draw3DBox()
		//}

		//smgr->drawAll();
		//guienv->drawAll();

		driver->endScene();
	}

	return 0;
}