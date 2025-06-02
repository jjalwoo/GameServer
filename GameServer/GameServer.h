#pragma once
#include <iostream>
#include "IOCPServer.h"
class GameServer : public IOCPServer
{
public:
    void OnConnect(const UINT32 clientIndex_) override;
    void OnClose(const UINT32 clientIndex_) override;
    void OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData_) override;  
};

