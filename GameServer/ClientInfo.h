#pragma once
#include "Define.h"
#include <iostream>
#include <mutex> 
#include <queue>
#include <concurrent_queue.h> // ms
#include <windows.h>


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

		int nRet = WSARecv(mSocket, &mRecvOverlappedEX.m_wsaBuf, 1, &dwRecvNumBytes, &dwFlag, (LPWSAOVERLAPPED) & (mRecvOverlappedEX), NULL);

		//nRet�� socket error���� üũ 

		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			std::cout << "WSARecv Error : " << WSAGetLastError() << std::endl;
			return false;
		}

		return true;
	}

	bool BindIOCompletionPort(HANDLE iocpHandle)
	{

		/*auto hIOCP = CreateIoCompletionPort((HANDLE)GetSock()
			, iocpHandle_
			, (ULONG_PTR)(this), 0);*/

		HANDLE h = CreateIoCompletionPort((HANDLE)mSocket, iocpHandle, (ULONG_PTR)this, 0);
		if (h == NULL)
		{
			std::cout << "CreateIoCompletionPort Error : " << WSAGetLastError() << std::endl;
			return false;
		}
		return true;
	}

	void SendCompleted(int size)
	{
		// send data ó�� �Ϸ�
		// �α�

		//sendqueue ? -> sendqueue  pop delete  

		// ����Ʈ�� ���� �ϴµ� �ٸ� �����忡�� ������ �޸𸮿� �����ϸ� ���ϴϱ� ���� �ɾ����
		std::lock_guard<std::mutex> lock(mSendLock);
		delete mSendDataQueue.front();
		mSendDataQueue.pop();

		// 1 - send

		if (mSendDataQueue.empty() == false)
		{
			//send queue�� �����Ͱ� ������ send�� �Ѵ�.
			SendIO();
		}
	}	

	bool SendMessage(int dataSize, char* data)
	{
		stOverlappedEx* pOverlappedEx = new stOverlappedEx;
		pOverlappedEx->m_Operation = IOOperation::SEND;

		pOverlappedEx->m_wsaBuf.len = dataSize;
		/*pOverlappedEx->m_wsaBuf.buf = data;*/		
		//pOverlappedEx->m_wsaBuf.buf = new char[dataSize];

		memcpy(pOverlappedEx->m_wsaBuf.buf, data, dataSize);		

		// send queue�� ����ִٸ� push, ������� �ʴٸ� SendIO()
		// 2025 04 27 �����������g
		if (mSendDataQueue.empty() == true)
		{
			mSendDataQueue.push(pOverlappedEx);
		}			
		else
		{
			SendIO();
		}			
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
			std::cout << "WSASend Error : " << WSAGetLastError() << std::endl;
			return false;
		}
		return true;
	}
	
private:
	int mIndex = 0;
	HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;

	int mIsConnect = 0;
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