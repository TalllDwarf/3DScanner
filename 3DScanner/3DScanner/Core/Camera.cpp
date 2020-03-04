#include "Camera.h"

#include <cmath>
#include <Windows.h>
#include <Ole2.h>
#include "NuiKinectFusionApi.h"

void Camera::GetKinectData()
{
	IMultiSourceFrame* frame = NULL;
	if (SUCCEEDED(reader->AcquireLatestFrame(&frame))) {
		GLubyte* ptr;
		glBindBuffer(GL_ARRAY_BUFFER, vboId);
		ptr = (GLubyte*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		if (ptr) {
			GetDepthData(frame, ptr);
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);

		glBindTexture(GL_TEXTURE_2D, kinectTexture[1]);
		glBindBuffer(GL_ARRAY_BUFFER, cboId);
		ptr = (GLubyte*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		if (ptr) {
			GetRgbData(frame, ptr);
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, RGB_SENSOR_WIDTH, RGB_SENSOR_HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)rgbimage);
	}
	if (frame) frame->Release();
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
	unsigned short* buf;
	depthframe->AccessUnderlyingBuffer(&sz, &buf);

	// Write vertex coordinates
	mapper->MapDepthFrameToCameraSpace(DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT, buf, DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT, depth2xyz);
	float* fdest = (float*)dest;
	for (int i = 0; i < sz; i++) {
		*fdest++ = depth2xyz[i].X;
		*fdest++ = depth2xyz[i].Y;
		*fdest++ = depth2xyz[i].Z;
	}

	// Fill in depth2rgb map
	mapper->MapDepthFrameToColorSpace(DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT, buf, DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT, depth2rgb);
	if (depthframe) depthframe->Release();
}

void Camera::GetRgbData(IMultiSourceFrame* frame, GLubyte* dest)
{
	IColorFrame* colorframe;
	IColorFrameReference* frameref = NULL;
	frame->get_ColorFrameReference(&frameref);
	frameref->AcquireFrame(&colorframe);
	if (frameref) frameref->Release();

	if (!colorframe) return;

	// Get data from frame
	colorframe->CopyConvertedFrameDataToArray(RGB_SENSOR_WIDTH * RGB_SENSOR_HEIGHT * 4, rgbimage, ColorImageFormat_Rgba);

	// Write color array for vertices
	float* fdest = (float*)dest;
	for (int i = 0; i < DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT; i++) {
		ColorSpacePoint p = depth2rgb[i];
		// Check if color pixel coordinates are in bounds
		if (p.X < 0 || p.Y < 0 || p.X > RGB_SENSOR_WIDTH || p.Y > RGB_SENSOR_HEIGHT) {
			*fdest++ = 0;
			*fdest++ = 0;
			*fdest++ = 0;
		}
		else {
			int idx = (int)p.X + RGB_SENSOR_WIDTH * (int)p.Y;
			*fdest++ = rgbimage[4 * idx + 0] / 255.;
			*fdest++ = rgbimage[4 * idx + 1] / 255.;
			*fdest++ = rgbimage[4 * idx + 2] / 255.;
		}
		// Don't copy alpha channel
	}
	if (colorframe) colorframe->Release();
}

void Camera::RenderToTexture()
{
	glBindFramebuffer(GL_FRAMEBUFFER, kinectFrameBuffer);
	glViewport(0, 0, DEPTH_SENSOR_WIDTH, DEPTH_SENSOR_HEIGHT);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, DEPTH_SENSOR_WIDTH / (GLdouble)DEPTH_SENSOR_HEIGHT, 0.1, 1000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, 0, 0, 0, 1, 0, 1, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glVertexPointer(3, GL_FLOAT, 0, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, cboId);
	glColorPointer(3, GL_FLOAT, 0, NULL);

	glPointSize(1.f);
	glDrawArrays(GL_POINTS, 0, DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint* Camera::GetTexture()
{
	return kinectTexture;
}

Camera::Camera()
{
	rgbimage = new unsigned char[RGB_SENSOR_WIDTH * RGB_SENSOR_HEIGHT * 4];
	depth2rgb = new ColorSpacePoint[DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT];
	depth2xyz = new CameraSpacePoint[DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT];

	kinectTexture = new GLuint[3];
}

Camera::~Camera()
{
	delete[] rgbimage;
	delete[] depth2rgb;
	delete[] depth2xyz;

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
		
		glGenTextures(3, kinectTexture);

		glBindTexture(GL_TEXTURE_2D, kinectTexture[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, DEPTH_SENSOR_WIDTH, DEPTH_SENSOR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, kinectTexture[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, RGB_SENSOR_WIDTH, RGB_SENSOR_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)rgbimage);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, kinectTexture[2]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, DEPTH_SENSOR_WIDTH, DEPTH_SENSOR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			
		//Kinect depth buffer
		glGenRenderbuffers(1, &kinectDepthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, kinectDepthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, DEPTH_SENSOR_WIDTH, DEPTH_SENSOR_HEIGHT);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, kinectDepthBuffer);

		//Set kinect texture to buffer
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, kinectTexture[0], 0);
		//glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, kinectTexture[1], 0);
		//glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, kinectTexture[2], 0);
		GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0};
		glDrawBuffers(1, DrawBuffers);

		//Check the frame buffer
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			return false;

		// Set up array buffers
		const int dataSize = DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT * 3 * 4;
		glGenBuffers(1, &vboId);
		glBindBuffer(GL_ARRAY_BUFFER, vboId);
		glBufferData(GL_ARRAY_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);
		glGenBuffers(1, &cboId);
		glBindBuffer(GL_ARRAY_BUFFER, cboId);
		glBufferData(GL_ARRAY_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);
		
		return reader;
	}

	return false;
}
