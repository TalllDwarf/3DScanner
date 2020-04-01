#pragma once
#include <GL/glew.h>
#include <vector>
#include <future>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "imfilebrowser.h"
#include "SerialCom.h"
#include "MeshGenerator.h"

#define CGAL_NO_GMP 1

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/property_map.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/video/background_segm.hpp>
#include <SDL/SDL.h>

#define SERIAL_DELAY 2.f

// types
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::Point_3 Point;
typedef Kernel::Vector_3 Vector;

typedef std::pair<Point, Vector> ColorNormal;
typedef std::pair<Point, ColorNormal> PointWithData;
typedef CGAL::First_of_pair_property_map<PointWithData> PointMap;
typedef CGAL::First_of_pair_property_map<CGAL::Second_of_pair_property_map<PointWithData>> ColorMap;
typedef CGAL::Second_of_pair_property_map<CGAL::Second_of_pair_property_map<PointWithData>> NormalMap;


struct ScanSettings
{
	float singleRotation = 0.01f;
	
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
	cv::Mat ignoreMask;
	cv::Ptr<cv::BackgroundSubtractor> backSub;

	//Background Subtractor settings
	int history = 1000;
	float varThreshold = 100;

	//Image data for ignore mask
	//unsigned char* rgbIgnoreImage; //[RGB_SENSOR_WIDTH * RGB_SENSOR_HEIGHT * 4];
};

struct PointModel
{
	std::vector<PointWithData> points;

	void AddPoint(float x, float y, float z, float r, float g, float b)
	{
		points.push_back(std::make_pair<Point, ColorNormal>(Point(x, y, z), std::make_pair<Point, Vector>(Point(r, g, b), Vector())));
	}

	//Rotate this vertex around a point
	Point RotateAroundPoint(int index, glm::vec3 point, float radian) const
	{
		if (radian == 0.0f)
			return points[index].first;

		glm::vec3 position(
			points[index].first.x(), 
			points[index].first.y(), 
			points[index].first.z());
		const glm::vec3 offset = position - point;

		float s = glm::sin(radian);
		float c = glm::cos(radian);
		
		glm::vec3 rotatedPos;
		rotatedPos.x = offset.x * c - offset.z * s;
		rotatedPos.z = offset.x * s + offset.z * c;
		rotatedPos.y = offset.y;

		rotatedPos += point;
		//rotatedPos += offset;

		return Point(rotatedPos.x, rotatedPos.y, rotatedPos.z);
	}
	
	//std::vector<Point> points;
	//std::vector<Point> colors;
	//std::vector<Vector> normals;

	////TODO:Get RGB image and try to stitch onto model

	//void Reserve(int amount)
	//{
	//	points.reserve(amount);
	//	colors.reserve(amount);
	//}
	//
	//void AddPoint(float x, float y, float z, float r, float g, float b)
	//{		
	//	points.emplace_back(x, y, z);
	//	colors.emplace_back(r, g, b);
	//}

	////Rotate this vertex around a point
	//Point RotateAroundPoint(int index, glm::vec3 point, float radian) const
	//{
	//	if (radian == 0.0f)
	//		return points[index];

	//	glm::vec3 position(points[index].x(), points[index].y(), points[index].z());
	//	const glm::vec3 offset = position - point;

	//	float s = glm::sin(radian);
	//	float c = glm::cos(radian);
	//	
	//	glm::vec3 rotatedPos;
	//	rotatedPos.x = offset.x * c - offset.z * s;
	//	rotatedPos.z = offset.x * s + offset.z * c;
	//	rotatedPos.y = offset.y;

	//	rotatedPos += point;
	//	//rotatedPos += offset;

	//	return Point(rotatedPos.x, rotatedPos.y, rotatedPos.z);
	//}
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
	std::vector<PointModel> currentModel;
	bool hasIgnore;
	int lastFrameID = -1;
	int lastIgnoreFrameID = -1;

	MeshGenerator meshGenerator;

	
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

	PointModel GenerateCombinedModel();

public:

	ModelCapture(Camera* camera);
	~ModelCapture();

	bool Init();
	
	void Render(float angle, float x, float y, float z);

	void ModelGatherTick(float time);
};