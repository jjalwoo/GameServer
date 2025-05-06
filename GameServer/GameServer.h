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
		// WinSock 초기화
		WSADATA WSAdATA;
		WSAStartup(MAKEWORD(2, 2), &WSAdATA);

		// 비동기 소켓 생성
		WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

		// 최대 작업 쓰레드 개수 설정
		MaxIOWorkerThreadCount = maxIOWorkerThreadCount;
	};

	bool StartServer(const int maxClientCount)
	{
		CreateClient(maxClientCount);

		// IOWorker 쓰레드 생성 실패 시 종료
		if (!CreateWokerThread())
		{
			return false;
		}

		// 접속 관리 쓰레드 생성 실패 시 종료
		if (!CreateAccepterThread())
		{
			return false;
		}	

		std::cout << std::format("서버 시작") << std::endl;
		return true;
	}

	virtual void OnConnect(const int index) {}
	virtual void OnClose(const int index) {}

	virtual void OnRecv(const int index, const char* data, const int size) {}

	bool SendMsg(int clientIndex, int datasize, char* data)
	{
		auto targetClient = GetClientInfo(clientIndex);		

		// 비동기 송신 요청
		return targetClient->SendMessage(datasize, data);		
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
	
	bool CreateAccepterThread()
	{

	}

	void WorkerThread()
	{		
		stClientInfo* pClientInfos = nullptr;		
		bool bSuccess = false;	
		DWORD dwIoSize = 0;	// IO에서 전달된 데이터 크기
		LPOVERLAPPED lpOverlapped = nullptr;		

		while (mIsWorkerRun)
		{
			// INFINITE >> wait time(대기 시간)이 무한대 
			bSuccess = GetQueuedCompletionStatus(mIOCPHandle, &dwIoSize, (PULONG_PTR)&pClientInfos, &lpOverlapped, INFINITE);
			auto pOverlappedEx = reinterpret_cast<stOverlappedEx*>(lpOverlapped);

			if (bSuccess == TRUE && dwIoSize == 0)
			{
				if (pOverlappedEx->m_Operation == IOOperation::ACCEPT)
				{
					// 클라이언트가 정상적으로 접속했으므로 종료하지 않음
					pClientInfos = GetClientInfo(pOverlappedEx->SessionIndex);
					if (pClientInfos->AcceptCompletion())
					{
						++mClientCnt;
						OnConnect(pClientInfos->GetIndex()); // 
					}
				}
				else
				{
					// 클라이언트 연결 종료
					pClientInfos->Close();
					mClientCnt--;
					mIsWorkerRun = false;
					continue;
				}				
			}			

			if (IOOperation::RECV == pOverlappedEx->m_Operation)
			{
				// recv data
				// pClientInfos->RecvData(pOverlappedEx, dwIoSize);
			}
			else if (IOOperation::SEND == pOverlappedEx->m_Operation)
			{
				// 데이터 송신 완료 처리
				pClientInfos->SendCompleted(dwIoSize);
			}			
		}
	}

	void CreateClient(const int maxClientCount)
	{
		// 서버 시작 시 최대 클라이언트 객수 만큼 객체 생성 후 관리
		for (int i = 0; i < maxClientCount; ++i)
		{
			auto client = new stClientInfo;
			// 클라이언트 인덱스 및 IOCP 핸들 설정
			client->Init(i, mIOCPHandle);

			// 벡터에 추가
			mClientInfos.push_back(client);
		}
	}

	stClientInfo* GetClientInfo(const int index)
	{	
		// 인덱스에 맞는 클라리언트 정보 반환
		return mClientInfos[index];
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

