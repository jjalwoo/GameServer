#include "User.h"

void User::Init()
{
	mPacketDataBuffer = new char[packet_data_buffer_size];
}

void User::SetPacketData(int dataSize, char* pData)
{
	// 들어온 패킷의 사이즈 + 기존 버퍼에 남아있는 사이즈 > 패킷 전체의 사이즈
	if (mPacketDataBufferWritePos + dataSize > packet_data_buffer_size)
	{
		// 쓴 데이터 - 읽은 데이터
		int remaining = mPacketDataBufferWritePos - mPacketDataBufferReadPos;

		// 0보다 크다면 읽지 않은 데이터가 있는 뜻이니까 읽지 않은 데이터를 앞으로 옮긴 후 채운다.
		if (remaining > 0)
		{
			// 버퍼에 리드한 데이터부터 처리되지 않은 데이터를 앞으로 옮긴다.
			memmove(mPacketDataBuffer, &mPacketDataBuffer[mPacketDataBufferReadPos], remaining);
			mPacketDataBufferWritePos = remaining;
		}
		else if (remaining == 0)	// 버퍼가 비웠거나, 다 읽었으므로 버퍼를 처음부터 시작한다.
		{
			mPacketDataBufferWritePos = 0;
		}

		mPacketDataBufferReadPos = 0;
	}


	// 버퍼에 들어온 데이터를 복사한다.
	CopyMemory(&mPacketDataBuffer[mPacketDataBufferWritePos], pData, dataSize);
	// 들어온 데이터만큼 Write포스를 올린다.
	mPacketDataBufferWritePos += dataSize;
}

void User::ReadPacketData(int readSize)
{
	// 읽을 데이터 크기가 현재 저장된 데이터 크기를 초과하면 남은 데이터만 읽음
	if (mPacketDataBufferReadPos + readSize > mPacketDataBufferWritePos)
	{
		readSize = mPacketDataBufferWritePos - mPacketDataBufferReadPos;
	}

	// 데이터 읽기
	char* pReadData = &mPacketDataBuffer[mPacketDataBufferReadPos];

	// 여기서 패킷 처리?

	// 읽은 만큼 ReadPos 증가
	mPacketDataBufferReadPos += readSize;

	// 만약 읽은 데이터가 WritePos와 같아지면 버퍼를 비운다.
	if (mPacketDataBufferReadPos == mPacketDataBufferWritePos)
	{
		mPacketDataBufferWritePos = 0;
		mPacketDataBufferReadPos = 0;
	}
}