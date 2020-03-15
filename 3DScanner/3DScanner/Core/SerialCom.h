#pragma once
#include <vector>
#include <string>
#include <Windows.h>
#include <Winbase.h>

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
	void ShowError();

	//Check auth code from serial com
	bool CheckAuthCode();

public:

	SerialCom();
	~SerialCom();

	[[nodiscard]] bool OpenPort(std::string comPort, int baudRate = CBR_9600);
	void ClosePort();

	[[nodiscard]] bool DataAvailable();
	[[nodiscard]] bool DataInBuffer();

	void SearchForAvailablePorts();

	[[nodiscard]] std::vector<std::string> GetAvailablePorts();

	char GetCharFromBuffer();
	std::string GetReadBuffer();

	bool WriteChar(char outChar);
	bool WriteString(std::string outString);

};