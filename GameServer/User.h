#pragma once

#include <Windows.h> // For CopyMemory

class User
{
	const int packet_data_buffer_size = 65536;

public:
	void Init();	

	// writePacket
	void SetPacketData(int dataSize, char* pData);
	void ReadPacketData(int readSize);	
	
public:
	// char mPacketDataBuffer[packet_data_buffer_size];
	char* mPacketDataBuffer = nullptr;
	int mPacketDataBufferWritePos = 0;
	int mPacketDataBufferReadPos = 0;
};

