/* aprscmdtcp v1.0 by  james@ustc.edu.cn 2015.12.19
aprscmdtcp [ -d ] local_ip local_port remote_ip remote_port
功能：
        从 14590 tcp端口接收数据
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
#include <string.h>
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

#include "db.h"

#define MAXLEN 16384

int debug = 0;

static char mycall[20];
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

void insertIG(char *buf, int *len)
{
	char *p;
	if (buf[*len - 1] == '\n')
		(*len)--;
	if (buf[*len - 1] == '\r')
		(*len)--;
	if (buf[*len - 1] == '\n')
		(*len)--;
	buf[*len] = 0;

	if ((strncmp(buf, "USER ", 5) == 0)
	    || (strncmp(buf, "user ", 5) == 0)) {
		p = buf + 5;
		int i = 0;
		while ((p < buf + *len) && (*p != ' ')) {
			mycall[i] = toupper(*p);
			if (i == 10)
				break;
			i++;
			p++;
		}
		mycall[i] = 0;
		if (debug)
			fprintf(stderr, "get icall %s\n", mycall);
		return;
	}

	if (mycall[0]) {
		if (buf[strlen(mycall)] == '>') {
			if (strncasecmp(buf, mycall, strlen(mycall)) == 0)
				return;
		}
		p = buf + *len;
		*len += snprintf(p, 15, "/IG:%s", mycall);
		if (debug)
			fprintf(stderr, "from igate %s\n", buf);
	}
}

void relayaprs(char *buf, int len)
{
	char mybuf[MAXLEN];
	strncpy(mybuf, buf, len);
	sendudp(mybuf, len, "120.25.100.30", 14580);	// forward to aprs.hellocq.net
	sendudp(buf, len, "106.15.35.48", 14580);	// forward to ouxun server
	if (strstr(mybuf, "-13>"))
		sendudp(mybuf, len, "114.55.54.60", 14580);	// forward -13 to lewei50.comI
	insertIG(mybuf, &len);
	sendudp(mybuf, len, "127.0.0.1", 14582);	// udptolog
	sendudp(mybuf, len, "127.0.0.1", 14583);	// udptomysql
}

char *my_stpcpy(char *dst, const char *src)
{
	char *q = dst;
	const char *p = src;
	while (*p)
		*q++ = *p++;
	return q;
}

void got_cmd_reply(char *buf, int len)
{
	char mybuf[MAXLEN];
	char sqlbuf[MAXLEN];
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (mycall[0] == 0)
		return;
	strncpy(mybuf, buf, len);
	if (mybuf[len - 1] == '\n')
		len--;
	if (mybuf[len - 1] == '\r')
		len--;
	if (mybuf[len - 1] == '\n')
		len--;
	mybuf[len] = 0;
	snprintf(sqlbuf, MAXLEN,
		 "select id from ykcmd where `call`=\"%s\" and TIMESTAMPDIFF(second,sendtm,now())<=30 and replytm=\"0000-00-00 00:00:00\" order by sendtm limit 1",
		 mycall);
	if (mysql_query(mysql, sqlbuf) != 0) {
		if (debug)
			fprintf(stderr, "sql %s error\n", sqlbuf);
		return;
	}
	result = mysql_store_result(mysql);
	if (result)		// there are rows
	{
		row = mysql_fetch_row(result);
		if (row) {
			char *end;
			end = my_stpcpy(sqlbuf, "update ykcmd set replytm=now(), reply=\"");
			end += mysql_real_escape_string(mysql, end, mybuf, strlen(mybuf));
			end = my_stpcpy(end, "\" where id=");
			end = my_stpcpy(end, row[0]);
			mysql_free_result(result);
			if (debug)
				fprintf(stderr, "sql %s \n", sqlbuf);
			if (mysql_real_query(mysql, sqlbuf, end - sqlbuf) != 0) {
				if (debug)
					fprintf(stderr, "sql %s error\n", sqlbuf);
				return;
			}

		} else
			mysql_free_result(result);
	}

}

void got_new_cmd(char *buf, int len)
{
	char mybuf[MAXLEN];
	char sqlbuf[MAXLEN];
	if (mycall[0] == 0)
		return;		// not login, return
	char *p, *dcall, *sn, *pass, *cmd;
	strncpy(mybuf, buf, len);
	if (mybuf[len - 1] == '\n')
		len--;
	if (mybuf[len - 1] == '\r')
		len--;
	if (mybuf[len - 1] == '\n')
		len--;
	mybuf[len] = 0;
	if (mybuf[0] != '$')
		return;
	dcall = mybuf + 1;
	p = dcall;
	while (*p && *p != ',')
		p++;
	if (*p != ',')
		return;
	*p = 0;
	p++;
	sn = p;
	while (*p && *p != ',')
		p++;
	if (*p != ',')
		return;
	*p = 0;
	p++;
	pass = p;
	while (*p && *p != ',')
		p++;
	if (*p != ',')
		return;
	*p = 0;
	p++;
	cmd = p;
	char *end;
	end = my_stpcpy(sqlbuf, "insert into ykcmd (cmdtm,`call`,sn,pass,cmd,sendtm,replytm) values (now(),\"");
	end += mysql_real_escape_string(mysql, end, dcall, strlen(dcall));
	end = my_stpcpy(end, "\",\"");
	end += mysql_real_escape_string(mysql, end, sn, strlen(sn));
	end = my_stpcpy(end, "\",\"");
	end += mysql_real_escape_string(mysql, end, pass, strlen(pass));
	end = my_stpcpy(end, "\",\"");
	end += mysql_real_escape_string(mysql, end, cmd, strlen(cmd));
	end = my_stpcpy(end, "\",\"0000-00-00 00:00:00\",\"0000-00-00 00:00:00\")");
	if (debug)
		fprintf(stderr, "sql %s \n", sqlbuf);
	if (mysql_real_query(mysql, sqlbuf, end - sqlbuf) != 0) {
		if (debug)
			fprintf(stderr, "sql %s error\n", sqlbuf);
	}
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
	char sqlbuf[MAXLEN];
	MYSQL_RES *result;
	MYSQL_ROW row;

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
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		m = Select(max_fd + 1, &rset, NULL, NULL, &tv);

		// if (m == 0) continue;

		if (FD_ISSET(r_fd, &rset)) {
			char buffer[MAXLEN];
			n = recv(r_fd, buffer, MAXLEN, 0);
			if (n <= 0) {
				PrintStats();
				exit(0);
			}
			Write(c_fd, buffer, n);
			rfwd += n;
		}
		if (FD_ISSET(c_fd, &rset)) {
			static char buffer[MAXLEN];
			static int lastread = 0;
			n = recv(c_fd, buffer + lastread, MAXLEN - lastread - 1 - 25, 0);
			if (n <= 0) {
				PrintStats();
				exit(0);
			}
			buffer[lastread + n] = 0;
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
				if (p[0] == '&') {
					if (debug)
						fprintf(stderr, "got & reply\n");
					got_cmd_reply(p, s - p + 1);
				} else if (p[0] == '$') {
					if (debug)
						fprintf(stderr, "got $ cmd\n");
					got_new_cmd(p, s - p + 1);
				} else {
					Write(r_fd, p, s - p + 1);
					relayaprs(p, s - p + 1);
				}
				p = s + 1;
			}
			if ((p - buffer) < n) {
				lastread = n - (p - buffer);
				memcpy(buffer, p, lastread);
			} else
				lastread = 0;
		}
		// try send command to client
		if (mycall[0] == 0)
			continue;	// send command after login 
		snprintf(sqlbuf, MAXLEN,
			 "select id,concat('$',`call`,',',sn,',',pass,',',cmd) from ykcmd where `call`=\"%s\" and sendtm=\"0000-00-00 00:00:00\" order by cmdtm limit 1",
			 mycall);
		if (mysql_query(mysql, sqlbuf) != 0) {
			if (debug)
				fprintf(stderr, "sql %s error\n", sqlbuf);
			continue;
		}
		result = mysql_store_result(mysql);
		if (result)	// there are rows
		{
			char cmdbuf[MAXLEN];
			row = mysql_fetch_row(result);
			if (row) {
				snprintf(cmdbuf, MAXLEN, "%s\r\n", row[1]);
				if (debug)
					fprintf(stderr, "cmd %s \n", cmdbuf);
				Write(c_fd, cmdbuf, strlen(cmdbuf));
				snprintf(sqlbuf, MAXLEN, "update ykcmd set sendtm=now() where id=%s", row[0]);
				mysql_free_result(result);
				if (debug)
					fprintf(stderr, "sql %s \n", sqlbuf);
				if (mysql_query(mysql, sqlbuf) != 0) {
					if (debug)
						fprintf(stderr, "sql %s error\n", sqlbuf);
					return;
				}

			} else
				mysql_free_result(result);
		}
	}
}

void usage()
{
	printf("\naprscmdtcp v1.0 - aprs relay by james@ustc.edu.cn\n");
	printf("aprscmdtcp [ -d ] [ local_ip local_port remote_ip remote_port ]\n");
	printf("default is: aprscmdtcp 0.0.0.0 14590 asia.aprs2.net 14580\n\n");
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
		lport = "14590";
		raddr = "asia.aprs2.net";
		rport = "14580";
	} else if (argc - i == 4) {
		laddr = argv[i];
		lport = argv[i + 1];
		raddr = argv[i + 2];
		rport = argv[i + 3];
	} else
		usage();

	printf("aprsrcmdtcp %s:%s -> %s:%s\n", laddr, lport, raddr, rport);
	signal(SIGCHLD, SIG_IGN);

	if (debug == 0)
		daemon_init("aprscmdtcp", LOG_DAEMON);

	listen_fd = Tcp_listen(laddr, lport, (socklen_t *) & llen);

	while (1) {
		struct sockaddr sa;
		int slen;
		slen = sizeof(sa);
		c_fd = Accept(listen_fd, &sa, (socklen_t *) & slen);
		if (debug) {
			mysql = connectdb();
			Process(c_fd);
		} else {
			if (Fork() == 0) {
				Close(listen_fd);
				mysql = connectdb();
				Process(c_fd);
			}
		}
		Close(c_fd);
	}
}
