#include <iostream>
#include "SerialCom.h"

void SerialCom::SearchForAvailablePorts()
{
	char lpTargetPath[5000]; // Stores path to the comports
	bool foundPorts = false;
	DWORD result;

	availableComs.clear();

	for (int i = 0; i < 255; ++i)
	{
		//Creating COM0, COM1, COM2
		std::string comString = "COM" + std::to_string(i);

		result = QueryDosDeviceA(comString.c_str(), lpTargetPath, 5000);

		if (result != 0)
		{
			availableComs.push_back(comString);
		}
	}
}

void SerialCom::ShowError()
{
	char lastError[1024];
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		lastError,
		1024,
		nullptr);

	//TODO:Display error
}

SerialCom::SerialCom() : serialHandle(nullptr)
{
	SearchForAvailablePorts();
}

SerialCom::~SerialCom()
{
	if (serialHandle)
		CloseHandle(serialHandle);
}

bool SerialCom::OpenPort(std::string comPort, int baudRate)
{
	serialHandle = CreateFileA(
		comPort.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (serialHandle == INVALID_HANDLE_VALUE)
	{
		ShowError();
		ClosePort();

		//if (GetLastError() == ERROR_FILE_NOT_FOUND)
		//{
		//	//TODO:Tell user the serial port does not exist
		//}
		////Or other error

		return false;
	}

	DCB serialParamDCB = { 0 };
	serialParamDCB.DCBlength = sizeof(serialParamDCB);

	if (!GetCommState(serialHandle, &serialParamDCB))
	{
		ShowError();
		ClosePort();
		return false;
	}

	serialParamDCB.BaudRate = baudRate;
	serialParamDCB.ByteSize = 8;

	if (!SetCommState(serialHandle, &serialParamDCB))
	{
		ShowError();
		ClosePort();
		return false;
	}

	COMMTIMEOUTS timeouts = { 0 };

	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	if (!SetCommTimeouts(serialHandle, &timeouts))
	{
		ShowError();
		ClosePort();
		return false;
	}

	return true;
}

void SerialCom::ClosePort()
{
	if (serialHandle)
	{
		CloseHandle(serialHandle);
		serialHandle = nullptr;
	}
}

bool SerialCom::DataAvailable()
{
	char buffer[BUFFER_SIZE + 1] = { 0 };
	DWORD bytesRead = 0;

	if (!ReadFile(serialHandle, buffer, BUFFER_SIZE, &bytesRead, nullptr))
	{
		ShowError();
		return false;
	}

	readBuffer = std::string(&buffer[0], (size_t)bytesRead);

	return (bytesRead > 0);
}

bool SerialCom::DataInBuffer() const
{
	return readBuffer.size() > 0;
}

std::vector<std::string> SerialCom::GetAvailablePorts() const
{
	return availableComs;
}

char SerialCom::GetCharFromBuffer()
{
	if (readBuffer.size() == 0)
		return '\0';

	char c = readBuffer[0];

	//Erase first char
	readBuffer.erase(0, 1);
	return c;
}

std::string SerialCom::GetReadBuffer() const
{
	return readBuffer;
}

bool SerialCom::WriteChar(char outChar)
{
	DWORD bytesWritten = 0;

	if (!WriteFile(serialHandle, &outChar, 1, &bytesWritten, nullptr))
	{
		ShowError();
		return false;
	}

	return true;
}

bool SerialCom::WriteString(const std::string outString)
{
	DWORD bytesWritten = 0;

	if (!WriteFile(serialHandle, outString.c_str(), outString.size(), &bytesWritten, nullptr))
	{
		ShowError();
		return false;
	}

	return true;
}
