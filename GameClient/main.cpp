#include <iostream>
#include "GameClient.h"

int main()
{
	GameClient client;

	// ���� IP�� ��Ʈ ��ȣ ����
	// bool initResult = client.Init("127.0.0.1", 8080);
	if (!client.Init("127.0.0.1", 8080))
	{
		std::cout << std::format("Ŭ���̾�Ʈ �ʱ�ȭ ����!") << std::endl;
		return -1;
	}

	// ������ ����
	if (!client.Connect())
	{
		std::cout << std::format("������ ���� ����!") << std::endl;
		return -1;
	}	

	// �޽��� �۽� �� �ޱ�
	client.SendMsg("Hello, Server! Nice to meet you!");	
	std::string response = client.RecvMsg();

	std::cout << std::format("���� ����: {}", response) << std::endl;

	// Ŭ���̾�Ʈ ����
	client.Close();

	return 0;
}