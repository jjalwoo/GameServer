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
		// ���� ��Ŷ�� ������ + ���� ���ۿ� �����ִ� ������ > ��Ŷ ��ü�� ������
		if (mPacketDataBufferWritePos + datasize > packet_data_buffer_size)
		{
			// �� ������ - ���� ������
			int remaining = mPacketDataBufferWritePos - mPacketDataBufferReadPos;

			// 0���� ũ�ٸ� ���� ���� �����Ͱ� �ִ� ���̴ϱ� ���� ���� �����͸� ������ �ű� �� ä���.
			if(remaining > 0)
			{
				// Move remaining data to the start of the buffer
				// ! 
			}
			else if(remaining == 0)	// ���۰� ����ų�, �� �о����Ƿ� ���۸� ó������ �����Ѵ�.
			{
				mPacketDataBufferWritePos = 0;
			}

			mPacketDataBufferReadPos = 0;
		}

		
		// ���ۿ� ���� �����͸� �����Ѵ�.
		CopyMemory(&mPacketDataBuffer[mPacketDataBufferWritePos], pdata, datasize);
		// ���� �����͸�ŭ Write������ �ø���.
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

