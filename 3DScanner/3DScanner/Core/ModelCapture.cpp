#include "ModelCapture.h"
#include <cmath>
#include "imgui.h"

#include <CGAL/wlop_simplify_and_regularize_point_set.h>
#include <CGAL/IO/write_ply_points.h>

#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>

#define BACK_SUB_AMOUNT 100

//TODO:CLIP CUBE

void ModelCapture::GetCameraFrame()
{
	const ModelShot* current_shot = camera_->GetCurrentModelShot();

	if (!current_shot)
		return;
	
	CGAL_Model shot;

	if (hasIgnore)
	{		
		cv::Mat imageMat = cv::Mat(RGB_SENSOR_WIDTH * 4, RGB_SENSOR_HEIGHT, CV_8UC1, current_shot->rgbimage);
		scan_settings_.backSub->apply(imageMat, scan_settings_.ignoreMask, 0);
	}
		
	//Create list
	for (int i = 0; i < (DEPTH_SENSOR_WIDTH * DEPTH_SENSOR_HEIGHT); ++i)
	{
		ColorSpacePoint p = current_shot->rgb[i];
		int idx = static_cast<int>(p.X) + RGB_SENSOR_WIDTH * static_cast<int>(p.Y);

		if (p.X < 0 || p.Y < 0 || p.X > RGB_SENSOR_WIDTH || p.Y > RGB_SENSOR_HEIGHT && (idx > 2073596))
			continue;

		float cubeMin[3];
		float cubeMax[3];

		cubeMin[0] = ((-0.1f * scan_settings_.cubeScale[0]) + scan_settings_.cubePos[0]);
		cubeMin[1] = ((-0.1f * scan_settings_.cubeScale[1]) + scan_settings_.cubePos[1]);
		cubeMin[2] = ((-0.1f * scan_settings_.cubeScale[2]) + scan_settings_.cubePos[2]);

		cubeMax[0] = ((0.1f * scan_settings_.cubeScale[0]) + scan_settings_.cubePos[0]);
		cubeMax[1] = ((0.1f * scan_settings_.cubeScale[1]) + scan_settings_.cubePos[1]);
		cubeMax[2] = ((0.1f * scan_settings_.cubeScale[2]) + scan_settings_.cubePos[2]);
		
		if (current_shot->xyz[i].Z >= scan_settings_.minDistance &&
			current_shot->xyz[i].Z <= scan_settings_.maxDistance &&
			(!hasIgnore ||				
			//check ignore mask
			(scan_settings_.ignoreMask.at<unsigned char>(idx * 4) > BACK_SUB_AMOUNT ||
				scan_settings_.ignoreMask.at<unsigned char>(idx * 4 + 1) > BACK_SUB_AMOUNT ||
				scan_settings_.ignoreMask.at<unsigned char>(idx * 4 + 2) > BACK_SUB_AMOUNT)) &&
			//Check Ignore cube
				(!scan_settings_.cubeSet ||
					(cubeMin[0] <= current_shot->xyz[i].X && cubeMax[0] >= current_shot->xyz[i].X) &&
					(cubeMin[1] <= current_shot->xyz[i].Y && cubeMax[1] >= current_shot->xyz[i].Y) &&
					(cubeMin[2] <= current_shot->xyz[i].Z && cubeMax[2] >= current_shot->xyz[i].Z)))
		{
			shot.AddPoint(
				glm::vec3(
					current_shot->xyz[i].X,
					current_shot->xyz[i].Y,
					current_shot->xyz[i].Z),
				glm::vec3(
					current_shot->rgbimage[(4 * idx)],
					current_shot->rgbimage[(4 * idx) + 1],
					current_shot->rgbimage[(4 * idx) + 2]
				));
		}
	}

	//If we didn't get any points separate
	if (shot.vertex.empty())
		return;
	
	currentModel.push_back(std::move(shot));
	
	//CGAL_Model output;
	//CGAL::wlop_simplify_and_regularize_point_set<CGAL::Sequential_tag>
	//	(shot.xyz,
	//		std::back_inserter(output.xyz),
	//		CGAL::parameters::select_percentage(scan_settings_.retainPercentage).
	//		neighbor_radius(scan_settings_.neighborRadius));

	////Add the colour to the output
	//for(auto xyz : output.xyz)
	//{
	//	auto it = std::find(shot.xyz.begin(), shot.xyz.end(), xyz);

	//	if(it != shot.xyz.end())
	//	{
	//		output.rgb.push_back(shot.rgb[std::distance(shot.xyz.begin(), it)]);
	//	}
	//}

	//currentModel.push_back(output);
}

