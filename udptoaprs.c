/*
udptoaprs [ -d ] 呼号
参数含义：呼号，用来连接china.aprs2.net服务器
 
功能：
	从 127.0.0.1 14581 UDP端口接收数据
	使用TCP转发给china.aprs2.net
	注意：BA BD BG BH BR BI BY VR +数字 的才转发
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <ctype.h>
#include "sock.h"

int debug = 0;

#define MAXLEN 16384

int checkcall(char *call)
{
	char *p;
	if (strlen(call) < 5)
		return 0;
	p = call;
	// 第3位必须是数字
	if (!isdigit(*(p + 2)))
		return 0;
	// BA BD BG BH BI BL BM BR BV BX BY
	if (*p == 'B') {
		if (*(p + 1) == 'A' ||
		    *(p + 1) == 'D' ||
		    *(p + 1) == 'G' ||
		    *(p + 1) == 'H' ||
		    *(p + 1) == 'I' || *(p + 1) == 'L' || *(p + 1) == 'M' || *(p + 1) == 'R' || *(p + 1) == 'V' || *(p + 1) == 'X' || *(p + 1) == 'Y')
			return 1;
	}
	// VR2
	if (*p == 'V') {
		if (*(p + 1) == 'R' && *(p + 2) == '2')
			return 1;
	}
	// XX9
	if (*p == 'X') {
		if (*(p + 1) == 'X' && *(p + 2) == '9')
			return 1;
	}

	return 0;
}

void Process(int u_fd, int r_fd)
{
	fd_set rset;
	char buff[MAXLEN];
	struct timeval tv;
	int m, n;
	int max_fd;
	char call[MAXLEN];

	int enable = 1;
	setsockopt(r_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable));

	while (1) {
		FD_ZERO(&rset);
		FD_SET(u_fd, &rset);
		FD_SET(r_fd, &rset);
		max_fd = max(u_fd, r_fd);
		tv.tv_sec = 300;
		tv.tv_usec = 0;

		m = Select(max_fd + 1, &rset, NULL, NULL, &tv);

		if (m == 0)
			continue;

		if (FD_ISSET(r_fd, &rset)) {
			n = recv(r_fd, buff, MAXLEN, 0);
			if ((n <= 0) && (errno == EINTR))
				continue;
			if (n <= 0) {
				err_sys("recv get %d from tcp server\n", n);
				exit(0);
			}
			buff[n] = 0;
			if (debug)
				fprintf(stderr, "S: %s", buff);
		}
		if (FD_ISSET(u_fd, &rset)) {
			n = recv(u_fd, buff, MAXLEN, 0);
			if (n < 0) {
				err_sys("recv get %d from udp client\n", n);
				exit(0);
			}
			if (n == 0)
				continue;
			buff[n] = 0;
			if (debug)
				fprintf(stderr, "C: %s", buff);
			strcpy(call, buff);
			char *p;
			p = strchr(call, '>');
			if (p)
				*p = 0;
			if (checkcall(call) == 0) {
				if (debug)
					fprintf(stderr, "!!  %s is not a china call!!!\n", call);
				continue;
			}
			Write(r_fd, buff, n);
		}
	}
}

#include "passcode.c"

void usage()
{
	printf("\nudptoaprs [ -d ] your_call_sign\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	int r_fd, u_fd;
	int llen;
	char buf[MAXLEN];
	int i = 1;
	int got_one = 0;
	do {
		got_one = 1;
		if (argc - i == 0)
			break;
		if (strcmp(argv[i], "-h") == 0)
			usage();
		else if (strcmp(argv[i], "-d") == 0)
			debug = 1;
		else
			got_one = 0;
		if (got_one)
			i++;
	} while (got_one);

	if (argc - i != 1)
		usage();
	signal(SIGCHLD, SIG_IGN);
	if (debug == 0) {
		daemon_init("udptoaprs", LOG_DAEMON);
		while (1) {
			int pid;
			pid = fork();
			if (pid == 0)	// i am child, will do the job
				break;
			else if (pid == -1)	// error
				exit(0);
			else
				wait(NULL);	// i am parent, wait for child
			sleep(2);	// if child exit, wait 2 second, and rerun
		}
	}
	err_msg("starting\n");
	u_fd = Udp_server("127.0.0.1", "14581", (socklen_t *) & llen);

	r_fd = Tcp_connect("asia.aprs2.net", "14580");
	snprintf(buf, MAXLEN, "user %s pass %d vers udptoaprs 1.0 filter r/31.83/117.29/1\r\n", argv[i], passcode(argv[i]));
	Write(r_fd, buf, strlen(buf));
	if (debug) {
		fprintf(stderr, "C: %s", buf);
		fprintf(stderr, "u_fd=%d, r_fd=%d\n", u_fd, r_fd);
	}
	Process(u_fd, r_fd);
	return 0;
}
