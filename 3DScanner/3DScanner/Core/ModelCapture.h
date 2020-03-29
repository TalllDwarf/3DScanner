#pragma once
#include <GL/glew.h>
#include <vector>
#include <future>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "imfilebrowser.h"
#include "SerialCom.h"

#define CGAL_NO_GMP 1

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/video/background_segm.hpp>
#include <SDL/SDL.h>

#define SERIAL_DELAY 2.f

// types
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::Point_3 Point;
typedef Kernel::Vector_3 Vector;

struct ScanSettings
{
	int numberOfImages = 30;
	float minDistance = 1.0f;
	float maxDistance = 10.0f;
	
	int ignoreFrames = 10;
	int currentIgnoreFrame = 10;

	bool cubeSet = false;
	bool showCube = false;
	float cubePos[3] = {0.0f, 0.0f, 0.0f};
	float cubeScale[3] = {1.0f, 1.0f, 1.0f};

	//Used to ignore the background
	cv::Mat ignoreMat;
	cv::Mat ignoreMask;
	cv::Ptr<cv::BackgroundSubtractor> backSub;

	//BackgroundSubtractor settings
	int history = 1000;
	float varThreshold = 100;

	//Image data for ignore mask
	unsigned char* rgbIgnoreImage; //[RGB_SENSOR_WIDTH * RGB_SENSOR_HEIGHT * 4];
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;

	Vertex(glm::vec3 xyz, glm::vec3 rgb) : pos(xyz), color(rgb)
	{}

	Vertex RotateAroundPoint(glm::vec3 point, float radian) const
	{
		if (radian == 0.0f)
			return Vertex(pos, color);
		
		glm::vec3 offset = pos - point;
		glm::vec3 rotatedPos;
		rotatedPos.x = offset.x * glm::cos(radian) - offset.z * glm::sin(radian);
		rotatedPos.z = offset.x * glm::sin(radian) + offset.z * glm::cos(radian);
		rotatedPos.y = offset.y;

		rotatedPos += offset;

		return Vertex(rotatedPos, color);
	}
};

struct CGAL_Model
{
	std::vector<Vertex> vertex;

	//TODO:Get RGB image and try to stitch onto model
	
	void AddPoint(glm::vec3 point, glm::vec3 color)
	{
		vertex.emplace_back(point, color);
	}
};


class ModelCapture
{
	//Model buffer
	GLuint modelFrameBuffer = 0;
	GLuint modelTexture;
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
	bool hasIgnore;
	int lastFrameID = -1;
	int lastIgnoreFrameID = -1;

	//Settings
	ScanSettings scan_settings_;
	std::string fileRoot;
	ImGui::FileBrowser fileDialog;

	void RenderToTexture(float angle, float x, float y, float z) const;
	void RenderToTexturePreview(float angle, float x, float y, float z);
	
	//Capturing model
	bool capturing = false;

	//Connected to serial connection
	bool connected = false;
	bool motorBusy = false;

	//Gets the frame from the camera ignoring points outside of the Scan Settings
	void GetCameraFrame();

	void CreateIgnoreFrame();

	//Add multiple ignore frames together to get a better comparison
	void AddIgnoreFrame();

public:

	ModelCapture(Camera* camera);
	~ModelCapture();

	bool Init();
	
	void Render(float angle, float x, float y, float z);

	void ModelGatherTick(float time);
};