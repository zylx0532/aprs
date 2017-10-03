/* resendaprsudp [ -d ] [ -h host ] [ -p port ] [ -i interval ] 
功能：
	从标准输入读入aprs，发送给host:port
	默认是 127.0.0.1 14580
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
char host[MAXLEN];
int port = 14580;
int sleep_t = 0;

#include "sendudp.c"

int main(int argc, char **argv)
{
	int c;
	char buf[MAXLEN];
	int len;
	while ((c = getopt(argc, argv, "dh:p:i:")) != EOF)
		switch (c) {
		case 'd':
			debug = 1;
			break;
		case 'h':
			strncpy(host, optarg, MAXLEN);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'i':
			sleep_t = atoi(optarg);
			break;
		}
	if (host[0] == 0)
		strcpy(host, "127.0.0.1");

	while (fgets(buf, MAXLEN, stdin)) {
		len = strlen(buf);
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
		if (debug)
			printf("got %s", buf);
		sendudp(buf, len, host, port);
		if (sleep_t > 0)
			sleep(sleep_t);
	}
	return 0;
}
