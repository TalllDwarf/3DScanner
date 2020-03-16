#pragma once
#include <GL/glew.h>

#include "Camera.h"
#include "SerialCom.h"

struct ScanSettings
{
	int numberOfImages;
	float minDistance;
	float maxDistance;

	ModelShot ignoreShot;
};


class ModelCapture
{
	GLuint modelFrameBuffer = 0;
	GLuint* modelTexture;
	GLuint modelBuffer;

	Camera* camera_;
	SerialCom serial_com_;
	const char* serialPort = nullptr;

	ModelShot* currentModel = nullptr;
	int modelSize = 0;

	ScanSettings scan_settings_;

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