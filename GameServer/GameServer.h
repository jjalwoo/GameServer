#pragma once

/*
*  socket 초기화 , 및 관련 함수 정의
*/

#include "ClientInfo.h"
#include "Define.h"
#include <thread>
#include <vector>
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")  // Winsock2 라이브러리 링크 추가

class GameServer
{
public:
	GameServer() {}

	~GameServer()
	{
		// ???
	}
public:

	virtual void OnConnect(const int index) {}
	virtual void OnClose(const int index) {}
	virtual void OnRecv(const int index, const char* data, const int size) {}

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
		// 1. WSASocket()을 호출하여 리슨 소켓 생성
		mListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (mListenSocket == INVALID_SOCKET)
		{
			std::cout << std::format("서버 소켓 생성 실패! (에러 코드: {})", WSAGetLastError()) << std::endl;
			return false;
		}

		// 2. IOCP 핸들 생성
		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (mIOCPHandle == NULL)
		{
			std::cout << "IOCP 핸들 생성 실패!" << std::endl;
			return false;
		}

		// 3. 서버 주소 설정
		sockaddr_in serverAddr{};
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY; // 모든 네트워크 인터페이스에서 접속 가능
		serverAddr.sin_port = htons(8080); // 포트 설정

		// 4. 소켓 바인딩
		if (bind(mListenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			std::cout << std::format("서버 소켓 바인딩 실패! (에러 코드: {})", WSAGetLastError()) << std::endl;
			return false;
		}

		// 5. 클라이언트 접속 대기 설정
		if (listen(mListenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			std::cout << std::format("클라이언트 접속 대기 실패! (에러 코드: {})", WSAGetLastError()) << std::endl;
			return false;
		}

		// 6. 리슨 소켓을 IOCP에 등록
		HANDLE hResult = CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, 0, 0);
		if (hResult == NULL)
		{
			std::cout << std::format("리슨 소켓을 IOCP에 등록 실패! (에러 코드: {})", WSAGetLastError()) << std::endl;
			return false;
		}

		// 7. 클라이언트 객체 생성 (최대 접속 수 설정)
		CreateClient(maxClientCount);

		// 8. IOWorker 쓰레드 생성 실패 시 종료
		if (!CreateWorkerThread())
		{
			return false;
		}

		// 9. 접속 관리 쓰레드 생성 실패 시 종료
		if (!CreateAccepterThread())
		{
			return false;
		}

		std::cout << std::format("서버 시작 - 최대 클라이언트 수: {}", maxClientCount) << std::endl;
		return true;
	}


	bool SendMsg(int clientIndex, int datasize, char* data)
	{
		auto targetClient = GetClientInfo(clientIndex);		
		if (targetClient == nullptr) // 클라이언트 객체가 존재하는지 확인
		{
			std::cout << std::format(" 잘못된 클라이언트 인덱스 [{}]", clientIndex) << std::endl;
			return false;
		}		

		int result = send(targetClient->GetSocket(), data, datasize, 0);
		if (result == SOCKET_ERROR)
		{
			std::cout << std::format("메시지 전송 실패! (에러 코드: {})", WSAGetLastError()) << std::endl;
			return false;
		}

		std::cout << std::format("클라이언트 [{}] 로 메시지 전송 완료 (보낸 바이트 수: {})", clientIndex, result) << std::endl;
		return true;
	}	

private:
	bool CreateWorkerThread()
	{
		MaxIOWorkerThreadCount = 10;

		for (int i = 0; i < MaxIOWorkerThreadCount; i++)
		{
			mIOWorkerThreads.emplace_back([this]() {WorkerThread();});
		}

		return true;
	}
	
	bool CreateAccepterThread()
	{
		mAccepterThread = std::thread([this]()
			{
				while (mIsAccepterRun)
				{
					std::cout << "클라이언트 연결을 기다리는 중..." << std::endl;

					// 클라이언트 요청이 있을 때만 실행
					SOCKET clientSocket = accept(mListenSocket, nullptr, nullptr);
					if (clientSocket == INVALID_SOCKET)
					{
						std::cout << "클라이언트 연결 실패! 대기 중..." << std::endl;
						continue;
					}

					// 이제 실제 요청된 클라이언트만 처리!
					HANDLE hResult = CreateIoCompletionPort((HANDLE)clientSocket, mIOCPHandle, (ULONG_PTR)clientSocket, 0);
					if (hResult == NULL)
					{
						std::cout << std::format("CreateIoCompletionPort 실패! (에러 코드: {})", WSAGetLastError()) << std::endl;
						closesocket(clientSocket);
						continue;
					}

					std::cout << std::format("클라이언트 연결 완료! 소켓 ID: {}", clientSocket) << std::endl;
				}
			});

		return true;
	}


	void WorkerThread()
	{
		stClientInfo* pClientInfos = nullptr;
		DWORD dwIoSize = 0;
		LPOVERLAPPED lpOverlapped = nullptr;

		while (mIsWorkerRun)
		{
			std::cout << "WorkerThread 실행 중..." << std::endl;

			BOOL bSuccess = GetQueuedCompletionStatus(mIOCPHandle, &dwIoSize, (PULONG_PTR)&pClientInfos, &lpOverlapped, INFINITE);
			auto pOverlappedEx = reinterpret_cast<stOverlappedEx*>(lpOverlapped);

			if (!bSuccess)
			{
				std::cout << std::format("IOCP 작업 실패! (에러 코드: {})", GetLastError()) << std::endl;
				continue;
			}

			std::cout << "IOCP에서 이벤트 감지 완료! (데이터 크기: " << dwIoSize << ")" << std::endl;


			// 클라이언트 연결 이벤트 처리
			if (dwIoSize == 0 && pOverlappedEx->m_Operation == IOOperation::ACCEPT)
			{ 
				pClientInfos = GetClientInfo(pOverlappedEx->SessionIndex);
				if (pClientInfos->AcceptCompletion())
				{
					++mClientCnt;
					OnConnect(pClientInfos->GetIndex()); // 클라이언트 연결 완료 이벤트 호출
				}
				else
				{
					std::cout << "클라이언트 접속 실패!" << std::endl;
					pClientInfos->Close();
				}
			}
			else if (dwIoSize == 0) // 클라이언트 연결 종료 처리
			{
				std::cout << "클라이언트 연결 종료!" << std::endl;
				RemoveClient(pClientInfos); // 클라이언트 안전하게 제거
				--mClientCnt;
				continue;
			}

			// 데이터 수신 이벤트 처리
			if (pOverlappedEx->m_Operation == IOOperation::RECV)
			{
				std::cout << "수신된 데이터 처리 중..." << std::endl;
				pClientInfos->Recv(pOverlappedEx, dwIoSize);
				pClientInfos->OnRecv(pClientInfos->GetIndex(), pOverlappedEx->m_wsaBuf.buf, dwIoSize);		
				std::cout << "OnRecv 호출!" << std::endl;

				// 데이터 수신 완료 후 다시 `WSARecv()` 호출하여 지속적인 수신 처리
				pClientInfos->BindRecv();
			}
			// 데이터 송신 완료 이벤트 처리
			else if (pOverlappedEx->m_Operation == IOOperation::SEND)
			{
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

	void RemoveClient(stClientInfo* client)
	{
		std::cout << std::format("클라이언트 [{}] 연결 종료", client->GetIndex()) << std::endl;
		client->Close();

		// 벡터에서 해당 클라이언트 삭제 (메모리 정리)
		auto it = std::find(mClientInfos.begin(), mClientInfos.end(), client);
		if (it != mClientInfos.end())
		{
			delete* it;
			mClientInfos.erase(it);
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

