/* aprsudp [ -d ]
功能：
	从 14580 UDP端口接收数据
	使用UDP转发给以下端口
	127.0.0.1 14581 
	127.0.0.1 14582
	127.0.0.1 14583
	120.25.100.30 14580 (aprs.helloce.net)
	106.15.35.48  14580 (欧讯服务器)
	如果SSID中有-13，发给
	114.55.54.60  14580（lewei50.com）
*/

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>

#include "sock.h"

#define MAXLEN 16384
#define PORT 14580

int debug = 0;

#include "sendudp.c"

void insertU(char *buf, int *len)
{
	char *p;
	if (buf[*len - 1] == '\n')
		(*len)--;
	if (buf[*len - 1] == '\r')
		(*len)--;
	if (buf[*len - 1] == '\n')
		(*len)--;
	buf[*len] = 0;

	p = buf + *len;
	*p = '/';
	*(p + 1) = 'U';
	*(p + 2) = 'D';
	*(p + 3) = 'P';
	*(p + 4) = 0;
	(*len) += 4;
}

void relayaprs(char *buf, int len)
{
	if (debug)
		fprintf(stderr, "recv len=%d from UDP: %s", len, buf);
	sendudp(buf, len, "127.0.0.1", 14581);	// forward to asia.aprs2.net
	sendudp(buf, len, "120.25.100.30", 14580);	// forward to aprs.hellocq.net
	sendudp(buf, len, "106.15.35.48", 14580);	// forward to ouxun server
	if (strstr(buf, "-13>"))
		sendudp(buf, len, "114.55.54.60", 14580);	// forward -13 to lewei50.comI
	insertU(buf, &len);
	sendudp(buf, len, "127.0.0.1", 14582);	// udptolog
	sendudp(buf, len, "127.0.0.1", 14583);	// udptomysql 
}

int main(int argc, char **argv)
{
	struct sockaddr_in si_me, si_other;
	int s, slen = sizeof(si_other);
	if (argc >= 2)
		debug = 1;
	if (debug == 0)
		daemon_init("aprsudp", LOG_DAEMON);
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		err_sys("socket");

	memset((char *)&si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (const struct sockaddr *)&si_me, sizeof(si_me)) == -1)
		err_sys("bind");

	while (1) {
		char buf[MAXLEN];
		int len;
		len = recvfrom(s, buf, MAXLEN - 10, 0, (struct sockaddr *)&si_other, (socklen_t *) & slen);
		if (len < 10)
			continue;
		buf[len] = 0;
		if ((buf[len - 1] == '\n') || (buf[len - 1] == '\r'))
			len--;
		if ((buf[len - 1] == '\n') || (buf[len - 1] == '\r'))
			len--;
		buf[len] = 0x0d;
		buf[len + 1] = 0x0a;
		buf[len + 2] = 0;
		len += 2;
		if (strncmp(buf, "user", 4) == 0) {
			char *p = strstr(buf, " pass ");
			if (p) {
				p += 6;
				while (isdigit(*p)) {
					*p = '*';
					p++;
				}
			}
		}
		if ((strncmp(buf, "user", 4) != 0)
		    && (strstr(buf, "AP51G2:>51G2 HELLO APRS-51G2") == 0))
			relayaprs(buf, len);
	}
	return 0;
}
