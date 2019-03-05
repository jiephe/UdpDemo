// UpdServer.cpp : 定义控制台应用程序的入口点。
//
#include <windows.h>
#include <stdio.h>
#include <string>
#include <deque>
#include <thread>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")  

#define SERVER_PORT				19000

SOCKET							server_fd;

void do_select()
{
	struct sockaddr_in cliaddr;
	char rcv_buffer[2048];
	char snd_buffer[2048];

	fd_set rset;
	FD_ZERO(&rset);

	int count = 0;

	while (1)
	{
		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 100 * 1000;	// 100 millisecond

		FD_ZERO(&rset);
		FD_SET(server_fd, &rset);

		int nRet = select(0, &rset, 0, 0, &timeout);
		if (nRet <= 0)
			continue;

		for (u_int i = 0; i < rset.fd_count; ++i)
		{
			memset(rcv_buffer, 0x0, sizeof(rcv_buffer));
			memset(snd_buffer, 0x0, sizeof(snd_buffer));

			int len = sizeof(cliaddr);
			int n = recvfrom(rset.fd_array[i], rcv_buffer, sizeof(rcv_buffer), 0, (struct sockaddr*)&cliaddr, &len);
			if (n <= 0)
			{
				printf("recvfrom fail!\n");
			}
			rcv_buffer[n] = '\0';

			if (strlen(rcv_buffer) > 0)
			{
				sprintf_s(snd_buffer, sizeof(snd_buffer), "%s-%d", rcv_buffer, 100);
				n = sendto(rset.fd_array[i], snd_buffer, strlen(snd_buffer) + 1, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
				if (n <= 0)
				{
					printf("sendto fail!\n");
				}
			}

			count++;

			//printf("deal self: %u count: %d\n", GetCurrentThreadId(), count);
		}

		FD_CLR(server_fd, &rset);
	}
}

int main()
{
	WSADATA wsaData;
	WORD wReqest = MAKEWORD(1, 1);
	if (WSAStartup(wReqest, &wsaData) != 0)
	{
		printf("init net error......\n");
		return -1;
	}

	server_fd = socket(AF_INET, SOCK_DGRAM, 0); 
	if (server_fd == INVALID_SOCKET)
	{
		printf("create socket fail!\n");
		WSACleanup();
		return -1;
	}

	struct sockaddr_in ser_addr;
	memset(&ser_addr, 0x0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	ser_addr.sin_port = htons(SERVER_PORT);  
	int ret = bind(server_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr));
	if (ret < 0)
	{
		printf("socket bind fail!\n");
		closesocket(server_fd);
		WSACleanup();
		return -1;
	}

	printf("\nbegin work ..... \n");

	//std::thread t(do_select);

	do_select();
}

