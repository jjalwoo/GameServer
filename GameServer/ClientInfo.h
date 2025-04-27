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

		// IO COMPORT PORT 객체와 소켓을 연결
		if (BindIOCompletionPort(iocpHandle) == false)
		{
			return false;
		}

		/// DDD 

		return  BindRecv();
	}


	bool BindRecv()
	{
		//wsa recv 처리. 

		DWORD dwFlag = 0;
		DWORD dwRecvNumBytes = 0;

		//overlapped io 
		mRecvOverlappedEX.m_wsaBuf.len = MAX_SOCK_RECVBUF;
		mRecvOverlappedEX.m_wsaBuf.buf = mRecvBuf;
		mRecvOverlappedEX.m_Operation = IOOperation::RECV;

		int nRet = WSARecv(mSocket, &mRecvOverlappedEX.m_wsaBuf, 1, &dwRecvNumBytes, &dwFlag, (LPWSAOVERLAPPED) & (mRecvOverlappedEX), NULL);

		//nRet이 socket error인지 체크 

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
		// send data 처리 완료
		// 로그

		//sendqueue ? -> sendqueue  pop delete  

		// 딜리트와 팝을 하는데 다른 쓰레드에서 없어진 메모리에 접근하면 망하니까 락을 걸어야함
		std::lock_guard<std::mutex> lock(mSendLock);
		delete mSendDataQueue.front();
		mSendDataQueue.pop();

		// 1 - send

		if (mSendDataQueue.empty() == false)
		{
			//send queue에 데이터가 있으면 send를 한다.
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

		// send queue가 비어있다면 push, 비어있지 않다면 SendIO()
		// 2025 04 27 여기까지ㅎㅎg
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
		//send data 처리
		
		// send queue에 있는 데이터를 꺼내온다.
		auto sendOverlappedEX = mSendDataQueue.front();

		// 비동기 send
		int res = WSASend(mSocket, &sendOverlappedEX->m_wsaBuf, 1, NULL, 0, (LPWSAOVERLAPPED)sendOverlappedEX, NULL);

		// 결과가 에러지만 펜딩이 아닐 시 (펜딩은 현재 대기중인 상태라 오류라고 볼 수 없다)
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
	//이걸 쓰면 락을 안걸어줘도 됨

};