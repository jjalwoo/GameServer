#pragma once
#include "Define.h"
#include <stdio.h>
#include <mutex>
#include <queue>


//Ŭ���̾�Ʈ ������ ������� ����ü
class ClientInfo
{
public:   
    // ������: ��� �ʱ�ȭ ����Ʈ�� �⺻�� ���� 
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

    SOCKET         mSocket;         //Cliet�� ����Ǵ� ����

    stOverlappedEx   mAcceptContext;
    char mAcceptBuf[64];

    stOverlappedEx   mRecvOverlappedEx;   //RECV Overlapped I/O�۾��� ���� ����   

    // c++ 17���ĺ��� byteŸ���� ����, _wsabuf������ char*�� ����ϱ⿡ byte ��� char*�� ����Ѵ�.s
    char         mRecvBuf[MAX_SOCK_RECVBUF]; //������ ����

    std::mutex mSendLock;
    std::queue<stOverlappedEx*> mSendDataqueue;


};

