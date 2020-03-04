// NodePlus.cpp : Defines the entry point for the application.
//
#include <cstdio>
#include <iostream>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "imfilebrowser.h"

#include "GL/glew.h"
#include "SDL/SDL.h"

#include <Windows.h>
#include <Ole2.h>

#include "Kinect.h" 

#define IMGUI_IMPL_OPENGL_LOADER_GLEW

const int width = 512;
const int height = 424;
const int colorwidth = 1920;
const int colorheight = 1080;

bool firstFrame = false;

// We'll be using buffer objects to store the kinect point cloud
GLuint vboId;
GLuint cboId;

// Intermediate Buffers
unsigned char rgbimage[colorwidth * colorheight * 4];    // Stores RGB color image
ColorSpacePoint depth2rgb[width * height];             // Maps depth pixels to rgb pixels
CameraSpacePoint depth2xyz[width * height];			 // Maps depth pixels to 3d coordinates

// Kinect Variables
IKinectSensor* sensor;             // Kinect sensor
IMultiSourceFrameReader* reader;   // Kinect data source
ICoordinateMapper* mapper;         // Converts between depth, color, and 3d coordinates

GLuint kinectFrameBuffer = 0;
GLuint kinectTexture;
GLuint kinectDepthBuffer;

bool initKinect() {
	if (FAILED(GetDefaultKinectSensor(&sensor))) {
		return false;
	}
	if (sensor) {
		sensor->get_CoordinateMapper(&mapper);

		sensor->Open();
		sensor->OpenMultiSourceFrameReader(
			FrameSourceTypes::FrameSourceTypes_Depth | FrameSourceTypes::FrameSourceTypes_Color,
			&reader);
		return reader;
	}
	else {
		return false;
	}
}

void getDepthData(IMultiSourceFrame* frame, GLubyte* dest) {
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
	mapper->MapDepthFrameToCameraSpace(width * height, buf, width * height, depth2xyz);
	float* fdest = (float*)dest;
	for (int i = 0; i < sz; i++) {
		*fdest++ = depth2xyz[i].X;
		*fdest++ = depth2xyz[i].Y;
		*fdest++ = depth2xyz[i].Z;
	}

	// Fill in depth2rgb map
	mapper->MapDepthFrameToColorSpace(width * height, buf, width * height, depth2rgb);
	if (depthframe) depthframe->Release();
}

void getRgbData(IMultiSourceFrame* frame, GLubyte* dest) {
	IColorFrame* colorframe;
	IColorFrameReference* frameref = NULL;
	frame->get_ColorFrameReference(&frameref);
	frameref->AcquireFrame(&colorframe);
	if (frameref) frameref->Release();

	if (!colorframe) return;

	// Get data from frame
	colorframe->CopyConvertedFrameDataToArray(colorwidth * colorheight * 4, rgbimage, ColorImageFormat_Rgba);

	// Write color array for vertices
	float* fdest = (float*)dest;
	for (int i = 0; i < width * height; i++) {
		ColorSpacePoint p = depth2rgb[i];
		// Check if color pixel coordinates are in bounds
		if (p.X < 0 || p.Y < 0 || p.X > colorwidth || p.Y > colorheight) {
			*fdest++ = 0;
			*fdest++ = 0;
			*fdest++ = 0;
		}
		else {
			int idx = (int)p.X + colorwidth * (int)p.Y;
			*fdest++ = rgbimage[4 * idx + 0] / 255.;
			*fdest++ = rgbimage[4 * idx + 1] / 255.;
			*fdest++ = rgbimage[4 * idx + 2] / 255.;
		}
		// Don't copy alpha channel
	}

	if (colorframe) colorframe->Release();
}