void ModelCapture::CreateIgnoreFrame()
{
	const ModelShot* current_shot = camera_->GetCurrentModelShot();

	if (!current_shot)
		return;

	memcpy(scan_settings_.rgbIgnoreImage, current_shot->rgbimage, (RGB_SENSOR_WIDTH * RGB_SENSOR_HEIGHT * 4));
	scan_settings_.ignoreMat = cv::Mat(RGB_SENSOR_WIDTH * 4, RGB_SENSOR_HEIGHT, CV_8UC1, scan_settings_.rgbIgnoreImage);
	scan_settings_.ignoreMask = cv::Mat(RGB_SENSOR_WIDTH * 4, RGB_SENSOR_HEIGHT, CV_8UC1);

	scan_settings_.backSub = cv::createBackgroundSubtractorMOG2(scan_settings_.history, scan_settings_.varThreshold, false);
	scan_settings_.backSub->apply(scan_settings_.ignoreMat, scan_settings_.ignoreMask, 1);

	scan_settings_.currentIgnoreFrame = 0;
	lastIgnoreFrameID = camera_->GetFrameID();

	hasIgnore = true;
}

void ModelCapture::AddIgnoreFrame()
{
	if (lastIgnoreFrameID != camera_->GetFrameID())
	{
		const ModelShot* current_shot = camera_->GetCurrentModelShot();

		if (!current_shot)
			return;
		
		scan_settings_.currentIgnoreFrame++;
		
		cv::Mat imageMat = cv::Mat(RGB_SENSOR_WIDTH * 4, RGB_SENSOR_HEIGHT, CV_8UC1, current_shot->rgbimage);
		scan_settings_.backSub->apply(imageMat, scan_settings_.ignoreMask);
	}	
}

ModelCapture::ModelCapture(Camera* camera) : camera_(camera), fileDialog(ImGuiFileBrowserFlags_EnterNewFilename)
{
	scan_settings_.rgbIgnoreImage = new unsigned char[RGB_SENSOR_WIDTH * RGB_SENSOR_HEIGHT * 4];
	
	serial_com_.SearchForAvailablePorts();

	if (!serial_com_.GetAvailablePorts().empty())
		serialPort = serial_com_.GetAvailablePorts()[0];
	
	fileDialog.SetTitle("Save mesh to");
}

ModelCapture::~ModelCapture()
{
	delete[] scan_settings_.rgbIgnoreImage;
}

