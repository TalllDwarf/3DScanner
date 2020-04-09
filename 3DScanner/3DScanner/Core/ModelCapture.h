#pragma once
#include <GL/glew.h>
#include <vector>
#include <future>

#include "Camera.h"
#include "imfilebrowser.h"
#include "SerialCom.h"
#include "MeshGenerator.h"

#define CGAL_NO_GMP 1

#include <opencv2/core/mat.hpp>
#include <opencv2/video/background_segm.hpp>

#define SERIAL_DELAY 2.f

struct ScanSettings
{
	float singleRotation = 0.01f;
	
	int numberOfImages = 30;
	float minDistance = 1.0f;
	float maxDistance = 10.0f;

	//Ignore 
	int ignoreFrames = 10;
	int currentIgnoreFrame = 10;

	bool cubeSet = true;
	bool showCube = true;
	float cubePos[3] = {0.05f, 0.0f, 1.18f};
	float cubeScale[3] = {0.76f, 0.79f, 0.76f};

	//Used to ignore the background
	cv::Mat ignoreMask;
	cv::Ptr<cv::BackgroundSubtractor> backSub;

	//Background Subtractor settings
	int history = 1000;
	float varThreshold = 100;

	//Image data for ignore mask
	//unsigned char* rgbIgnoreImage; //[RGB_SENSOR_WIDTH * RGB_SENSOR_HEIGHT * 4];
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

	//Point map manipulation
	MeshGenerator meshGenerator;
	std::future<bool> meshGeneratingFuture;
	
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

	template<typename T>
	static bool is_Ready(std::future<T> const& f)
	{
		return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
	}
	
public:

	ModelCapture(Camera* camera);
	~ModelCapture();

	bool Init();
	
	void Render(float angle, float x, float y, float z);

	void ModelGatherTick(float time);
};