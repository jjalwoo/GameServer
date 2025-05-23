#pragma once

#include "Define.h"
#include <stdio.h>
#include <mutex>
#include <queue>
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <iostream>

class ClientInfo
{
public:
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

    INT32 mIndex;
    HANDLE mIOCPHandle;

    INT64 mIsConnect;
    UINT64 mLatestClosedTimeSec;

    SOCKET mSocket;
    stOverlappedEx mAcceptContext;
    char mAcceptBuf[64];

    stOverlappedEx mRecvOverlappedEx;
    char mRecvBuf[MAX_SOCK_RECVBUF];

    std::mutex mSendLock;
    std::queue<stOverlappedEx*> mSendDataqueue;
};