bool ModelCapture::Init()
{
	glGenFramebuffers(1, &modelFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, modelFrameBuffer);
	
	glGenTextures(1, &modelTexture);
	
	glBindTexture(GL_TEXTURE_2D, modelTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, DEPTH_SENSOR_WIDTH, DEPTH_SENSOR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenRenderbuffers(1, &modelBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, modelBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, DEPTH_SENSOR_WIDTH, DEPTH_SENSOR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, modelBuffer);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, modelTexture, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false; 
	
	return true;	
}

void ModelCapture::RenderToTexture(float angle, float x, float y, float z) const
{
	glBindFramebuffer(GL_FRAMEBUFFER, modelBuffer);
	glViewport(0, 0, DEPTH_SENSOR_WIDTH, DEPTH_SENSOR_HEIGHT);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(80, DEPTH_SENSOR_WIDTH / static_cast<GLdouble>(DEPTH_SENSOR_HEIGHT), 0.1, 1000);

	const float eyex = 1 * cos(angle);// -z * sin(angle) + x;
	const float eyey = 1 * sin(angle);// +z * cos(angle) + z;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(x, y, z, x + eyex, y, z + eyey, 0, y, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glPointSize(1.0f);
	glBegin(GL_POINTS);

	if (!currentModel.empty())
	{
		//					get angle for each image
		float singleTurn = (360.f / scan_settings_.numberOfImages) * (glm::pi<float>() / 180.f);
		glm::vec3 centerPoint(
			scan_settings_.cubePos[0],
			scan_settings_.cubePos[1],
			scan_settings_.cubePos[2]
		);
		
		//for (const auto& model : currentModel)
		for(int i = 0; i < currentModel.size(); ++i)
		{
			if (!currentModel.at(i).vertex.empty())
			{
				for(Vertex v : currentModel.at(i).vertex)
				{					
					Vertex rotatedV = v.RotateAroundPoint(centerPoint, singleTurn * i);
					glColor3ub(rotatedV.color.r, rotatedV.color.g, rotatedV.color.b);
					glVertex3f(rotatedV.pos.x, rotatedV.pos.y, rotatedV.pos.z);
				}
			}
		}
	}
	glEnd();
	
	if(scan_settings_.showCube)
	{		
		glBegin(GL_LINES);
		glColor3ub(255, 0, 0);

		float cubeMin[3];
		float cubeMax[3];

		cubeMin[0] = ((-0.1f * scan_settings_.cubeScale[0]) + scan_settings_.cubePos[0]);
		cubeMin[1] = ((-0.1f * scan_settings_.cubeScale[1]) + scan_settings_.cubePos[1]);
		cubeMin[2] = ((-0.1f * scan_settings_.cubeScale[2]) + scan_settings_.cubePos[2]);

		cubeMax[0] = ((0.1f * scan_settings_.cubeScale[0]) + scan_settings_.cubePos[0]);
		cubeMax[1] = ((0.1f * scan_settings_.cubeScale[1]) + scan_settings_.cubePos[1]);
		cubeMax[2] = ((0.1f * scan_settings_.cubeScale[2]) + scan_settings_.cubePos[2]);
		
		glVertex3f(cubeMin[0], cubeMin[1], cubeMin[2]);
		glVertex3f(cubeMax[0], cubeMin[1], cubeMin[2]);

		glVertex3f(cubeMin[0], cubeMin[1], cubeMin[2]);
		glVertex3f(cubeMin[0], cubeMax[1], cubeMin[2]);

		glVertex3f(cubeMin[0], cubeMin[1], cubeMin[2]);
		glVertex3f(cubeMin[0], cubeMin[1], cubeMax[2]);

		glVertex3f(cubeMax[0], cubeMax[1], cubeMax[2]);
		glVertex3f(cubeMin[0], cubeMax[1], cubeMax[2]);

		glVertex3f(cubeMax[0], cubeMax[1], cubeMax[2]);
		glVertex3f(cubeMax[0], cubeMin[1], cubeMax[2]);

		glVertex3f(cubeMax[0], cubeMax[1], cubeMax[2]);
		glVertex3f(cubeMax[0], cubeMax[1], cubeMin[2]);

		glVertex3f(cubeMin[0], cubeMin[1], cubeMax[2]);
		glVertex3f(cubeMax[0], cubeMin[1], cubeMax[2]);

		glVertex3f(cubeMax[0], cubeMin[1], cubeMin[2]);
		glVertex3f(cubeMax[0], cubeMin[1], cubeMax[2]);

		glVertex3f(cubeMin[0], cubeMin[1], cubeMax[2]);
		glVertex3f(cubeMin[0], cubeMax[1], cubeMax[2]);

		glVertex3f(cubeMin[0], cubeMax[1], cubeMin[2]);
		glVertex3f(cubeMin[0], cubeMax[1], cubeMax[2]);

		glVertex3f(cubeMin[0], cubeMax[1], cubeMin[2]);
		glVertex3f(cubeMax[0], cubeMax[1], cubeMin[2]);

		glVertex3f(cubeMax[0], cubeMin[1], cubeMin[2]);
		glVertex3f(cubeMax[0], cubeMax[1], cubeMin[2]);
				
		glEnd();
	}

	glFlush();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ModelCapture::RenderToTexturePreview(float angle, float x, float y, float z)
{
	if (lastFrameID != camera_->GetFrameID())
	{
		currentModel.clear();
		GetCameraFrame();

		RenderToTexture(angle, x, y, z);		
		lastFrameID = camera_->GetFrameID();
	}
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

			ImGui::DragInt("Number of Images:", &scan_settings_.numberOfImages, 1, 20, 255);
			ImGui::DragFloat("Min Distance:", &scan_settings_.minDistance, 0.01f, 0.0f, 10.0f);
			ImGui::DragFloat("Max Distance:", &scan_settings_.maxDistance, 0.01f, 0.0f, 10.0f);

			ImGui::Separator();

			ImGui::DragInt("Ignore History:", &scan_settings_.history, 1, 20);
			ImGui::DragFloat("Ignore Threshold:", &scan_settings_.varThreshold, 0.1f, 0.0f, 100.0f);

			ImGui::Separator();
			
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

				if ( ImGui::Button("Disconnect"))
				{
					serial_com_.ClosePort();
					connected = false;
					motorBusy = false;
				}
			}
			
			//Start scanning the model
			if (ImGui::Button("Start"))
			{
				if (!currentModel.empty())				
					currentModel.clear();
					
				//Create a model shot for each shot
				currentModel.reserve(scan_settings_.numberOfImages);
				capturing = true;
			}
		
		}
		ImGui::End();

		RenderToTexturePreview(angle, x, y, z);

		if (ImGui::Begin("Scan Preview"))
		{
			/*ImVec2 vMin = ImGui::GetWindowContentRegionMin();
			ImVec2 vMax = ImGui::GetWindowContentRegionMax();

			ImVec2 size(vMax.x - vMin.x, vMax.y - vMin.y);*/

			ImVec2 size(DEPTH_SENSOR_WIDTH, DEPTH_SENSOR_HEIGHT);

			ImGui::Image(reinterpret_cast<void*>(modelTexture), size, ImVec2(0, 1), ImVec2(1, 0));

			ImGui::Separator();

			ImGui::DragInt("Number Ignore images", &scan_settings_.ignoreFrames, 1, 1, 100);

			if(scan_settings_.ignoreFrames > scan_settings_.currentIgnoreFrame)
			scan_settings_.currentIgnoreFrame = scan_settings_.ignoreFrames;

			if (ImGui::Button("Take Ignore Shot"))
			{
				CreateIgnoreFrame();
			}

			ImGui::Checkbox("Show Ignore Box", &scan_settings_.showCube);

			if (scan_settings_.showCube)
			{
				ImGui::Checkbox("Clip Ignore box", &scan_settings_.cubeSet);
				ImGui::DragFloat3("Cube Pos:", &scan_settings_.cubePos[0], 0.01f, -100.0f, 100.0f);
				ImGui::DragFloat3("Cube Scale:", &scan_settings_.cubeScale[0], 0.01f, -100.0f, 100.0f);
			}
		}
		ImGui::End();
	}
	else
	{
		if(ImGui::Begin("Model Preview"))
		{
			
		}
		ImGui::End();
	}
	
}

void ModelCapture::ModelGatherTick(float time)
{
	if (!capturing && hasIgnore && scan_settings_.ignoreFrames > scan_settings_.currentIgnoreFrame)
		AddIgnoreFrame();
	
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

					//Convert multiple point clouds into mesh
				}

				c = serial_com_.GetCharFromBuffer();
			}
		}
		
		serialInFuture = std::async(std::launch::async, &SerialCom::DataAvailable, &serial_com_);
	}
}