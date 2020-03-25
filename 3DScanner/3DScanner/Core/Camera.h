#pragma once

#include <GL/glew.h>

#include "Kinect.h"

#define DEPTH_SENSOR_WIDTH 512
#define DEPTH_SENSOR_HEIGHT 424
#define RGB_SENSOR_WIDTH 1920
#define RGB_SENSOR_HEIGHT 1080

class ModelShot
{
	
public:
	ColorSpacePoint* rgb;
	CameraSpacePoint* xyz;

	unsigned char* rgbimage; //[RGB_SENSOR_WIDTH * RGB_SENSOR_HEIGHT * 4];    // Stores RGB color image

	// We'll be using buffer objects to store the kinect point cloud
	GLuint vboId;
	GLuint cboId;

	void Dispose() const
	{
		delete[] rgbimage;
		delete[] rgb;
		delete[] xyz;
	}
};

class Camera
{
	// Intermediate Buffers
	
	//ColorSpacePoint* depth2rgb; //[DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT];             // Maps depth pixels to rgb pixels
	//CameraSpacePoint* depth2xyz; //[DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT] ;			 // Maps depth pixels to 3d coordinates
	ModelShot modelShot;

	// Kinect Variables
	IKinectSensor* sensor;             // Kinect sensor
	IMultiSourceFrameReader* reader;   // Kinect data source
	ICoordinateMapper* mapper;         // Converts between depth, color, and 3d coordinates

	GLuint kinectFrameBuffer = 0;
	GLuint* kinectTexture;
	GLuint kinectDepthBuffer;

	void GetDepthData(IMultiSourceFrame* frame, GLubyte* dest);
	void GetRgbData(IMultiSourceFrame* frame, GLubyte* dest);

public:

	Camera();
	~Camera();

	bool Init();
	void GetKinectData();
	
	void RenderToTexture(float angle, float x, float y, float z) const;
	GLuint* GetTexture();

	const ModelShot* GetCurrentModelShot() const;

};