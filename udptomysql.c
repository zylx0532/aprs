/*
功能：
	从 127.0.0.1 14583 UDP端口接收数据
	在mysql数据库中记录收到的APRS数据包
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

#include "db.h"

#include "tomysql.c"

void Process(int u_fd)
{
	char buff[MAXLEN];
	int n;

	while (1) {
		n = recv(u_fd, buff, MAXLEN, 0);
		if (n < 0) {
			err_sys("recv get %d from udp client\n", n);
			exit(0);
		}
		if (n == 0)
			continue;
		buff[n] = 0;
		if (debug)
			err_msg("C: %s", buff);
		ToMysql(buff, n);
	}
}

int main(int argc, char *argv[])
{
	int u_fd;
	int llen;

	signal(SIGCHLD, SIG_IGN);

	if (argc > 1)
		debug = 1;

	if (debug == 0) {
		daemon_init("udptomysql", LOG_DAEMON);
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
	u_fd = Udp_server("127.0.0.1", "14583", (socklen_t *) & llen);
	if (debug)
		err_msg("u_fd=%d\n", u_fd);
	mysql = connectdb();
	Process(u_fd);
	return 0;
}
