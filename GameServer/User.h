#pragma once

#include <Windows.h> // For CopyMemory

class User
{
	const int packet_data_buffer_size = 65536;
public:
	void Init()
	{
		mPacketDataBuffer = new char[packet_data_buffer_size];
	}

	// writePacket
	void SetPacketData(int datasize, char* pdata)
	{
		// 들어온 패킷의 사이즈 + 기존 버퍼에 남아있는 사이즈 > 패킷 전체의 사이즈
		if (mPacketDataBufferWritePos + datasize > packet_data_buffer_size)
		{
			// 쓴 데이터 - 읽은 데이터
			int remaining = mPacketDataBufferWritePos - mPacketDataBufferReadPos;

			// 0보다 크다면 읽지 않은 데이터가 있는 뜻이니까 읽지 않은 데이터를 앞으로 옮긴 후 채운다.
			if(remaining > 0)
			{
				// Move remaining data to the start of the buffer
				// ! 
			}
			else if(remaining == 0)	// 버퍼가 비웠거나, 다 읽었으므로 버퍼를 처음부터 시작한다.
			{
				mPacketDataBufferWritePos = 0;
			}

			mPacketDataBufferReadPos = 0;
		}

		
		// 버퍼에 들어온 데이터를 복사한다.
		CopyMemory(&mPacketDataBuffer[mPacketDataBufferWritePos], pdata, datasize);
		// 들어온 데이터만큼 Write포스를 올린다.
		mPacketDataBufferWritePos += datasize;
	}

	void ReadPacketData()
	{

	}
	
	// char mPacketDataBuffer[packet_data_buffer_size];
	char* mPacketDataBuffer = nullptr;
	int mPacketDataBufferWritePos = 0;
	int mPacketDataBufferReadPos = 0;


};

