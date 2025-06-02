#include "ClientInfo.h"

ClientInfo::ClientInfo()
{
    ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
    mSocket = INVALID_SOCKET;
}

void ClientInfo::Init(const UINT32 index, HANDLE iocpHandle_)
{
    mIndex = index;
    mIOCPHandle = iocpHandle_;
}

UINT32 ClientInfo::GetIndex() { return mIndex; }

bool ClientInfo::IsConnectd() { return mIsConnect == 1; }

SOCKET ClientInfo::GetSock() { return mSocket; }

UINT64 ClientInfo::GetLatestClosedTimeSec() { return mLatestClosedTimeSec; }

char* ClientInfo::RecvBuffer() { return mRecvBuf; }

bool ClientInfo::OnConnect(HANDLE iocpHandle_, SOCKET socket_)
{
    mSocket = socket_;
    mIsConnect = 1;

    Clear();

    //I/O Completion Port��ü�� ������ �����Ų��.
    if (BindIOCompletionPort(iocpHandle_) == false)
    {
        return false;
    }

    return BindRecv();
}

void ClientInfo::Close(bool bIsForce)
{
    struct linger stLinger = { 0, 0 };   // SO_DONTLINGER�� ����

    // bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ� ���� ���� ��Ų��. ���� : ������ �ս��� ������ ���� 
    if (true == bIsForce)
    {
        stLinger.l_onoff = 1;
    }

    //socketClose������ ������ �ۼ����� ��� �ߴ� ��Ų��.
    shutdown(mSocket, SD_BOTH);

    //���� �ɼ��� �����Ѵ�.
    setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

    mIsConnect = 0;
    mLatestClosedTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    //���� ������ ���� ��Ų��.
    closesocket(mSocket);
    mSocket = INVALID_SOCKET;
}

void ClientInfo::Clear()
{

}

bool ClientInfo::PostAccept(SOCKET listenSock_, const UINT64 curTimeSec_)
{
    // printf_s("PostAccept. client Index: %d\n", GetIndex());

    mLatestClosedTimeSec = UINT32_MAX;

    mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP,
        NULL, 0, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == mSocket)
    {
        printf_s("client Socket WSASocket Error : %d\n", GetLastError());
        return false;
    }

    ZeroMemory(&mAcceptContext, sizeof(stOverlappedEx));

    DWORD bytes = 0;
    DWORD flags = 0;
    mAcceptContext.m_wsaBuf.len = 0;
    mAcceptContext.m_wsaBuf.buf = nullptr;
    mAcceptContext.m_eOperation = IOOperation::ACCEPT;
    mAcceptContext.SessionIndex = mIndex;

    if (FALSE == AcceptEx(listenSock_, mSocket, mAcceptBuf, 0,
        sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes, (LPWSAOVERLAPPED) & (mAcceptContext)))
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            printf_s("AcceptEx Error : %d\n", GetLastError());
            return false;
        }
    }

    return true;
}

bool ClientInfo::AcceptCompletion()
{
    printf_s("AcceptCompletion : SessionIndex(%d)\n", mIndex);

    // �翬�� Accept �� �Ϸ�Ǹ� Ŭ���̾�Ʈ ����ȰŴ� RECV�ɾ������
    if (OnConnect(mIOCPHandle, mSocket) == false)
    {
        return false;
    }

    SOCKADDR_IN      stClientAddr;
    int nAddrLen = sizeof(SOCKADDR_IN);
    char clientIP[32] = { 0, };
    inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
    printf("Ŭ���̾�Ʈ ���� : IP(%s) SOCKET(%d)\n", clientIP, (int)mSocket);

    return true;
}

bool ClientInfo::BindIOCompletionPort(HANDLE iocpHandle_)
{
    //socket�� pClientInfo�� CompletionPort��ü�� �����Ų��.
    auto hIOCP = CreateIoCompletionPort((HANDLE)GetSock()
        , iocpHandle_
        , (ULONG_PTR)(this), 0);

    if (hIOCP == INVALID_HANDLE_VALUE)
    {
        printf("[����] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
        return false;
    }

    return true;
}

bool ClientInfo::BindRecv()
{
    DWORD dwFlag = 0;
    DWORD dwRecvNumBytes = 0;

    //Overlapped I/O�� ���� �� ������ ������ �ش�.
    mRecvOverlappedEx.m_wsaBuf.len = MAX_SOCK_RECVBUF;
    mRecvOverlappedEx.m_wsaBuf.buf = mRecvBuf;
    mRecvOverlappedEx.m_eOperation = IOOperation::RECV;

    int nRet = WSARecv(mSocket, &(mRecvOverlappedEx.m_wsaBuf), 1, &dwRecvNumBytes, &dwFlag, (LPWSAOVERLAPPED) & (mRecvOverlappedEx), NULL);

    //socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
    if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
    {
        printf("[����] WSARecv()�Լ� ���� : %d\n", WSAGetLastError());
        return false;
    }

    return true;
}

bool ClientInfo::SendMsg(const UINT32 dataSize_, char* pMsg_)
{
    auto sendOverlappedEx = new stOverlappedEx;
    ZeroMemory(sendOverlappedEx, sizeof(stOverlappedEx));

    sendOverlappedEx->m_wsaBuf.len = dataSize_;
    sendOverlappedEx->m_wsaBuf.buf = new char[dataSize_];
    //���� new�� ���ϰ� �ٷ�  = pMSG�� �Ѵٸ� ? 
    CopyMemory(sendOverlappedEx->m_wsaBuf.buf, pMsg_, dataSize_);

    sendOverlappedEx->m_eOperation = IOOperation::SEND;

    // ����� �α� �߰�
    printf("[DEBUG] SendMsg this=%p, &mSendLock=%p, mIndex=%d\n",
        this, (void*)&mSendLock, mIndex);


    std::lock_guard<std::mutex> guard(mSendLock);

    mSendDataqueue.push(sendOverlappedEx);
   
    if (mSendDataqueue.size() == 1)
    {
        SendIO();
    }

    return true;
}

// 
void ClientInfo::SendCompleted(const UINT32 dataSize_)
{
    printf("[�۽� �Ϸ�] bytes : %d\n", dataSize_);

    std::lock_guard<std::mutex> guard(mSendLock);

    // SendMSG SendsendOverlappedEx->m_wsaBuf.buf = new char[dataSize_];
    delete[] mSendDataqueue.front()->m_wsaBuf.buf;
    delete mSendDataqueue.front();

    mSendDataqueue.pop();

    if (mSendDataqueue.empty() == false)
    {
        SendIO();
    }
}

bool ClientInfo::SendIO()
{
    auto sendOverlappedEx = mSendDataqueue.front();

    DWORD dwRecvNumBytes = 0;

    int nRet = WSASend(mSocket, &(sendOverlappedEx->m_wsaBuf), 1, &dwRecvNumBytes, 0, (LPWSAOVERLAPPED)sendOverlappedEx, NULL);

    //socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
    if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
    {
        printf("[����] WSASend()�Լ� ���� : %d\n", WSAGetLastError());
        return false;
    }

    return true;
}

bool ClientInfo::SetSocketOption()
{
    int opt = 1;
    if (SOCKET_ERROR == setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)))
    {
        printf_s("[DEBUG] TCP_NODELAY error: %d\n", GetLastError());
        return false;
    }

    opt = 0;
    if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)))
    {
        printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError());
        return false;
    }

    return true;
}