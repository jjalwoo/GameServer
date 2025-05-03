#pragma once

/*
*  socket 초기화 , 및 관련 함수 정의
*/

#include "ClientInfo.h"
#include "Define.h"
#include <thread>
#include <vector>

class GameServer
{
public:
	GameServer() {}

	~GameServer()
	{
		// ???
	}
public:
	void Init(int maxIOWorkerThreadCount)
	{
		//SOCKET 초기화 
		WSADATA WSAdATA;
		WSAStartup(MAKEWORD(2, 2), &WSAdATA);

		WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);


		MaxIOWorkerThreadCount = maxIOWorkerThreadCount;
	};

	bool StartServer(const UINT32 maxClientCount_)
	{
		CreateClient(maxClientCount_);

		//접속된 클라이언트 주소 정보를 저장할 구조체
		bool bRet = CreateWokerThread();
		if (false == bRet) {
			return false;
		}

		// bRet = CreateAccepterThread();
		if (false == bRet) {
			return false;
		}

		printf("서버 시작\n");
		return true;
	}

	virtual void OnConnect(const int index) {}
	virtual void OnClose(const int index) {}

	virtual void OnRecv(const int index, const char* data, const int size) {}

	bool SendMsg(int clientIndex, int datasize, char* data)
	{
		auto targetClient = GetClientInfo(clientIndex);
		// return targetClient->sendMsg(datasize, data);

		bool result = targetClient->SendMessage(datasize, data);
		return result;
		// return  targetClient->sendMsg(datasize, data);
	}	

private:
	bool CreateWokerThread()
	{
		MaxIOWorkerThreadCount = 10;

		for (int i = 0; i < MaxIOWorkerThreadCount; i++)
		{
			mIOWorkerThreads.emplace_back([this]() {WorkerThread();});
		}
	}
	// 
	void WorkerThread()
	{
		// ! 
		stClientInfo* pClientInfos = nullptr;

		bool bSuccess = false;

		DWORD dwIoSize = 0;

		// ?
		LPOVERLAPPED lpOverlapped = nullptr;


		while (mIsWorkerRun)
		{
			// INFINITE   wait time(대기 시간)이 무한대 
			bSuccess = GetQueuedCompletionStatus(mIOCPHandle, &dwIoSize, (PULONG_PTR)&pClientInfos, &lpOverlapped, INFINITE);

			if (TRUE == bSuccess && 0 == dwIoSize)
			{
				//클라이언트 접속 종료
				//클라이언트 정보 초기화
				pClientInfos->Close();
				mClientCnt--;
				mIsWorkerRun = false;
				continue;
			}			 

			auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;

			/*if (IOOperation::ACCEPT != pOverlappedEx->m_Operation && dwIoSize == 0)
			{
				pClientInfos->Close();
			}*/

			if (IOOperation::RECV == pOverlappedEx->m_Operation)
			{
				// recv data
				// pClientInfos->RecvData(pOverlappedEx, dwIoSize);
			}
			else if (IOOperation::SEND == pOverlappedEx->m_Operation)
			{
				// send data
				// pClientInfos->SendData(pOverlappedEx, dwIoSize);
				pClientInfos->SendCompleted(dwIoSize);
			}
			else if (IOOperation::ACCEPT == pOverlappedEx->m_Operation)
			{
				// accept
				pClientInfos = GetClientInfo(pOverlappedEx->SessionIndex);
				if (pClientInfos->AcceptCompletion())
				{
					++mClientCnt;

					OnConnect(pClientInfos->GetIndex());
				}

			}

		}

	}

	void CreateClient(const UINT32 maxClientCount_)
	{
		for (UINT32 i = 0; i < maxClientCount_; ++i)
		{
			auto client = new stClientInfo;
			client->Init(i, mIOCPHandle);

			mClientInfos.push_back(client);
		}
	}

	stClientInfo* GetClientInfo(const int cIndex)
	{
		/*for (int i = 0; i < mClientInfos.size(); i++)
		{
			if (mClientInfos[i]->GetIndex() == cIndex)
			{
				return mClientInfos[i];
			}
		}*/

		return mClientInfos[cIndex];
	}

private:
	// 워커 쓰레드 개수
	UINT32 MaxIOWorkerThreadCount = 0;

	//클라이언트 정보 저장 구조체
	std::vector<stClientInfo*> mClientInfos;

	//클라이언트의 접속을 받기위한 리슨 소켓
	SOCKET      mListenSocket = INVALID_SOCKET;

	//접속 되어있는 클라이언트 수
	int         mClientCnt = 0;

	//IO Worker 스레드
	std::vector<std::thread> mIOWorkerThreads;


	//Accept 스레드
	std::thread   mAccepterThread;

	//CompletionPort객체 핸들
	HANDLE      mIOCPHandle = INVALID_HANDLE_VALUE;

	//작업 쓰레드 동작 플래그
	bool      mIsWorkerRun = true;

	//접속 쓰레드 동작 플래그
	bool      mIsAccepterRun = true;
};

