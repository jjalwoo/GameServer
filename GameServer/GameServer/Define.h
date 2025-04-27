#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>

const int MAX_SOCK_RECVBUF = 256;
const int MAX_SOCK_SENDBUF = 4096;

enum class IOOperation
{
	ACCEPT,
	RECV,
	SEND
};

struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;
	WSABUF		  m_wsaBuf;
	IOOperation	  m_Operation; // !     
	int			  SessionIndex = 0;
};