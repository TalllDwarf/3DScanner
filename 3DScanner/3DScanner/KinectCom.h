#pragma once

#include <Kinect.h>
#include <SDL/SDL_opengl.h>

#define RGB_WIDTH 1920
#define RGB_HEIGHT 1080

#define IR_WIDTH 512
#define IR_HEIGHT 424

class KinectCom
{
	//Kinect
	IKinectSensor* sensor;

	// Reads connect data
	IMultiSourceFrameReader* reader; 

	//Converts data between depth, color and 3d 
	ICoordinateMapper* mapper; 
	
	GLuint rgbTexID = 0;
	GLuint depthTexID = 0;

	GLubyte RgbData[RGB_WIDTH * RGB_HEIGHT * 4] = { 0 };
	
	ColorSpacePoint depthToColour[IR_WIDTH * IR_HEIGHT];
	CameraSpacePoint depthToVector[IR_WIDTH * IR_HEIGHT];

	void UpdateRGBData(IMultiSourceFrame* frame);
	void UpdateDepthData(IMultiSourceFrame* frame);

public:

	bool Init();
	void InitTexture();

	GLuint GetRGBTextureId();
	GLuint GetDepthTextureId();

	//Updates all the frame data
	void Update();



};