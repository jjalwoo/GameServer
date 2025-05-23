#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "mswsock.lib")

#include "ClientInfo.h"
#include "Define.h"
#include <thread>
#include <vector>

class IOCPServer
{
public:
    IOCPServer();
    virtual ~IOCPServer();

    bool Init(const UINT32 maxIOWorkerThreadCount_);
    bool InitLogger();
    bool BindandListen(int bindPort_);
    bool StartServer(const UINT32 maxClientCount_);
    void DestroyThread();
    bool SendMsg(const UINT32 clientIndex_, const UINT32 dataSize_, char* pData);

    virtual void OnConnect(const UINT32 clientIndex_) {}
    virtual void OnClose(const UINT32 clientIndex_) {}
    virtual void OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData_) {}

private:
    void CreateClient(const UINT32 maxClientCount_);
    bool CreateWokerThread();
    bool CreateAccepterThread();
    void WokerThread();
    void AccepterThread();
    ClientInfo* GetEmptyClientInfo();
    ClientInfo* GetClientInfo(const UINT32 clientIndex_) const;
    void CloseSocket(ClientInfo* clientInfo_, bool isForce_ = false);

    UINT32 MaxIOWorkerThreadCount = 0;

    //클라이언트 정보 저장 구조체
    std::vector<ClientInfo*> mClientInfos;

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