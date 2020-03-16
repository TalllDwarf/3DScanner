#pragma once
#include <GL/glew.h>

#include "Camera.h"
#include "imfilebrowser.h"
#include "SerialCom.h"

struct ScanSettings
{
	int numberOfImages = 20;
	float minDistance = 1.0f;
	float maxDistance = 1000.0f;

	ModelShot ignoreShot;
};


class ModelCapture
{
	GLuint modelFrameBuffer = 0;
	GLuint* modelTexture;
	GLuint modelBuffer;

	Camera* camera_;
	SerialCom serial_com_;
	std::string serialPort;

	ModelShot* currentModel = nullptr;
	int modelSize = 0;

	ScanSettings scan_settings_;
	std::string fileRoot;
	ImGui::FileBrowser fileDialog;

	void RenderToTexture(float angle, float x, float y, float z) const;

	//Capturing model
	bool capturing = false;

	//Connected to serial connection
	bool connected = false;

	bool motorBusy = false;
	
public:

	//
	ModelCapture(Camera* camera);

	bool Init();
	
	void Render(float angle, float x, float y, float z);

	void ModelGatherTick(float time);
};