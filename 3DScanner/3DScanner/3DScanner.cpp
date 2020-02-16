// 3DScanner.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

//#include <Windows.h>
//#include <Ole2.h>
//
//#include <SDL_opengl.h>
//#include <SDL.h>
//#undef main
//
//#include "KinectCom.h"
//
////OpenGL
//GLuint rgbTexID;
//GLuint depthTexID;
//
//SDL_Window* screen;
//
//
//bool initSDL()
//{
//	SDL_Init(SDL_INIT_EVERYTHING);
//	screen = SDL_CreateWindow("Kinect V2 Scanner", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, RGB_WIDTH, RGB_HEIGHT, SDL_WINDOW_OPENGL);
//	SDL_GL_CreateContext(screen);
//	return screen;
//}
//
//void drawKinectData() 
//{
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//	glBindTexture(GL_TEXTURE_2D, rgbTexID);
//	//getKinectData(RgbData);
//	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, RGB_WIDTH, RGB_HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)RgbData);
//	glEnable(GL_TEXTURE_2D);
//
//	glBegin(GL_QUADS);
//	glTexCoord2f(0.0f, 0.0f);
//	glVertex3f(0, 0, 0);
//	glTexCoord2f(1.0f, 0.0f);
//	glVertex3f(IR_WIDTH, 0, 0);
//	glTexCoord2f(1.0f, 1.0f);
//	glVertex3f(IR_WIDTH, IR_HEIGHT, 0.0f);
//	glTexCoord2f(0.0f, 1.0f);
//	glVertex3f(0, IR_HEIGHT, 0.0f);
//	glEnd();
//
//
//
//	//glBindTexture(GL_TEXTURE_2D, depthTexID);
//	//getKinectDepthData(depthData);
//	//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, IR_WIDTH, IR_HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)depthData);
//	//glEnable(GL_TEXTURE_2D);
//
//	//glBegin(GL_QUADS);
//	//glTexCoord2f(0.0f, 0.0f);
//	//glVertex3f(IR_WIDTH, 0, 0);
//	//glTexCoord2f(1.0f, 0.0f);
//	//glVertex3f(IR_WIDTH * 2, 0, 0);
//	//glTexCoord2f(1.0f, 1.0f);
//	//glVertex3f(IR_WIDTH * 2, IR_HEIGHT, 0.0f);
//	//glTexCoord2f(0.0f, 1.0f);
//	//glVertex3f(IR_WIDTH, IR_HEIGHT, 0.0f);
//	//glEnd();
//}


//void execute()
//{
//	SDL_Event ev;
//	bool running = true;
//	while (running) {
//		while (SDL_PollEvent(&ev)) {
//			if (ev.type == SDL_QUIT) running = false;
//		}
//		drawKinectData();
//		SDL_GL_SwapWindow(screen);
//	}
//}

#include "SerialCom.h"

int main()
{
	//if (!initSDL())
	//	return 1;

	////if (!InitKinect())
	////	return 1;

	//// Initialize textures
	////glGenTextures(1, &rgbTexID);
	////glBindTexture(GL_TEXTURE_2D, rgbTexID);

	////glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	////glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	////glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, RGB_WIDTH, RGB_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)RgbData);
	////glBindTexture(GL_TEXTURE_2D, 0);

	////glGenTextures(1, &depthTexID);
	////glBindTexture(GL_TEXTURE_2D, depthTexID);

	////glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	////glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	////glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, IR_WIDTH, IR_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)depthData);
	////glBindTexture(GL_TEXTURE_2D, 0);

	//// OpenGL setup
	//glClearColor(0, 0, 0, 0);
	//glClearDepth(1.0f);
	//glEnable(GL_TEXTURE_2D);

	//// Camera setup
	//glViewport(0, 0, RGB_WIDTH, RGB_HEIGHT);
	//glMatrixMode(GL_PROJECTION);
	//glLoadIdentity();
	//glOrtho(0, RGB_WIDTH, RGB_HEIGHT, 0, 1, -1);
	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();

	//execute();

	SerialCom serial;

	std::vector<std::string> ports = serial.GetAvailablePorts();

	if (ports.size() > 0)
	{
		serial.OpenPort("\\\\.\\COM11");
	}

	return 0;
}