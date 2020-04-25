#pragma once
using namespace std;
#include <Windows.h>
#include <string>
#include <iostream>
#include <fstream>
 

#define SOH 0x01 
#define EOT 0x04 
#define ACK 0x06 
#define NAK 0x43
#define CAN 0x18 
#define C 0x43	
#define END 0x1A 


class Port
{
private:
	HANDLE handle;
	DCB setupPort;
	COMMTIMEOUTS timeouts;
	COMSTAT portResources;
	DWORD   error;

public:
	Port();
	virtual ~Port();
	void openPort();
	void communicationTest();
	void receiver(bool withCRC);
	void transmitter();
	void readCOM(char* sign);
	void readCOM(char*, int lenght);
	void writeCOM(char* sign, int length);

};

