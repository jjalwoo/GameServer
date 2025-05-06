#pragma once
#include "Define.h"
#include <iostream>
#include <mutex> 
#include <queue>
#include <concurrent_queue.h> // ms
#include <windows.h>
#include <format>


class stClientInfo
{
public:
	stClientInfo()
	{
		ZeroMemory(&mRecvOverlappedEX, sizeof(mRecvOverlappedEX));
	}

	void Init(const int index, HANDLE iocpHandle)
	{
		mIndex = index;
		mIOCPHandle = iocpHandle;
	}

	int GetIndex() { return mIndex; }

	SOCKET GetSocket() { return mSocket; }

	bool AcceptCompletion()
	{

		if (OnConnect(mIOCPHandle, mSocket) == false)
		{
			return false;
		}
		return true;
	}

	bool OnConnect(HANDLE iocpHandle, SOCKET socket)
	{
		mSocket = socket;
		mIsConnect = 1;

		// IO COMPORT PORT ��ü�� ������ ����
		if (BindIOCompletionPort(iocpHandle) == false)
		{
			return false;
		}

		/// DDD 

		return  BindRecv();
	}


	bool BindRecv()
	{
		//wsa recv ó��. 
		DWORD dwFlag = 0;
		DWORD dwRecvNumBytes = 0;

		//overlapped io 
		mRecvOverlappedEX.m_wsaBuf.len = MAX_SOCK_RECVBUF;
		mRecvOverlappedEX.m_wsaBuf.buf = mRecvBuf;
		mRecvOverlappedEX.m_Operation = IOOperation::RECV;

		std::cout << "WSARecv ȣ�� ����!" << std::endl;


		int nRet = WSARecv(mSocket, &mRecvOverlappedEX.m_wsaBuf, 1, &dwRecvNumBytes, &dwFlag, (LPWSAOVERLAPPED) & (mRecvOverlappedEX), NULL);

		//nRet�� socket error���� üũ 

		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			std::cout << std::format("WSARecv Error: {}, WSAGETLastError()") << std::endl;
			return false;
		}
				
		std::cout << "WSARecv ȣ�� ����! ������ ��� ��..." << std::endl;
		return true;

	}

	void OnRecv(const int index, const char* data, const int size)
	{
		// ���� ������ üũ(nullptr�̰ų� ��� �ִٸ� ��ȯ���� �ʵ���)
		if (data == nullptr || size <= 0)
		{
			std::cout << std::format("Ŭ���̾�Ʈ [{}] �κ��� �߸��� ������ ����", index) << std::endl;
			return;
		}

		// ���� ������ ó��
		std::cout << std::format("Ŭ���̾�Ʈ [{}] �κ��� ���� ������: {}", index, data) << std::endl;

		// ���� �����͸� �״�� Ŭ���̾�Ʈ���� �ٽ� ����
		SendMessage(size, (char*)data);
	}

	void Recv(stOverlappedEx* overlappedEx, DWORD dwIoSize)
	{
		if (dwIoSize <= 0)
		{
			std::cout << std::format("Ŭ���̾�Ʈ [{}] ������ ���� ���� �Ǵ� ���� ����", GetIndex()) << std::endl;
			Close();
			return;
		}

		// ���� ������ Ȯ��
		std::cout << std::format("Ŭ���̾�Ʈ [{}] ���� ������: {}", GetIndex(), std::string(overlappedEx->m_wsaBuf.buf, dwIoSize)) << std::endl;
		
	}


	bool BindIOCompletionPort(HANDLE iocpHandle)
	{

		/*auto hIOCP = CreateIoCompletionPort((HANDLE)GetSock()
			, iocpHandle_
			, (ULONG_PTR)(this), 0);*/

		HANDLE h = CreateIoCompletionPort((HANDLE)mSocket, iocpHandle, (ULONG_PTR)this, 0);
		if (h == NULL)
		{
			std::cout << std::format("CreateIoCompletionPort Error: [{}]", WSAGetLastError()) << std::endl;
			return false;
		}
		return true;
	}

	void SendCompleted(int size)
	{
		// send data ó�� �Ϸ�
		std::cout << std::format("Send Completed: {}", size) << std::endl;

		// ����Ʈ�� ���� �ϴµ� �ٸ� �����忡�� ������ �޸𸮿� �����ϸ� ���ϴϱ� ���� �ɾ����
		std::lock_guard<std::mutex> lock(mSendLock);
		delete mSendDataQueue.front();
		mSendDataQueue.front() = nullptr; // �޸� ���� �� nullptr�� �ʱ�ȭ	
		mSendDataQueue.pop();
		
		//send queue�� ���� �����Ͱ� ������ SenIO()�� ȣ���Ͽ� ��� ó��
		if (mSendDataQueue.empty() == false)
		{			
			SendIO();
		}
	}	

	bool SendMessage(int dataSize, char* data)
	{
		stOverlappedEx* pOverlappedEx = new stOverlappedEx;
		pOverlappedEx->m_Operation = IOOperation::SEND;

		pOverlappedEx->m_wsaBuf.len = dataSize;		
		pOverlappedEx->m_wsaBuf.buf = new char[dataSize];
		memcpy(pOverlappedEx->m_wsaBuf.buf, data, dataSize);		

		// ������ ť�� �߰��� ��, ť�� ����ִٸ� ��� ����
		mSendDataQueue.push(pOverlappedEx);
		if (mSendDataQueue.size() == 1)
		{
			SendIO();		
		}

		return true;			
	}

	void Close()
	{
		std::cout << std::format("Ŭ���̾�Ʈ [{}] ���� ����", mIndex) << std::endl;

		// ������ ��ȿ���� üũ �� ��ȿ�ϸ� ���� ����
		if (mSocket != INVALID_SOCKET)
		{
			closesocket(mSocket);
			mSocket = INVALID_SOCKET;
		}

		// SendDataQueue ����
		std::lock_guard<std::mutex> lock(mSendLock);
		while (!mSendDataQueue.empty())
		{
			delete mSendDataQueue.front();
			mSendDataQueue.front() = nullptr; // �޸� ���� �� nullptr�� �ʱ�ȭ
			mSendDataQueue.pop();
		}

		mIsConnect = false;		
	}

private:
	bool SendIO()
	{
		//send data ó��
		
		// send queue�� �ִ� �����͸� �����´�.
		auto sendOverlappedEX = mSendDataQueue.front();

		// �񵿱� send
		int res = WSASend(mSocket, &sendOverlappedEX->m_wsaBuf, 1, NULL, 0, (LPWSAOVERLAPPED)sendOverlappedEX, NULL);

		// ����� �������� ����� �ƴ� �� (����� ���� ������� ���¶� ������� �� �� ����)
		if (res == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			std::cout << std::format("WSARecv Error: {}", WSAGetLastError()) << std::endl;
			return false;
		}
		return true;
	}
	
private:
	int mIndex = 0;
	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;

	bool mIsConnect = false;
	int mLastestClosedTimeSec = 0;

	SOCKET		mSocket = INVALID_SOCKET;
	
	stOverlappedEx mAcceptContext;
	char mAcceptBuf[64];

	stOverlappedEx mRecvOverlappedEX;
	char				mRecvBuf[MAX_SOCK_RECVBUF];

	std::mutex			mSendLock;
	std::queue<stOverlappedEx*> mSendDataQueue;

	//Concurrency::concurrent_queue<stOverlappededEx> mSendDataQueueConcurrent; /
	//�̰� ���� ���� �Ȱɾ��൵ ��

};