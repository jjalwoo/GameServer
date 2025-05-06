#include "GameClient.h"

GameClient::GameClient() : mClientSocket(INVALID_SOCKET)
{
	// 소켓 사용 시 먼저 WSAStartup()을 호출해야함
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::runtime_error(std::format("WSAStartup 실패! (에러 코드: {})", WSAGetLastError()));
	}

	mClientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mClientSocket == INVALID_SOCKET)
	{		
		WSACleanup(); // 소켓 생성 실패 시 Cleanup 수행
		throw std::runtime_error(std::format("소켓 생성 실패! (에러 코드: {})", WSAGetLastError()));		
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

	// `inet_pton()`을 사용하여 `sin_addr`에 변환된 값 저장
	if (inet_pton(AF_INET, serverIP.c_str(), &mServerAddr.sin_addr) <= 0)
	{
		std::cout << std::format("IP 주소 변환 실패! (잘못된 형식: {})", serverIP) << std::endl;
		return false;
	}

	return true;

}

bool GameClient::Connect()
{
	int result = connect(mClientSocket, (sockaddr*)&mServerAddr, sizeof(mServerAddr));
	if (result == SOCKET_ERROR)
	{
		std::cout << std::format("연결 오류! (에러 코드: {})", WSAGetLastError()) << std::endl;
		return false;
	}
	return true;
}

void GameClient::SendMsg(const std::string& message)
{
	// c_str(): string을 const char*로 변환
	int sendBytes = send(mClientSocket, message.c_str(), message.length(), 0);
	if (sendBytes == SOCKET_ERROR)
	{
		std::cout << std::format("메시지 전송 실패! (에러 코드: {})", WSAGetLastError()) << std::endl;
	}

	std::cout << std::format("서버에 전송한 바이트 수: {}", sendBytes) << std::endl;
}

std::string GameClient::RecvMsg()
{
	char buffer[1024]{ 0 };
	int recvResult = recv(mClientSocket, buffer, sizeof(buffer), 0);

	if (recvResult == 0)
	{
		std::cout << std::format("서버와 연결이 종료되었습니다!") << std::endl;
		Close();
		return "";
	}
	else if (recvResult == SOCKET_ERROR)
	{
		std::cout << std::format("데이터 수신 실패! 에러 코드: {}", WSAGetLastError()) << std::endl;
		Close();
		return "";
	}	

	// 스트링 객체로 변환 후 수신된 데이터 크기만큼 반환
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
