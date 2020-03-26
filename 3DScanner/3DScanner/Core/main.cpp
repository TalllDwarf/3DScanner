#include <cstdio>
#include <iostream>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "imfilebrowser.h"

#include "GL/glew.h"
#include "SDL/SDL.h"

#include "Camera.h"
#include "ModelCapture.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLEW

int SDL_main(int, char**)
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
		return EXIT_FAILURE;
	}

	SDL_Window* window = SDL_CreateWindow("Node++", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL
		| SDL_WINDOW_RESIZABLE
		| SDL_WINDOW_ALLOW_HIGHDPI);

	if (window == nullptr)
	{
		printf("SDL Window could not be created! SDL Error: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	const SDL_GLContext gl_context = SDL_GL_CreateContext(window);

	if (gl_context == nullptr)
	{
		printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	std::cout << SDL_GetError() << std::endl;

	const GLenum err = glewInit();

	if (err != GLEW_OK)
	{
		fprintf(stderr, "Failed to init OpenGL loader!\n");
		return EXIT_FAILURE;
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

	const ImVec4 clear_color = ImVec4(1.f, 1.f, 1.f, 1.00f);

	// Main loop
	bool done = false;

	Camera kinect;
	if (!kinect.Init())
		return EXIT_FAILURE;
	
	ModelCapture capture(&kinect);
	if (!capture.Init())
		return EXIT_FAILURE;

	float preview_angle = 1;
	float preview_xyz[3] = {0.0f, 0.001f, 0.0f};

	//Frame delta time
	Uint64 NOW = SDL_GetPerformanceCounter();
	Uint64 LAST = 0;
	float deltaTime = 0;
	
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

		LAST = NOW;
		NOW = SDL_GetPerformanceCounter();

		deltaTime = static_cast<float>(((NOW - LAST) * 1000 / static_cast<float>(SDL_GetPerformanceFrequency())) * 0.001f);
		
		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		//getKinectData();
		kinect.GetKinectData();
		kinect.RenderToTexture(preview_angle, preview_xyz[0], preview_xyz[1], preview_xyz[2]);

		ImVec2 vMin = ImGui::GetWindowContentRegionMin();
		ImVec2 vMax = ImGui::GetWindowContentRegionMax();

		ImVec2 size(vMax.x - vMin.x, vMax.y - vMin.y);
		
		if(ImGui::Begin("Point Cloud"))
		{			
			ImGui::Image(reinterpret_cast<void*>(kinect.GetTexture()[0]), size, ImVec2(0, 1), ImVec2(1, 0));

			ImGui::Separator();
			
			ImGui::DragFloat("Angle", &preview_angle, 0.01f, -6.283f, 6.283f);
			ImGui::DragFloat3("Position", preview_xyz, 0.01f);
		}
		ImGui::End();

		if (ImGui::Begin("RGB Image"))
		{
			ImGui::Image(reinterpret_cast<void*>(kinect.GetTexture()[1]), size, ImVec2(1, 0), ImVec2(0, 1));
		}
		ImGui::End();

		capture.Render(preview_angle, preview_xyz[0], preview_xyz[1], preview_xyz[2]);
		capture.ModelGatherTick(deltaTime);

		// Rendering
		ImGui::Render();
		
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