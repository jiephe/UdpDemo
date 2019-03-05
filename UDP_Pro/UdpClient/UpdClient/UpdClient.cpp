#include <windows.h>
#include <stdio.h>
#include <string>
#pragma comment(lib, "ws2_32.lib")  

#define SERVER_PORT		19000

int main()
{
	WSADATA wsaData;
	WORD wReqest = MAKEWORD(1, 1);
	if (WSAStartup(wReqest, &wsaData) != 0)
	{
		printf("init net error......\n");
		return -1;
	}

	SOCKET client_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (client_fd == INVALID_SOCKET)
	{
		printf("create socket fail!\n");
		WSACleanup();
		return -1;
	}

	struct sockaddr_in ser_addr;
	memset(&ser_addr, 0x0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	//for linux 
	//ser_addr.sin_addr.s_addr = inet_addr("192.168.17.128");
	
	//for windows
	ser_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	ser_addr.sin_port = htons(SERVER_PORT);

	int snd_size = 1*1024;
	int rcv_size = 10*1024;
	char* snd_buffer = new char[snd_size];
	char* rcv_buffer = new char[rcv_size];
	for (int i = 0; i < snd_size; ++i)
		snd_buffer[i] = 'a';
	snd_buffer[snd_size -1] = '\0';

	int try_count = 0;
		
	while (try_count < 100)
	{
		int n = sendto(client_fd, (const char*)snd_buffer, snd_size, 0, (struct sockaddr*)&ser_addr, sizeof(ser_addr));
		if (n < 0)
		{
			printf("ERROR in sendto\n");
			closesocket(client_fd);
			WSACleanup();
			return -1;
		}


		//如服务端接到数据后 延迟20秒发送 recvfrom会阻塞等待20S so客户端需要用到select
		memset(rcv_buffer, 0x0, rcv_size);
		int len = sizeof(ser_addr);
		n = recvfrom(client_fd, rcv_buffer, rcv_size, 0, (struct sockaddr*)&ser_addr, &len);
		if (n < 0)
		{
			printf("ERROR in recvfrom\n");
			closesocket(client_fd);
			WSACleanup();
			return -1;
		}

		try_count++;
	}

// 	while (true)
// 	{
// 		Sleep(100);
// 	}
	closesocket(client_fd);
	WSACleanup();

	return 0;
}