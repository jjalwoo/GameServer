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

		std::cout << "WSARecv 호출 시작!" << std::endl;


		int nRet = WSARecv(mSocket, &mRecvOverlappedEX.m_wsaBuf, 1, &dwRecvNumBytes, &dwFlag, (LPWSAOVERLAPPED) & (mRecvOverlappedEX), NULL);

		//nRet이 socket error인지 체크 

		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			std::cout << std::format("WSARecv Error: {}, WSAGETLastError()") << std::endl;
			return false;
		}
				
		std::cout << "WSARecv 호출 성공! 데이터 대기 중..." << std::endl;
		return true;

	}

	void OnRecv(const int index, const char* data, const int size)
	{
		// 받은 데이터 체크(nullptr이거나 비어 있다면 반환하지 않도록)
		if (data == nullptr || size <= 0)
		{
			std::cout << std::format("클라이언트 [{}] 로부터 잘못된 데이터 수신", index) << std::endl;
			return;
		}

		// 받은 데이터 처리
		std::cout << std::format("클라이언트 [{}] 로부터 받은 데이터: {}", index, data) << std::endl;

		// 받은 데이터를 그대로 클라이언트에게 다시 전송
		SendMessage(size, (char*)data);
	}

	void Recv(stOverlappedEx* overlappedEx, DWORD dwIoSize)
	{
		if (dwIoSize <= 0)
		{
			std::cout << std::format("클라이언트 [{}] 데이터 수신 실패 또는 연결 종료", GetIndex()) << std::endl;
			Close();
			return;
		}

		// 받은 데이터 확인
		std::cout << std::format("클라이언트 [{}] 수신 데이터: {}", GetIndex(), std::string(overlappedEx->m_wsaBuf.buf, dwIoSize)) << std::endl;
		
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
		// send data 처리 완료
		std::cout << std::format("Send Completed: {}", size) << std::endl;

		// 딜리트와 팝을 하는데 다른 쓰레드에서 없어진 메모리에 접근하면 망하니까 락을 걸어야함
		std::lock_guard<std::mutex> lock(mSendLock);
		delete mSendDataQueue.front();
		mSendDataQueue.front() = nullptr; // 메모리 해제 후 nullptr로 초기화	
		mSendDataQueue.pop();
		
		//send queue에 남은 데이터가 있으면 SenIO()를 호출하여 계속 처리
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

		// 데이터 큐에 추가한 후, 큐가 비어있다면 즉시 전송
		mSendDataQueue.push(pOverlappedEx);
		if (mSendDataQueue.size() == 1)
		{
			SendIO();		
		}

		return true;			
	}

	void Close()
	{
		std::cout << std::format("클라이언트 [{}] 연결 종료", mIndex) << std::endl;

		// 소켓이 유효한지 체크 후 유효하면 소켓 종료
		if (mSocket != INVALID_SOCKET)
		{
			closesocket(mSocket);
			mSocket = INVALID_SOCKET;
		}

		// SendDataQueue 정리
		std::lock_guard<std::mutex> lock(mSendLock);
		while (!mSendDataQueue.empty())
		{
			delete mSendDataQueue.front();
			mSendDataQueue.front() = nullptr; // 메모리 해제 후 nullptr로 초기화
			mSendDataQueue.pop();
		}

		mIsConnect = false;		
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
	//이걸 쓰면 락을 안걸어줘도 됨

};