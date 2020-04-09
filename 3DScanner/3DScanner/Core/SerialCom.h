#pragma once
#include <vector>
#include <string>
#include <future>
#include <Windows.h>
#include <Winbase.h>

//Windows defines min max breaks CGAL library
#undef max;
#undef min;

#define BUFFER_SIZE 255
#define AUTH_SIZE 3

class SerialCom
{
	//The handle to the serial port
	HANDLE serialHandle;

	//list of availble serial ports
	std::vector<std::string> availableComs;
	std::string readBuffer;

	//Gets the last error and stores it in lastError
	static void ShowError();

public:

	SerialCom();
	~SerialCom();

	[[nodiscard]] bool OpenPort(std::string comPort, int baudRate = CBR_9600);
	void ClosePort();
	
	[[nodiscard]] bool DataAvailable();
	[[nodiscard]] bool DataInBuffer() const;

	void SearchForAvailablePorts();

	[[nodiscard]] std::vector<std::string> GetAvailablePorts() const;

	char GetCharFromBuffer();
	std::string GetReadBuffer() const;

	bool WriteChar(char outChar) const;
	bool WriteString(std::string outString) const;

};