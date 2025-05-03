#pragma once

/*
*  socket �ʱ�ȭ , �� ���� �Լ� ����
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
		//SOCKET �ʱ�ȭ 
		WSADATA WSAdATA;
		WSAStartup(MAKEWORD(2, 2), &WSAdATA);

		WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);


		MaxIOWorkerThreadCount = maxIOWorkerThreadCount;
	};

	bool StartServer(const UINT32 maxClientCount_)
	{
		CreateClient(maxClientCount_);

		//���ӵ� Ŭ���̾�Ʈ �ּ� ������ ������ ����ü
		bool bRet = CreateWokerThread();
		if (false == bRet) {
			return false;
		}

		// bRet = CreateAccepterThread();
		if (false == bRet) {
			return false;
		}

		printf("���� ����\n");
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
			// INFINITE   wait time(��� �ð�)�� ���Ѵ� 
			bSuccess = GetQueuedCompletionStatus(mIOCPHandle, &dwIoSize, (PULONG_PTR)&pClientInfos, &lpOverlapped, INFINITE);

			if (TRUE == bSuccess && 0 == dwIoSize)
			{
				//Ŭ���̾�Ʈ ���� ����
				//Ŭ���̾�Ʈ ���� �ʱ�ȭ
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
	// ��Ŀ ������ ����
	UINT32 MaxIOWorkerThreadCount = 0;

	//Ŭ���̾�Ʈ ���� ���� ����ü
	std::vector<stClientInfo*> mClientInfos;

	//Ŭ���̾�Ʈ�� ������ �ޱ����� ���� ����
	SOCKET      mListenSocket = INVALID_SOCKET;

	//���� �Ǿ��ִ� Ŭ���̾�Ʈ ��
	int         mClientCnt = 0;

	//IO Worker ������
	std::vector<std::thread> mIOWorkerThreads;


	//Accept ������
	std::thread   mAccepterThread;

	//CompletionPort��ü �ڵ�
	HANDLE      mIOCPHandle = INVALID_HANDLE_VALUE;

	//�۾� ������ ���� �÷���
	bool      mIsWorkerRun = true;

	//���� ������ ���� �÷���
	bool      mIsAccepterRun = true;
};

