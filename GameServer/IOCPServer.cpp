#include "IOCPServer.h"

IOCPServer::IOCPServer()
{
}

IOCPServer::~IOCPServer()
{
    WSACleanup();
}

bool IOCPServer::Init(const UINT32 maxIOWorkerThreadCount_)
{
    WSADATA wsaData;
    int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (0 != nRet)
    {
        printf("[에러] WSAStartup()함수 실패 : %d\n", WSAGetLastError());
        return false;
    }

    mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == mListenSocket)
    {
        printf("[에러] socket()함수 실패 : %d\n", WSAGetLastError());
        return false;
    }

    MaxIOWorkerThreadCount = maxIOWorkerThreadCount_;

    printf("소켓 초기화 성공\n");
    return true;
}

bool IOCPServer::InitLogger()
{
    return true;
}

bool IOCPServer::BindandListen(int bindPort_)
{
    SOCKADDR_IN stServerAddr;
    stServerAddr.sin_family = AF_INET;
    stServerAddr.sin_port = htons(bindPort_);
    stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
    if (0 != nRet)
    {
        printf("[에러] bind()함수 실패 : %d\n", WSAGetLastError());
        return false;
    }

    nRet = listen(mListenSocket, 5);
    if (0 != nRet)
    {
        printf("[에러] listen()함수 실패 : %d\n", WSAGetLastError());
        return false;
    }

    mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MaxIOWorkerThreadCount);
    if (NULL == mIOCPHandle)
    {
        printf("[에러] CreateIoCompletionPort()함수 실패: %d\n", GetLastError());
        return false;
    }

    auto hIOCPHandle = CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, (UINT32)0, 0);
    if (nullptr == hIOCPHandle)
    {
        printf("[에러] listen socket IOCP bind 실패 : %d\n", WSAGetLastError());
        return false;
    }

    printf("서버 등록 성공..\n");
    return true;
}

bool IOCPServer::StartServer(const UINT32 maxClientCount_)
{
    CreateClient(maxClientCount_);

    bool bRet = CreateWokerThread();
    if (false == bRet)
    {
        return false;
    }

    bRet = CreateAccepterThread();
    if (false == bRet)
    {
        return false;
    }

    printf("서버 시작\n");
    return true;
}

void IOCPServer::DestroyThread()
{
    mIsWorkerRun = false;
    CloseHandle(mIOCPHandle);

    for (auto& th : mIOWorkerThreads)
    {
        if (th.joinable())
        {
            th.join();
        }
    }

    mIsAccepterRun = false;
    closesocket(mListenSocket);

    if (mAccepterThread.joinable())
    {
        mAccepterThread.join();
    }
}

bool IOCPServer::SendMsg(const UINT32 clientIndex_, const UINT32 dataSize_, char* pData)
{
    auto pClient = GetClientInfo(clientIndex_);
    return pClient->SendMsg(dataSize_, pData);
}

void IOCPServer::CreateClient(const UINT32 maxClientCount_)
{
    for (UINT32 i = 0; i < maxClientCount_; ++i)
    {
        auto client = new ClientInfo;
        client->Init(i, mIOCPHandle);

        mClientInfos.push_back(client);
    }
}

bool IOCPServer::CreateWokerThread()
{
    auto threadCount = (MaxIOWorkerThreadCount * 2) + 1;

    for (UINT32 i = 0; i < threadCount; i++)
    {
        mIOWorkerThreads.emplace_back([this]() { WokerThread(); });
    }

    printf("WokerThread 시작..\n");
    return true;
}

ClientInfo* IOCPServer::GetEmptyClientInfo()
{
    for (auto& client : mClientInfos)
    {
        if (client->IsConnectd() == false)
        {
            return client;
        }
    }

    return nullptr;
}

ClientInfo* IOCPServer::GetClientInfo(const UINT32 clientIndex_) const
{
    return mClientInfos[clientIndex_];
}

void IOCPServer::CloseSocket(ClientInfo* clientInfo_, bool isForce_)
{
    if (clientInfo_->IsConnectd() == false)
    {
        return;
    }

    auto clientIndex = clientInfo_->GetIndex();

    clientInfo_->Close(isForce_);

    OnClose(clientIndex);
}

bool IOCPServer::CreateAccepterThread()
{
    mAccepterThread = std::thread([this]() { AccepterThread(); });

    printf("AccepterThread 시작..\n");
    return true;
}

void IOCPServer::WokerThread()
{
    ClientInfo* pClientInfo = nullptr;
    BOOL bSuccess = TRUE;
    DWORD dwIoSize = 0;
    LPOVERLAPPED lpOverlapped = NULL;

    while (mIsWorkerRun)
    {
        bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
            &dwIoSize,
            (PULONG_PTR)&pClientInfo,
            &lpOverlapped,
            INFINITE);

        if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
        {
            mIsWorkerRun = false;
            continue;
        }

        auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;

        if (FALSE == bSuccess || (IOOperation::ACCEPT != pOverlappedEx->m_eOperation && dwIoSize == 0))
        {
            CloseSocket(pClientInfo);
            continue;
        }

        if (IOOperation::ACCEPT == pOverlappedEx->m_eOperation)
        {
            pClientInfo = GetClientInfo(pOverlappedEx->SessionIndex);
            if (pClientInfo->AcceptCompletion())
            {
                ++mClientCnt;
                OnConnect(pClientInfo->GetIndex());
            }
            else
            {
                CloseSocket(pClientInfo, true);
            }
        }
        else if (IOOperation::RECV == pOverlappedEx->m_eOperation)
        {
            OnReceive(pClientInfo->GetIndex(), dwIoSize, pClientInfo->RecvBuffer());
            pClientInfo->BindRecv();
        }
        else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
        {
            pClientInfo->SendCompleted(dwIoSize);
        }
        else
        {
            printf("Client Index(%d)에서 예외상황\n", pClientInfo->GetIndex());
        }
    }
}

void IOCPServer::AccepterThread()
{
    while (mIsAccepterRun)
    {
        auto curTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

        for (auto client : mClientInfos)
        {
            if (client->IsConnectd())
            {
                continue;
            }

            if ((UINT64)curTimeSec < client->GetLatestClosedTimeSec())
            {
                continue;
            }

            auto diff = curTimeSec - client->GetLatestClosedTimeSec();
            if (diff <= RE_USE_SESSION_WAIT_TIMESEC)
            {
                continue;
            }

            client->PostAccept(mListenSocket, curTimeSec);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(32));
    }
}
