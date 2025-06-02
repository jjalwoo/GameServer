#include "GameServer.h"

IOCPServer::IOCPServer()
{
}

bool IOCPServer::Init(const UINT32 maxIOWorkerThreadCount_)
{
    WSADATA wsaData;

    int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (0 != nRet)
    {
        printf("[����] WSAStartup()�Լ� ���� : %d\n", WSAGetLastError());
        return false;
    }

    //���������� TCP , Overlapped I/O ������ ����
    mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

    if (INVALID_SOCKET == mListenSocket)
    {
        printf("[����] socket()�Լ� ���� : %d\n", WSAGetLastError());
        return false;
    }

    MaxIOWorkerThreadCount = maxIOWorkerThreadCount_;


    printf("���� �ʱ�ȭ ����\n");
    return true;
}

bool IOCPServer::InitLogger()
{
    return true;
}

bool IOCPServer::BindandListen(int bindPort_)
{
    SOCKADDR_IN      stServerAddr;
    stServerAddr.sin_family = AF_INET;
    stServerAddr.sin_port = htons(bindPort_);
    stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));


    if (0 != nRet)
    {
        printf("[����] bind()�Լ� ���� : %d\n", WSAGetLastError());
        return false;
    }

    //���� ��û�� �޾Ƶ��̱� ���� cIOCompletionPort������ ����ϰ� 
    //���Ӵ��ť�� 5���� ���� �Ѵ�.
    nRet = listen(mListenSocket, 5);
    if (0 != nRet)
    {
        printf("[����] listen()�Լ� ���� : %d\n", WSAGetLastError());
        return false;
    }

    //CompletionPort��ü ���� ��û�� �Ѵ�.
    mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MaxIOWorkerThreadCount);
    if (NULL == mIOCPHandle)
    {
        printf("[����] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
        return false;
    }

    auto hIOCPHandle = CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, (UINT32)0, 0);
    if (nullptr == hIOCPHandle)
    {
        printf("[����] listen socket IOCP bind ���� : %d\n", WSAGetLastError());
        return false;
    }

    printf("���� ��� ����..\n");
    return true;
}

bool IOCPServer::StartServer(const UINT32 maxClientCount_)
{
    CreateClient(maxClientCount_);

    //���ӵ� Ŭ���̾�Ʈ �ּ� ������ ������ ����ü
    bool bRet = CreateWokerThread();
    if (false == bRet) {
        return false;
    }

    bRet = CreateAccepterThread();
    if (false == bRet) {
        return false;
    }

    printf("���� ����\n");
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

    //Accepter �����带 �����Ѵ�.
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
    //WaingThread Queue�� ��� ���·� ���� ������� ���� ����Ǵ� ���� : (cpu���� * 2) + 1 
    auto threadCount = (MaxIOWorkerThreadCount * 2) + 1;

    for (UINT32 i = 0; i < threadCount; i++)
    {
        mIOWorkerThreads.emplace_back([this]() { WokerThread(); });
    }

    printf("WokerThread ����..\n");
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

bool IOCPServer::CreateAccepterThread()
{
    mAccepterThread = std::thread([this]() { AccepterThread(); });

    printf("AccepterThread ����..\n");
    return true;
}

void IOCPServer::WokerThread()
{
    //CompletionKey�� ���� ������ ����
    ClientInfo* pClientInfo = nullptr;
    //�Լ� ȣ�� ���� ����
    BOOL bSuccess = TRUE;
    //Overlapped I/O�۾����� ���۵� ������ ũ��
    DWORD dwIoSize = 0;
    //I/O �۾��� ���� ��û�� Overlapped ����ü�� ���� ������
    LPOVERLAPPED lpOverlapped = NULL;

    while (mIsWorkerRun)
    {
        bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
            &dwIoSize,               // ������ ���۵� ����Ʈ
            (PULONG_PTR)&pClientInfo,      // CompletionKey
            &lpOverlapped,            // Overlapped IO ��ü
            INFINITE);               // ����� �ð�

        //����� ������ ���� �޼��� ó��..
        if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
        {
            mIsWorkerRun = false;
            continue;
        }

        /*if (NULL == lpOverlapped)
        {
           continue;
        }*/

        auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;

        //client�� ������ ��������..      
        // send or recv Datasize�� 0�ΰ� �̻��ϳ�? �߸���.    
        if (FALSE == bSuccess || (IOOperation::ACCEPT != pOverlappedEx->m_eOperation && dwIoSize == 0))
        {
            //printf("socket(%d) ���� ����\n", (int)pClientInfo->m_socketClient);
            CloseSocket(pClientInfo); //Caller WokerThread()
            continue;
        }

        /*
        * ????????????????????? �ñ��ϳ�. IO���꿡�� ACCEPT, RECV, SEND���ϴµ� Accept���� recv�� ����ǰ��ֳ�?????????????????????
           GTP�� �ѹ� �����.
        */
        if (IOOperation::ACCEPT == pOverlappedEx->m_eOperation)
        {
            pClientInfo = GetClientInfo(pOverlappedEx->SessionIndex);
            if (pClientInfo->AcceptCompletion())
            {
                //Ŭ���̾�Ʈ ���� ����
                ++mClientCnt;

                //IOCP���� Connect���ƴ϶�, iocp�� ��� ���� ���� application���� ����� �κ�.
                OnConnect(pClientInfo->GetIndex());
            }
            else
            {
                CloseSocket(pClientInfo, true);  //Caller WokerThread()
            }
        }
        //Overlapped I/O Recv�۾� ��� �� ó��
        else if (IOOperation::RECV == pOverlappedEx->m_eOperation)
        {
            OnReceive(pClientInfo->GetIndex(), dwIoSize, pClientInfo->RecvBuffer());

            pClientInfo->BindRecv();
        }
        //Overlapped I/O Send�۾� ��� �� ó��
        else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
        {
            pClientInfo->SendCompleted(dwIoSize);
        }
        //���� ��Ȳ
        else
        {
            printf("Client Index(%d)���� ���ܻ�Ȳ\n", pClientInfo->GetIndex());
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

void IOCPServer::CloseSocket(ClientInfo* clientInfo_, bool isForce_)
{
    if (clientInfo_->IsConnectd() == false)
    {
        return;
    }

    auto clientIndex = clientInfo_->GetIndex();

    clientInfo_->Close(isForce_);

    //IOCP ��ӹ޴� �ܿ��� OnClose. 
    OnClose(clientIndex);
}