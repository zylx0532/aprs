/* aprstcp v1.0 by  james@ustc.edu.cn 2015.12.19

aprstcp [ -d ] local_ip local_port remote_ip remote_port
功能：
	从 14580 tcp端口接收数据
	使用TCP转发给asia.aprs2.net
	使用UDP转发给以下端口
		127.0.0.1 14582
		127.0.0.1 14583
        	120.25.100.30 14580 (aprs.helloce.net)
        	106.15.35.48  14580 (欧讯服务器)
        	如果SSID中有-13，发给
        	114.55.54.60  14580（lewei50.com）

	aprscmdtcp从14590 tcp端口接收数据，并且会处理命令的传递

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

#define MAXLEN 16384

#define PORT 14580

int debug = 0;

char *laddr, *lport, *raddr, *rport;
unsigned long fwd, rfwd;
int r_fd, c_fd;
char scaddr[MAXLEN], sladdr[MAXLEN], sraddr[MAXLEN], srcaddr[MAXLEN];

char *PrintAddr(struct sockaddr *sa)
{
	struct sockaddr_in *sa_in;
	struct sockaddr_in6 *sa_in6;
	static char buf[MAXLEN];
	char buf2[MAXLEN];

	if (sa->sa_family == AF_INET) {
		sa_in = (struct sockaddr_in *)sa;
		snprintf(buf, MAXLEN, "%s:%d", inet_ntop(sa_in->sin_family, &sa_in->sin_addr, buf2, MAXLEN), ntohs(sa_in->sin_port));
	} else if (sa->sa_family == AF_INET6) {
		sa_in6 = (struct sockaddr_in6 *)sa;
		snprintf(buf, MAXLEN, "%s:%d", inet_ntop(sa_in6->sin6_family, &sa_in6->sin6_addr, buf2, MAXLEN), ntohs(sa_in6->sin6_port));
	} else
		snprintf(buf, MAXLEN, "unknow family %d", sa->sa_family);
	return buf;
}

void PrintStats(void)
{
	syslog(LOG_INFO, "%s->%s ", scaddr, sladdr);
	syslog(LOG_INFO, "==> %s->%s\n", srcaddr, sraddr);
	syslog(LOG_INFO, "===> %8lu bytes\n", fwd);
	syslog(LOG_INFO, "<=== %8lu bytes\n", rfwd);

}

#include "sendudp.c"

void relayaprs(char *buf, int len)
{
	sendudp(buf, len, "120.25.100.30", 14580);	// forward to aprs.hellocq.net
	sendudp(buf, len, "106.15.35.48", 14580);	// forward to ouxun server
	sendudp(buf, len, "127.0.0.1", 14582);	// udptolog
	sendudp(buf, len, "127.0.0.1", 14583);	// udptomysql
	if (strstr(buf, "-13>"))
		sendudp(buf, len, "114.55.54.60", 14580);	// forward -13 to lewei50.comI

}

void Process(int c_fd)
{
	fd_set rset;
	struct timeval tv;
	int m, n;
	int max_fd;
	struct sockaddr_in6 sa;
	int salen;
	fwd = rfwd = 0;

	r_fd = Tcp_connect(raddr, rport);

	scaddr[0] = sladdr[0] = sraddr[0] = srcaddr[0] = 0;
	salen = sizeof(sa);
	if (getpeername(c_fd, (struct sockaddr *)&sa, (socklen_t *) & salen) == 0)
		strncpy(scaddr, PrintAddr((struct sockaddr *)&sa), MAXLEN);
	salen = sizeof(sa);
	if (getsockname(c_fd, (struct sockaddr *)&sa, (socklen_t *) & salen) == 0)
		strncpy(sladdr, PrintAddr((struct sockaddr *)&sa), MAXLEN);

	salen = sizeof(sa);
	if (getpeername(r_fd, (struct sockaddr *)&sa, (socklen_t *) & salen) == 0)
		strncpy(sraddr, PrintAddr((struct sockaddr *)&sa), MAXLEN);
	salen = sizeof(sa);
	if (getsockname(r_fd, (struct sockaddr *)&sa, (socklen_t *) & salen) == 0)
		strncpy(srcaddr, PrintAddr((struct sockaddr *)&sa), MAXLEN);

	int enable = 1;
	setsockopt(c_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable));
	setsockopt(r_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable));

	while (1) {
		FD_ZERO(&rset);
		FD_SET(c_fd, &rset);
		FD_SET(r_fd, &rset);
		max_fd = max(c_fd, r_fd);
		tv.tv_sec = 300;
		tv.tv_usec = 0;

		m = Select(max_fd + 1, &rset, NULL, NULL, &tv);

		if (m == 0)
			continue;

		if (FD_ISSET(r_fd, &rset)) {
			char buffer[MAXLEN];
			n = recv(r_fd, buffer, MAXLEN, 0);
			if (n <= 0) {
				PrintStats();
				exit(0);
			}
			buffer[n] = 0;
			if (debug)
				fprintf(stderr, "from server: %s", buffer);
			Write(c_fd, buffer, n);
			rfwd += n;
		}
		if (FD_ISSET(c_fd, &rset)) {
			static char buffer[MAXLEN];
			static int lastread = 0;
			n = recv(c_fd, buffer + lastread, MAXLEN - lastread - 1, 0);
			if (n <= 0) {
				PrintStats();
				exit(0);
			}
			buffer[lastread + n] = 0;
			if (debug)
				fprintf(stderr, "from client: %s", buffer);
			Write(r_fd, buffer + lastread, n);
			fwd += n;
			char *p, *s;
			n = lastread + n;
			p = buffer;
			while (1) {
				if ((p - buffer) >= n)
					break;
				s = strchr(p, '\n');
				if (s == NULL)
					s = strchr(p, '\r');
				if (s == NULL)
					break;
				relayaprs(p, s - p + 1);
				p = s + 1;
			}
			if ((p - buffer) < n) {
				lastread = n - (p - buffer);
				memcpy(buffer, p, lastread);
			} else
				lastread = 0;
		}
	}
}

void usage()
{
	printf("\naprstcp v1.0 - aprs relay by james@ustc.edu.cn\n");
	printf("aprstcp [ -d ] [ local_ip local_port remote_ip remote_port ]\n");
	printf("default is: aprstcp 0.0.0.0 14580 asia.aprs2.net 14580\n\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	int listen_fd;
	int llen;
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

	if (argc - i == 0) {
		laddr = "0.0.0.0";
		lport = "14580";
		raddr = "asia.aprs2.net";
		rport = "14580";
	} else if (argc - i == 4) {
		laddr = argv[i];
		lport = argv[i + 1];
		raddr = argv[i + 2];
		rport = argv[i + 3];
	} else
		usage();
	printf("aprstcp %s:%s -> %s:%s\n", laddr, lport, raddr, rport);
	signal(SIGCHLD, SIG_IGN);

	if (debug == 0)
		daemon_init("aprstcp", LOG_DAEMON);

	listen_fd = Tcp_listen(laddr, lport, (socklen_t *) & llen);

	while (1) {
		struct sockaddr sa;
		int slen;
		slen = sizeof(sa);
		c_fd = Accept(listen_fd, &sa, (socklen_t *) & slen);
		if (debug)
			Process(c_fd);
		else {
			if (Fork() == 0) {
				Close(listen_fd);
				Process(c_fd);
			}
		}
		Close(c_fd);
	}
}
