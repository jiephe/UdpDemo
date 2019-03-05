// UpdServer.cpp : 定义控制台应用程序的入口点。
//

//size 默认64 要自己设置
#define FD_SETSIZE	200

#include <windows.h>
#include <stdio.h>
#include <string>
#include <deque>
#include <thread>
#include <mutex>

#define SERVER_PORT		17000

#pragma comment(lib, "ws2_32.lib")  

//适用于面向多客户端并发

SOCKET							server_fd;

fd_set g_rset;
std::mutex _mutex;

using MutexLockGuard = std::lock_guard<std::mutex>;

//问题 ： 服务端感知不到断开的upd客户端 无法从fd_set里面清除
// 导致fd_set 的count达到了上限 其他客户端连接不上来
void deal_with()
{
	int size = 10 * 1024;
	char* rcv_buffer = new char[size];
	char* snd_buffer = new char[size];

	FD_ZERO(&g_rset);

	int deal_count = 0;

	while (1) 
	{
		fd_set rset;
	
		{
			MutexLockGuard lock(_mutex);
			memcpy(&rset, &g_rset, sizeof(fd_set));
		}

		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 100 * 1000;	// 10 millisecond

		int nRet = select(0, &rset, 0, 0, &timeout);

		for (int i = 0; i < rset.fd_count; ++i)
		{
			int n = recv(rset.fd_array[i], rcv_buffer, size, 0);
			if (n < 0)
			{
				printf("ERROR in recvfrom\n");
				continue;
			}
			rcv_buffer[n] = '\0';

			if (strlen(rcv_buffer) > 0)
			{
				sprintf_s(snd_buffer, size, "%s-->%d", rcv_buffer, 100);
				int n = send(rset.fd_array[i], snd_buffer, strlen(snd_buffer), 0);
				if (n < 0)
				{
					printf("ERROR in sendto\n");
				}
			}

			deal_count++;

			if (deal_count % 100 == 0)
				printf("deal count : %d\n", deal_count);
		}
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

	struct sockaddr_in cliaddr;
	int size = 10*1024;
	char* rcv_buffer = new char[size];
	char* snd_buffer = new char[size];
	int seq = 0;

	printf("\nbegin work ..... \n");

	std::thread* t3 = new std::thread(deal_with);

	while (1) {
		memset(rcv_buffer, 0x0, size);
		memset(snd_buffer, 0x0, size);


		int len = sizeof(cliaddr);
		int n = recvfrom(server_fd, rcv_buffer, size, 0, (struct sockaddr*)&cliaddr, &len);

		SOCKET peer = socket(AF_INET, SOCK_DGRAM, 0);
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(0);
		int ret = bind(peer, (struct sockaddr*)&addr, sizeof(addr));
		int nret = connect(peer, (struct sockaddr *)&cliaddr, sizeof(struct sockaddr));
		nret = send(peer, "testt", 5, 0);

		{
			MutexLockGuard lock(_mutex);
			FD_SET(peer, &g_rset);
		}
	}
}

