#include "GameClient.h"

GameClient::GameClient() : mClientSocket(INVALID_SOCKET)
{
	// ���� ��� �� ���� WSAStartup()�� ȣ���ؾ���
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::runtime_error(std::format("WSAStartup ����! (���� �ڵ�: {})", WSAGetLastError()));
	}

	mClientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mClientSocket == INVALID_SOCKET)
	{		
		WSACleanup(); // ���� ���� ���� �� Cleanup ����
		throw std::runtime_error(std::format("���� ���� ����! (���� �ڵ�: {})", WSAGetLastError()));		
	}
}

GameClient::~GameClient()
{
	Close();
	WSACleanup();
}

bool GameClient::Init(const std::string& serverIP, int port)
{	
	mServerAddr.sin_family = AF_INET;
	mServerAddr.sin_port = htons(port);

	// `inet_pton()`�� ����Ͽ� `sin_addr`�� ��ȯ�� �� ����
	if (inet_pton(AF_INET, serverIP.c_str(), &mServerAddr.sin_addr) <= 0)
	{
		std::cout << std::format("IP �ּ� ��ȯ ����! (�߸��� ����: {})", serverIP) << std::endl;
		return false;
	}

	return true;

}

bool GameClient::Connect()
{
	int result = connect(mClientSocket, (sockaddr*)&mServerAddr, sizeof(mServerAddr));
	if (result == SOCKET_ERROR)
	{
		std::cout << std::format("���� ����! (���� �ڵ�: {})", WSAGetLastError()) << std::endl;
		return false;
	}
	return true;
}

void GameClient::SendMsg(const std::string& message)
{
	// c_str(): string�� const char*�� ��ȯ
	int sendBytes = send(mClientSocket, message.c_str(), message.length(), 0);
	if (sendBytes == SOCKET_ERROR)
	{
		std::cout << std::format("�޽��� ���� ����! (���� �ڵ�: {})", WSAGetLastError()) << std::endl;
	}

	std::cout << std::format("������ ������ ����Ʈ ��: {}", sendBytes) << std::endl;
}

std::string GameClient::RecvMsg()
{
	char buffer[1024]{ 0 };
	int recvResult = recv(mClientSocket, buffer, sizeof(buffer), 0);

	if (recvResult == 0)
	{
		std::cout << std::format("������ ������ ����Ǿ����ϴ�!") << std::endl;
		Close();
		return "";
	}
	else if (recvResult == SOCKET_ERROR)
	{
		std::cout << std::format("������ ���� ����! ���� �ڵ�: {}", WSAGetLastError()) << std::endl;
		Close();
		return "";
	}	

	// ��Ʈ�� ��ü�� ��ȯ �� ���ŵ� ������ ũ�⸸ŭ ��ȯ
	return std::string(buffer, recvResult);
}

void GameClient::Close()
{
	if (mClientSocket != INVALID_SOCKET)
	{
		closesocket(mClientSocket);
		mClientSocket = INVALID_SOCKET;
	}
}
