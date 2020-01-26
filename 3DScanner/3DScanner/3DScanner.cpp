// 3DScanner.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <Ole2.h>

#include <SDL_opengl.h>
#include <SDL.h>
#undef main

#include <Kinect.h>

#define WIDTH 1920
#define HEIGHT 1080

//OpenGL
GLuint textureId;
GLubyte data[WIDTH * HEIGHT * 4];

SDL_Window* screen;

//Kinect
IKinectSensor* sensor;
IColorFrameReader* reader;

bool InitKinect()
{
	if (FAILED(GetDefaultKinectSensor(&sensor)))
		return false;
	
	if (sensor)
	{
		sensor->Open();

		IColorFrameSource* frameSource = nullptr;
		sensor->get_ColorFrameSource(&frameSource);
		frameSource->OpenReader(&reader);

		if (frameSource)
		{
			frameSource->Release();
			frameSource = nullptr;
		}

		return true;
	}
	else
		return false;
}

void getKinectData(GLubyte* dest)
{
	IColorFrame* frame = nullptr;

	if (SUCCEEDED(reader->AcquireLatestFrame(&frame)))
		frame->CopyConvertedFrameDataToArray((WIDTH * HEIGHT * 4), data, ColorImageFormat_Bgra);

	if (frame)
		frame->Release();
}

bool initSDL()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	screen = SDL_CreateWindow("Kinect V2 Scanner", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
	SDL_GL_CreateContext(screen);
	return screen;
}

void drawKinectData() 
{
	glBindTexture(GL_TEXTURE_2D, textureId);
	getKinectData(data);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)data);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(0, 0, 0);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(WIDTH, 0, 0);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(WIDTH, HEIGHT, 0.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(0, HEIGHT, 0.0f);
	glEnd();
}


void execute()
{
	SDL_Event ev;
	bool running = true;
	while (running) {
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT) running = false;
		}
		drawKinectData();
		SDL_GL_SwapWindow(screen);
	}
}

int main()
{
	if (!initSDL())
		return 1;

	if (!InitKinect())
		return 1;

	// Initialize textures
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT,
		0, GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)data);
	glBindTexture(GL_TEXTURE_2D, 0);

	// OpenGL setup
	glClearColor(0, 0, 0, 0);
	glClearDepth(1.0f);
	glEnable(GL_TEXTURE_2D);

	// Camera setup
	glViewport(0, 0, WIDTH, HEIGHT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, WIDTH, HEIGHT, 0, 1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	execute();

	return 0;
}
