#include "User.h"

void User::Init()
{
	mPacketDataBuffer = new char[packet_data_buffer_size];
}

void User::SetPacketData(int dataSize, char* pData)
{
	// ���� ��Ŷ�� ������ + ���� ���ۿ� �����ִ� ������ > ��Ŷ ��ü�� ������
	if (mPacketDataBufferWritePos + dataSize > packet_data_buffer_size)
	{
		// �� ������ - ���� ������
		int remaining = mPacketDataBufferWritePos - mPacketDataBufferReadPos;

		// 0���� ũ�ٸ� ���� ���� �����Ͱ� �ִ� ���̴ϱ� ���� ���� �����͸� ������ �ű� �� ä���.
		if (remaining > 0)
		{
			// ���ۿ� ������ �����ͺ��� ó������ ���� �����͸� ������ �ű��.
			memmove(mPacketDataBuffer, &mPacketDataBuffer[mPacketDataBufferReadPos], remaining);
			mPacketDataBufferWritePos = remaining;
		}
		else if (remaining == 0)	// ���۰� ����ų�, �� �о����Ƿ� ���۸� ó������ �����Ѵ�.
		{
			mPacketDataBufferWritePos = 0;
		}

		mPacketDataBufferReadPos = 0;
	}


	// ���ۿ� ���� �����͸� �����Ѵ�.
	CopyMemory(&mPacketDataBuffer[mPacketDataBufferWritePos], pData, dataSize);
	// ���� �����͸�ŭ Write������ �ø���.
	mPacketDataBufferWritePos += dataSize;
}

void User::ReadPacketData(int readSize)
{
	// ���� ������ ũ�Ⱑ ���� ����� ������ ũ�⸦ �ʰ��ϸ� ���� �����͸� ����
	if (mPacketDataBufferReadPos + readSize > mPacketDataBufferWritePos)
	{
		readSize = mPacketDataBufferWritePos - mPacketDataBufferReadPos;
	}

	// ������ �б�
	char* pReadData = &mPacketDataBuffer[mPacketDataBufferReadPos];

	// ���⼭ ��Ŷ ó��?

	// ���� ��ŭ ReadPos ����
	mPacketDataBufferReadPos += readSize;

	// ���� ���� �����Ͱ� WritePos�� �������� ���۸� ����.
	if (mPacketDataBufferReadPos == mPacketDataBufferWritePos)
	{
		mPacketDataBufferWritePos = 0;
		mPacketDataBufferReadPos = 0;
	}
}