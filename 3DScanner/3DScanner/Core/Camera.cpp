#include "Camera.h"

#include <cmath>
#include <Windows.h>
#include <Ole2.h>
#include "NuiKinectFusionApi.h"

void Camera::GetKinectData()
{
	IMultiSourceFrame* frame = NULL;
	if (SUCCEEDED(reader->AcquireLatestFrame(&frame))) {
		frameID++;
		
		GLubyte* ptr;
		
		glBindBuffer(GL_ARRAY_BUFFER, modelShot.vboId);
		ptr = static_cast<GLubyte*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
		if (ptr) {
			GetDepthData(frame, ptr);
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);

		glBindTexture(GL_TEXTURE_2D, kinectTexture[1]);
		glBindBuffer(GL_ARRAY_BUFFER, modelShot.cboId);
		ptr = static_cast<GLubyte*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
		if (ptr) {
			GetRgbData(frame, ptr);
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, RGB_SENSOR_WIDTH, RGB_SENSOR_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLvoid*>(modelShot.rgbimage));
	}
	if (frame) frame->Release();
}

int Camera::GetFrameID() const
{
	return frameID;
}


void Camera::GetDepthData(IMultiSourceFrame* frame, GLubyte* dest)
{
	IDepthFrame* depthframe;
	IDepthFrameReference* frameref = NULL;
	frame->get_DepthFrameReference(&frameref);
	frameref->AcquireFrame(&depthframe);
	if (frameref) frameref->Release();

	if (!depthframe) return;

	// Get data from frame
	unsigned int sz;
	UINT16* buf;
	depthframe->AccessUnderlyingBuffer(&sz, &buf);

	// Write vertex coordinates
	mapper->MapDepthFrameToCameraSpace(DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT, buf, DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT, modelShot.xyz);
	auto fdest = reinterpret_cast<float*>(dest);
	for (int i = 0; i < sz; i++) {
		*fdest++ = modelShot.xyz[i].X;
		*fdest++ = modelShot.xyz[i].Y;
		*fdest++ = modelShot.xyz[i].Z;
	}

	// Fill in depth2rgb map
	mapper->MapDepthFrameToColorSpace(DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT, buf, DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT, modelShot.rgb);
	if (depthframe) depthframe->Release();

	depthReady = true;
}

void Camera::GetRgbData(IMultiSourceFrame* frame, GLubyte* dest)
{
	IColorFrame* colorframe;
	IColorFrameReference* frameref = nullptr;
	frame->get_ColorFrameReference(&frameref);
	frameref->AcquireFrame(&colorframe);
	if (frameref) frameref->Release();

	if (!colorframe) return;

	// Get data from frame
	colorframe->CopyConvertedFrameDataToArray(RGB_SENSOR_WIDTH * RGB_SENSOR_HEIGHT * 4, modelShot.rgbimage, ColorImageFormat_Rgba);

	// Write color array for vertices
	auto fdest = reinterpret_cast<float*>(dest);
	for (int i = 0; i < DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT; i++) {
		ColorSpacePoint p = modelShot.rgb[i];
		// Check if color pixel coordinates are in bounds
		if (p.X < 0 || p.Y < 0 || p.X > RGB_SENSOR_WIDTH || p.Y > RGB_SENSOR_HEIGHT) {
			*fdest++ = 0;
			*fdest++ = 0;
			*fdest++ = 0;
		}
		else {
			int idx = static_cast<int>(p.X) + RGB_SENSOR_WIDTH * static_cast<int>(p.Y);
			*fdest++ = modelShot.rgbimage[4 * idx + 0] / 255.;
			*fdest++ = modelShot.rgbimage[4 * idx + 1] / 255.;
			*fdest++ = modelShot.rgbimage[4 * idx + 2] / 255.;
		}
		// Don't copy alpha channel
	}
	if (colorframe) colorframe->Release();

	colorReady = true;
}

void Camera::RenderToTexture(float angle, float x, float y, float z) const
{
	//Draw full image
	glBindFramebuffer(GL_FRAMEBUFFER, kinectFrameBuffer);
	glViewport(0, 0, DEPTH_SENSOR_WIDTH, DEPTH_SENSOR_HEIGHT);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(80, DEPTH_SENSOR_WIDTH / static_cast<GLdouble>(DEPTH_SENSOR_HEIGHT), 0.1, 1000);

	const float eyex = 1 * cos(angle);// -z * sin(angle) + x;
	const float eyey = 1 * sin(angle);// +z * cos(angle) + z;
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(x, y, z, x + eyex, y,  z + eyey, 0, std::abs(y), 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, modelShot.vboId);
	glVertexPointer(3, GL_FLOAT, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, modelShot.cboId);
	glColorPointer(3, GL_FLOAT, 0, nullptr);

	glPointSize(1.f);
	glDrawArrays(GL_POINTS, 0, DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	//End	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint* Camera::GetTexture()
{
	return kinectTexture;
}

const ModelShot* Camera::GetCurrentModelShot() const
{
	if (!depthReady || !colorReady)
		return nullptr;
	
	return &modelShot;
}

Camera::Camera()
{
	modelShot.rgbimage = new unsigned char[RGB_SENSOR_WIDTH * RGB_SENSOR_HEIGHT * 4];
	modelShot.rgb = new ColorSpacePoint[DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT];
	modelShot.xyz = new CameraSpacePoint[DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT];

	kinectTexture = new GLuint[2];
}

Camera::~Camera()
{
	modelShot.Dispose();
	//delete[] depth2rgb;
	//delete[] depth2xyz;

	delete[] kinectTexture;
}

bool Camera::Init()
{
	if (FAILED(GetDefaultKinectSensor(&sensor))) {
		return false;
	}
	
	if (sensor) {
		sensor->get_CoordinateMapper(&mapper);

		sensor->Open();
		sensor->OpenMultiSourceFrameReader(
			FrameSourceTypes_Depth | FrameSourceTypes_Color,
			&reader);

		//Create kinect frame buffer
		glGenFramebuffers(1, &kinectFrameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, kinectFrameBuffer);

		//Create texutre to render to		
		glGenTextures(2, kinectTexture);

		glBindTexture(GL_TEXTURE_2D, kinectTexture[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, DEPTH_SENSOR_WIDTH, DEPTH_SENSOR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, kinectTexture[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, RGB_SENSOR_WIDTH, RGB_SENSOR_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, static_cast<GLvoid*>(modelShot.rgbimage));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		
		//Kinect depth buffer
		glGenRenderbuffers(1, &kinectDepthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, kinectDepthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, DEPTH_SENSOR_WIDTH, DEPTH_SENSOR_HEIGHT);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, kinectDepthBuffer);

		//Set kinect texture to buffer
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, kinectTexture[0], 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		

		//Check the frame buffer
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			return false;

		// Set up array buffers
		const int dataSize = DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT * 3 * 4;
		glGenBuffers(1, &modelShot.vboId);
		glBindBuffer(GL_ARRAY_BUFFER, modelShot.vboId);
		glBufferData(GL_ARRAY_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);
		glGenBuffers(1, &modelShot.cboId);
		glBindBuffer(GL_ARRAY_BUFFER, modelShot.cboId);
		glBufferData(GL_ARRAY_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);
		
		return reader;
	}

	return false;
}
