#pragma once
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <format>
#pragma comment(lib, "ws2_32.lib") 



class GameClient
{
public:
	GameClient();
	~GameClient();

	bool Init(const std::string& serverIP, int port);
	bool Connect();
	void SendMsg(const std::string& message);
	std::string RecvMsg();
	void Close();

private:
	SOCKET mClientSocket;
	sockaddr_in mServerAddr;
};

