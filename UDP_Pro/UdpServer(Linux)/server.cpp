#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <mutex>
#include <map>

#define ADDR  INADDR_ANY

typedef struct {
	int fd;
	unsigned int readable : 1;
	unsigned int writable : 1;
} peer_t;

static int tmfd = -1;
static int epfd = -1;
static int npeers;

static unsigned long bytes_read;
static unsigned long bytes_written;

static std::mutex _mutex;
using MutexLockGuard = std::lock_guard<std::mutex>;

std::map<void*, int>	g_map;

static int add_fd(int fd, void *arg, int events) {
	struct epoll_event ee;
	ee.events = events | EPOLLET;
	ee.data.ptr = arg;
	return epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ee);
}

static int mod_fd(int fd, void *arg, int events) {
	struct epoll_event ee;
	ee.events = events;
	ee.data.ptr = arg;
	return epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ee);
}

static int make_sock_fd(unsigned short port) {
	struct sockaddr_in sin;
	int sockfd;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		return -1;

	memset(&sin, 0, sizeof sin);
	sin.sin_addr.s_addr = htonl(ADDR);
	sin.sin_port = htons(port);
	sin.sin_family = AF_INET;

	if (bind(sockfd, (struct sockaddr *) &sin, sizeof sin) == -1)
		return -1;

	return sockfd;
}

static void do_epoll(void) 
{
	struct epoll_event* out = new struct epoll_event[10000];
	peer_t *peer;
	int i, n, r;
	int events;
	int fd;
	
	int count = 0;

	while (1) 
	{
		n = epoll_wait(epfd, out, 10000, 5000);
		if (n <= 0)
			continue;
	
		for (i = 0; i < n; i++) 
		{
			events = out[i].events;
			peer = (peer_t *)out[i].data.ptr;
			fd = peer->fd;
			
			if (events & EPOLLIN)
			{
				//printf("fd is : %d\n", fd);
				
				if(fd != tmfd)
				{
					size_t size = sizeof(long long);
					long long llExpirations;
					int a = read(fd, &llExpirations, size);
					
					//每个连接的定时器 几次*时间间隔 没有收到包 就断开连接 即使没有断开 发送的时候再重新创建也可以 
					// 就是为了定时清理内存 防止内存过高
				}

				struct sockaddr_in cliaddr;
				socklen_t len = sizeof(cliaddr);
				char rcv_buffer[1024];
				int n = recvfrom(tmfd, rcv_buffer, 1024, 0, (struct sockaddr*)&cliaddr, &len);
				if (n <= 0)
					int a = 1;
				
				//用&cliaddr来标识客户端 同一个客户端不重启的情况下 &cliaddr的值不变 内核中的同一个对象
				//printf("addr is : %p\n", &cliaddr);
				printf("addr is : %u %u\n", pthread_self(), count++);
								
				if (g_map.find(&cliaddr) == g_map.end())
				{
					g_map[&cliaddr] = 1;
#if 0
					int tv_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
					itimerspec its;
#define SET_TS(ts, ms) ((ts)->tv_sec = (ms) / 1000, (ts)->tv_nsec = ((ms) % 1000) * 1e6)
					SET_TS(&its.it_interval, 3000);
					SET_TS(&its.it_value, 3000);
					timerfd_settime(tv_fd, 0, &its, nullptr);
					peer_t tmp1;
					tmp1.fd = tv_fd;
					add_fd(tv_fd, &tmp1, EPOLLIN);
#endif
				}
				
				const char reply[] = "PING";
				n = sendto(tmfd, (const char*)reply, sizeof(reply) - 1, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
				if (n <= 0)
					int a = 1;
				
				peer_t tmp;
				tmp.fd = fd;
				mod_fd(fd, &tmp, EPOLLIN | EPOLLET);
			}	
			else
			{
				int a = 1;
			}
		}
	}
}

int main(int argc, char **argv)
{
	peer_t *peers;
	peer_t tmp;
	int i, r;

	if ((epfd = epoll_create1(EPOLL_CLOEXEC)) == -1) {
		perror("make_epoll_fd");
		exit(1);
	}
		
	tmfd = make_sock_fd(18000);
	fcntl(tmfd, F_SETFL, fcntl(tmfd, F_GETFD, 0) | O_NOATIME | O_NONBLOCK | O_CLOEXEC);	
	tmp.fd = tmfd;
	add_fd(tmfd, &tmp, EPOLLIN);
	
	std::thread t1(do_epoll);
	do_epoll();
	
	while (true)
	{
		auto c = getchar();
		if (c == 'q') 
		{
			printf("q pressed, quiting...\n");
			break;
		} 
		
		usleep(10 * 1000);
	}

	return 0;
}