void getKinectData() {
	IMultiSourceFrame* frame = NULL;
	if (SUCCEEDED(reader->AcquireLatestFrame(&frame))) {
		GLubyte* ptr;
		glBindBuffer(GL_ARRAY_BUFFER, vboId);
		ptr = (GLubyte*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		if (ptr) {
			getDepthData(frame, ptr);
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, cboId);
		ptr = (GLubyte*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		if (ptr) {
			getRgbData(frame, ptr);
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);

		firstFrame = true;
	}
	if (frame) frame->Release();


	if(ImGui::Begin("Point Cloud"))
	{
		ImVec2 pos = ImGui::GetCursorScreenPos();

		glBindFramebuffer(GL_FRAMEBUFFER, kinectFrameBuffer);
		glViewport(0, 0, width, height);

		static double angle = 0.;
		static double radius = 3.;
		double x = radius * sin(angle);
		double z = radius * (1 - cos(angle)) - radius / 2;
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(x, 0, z, 0, 0, radius / 2, 0, 1, 0);
		angle += 0.002;
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		glBindBuffer(GL_ARRAY_BUFFER, vboId);
		glVertexPointer(3, GL_FLOAT, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, cboId);
		glColorPointer(3, GL_FLOAT, 0, NULL);

		glPointSize(1.f);
		glDrawArrays(GL_POINTS, 0, width * height);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		ImVec2 vMin = ImGui::GetWindowContentRegionMin();
		ImVec2 vMax = ImGui::GetWindowContentRegionMax();

		ImVec2 size(vMax.x - vMin.x, vMax.y - vMin.y);

		//ImGui::GetWindowDrawList()->AddImage((void*)kinectTexture, ImVec2(ImGui::GetItemRectMin().x + pos.x,
		//	ImGui::GetItemRectMin().y + pos.y), ImVec2(pos.x + H / 2, pos.y + W / 2), ImVec2(0, 1), ImVec2(1, 0));
		ImGui::Image((void*)kinectTexture, size, ImVec2(0, 1), ImVec2(1,0));
	}
	ImGui::End();
	
	
}

int main(int, char**)
{
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 440";
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	// Setup SDL
	// (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
	// depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	SDL_Window* window = SDL_CreateWindow("Node++", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL
		| SDL_WINDOW_RESIZABLE
		| SDL_WINDOW_ALLOW_HIGHDPI);

	if (window == nullptr)
	{
		printf("SDL Window could not be created! SDL Error: %s\n", SDL_GetError());
		return 1;
	}

	const SDL_GLContext gl_context = SDL_GL_CreateContext(window);

	if (gl_context == nullptr)
	{
		printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
		return 1;
	}
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	std::cout << SDL_GetError() << std::endl;

	const GLenum err = glewInit();

	if (err != GLEW_OK)
	{
		fprintf(stderr, "Failed to init OpenGL loader!\n");
		return 1;
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		io.ConfigDockingTransparentPayload = true;
	}

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	const ImVec4 clear_color = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

	// Main loop
	bool done = false;

	std::string fileRoot;
	ImGui::FileBrowser fileDialog(ImGuiFileBrowserFlags_CreateNewDir);
	
	fileDialog.SetTitle("Save mesh to");

	//Try to initialise the kinect
	if (!initKinect())
		return 1;

	//Create kinect frame buffer
	glGenFramebuffers(1, &kinectFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, kinectFrameBuffer);

	//Create texutre to render to
	glGenTextures(1, &kinectTexture);
	glBindTexture(GL_TEXTURE_2D, kinectTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	//Kinect depth buffer
	glGenRenderbuffers(1, &kinectDepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, kinectDepthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, kinectDepthBuffer);
	
	//Set kinect texture to buffer
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, kinectTexture, 0);
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);

	//Check the frame buffer
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;
	
	// Set up array buffers
	const int dataSize = width * height * 3 * 4;
	glGenBuffers(1, &vboId);
	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glBufferData(GL_ARRAY_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);
	glGenBuffers(1, &cboId);
	glBindBuffer(GL_ARRAY_BUFFER, cboId);
	glBufferData(GL_ARRAY_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);

	// Camera setup
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, width / (GLdouble)height, 0.1, 1000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, 0, 0, 0, 1, 0, 1, 0);
	
	while (!done)
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}
	
		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		getKinectData();
		
		if(ImGui::Begin("TEST!"))
		{
			ImGui::Text("Hello World!");
		}
		ImGui::End();				

		// Rendering
		ImGui::Render();

		// Camera setup
		glViewport(0, 0, width, height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45, width / (GLdouble)height, 0.1, 1000);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(0, 0, 0, 0, 0, 1, 0, 1, 0);
		
		glViewport(0, 0, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);

	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}