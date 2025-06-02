#pragma once
#include "Define.h"
#include <stdio.h>
#include <mutex>
#include <queue>


//클라이언트 정보를 담기위한 구조체
class ClientInfo
{
public:   
    // 생성자: 멤버 초기화 리스트로 기본값 설정 
    ClientInfo();    

    void Init(const UINT32 index, HANDLE iocpHandle_);    

    UINT32 GetIndex();

    bool IsConnectd();

    SOCKET GetSock();

    UINT64 GetLatestClosedTimeSec();

    char* RecvBuffer();

    bool OnConnect(HANDLE iocpHandle_, SOCKET socket_);   

    void Close(bool bIsForce = false);

    void Clear();   

    bool PostAccept(SOCKET listenSock_, const UINT64 curTimeSec_);   
  
    bool AcceptCompletion();    

    bool BindIOCompletionPort(HANDLE iocpHandle_);  
        
    bool BindRecv();  
   
    bool SendMsg(const UINT32 dataSize_, char* pMsg_);
    
    void SendCompleted(const UINT32 dataSize_);

private:
    bool SendIO();  
    bool SetSocketOption();

private:
    INT32 mIndex = 0;
    HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;

    INT64 mIsConnect = 0;
    UINT64 mLatestClosedTimeSec = 0;

    SOCKET         mSocket;         //Cliet와 연결되는 소켓

    stOverlappedEx   mAcceptContext;
    char mAcceptBuf[64];

    stOverlappedEx   mRecvOverlappedEx;   //RECV Overlapped I/O작업을 위한 변수   

    // c++ 17이후부터 byte타입이 나옴, _wsabuf에서는 char*를 사용하기에 byte 대신 char*를 사용한다.s
    char         mRecvBuf[MAX_SOCK_RECVBUF]; //데이터 버퍼

    std::mutex mSendLock;
    std::queue<stOverlappedEx*> mSendDataqueue;


};

