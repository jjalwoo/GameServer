#pragma once
#include <iostream>
#include "IOCPServer.h"
class GameServer : public IOCPServer
{
public:
    void OnConnect(const UINT32 clientIndex_) override {}
    void OnClose(const UINT32 clientIndex_) override  {}
    void OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData_) override 
    {
        std::cout << "Client " << clientIndex_ << " " << size_ << "만큼의 데이터를 전송" << pData_ << std::endl;
    }
};

