#include "GameServer.h"

void GameServer::OnConnect(const UINT32 clientIndex_)
{
}

void GameServer::OnClose(const UINT32 clientIndex_)
{
}

void GameServer::OnReceive(const UINT32 clientIndex_, const UINT32 size_, char* pData_)
{
    std::cout << "Client " << clientIndex_ << " " << size_ << "��ŭ�� �����͸� ���� ����" << pData_ << std::endl;

    SendMsg(clientIndex_, size_, pData_);
}