/* aprs.tomysql.c v1.0 by  james@ustc.edu.cn 2015.12.19
命令行： aprstomyql [ -d ] [ server 呼号 ]
参数含义：呼号，用来连接china.aprs2.net服务器
功能：
	登录china.aprs2.net服务器，获取呼号前缀为B和VR2的信息
	存放到数据库china中
	如果SSID中有-13，并且路径中没有BG6CQ，发给
                114.55.54.60  14580（lewei50.com）

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
#include "sock.h"
#include <ctype.h>

int debug = 0;

#define MAXLEN 16384

#include "db.h"

#include "tomysql.c"

#include "passcode.c"

#include "sendudp.c"

void Process(char *server, char *call)
{
	char buf[MAXLEN];
	int r_fd;
	int n;
	int optval;
	socklen_t optlen = sizeof(optval);
	r_fd = Tcp_connect(server, "14580");
	optval = 1;
	Setsockopt(r_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
	optval = 3;
	Setsockopt(r_fd, SOL_TCP, TCP_KEEPCNT, &optval, optlen);
	optval = 2;
	Setsockopt(r_fd, SOL_TCP, TCP_KEEPIDLE, &optval, optlen);
	optval = 2;
	Setsockopt(r_fd, SOL_TCP, TCP_KEEPINTVL, &optval, optlen);

	snprintf(buf, MAXLEN, "user %s pass %d vers aprs.fi.toudp 1.5 filter p/B p/VR2 p/9\r\n", call, passcode(call));
	Write(r_fd, buf, strlen(buf));

	while (1) {
		n = Readline(r_fd, buf, MAXLEN);
		if (n <= 0) {
			exit(0);
		}
		if (buf[0] == '#')
			continue;
		buf[n] = 0;
		if (debug)
			fprintf(stderr, "S %s", buf);
		if (strstr(buf, "-13>") && (strstr(buf, ",BG6CQ:") == 0))
			sendudp(buf, n, "114.55.54.60", 14580);	// forward -13 to lewei50.comI
		ToMysql(buf, n);
	}
}

void usage()
{
	printf("\naprstomysql v1.0 - aprs relay by james@ustc.edu.cn\n");
	printf("\naprstomysql [ x.x.x.x CALL ]\n\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	char *call = "BG6DA-4";
	char *server = "asia.aprs2.net";

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

	if (argc - i == 2) {
		server = argv[i];
		call = argv[i + 1];
	} else if (argc - i != 0)
		usage();

	signal(SIGCHLD, SIG_IGN);
	if (debug == 0) {
		daemon_init("aprstomysql", LOG_DAEMON);
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
	mysql = connectdb();

	Process(server, call);
	return (0);
}
