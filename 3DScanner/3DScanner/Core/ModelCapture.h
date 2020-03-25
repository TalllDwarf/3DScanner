#pragma once
#include <GL/glew.h>
#include <vector>
#include <thread>
#include <future>

#include "Camera.h"
#include "imfilebrowser.h"
#include "SerialCom.h"

#define CGAL_NO_GMP 1

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#define SERIAL_DELAY 2.f

// types
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::Point_3 Point;
typedef Kernel::Vector_3 Vector;
typedef Kernel::Point_3 Color;

struct ScanSettings
{
	int numberOfImages = 30;
	float minDistance = 1.0f;
	float maxDistance = 1000.0f;

	//WLOP processing
	float retainPercentage = 100;
	float neighborRadius = 0.5f;
	
	
	std::vector<Point> ignoreShot; 
};


struct CGAL_Model
{
	std::vector<Point> xyz;
	std::vector<Color> rgb;

	// We'll be using buffer objects to store the kinect point cloud
	GLuint vboId;
	GLuint cboId;
	
	void AddPoint(Point point, Color color)
	{
		xyz.push_back(point);
		rgb.push_back(color);
	}
};


class ModelCapture
{
	//Model buffer
	GLuint modelFrameBuffer = 0;
	GLuint* modelTexture;
	GLuint modelBuffer;

	//Camera that contains 
	Camera* camera_;

	//Turntable communication
	SerialCom serial_com_;
	std::string serialPort;

	//Serial thread
	std::future<bool> serialInFuture;
	float serialCheck = SERIAL_DELAY;
	
	//Model
	std::vector<CGAL_Model> currentModel;
	CGAL_Model ignoreModel;

	//Settings
	ScanSettings scan_settings_;
	std::string fileRoot;
	ImGui::FileBrowser fileDialog;

	void RenderToTexture(float angle, float x, float y, float z) const;

	//Capturing model
	bool capturing = false;

	//Connected to serial connection
	bool connected = false;
	bool motorBusy = false;

	//Gets the frame from the camera ignoring points outside of the Scan Settings
	void GetCameraFrame();

public:

	ModelCapture(Camera* camera);

	bool Init();
	
	void Render(float angle, float x, float y, float z);

	void ModelGatherTick(float time);
};