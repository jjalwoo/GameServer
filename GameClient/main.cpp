#include <iostream>
#include "GameClient.h"

int main()
{
	GameClient client;

	// 서버 IP와 포트 번호 설정
	// bool initResult = client.Init("127.0.0.1", 8080);
	if (!client.Init("127.0.0.1", 8080))
	{
		std::cout << std::format("클라이언트 초기화 실패!") << std::endl;
		return -1;
	}

	// 서버와 연결
	if (!client.Connect())
	{
		std::cout << std::format("서버와 연결 실패!") << std::endl;
		return -1;
	}	

	// 메시지 송신 및 받기
	client.SendMsg("Hello, Server! Nice to meet you!");	
	std::string response = client.RecvMsg();

	std::cout << std::format("서버 응답: {}", response) << std::endl;

	// 클라이언트 종료
	client.Close();

	return 0;
}