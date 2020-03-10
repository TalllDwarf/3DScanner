#pragma once
#include <GL/glew.h>

#include "Camera.h"

class ModelCapture
{
	GLuint modelFrameBuffer = 0;
	GLuint* modelTexture{};
	GLuint modelDepthBuffer{};

	Camera* camera_;

	ModelShot currentModel;
	
public:

	//
	ModelCapture(Camera* camera);
	
};