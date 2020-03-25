#include "ModelCapture.h"
#include <cmath>
#include "imgui.h"

#include <CGAL/wlop_simplify_and_regularize_point_set.h>
#include <CGAL/property_map.h>
#include <CGAL/IO/write_ply_points.h>


void ModelCapture::GetCameraFrame()
{
	const ModelShot* current_shot = camera_->GetCurrentModelShot();

	CGAL_Model shot;

	for (int i = 0; i < (DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT); ++i)
	{
		if (current_shot->xyz[i].Z >= scan_settings_.minDistance &&
			current_shot->xyz[i].Z <= scan_settings_.maxDistance)
		{
			shot.AddPoint(
				Point(
				current_shot->xyz[i].X,
				current_shot->xyz[i].Y,
				current_shot->xyz[i].Z),
				Color(
				current_shot->rgbimage[(i * 4)],
				current_shot->rgbimage[(i * 4) + 1],
				current_shot->rgbimage[(i * 4) + 2]
				));
		}
	}

	CGAL_Model output;
	CGAL::wlop_simplify_and_regularize_point_set<CGAL::Sequential_tag>
		(shot.xyz,
			std::back_inserter(output.xyz),
			CGAL::parameters::select_percentage(scan_settings_.retainPercentage).
			neighbor_radius(scan_settings_.neighborRadius));

	//Add the colour to the output
	for(auto xyz : output.xyz)
	{
		auto it = std::find(shot.xyz.begin(), shot.xyz.end(), xyz);

		if(it != shot.xyz.end())
		{
			output.rgb.push_back(shot.rgb[std::distance(shot.xyz.begin(), it)]);
		}
	}

	currentModel.push_back(output);
}

ModelCapture::ModelCapture(Camera* camera) : camera_(camera), fileDialog(ImGuiFileBrowserFlags_EnterNewFilename)
{
	serial_com_.SearchForAvailablePorts();

	if (!serial_com_.GetAvailablePorts().empty())
		serialPort = serial_com_.GetAvailablePorts()[0];
	
	fileDialog.SetTitle("Save mesh to");
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

	if (!currentModel.empty())
	{
		for (int i = 0; i < currentModel.size(); ++i)
		{
			if (!currentModel[i].xyz.empty())
			{
				glBindBuffer(GL_ARRAY_BUFFER, currentModel[i].vboId);
				glVertexPointer(3, GL_FLOAT, 0, nullptr);

				glBindBuffer(GL_ARRAY_BUFFER, currentModel[i].cboId);
				glColorPointer(3, GL_FLOAT, 0, nullptr);

				glPointSize(1.f);
				glDrawArrays(GL_POINTS, 0, currentModel[i].xyz.size());
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
				if (ImGui::BeginCombo("Serial Port:", serialPort.c_str()))
				{
					std::vector<std::string> ports = serial_com_.GetAvailablePorts();
					
					for (std::string port : ports)
					{
						bool isSelected = port == serialPort;
						if (ImGui::Selectable(port.c_str(), isSelected))
							serialPort = port;
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}
				
				if (ImGui::Button("Connect"))
				{
					std::string port(R"(\\.\)");
					port += serialPort;
					if (serial_com_.OpenPort(port))
					{
						connected = true;
						serialInFuture = std::async(std::launch::async, &SerialCom::DataAvailable, &serial_com_);
					}
					else
						connected = false;
					
				}
			}
			else
			{
				if(motorBusy)
				{
					ImGui::TextColored(ImVec4(1,0,0,1),"Motor Busy!");
				}

				if (!motorBusy)
				{
					if (ImGui::Button("Single Turn"))
					{
						//Reset turntable
						serial_com_.WriteChar('R');

						//We are changing the number of turns we need
						serial_com_.WriteChar('T');

						//Set number of Images
						serial_com_.WriteChar(static_cast<char>(scan_settings_.numberOfImages));

						//Do a single turn
						serial_com_.WriteChar('I');
						motorBusy = true;
					}

					if(ImGui::Button("Start Scan"))
					{
						//Reset turntable
						serial_com_.WriteChar('R');

						//We are changing the number of turns we need
						serial_com_.WriteChar('T');
						
						//Set number of Images
						serial_com_.WriteChar(static_cast<char>(scan_settings_.numberOfImages));

						capturing = true;
					}
				}

				if (ImGui::Button("Disconnect"))
				{
					serial_com_.ClosePort();
					connected = false;
					motorBusy = false;
				}
			}
				
			ImGui::DragInt("Number of Images:", &scan_settings_.numberOfImages, 1, 20, 255);

			if (ImGui::Button("Start"))
			{
				if (!currentModel.empty())
					 currentModel.clear();

				//Create a model shot for each shot
				currentModel.reserve(scan_settings_.numberOfImages);
				capturing = true;
			}

			if (!currentModel.empty())
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
					//TODO:Export model
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
	if (!connected)
		return;

	if (serialInFuture.valid() && SerialCom::is_Ready(serialInFuture))
	{
		if (serialInFuture.get())
		{
			char c = serial_com_.GetCharFromBuffer();

			while (c != '\0')
			{
				if (c == 'D')
				{
					motorBusy = false;
				}
				else if (capturing && !motorBusy)
				{
					GetCameraFrame();
					serial_com_.WriteChar('S');
					motorBusy = true;
				}
				//Finished all rotation
				else if (c == 'F')
				{
					motorBusy = false;
					capturing = false;

					GetCameraFrame();
				}

				c = serial_com_.GetCharFromBuffer();
			}
		}
		
		serialInFuture = std::async(std::launch::async, &SerialCom::DataAvailable, &serial_com_);
	}
}