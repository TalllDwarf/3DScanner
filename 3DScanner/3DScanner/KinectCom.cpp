#include "KinectCom.h"

void KinectCom::UpdateRGBData(IMultiSourceFrame* frame)
{
	IColorFrame* colourFrame = nullptr;
	IColorFrameReference* frameRef = nullptr;

	frame->get_ColorFrameReference(&frameRef);
	frameRef->AcquireFrame(&colourFrame);

	if (frameRef)
		frameRef->Release();

	if (!colourFrame)
		return;

	colourFrame->CopyConvertedFrameDataToArray((RGB_WIDTH * RGB_HEIGHT * 4), RgbData, ColorImageFormat_Bgra);

	if (colourFrame)
		colourFrame->Release();
}

void KinectCom::UpdateDepthData(IMultiSourceFrame* frame)
{
	IDepthFrame* depthFrame = nullptr;
	IDepthFrameReference* frameRef = nullptr;

	frame->get_DepthFrameReference(&frameRef);
	frameRef->AcquireFrame(&depthFrame);

	if (frameRef)
		frameRef->Release();

	if (!depthFrame)
		return;

	unsigned int size;
	unsigned short* depthBuffer;

	depthFrame->AccessUnderlyingBuffer(&size, &depthBuffer);

	mapper->MapDepthFrameToCameraSpace(
		IR_WIDTH * IR_HEIGHT, depthBuffer,
		IR_WIDTH * IR_HEIGHT, depthToVector);

	mapper->MapDepthFrameToColorSpace(
		IR_WIDTH * IR_HEIGHT, depthBuffer,
		IR_WIDTH * IR_HEIGHT, depthToColour);

	if (depthFrame)
		depthFrame->Release();
}

bool KinectCom::Init()
{
	if (FAILED(GetDefaultKinectSensor(&sensor)))
		return false;

	if (sensor)
	{
		sensor->get_CoordinateMapper(&mapper);
		sensor->Open();
		sensor->OpenMultiSourceFrameReader(
			FrameSourceTypes::FrameSourceTypes_Depth | FrameSourceTypes::FrameSourceTypes_Color,
			&reader);

		return reader;
	}
	else
		return false;
}

void KinectCom::InitTexture()
{
	glGenTextures(1, &rgbTexID);
	glBindTexture(GL_TEXTURE_2D, rgbTexID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, RGB_WIDTH, RGB_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)RgbData);
	glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint KinectCom::GetRGBTextureId()
{
	return rgbTexID;
}

GLuint KinectCom::GetDepthTextureId()
{
	return GLuint();
}

void KinectCom::Update()
{
	IMultiSourceFrame* frame = nullptr;
	reader->AcquireLatestFrame(&frame);

	if (frame)
	{
		UpdateRGBData(frame);
		UpdateDepthData(frame);

		frame->Release();
	}
}