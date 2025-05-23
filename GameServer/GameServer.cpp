#include "GameServer.h"

void GameServer::OnConnect(const UINT32 clientIndex_)
{
    std::cout << "Client " << clientIndex_ << " 연결됨!" << std::endl;
}

void GameServer::OnClose(const UINT32 clientIndex_)
{
    std::cout << "Client " << clientIndex_ << " 연결 종료!" << std::endl;
}

void GameServer::OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData_)
{
    std::cout << "Client " << clientIndex_ << " " << size_ << "만큼의 데이터를 전송: " << pData_ << std::endl;
}