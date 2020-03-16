#include "ModelCapture.h"

#include <cmath>

#include "imgui.h"

ModelCapture::ModelCapture(Camera* camera) : camera_(camera)
{
	serial_com_.SearchForAvailablePorts();
	
}

bool ModelCapture::Init()
{
	glGenFramebuffers(1, &modelFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, modelFrameBuffer);

	glGenTextures(1, modelTexture);

	glBindTexture(GL_TEXTURE_2D, *modelTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, RGB_SENSOR_WIDTH, RGB_SENSOR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenRenderbuffers(1, &modelBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, modelBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, RGB_SENSOR_WIDTH, RGB_SENSOR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, modelBuffer);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *modelTexture, 0);
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	return true;	
}

void ModelCapture::RenderToTexture(float angle, float x, float y, float z) const
{
	glBindFramebuffer(GL_FRAMEBUFFER, modelBuffer);
	glViewport(0, 0, RGB_SENSOR_WIDTH, RGB_SENSOR_HEIGHT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(80, RGB_SENSOR_WIDTH / static_cast<GLdouble>(RGB_SENSOR_HEIGHT), 0.1, 1000);

	const float eyex = 1 * cos(angle);// -z * sin(angle) + x;
	const float eyey = 1 * sin(angle);// +z * cos(angle) + z;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(x, y, z, x + eyex, y, z + eyey, 0, y, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	if (currentModel != nullptr && modelSize != 0)
	{
		for (int i = 0; i < modelSize; ++i)
		{
			if (currentModel->point_size > 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, (currentModel + i)->vboId);
				glVertexPointer(3, GL_FLOAT, 0, nullptr);

				glBindBuffer(GL_ARRAY_BUFFER, (currentModel + i)->cboId);
				glColorPointer(3, GL_FLOAT, 0, nullptr);

				glPointSize(1.f);
				glDrawArrays(GL_POINTS, 0, currentModel->point_size);
			}
		}
	}
		
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ModelCapture::Render(float angle, float x, float y, float z)
{
	if (!capturing)
	{
		if (ImGui::Begin("Scan Settings"))
		{
			//Full revolution
			//2049 = full motor rotation
			//14343 = Full turntable rotation
			//
			//20-255 images to generate model
			//

			if (!connected)
			{
				if (ImGui::BeginCombo("Serial Port:", serialPort))
				{
					std::vector<std::string> ports = serial_com_.GetAvailablePorts();

					for (std::string port : ports)
					{
						bool isSelected = port.c_str() == serialPort;
						if (ImGui::Selectable(serialPort, isSelected))
							serialPort = port.c_str();
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}
				
				if (ImGui::Button("Connect"))
				{
					std::string port(R"(\\.\)");
					port += serialPort;
					connected = serial_com_.OpenPort(port);
				}
			}
			else
			{
				if(motorBusy)
				{
					ImGui::TextColored(ImVec4(1,0,0,1),"Motor Busy!");
				}
				
				if(ImGui::Button("Single Turn"))
				{
					serial_com_.WriteChar('I');
					motorBusy = true;
				}				
				
				if(ImGui::Button("Disconnect"))
				{
					serial_com_.ClosePort();
					connected = false;
					motorBusy = false;
				}
			}
				
			ImGui::DragInt("Number of Images:", &scan_settings_.numberOfImages, 1, 20, 255);

			if (ImGui::Button("Start"))
			{
				if (currentModel != nullptr)
				{
					for (int i = 0; i < modelSize; ++i)
					{
						(currentModel + i)->Dispose();
					}

					delete[] currentModel;
				}

				//Create a model shot for each shot
				currentModel = new ModelShot[scan_settings_.numberOfImages];
				capturing = true;
			}

			if (currentModel != nullptr)
			{
				RenderToTexture(angle, x, y, z);

				if (ImGui::Begin("Model"))
				{
					ImVec2 vMin = ImGui::GetWindowContentRegionMin();
					ImVec2 vMax = ImGui::GetWindowContentRegionMax();

					ImVec2 size(vMax.x - vMin.x, vMax.y - vMin.y);

					ImGui::Image(reinterpret_cast<void*>(modelTexture), size, ImVec2(0, 1), ImVec2(1, 0));
				}
				ImGui::End();


				if (ImGui::Button("Export"))
				{

				}
			}
		}
		ImGui::End();
	}
	else
	{
		
	}
	
}

void ModelCapture::ModelGatherTick(float time)
{
	char c = serial_com_.GetCharFromBuffer();

	if(c == 'D' || c == 'F')
	{
		motorBusy = false;
	}
	
	if(capturing)
	{
		
	}	
}