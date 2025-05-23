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

    if (BindIOCompletionPort(iocpHandle_) == false)
    {
        return false;
    }

    return BindRecv();
}

void ClientInfo::Close(bool bIsForce)
{
    struct linger stLinger = { 0, 0 };

    if (bIsForce)
    {
        stLinger.l_onoff = 1;
    }

    shutdown(mSocket, SD_BOTH);
    setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

    mIsConnect = 0;
    mLatestClosedTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

    closesocket(mSocket);
    mSocket = INVALID_SOCKET;
}

void ClientInfo::Clear()
{
    ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
    ZeroMemory(&mAcceptContext, sizeof(stOverlappedEx));

    mSocket = INVALID_SOCKET;
    mIsConnect = 0;
    mLatestClosedTimeSec = 0;

    while (!mSendDataqueue.empty())
    {
        delete[] mSendDataqueue.front()->m_wsaBuf.buf;
        delete mSendDataqueue.front();
        mSendDataqueue.pop();
    }
}

bool ClientInfo::PostAccept(SOCKET listenSock_, const UINT64 curTimeSec_)
{
    printf_s("PostAccept. client Index: %d\n", GetIndex());

    mLatestClosedTimeSec = UINT32_MAX;

    mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
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

    if (OnConnect(mIOCPHandle, mSocket) == false)
    {
        return false;
    }

    SOCKADDR_IN stClientAddr;
    int nAddrLen = sizeof(SOCKADDR_IN);
    char clientIP[32] = { 0 };
    inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
    printf("클라이언트 접속 : IP(%s) SOCKET(%d)\n", clientIP, (int)mSocket);

    return true;
}

bool ClientInfo::BindIOCompletionPort(HANDLE iocpHandle_)
{
    auto hIOCP = CreateIoCompletionPort((HANDLE)GetSock(), iocpHandle_, (ULONG_PTR)(this), 0);

    if (hIOCP == INVALID_HANDLE_VALUE)
    {
        printf("[에러] CreateIoCompletionPort()함수 실패: %d\n", GetLastError());
        return false;
    }

    return true;
}

bool ClientInfo::BindRecv()
{  
    DWORD dwFlag = 0;
    DWORD dwRecvNumBytes = 0;

    //mRecvBuf가 문제가 있는 상황에서 mRecvOverlappedEx.m_wsaBuf.buf = mRecvBuf;를 행하면 문제가 발생 함  
    //mRecvBuf가 유효한 메모리인지 확인
    if (mRecvBuf == nullptr)
    {
        std::cout << "BIND RECV ERROR: mRecvBuf is null." << std::endl;
        return false;
    }

    //Overlapped I/O을 위해 각 정보를 셋팅해 준다.
    mRecvOverlappedEx.m_wsaBuf.len = MAX_SOCK_RECVBUF;
    mRecvOverlappedEx.m_wsaBuf.buf = mRecvBuf;
    mRecvOverlappedEx.m_eOperation = IOOperation::RECV;

    int nRet = WSARecv(mSocket, &(mRecvOverlappedEx.m_wsaBuf), 1, &dwRecvNumBytes, &dwFlag, (LPWSAOVERLAPPED) & (mRecvOverlappedEx), NULL);

    //socket_error이면 client socket이 끊어진걸로 처리한다.
    if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
    {
        printf("[에러] WSARecv()함수 실패 : %d\n", WSAGetLastError());
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

    // 만약 new를 안 하고 바로 = pMsg_를 한다면? (메모리 복사 필요)
    CopyMemory(sendOverlappedEx->m_wsaBuf.buf, pMsg_, dataSize_);

    sendOverlappedEx->m_eOperation = IOOperation::SEND;

    // 디버그 로그 추가
    printf("[DEBUG] SendMsg this=%p, &mSendLock=%p, mIndex=%d\n", this, (void*)&mSendLock, mIndex);

    std::lock_guard<std::mutex> guard(mSendLock);

    // 큐에 메시지 추가
    mSendDataqueue.push(sendOverlappedEx);

    // 큐에 하나만 있다면 즉시 전송 시작
    if (mSendDataqueue.size() == 1)
    {
        SendIO();
    }

    return true;
}

void ClientInfo::SendCompleted(const UINT32 dataSize_)
{
    printf("[송신 완료] bytes : %d\n", dataSize_);

    std::lock_guard<std::mutex> guard(mSendLock);

    delete[] mSendDataqueue.front()->m_wsaBuf.buf;
    delete mSendDataqueue.front();
    mSendDataqueue.pop();

    /*
    * - 실질적인 Send는 SendIO 함수에서 처리한다.
    * - sendQueue의 데이터는 하나씩, 처리될 때마다 SendIO를 통한다.
    */
    /*
    * 예를 들어, 송신 요청 A가 진행 중일 때 B, C 요청이 추가되면,
    * 이들은 단순히 큐에 추가되고, A가 완료된 후에 B가, 그 후 C가 순차적으로 처리된다.
    */
    if (!mSendDataqueue.empty())
    {
        SendIO();
    }
    else
    {       
        const char* reply = "Hi Client!";
        SendMsg(strlen(reply), (char*)reply);
    }


}

bool ClientInfo::SendIO()
{
    auto sendOverlappedEx = mSendDataqueue.front();

    DWORD dwRecvNumBytes = 0;

    int nRet = WSASend(mSocket, &(sendOverlappedEx->m_wsaBuf), 1, &dwRecvNumBytes, 0, (LPWSAOVERLAPPED)sendOverlappedEx, NULL);

    if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
    {
        printf("[에러] WSASend()함수 실패 : %d\n", WSAGetLastError());
        return false;
    }

    return true;    
}

bool ClientInfo::SetSocketOption()
{
    /*
   * WSASend()에서 에러 코드 6 (ERROR_INVALID_HANDLE)이 뜨는 가장 흔한 원인은,
   * AcceptEx 로 생성된 소켓에 대해 SO_UPDATE_ACCEPT_CONTEXT 를 아직 호출하지 않아서
   * 소켓 컨텍스트가 갱신되지 않았기 때문입니다.
   */

   /* if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
        (char*)GIocpManager->GetListenSocket(), sizeof(SOCKET)))
   {
       printf_s("[DEBUG] SO_UPDATE_ACCEPT_CONTEXT error: %d\n", GetLastError());
       return false;
   } */

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
