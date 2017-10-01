/* gt02 v1.0 by  james@ustc.edu.cn 2016.05.11
   accept GT02 connection on tcp port 8821
	if IMEI was found in file imei_call.txt, 
		send to udp 127.0.0.1:14580, then go to aprs.fi
	else 
		use GT2UN-9 as call and send to 127.0.0.1:14582, 127.0.0.1:14583 for local display
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
#include <math.h>

#define MAXLEN 16384

char last_status[MAXLEN];
int debug = 0;

void dump_pkt(unsigned char *buf, int len)
{
	int i;
	fprintf(stderr, "pkt len=%d:", len);
	for (i = 0; i < len; i++) {
		if (i % 8 == 0)
			fprintf(stderr, " ");
		fprintf(stderr, "%02X", *(buf + i));
	}
	fprintf(stderr, "\n");
}

#include "sendudp.c"

char *imei_call(unsigned char *imei)
{				// imei 8 bytes
	FILE *fp;
	static char call[MAXLEN];
	char ibuf[MAXLEN];
	int i;

	for (i = 0; i < 8; i++) {
		sprintf(call + 2 * i, "%02X", *(imei + i));
	}
	if (debug)
		fprintf(stderr, "%s\n", call);
	fp = fopen("/usr/src/aprs/imei_call.txt", "r");	// IMEI 0102030405060708 BG?-YYY
	if (fp == NULL) {
		if (debug)
			fprintf(stderr, "open imei_call.txt error\n");
	} else {
		while (fgets(ibuf, MAXLEN, fp)) {
			if (strlen(ibuf) < 8)
				continue;
			if (ibuf[strlen(ibuf) - 1] == '\n')
				ibuf[strlen(ibuf) - 1] = 0;
			if (memcmp(ibuf, call, 16) == 0) {
				fclose(fp);
				strncpy(call, ibuf + 17, 10);
				return call;
			}
		}
		fclose(fp);
	}
	sprintf(call, "GT2UN-9");
	return call;
}

void processaprs(unsigned char *buf, int len)
{
	char abuf[MAXLEN];
	char *call;
	static time_t last_tm;
	time_t now_tm;
	now_tm = time(NULL);
	if (now_tm - last_tm < 5) {
		if (debug)
			fprintf(stderr, "packet interval < 5, skip\n");
		return;
	}
	last_tm = now_tm;

	call = imei_call(buf + 5);
	if (call[0] == 0)
		return;
	int n = 0, i;
	n = sprintf(abuf, "%s>GT02,TCPIP*:=", call);
	float l;
	l = ((buf[22] * 256 + buf[23]) * 256 + buf[24]) * 256 + buf[25];
	l = l / 30000;
	if (debug)
		fprintf(stderr, "%f\n", l);
	n += sprintf(abuf + n, "%02d%05.2f%c/", (int)(l / 60), l - 60 * ((int)(l / 60)), (buf[39] & 2) == 0 ? 'S' : 'N');
	l = ((buf[26] * 256 + buf[27]) * 256 + buf[28]) * 256 + buf[29];
	l = l / 30000;
	if (debug)
		fprintf(stderr, "%f, %d\n", l, (int)(l / 60));
	n += sprintf(abuf + n, "%03d%05.2f%c>", (int)(l / 60), l - 60 * ((int)(l / 60)), (buf[39] & 4) == 0 ? 'W' : 'E');
	n += sprintf(abuf + n, "%03d/%03d", buf[31] * 256 + buf[32], (int)(buf[30] * 0.62));
	n += sprintf(abuf + n, "IMEI:");
	for (i = 6; i < 8; i++)
		n += sprintf(abuf + n, "%02X", *(buf + 5 + i));
	n += sprintf(abuf + n, " %s\r\n", last_status);
	if (debug)
		fprintf(stderr, "APRS: %s\n", abuf);
	if (strstr(abuf, "GT2UN-9") == 0)	// imei_call
		sendudp(abuf, n, "127.0.0.1", 14580);
	else {
		sendudp(abuf, n, "127.0.0.1", 14582);
		sendudp(abuf, n, "127.0.0.1", 14583);
	}
}

void process_gumi(int c_fd, unsigned char cmd)
{
	unsigned char buffer[MAXLEN];
	int n;
	int pkt_len;
	char *call;
	static char imei[20];
	static char last_aprs[200];	// last aprs_head

	n = Readn(c_fd, buffer, 2);	// read packet len
	if (n <= 0)
		exit(0);
	pkt_len = buffer[0] * 256 + buffer[1];
	if (pkt_len > MAXLEN - 10)
		if (debug) {
			fprintf(stderr, "too long packet, len=%d\n", pkt_len);
			exit(0);
		}
	n = Readn(c_fd, buffer + 5, pkt_len);
	if (n != pkt_len)
		exit(0);
	if (debug) {
		fprintf(stderr, "gumi: 0x67 0x67 cmd=%02X len=%d\n", cmd, pkt_len);
		dump_pkt(buffer + 5, n);
	}
	if (cmd == 0x01) {	// login command
		if (pkt_len < 10) {
			if (debug)
				fprintf(stderr, "login cmd, pkt_len should be >=10, but now is %d\n", pkt_len);
			exit(0);
		}
		memcpy(imei, buffer + 7, 8);
		if (debug) {
			fprintf(stderr, "IMEI: ");
			int i;
			for (i = 0; i < 8; i++) {
				fprintf(stderr, "%02X", *(imei + i));
			}
			fprintf(stderr, "\n");
		}
		err_msg("keeplive from %02X%02X%02X%02X%02X%02X%02X%02X",
				imei[0],imei[1],imei[2],imei[3],imei[4],imei[5],imei[6],imei[7]);
		buffer[0] = buffer[1] = 0x67;
		buffer[2] = 1;
		buffer[3] = 0;
		buffer[4] = 2;
		Write(c_fd, buffer, 7);
		return;
	}
	if ((cmd == 0x02) || (cmd == 0x04) || (cmd == 0x05) || (cmd == 0x06)) {	// GPS infomation
		if (pkt_len < 25) {
			if (debug)
				fprintf(stderr, "GPS cmd, pkt_len should be >=25, but now is %d\n", pkt_len);
			exit(0);
		}

		int status = buffer[7 + 24] & 1;
		if (debug) {
			fprintf(stderr, "GPS status: %d\n", status);
		}
		// if(status==0) return;
		static time_t last_tm;
		time_t now_tm;
		now_tm = time(NULL);
		if (now_tm - last_tm < 3) {
			if (debug)
				fprintf(stderr, "packet interval < 3, skip\n");
			return;
		}
		last_tm = now_tm;

		call = imei_call((unsigned char *)imei);
		if (call[0] == 0)
			return;
		n = sprintf(last_aprs, "%s>GUMI,TCPIP*:=", call);
		int tmp;
		float l;
		tmp = ntohl(*((int *)(buffer + 11)));
		l = tmp / 30000.0;
		if (debug)
			fprintf(stderr, "%f\n", l);
		char d;
		if (l < 0) {
			l = -l;
			d = 'S';
		} else
			d = 'N';
		n += sprintf(last_aprs + n, "%02d%05.2f%c/", (int)(l / 60), l - 60 * ((int)(l / 60)), d);

		tmp = ntohl(*((int *)(buffer + 15)));
		l = tmp / 30000.0;

		if (debug)
			fprintf(stderr, "%f\n", l);
		if (l < 0) {
			l = -l;
			d = 'W';
		} else
			d = 'E';
		n += sprintf(last_aprs + n, "%03d%05.2f%c>", (int)(l / 60), l - 60 * ((int)(l / 60)), d);
		n += sprintf(last_aprs + n, "%03d/%03d", buffer[20] * 256 + buffer[21], (int)(buffer[19] * 0.62));

		char abuf[MAXLEN];

		n = sprintf(abuf, "%s IMEI:", last_aprs);
		int i;
		for (i = 6; i < 8; i++)
			n += sprintf(abuf + n, "%02X", *(imei + i));
		n += sprintf(abuf + n, "\r\n");
		if (debug)
			fprintf(stderr, "APRS: %s\n", abuf);
		if (strstr(abuf, "GT2UN-9") == 0)	// imei_call
			sendudp(abuf, n, "127.0.0.1", 14580);
		else {
			sendudp(abuf, n, "127.0.0.1", 14582);
			sendudp(abuf, n, "127.0.0.1", 14583);
		}
		if (cmd == 0x02)
			return;
		if ((cmd == 0x04) || (cmd == 0x05)) {
			buffer[0] = buffer[1] = 0x67;
			buffer[2] = cmd;
			buffer[3] = 0;
			buffer[4] = 2;
			Write(c_fd, buffer, 7);
			return;
		}
		return;
	}
	if (cmd == 0x03) {	// keep live
		buffer[0] = buffer[1] = 0x67;
		buffer[2] = cmd;
		buffer[3] = 0;
		buffer[4] = 2;
		Write(c_fd, buffer, 7);
		return;
	}
}

void process_7878(int c_fd, unsigned char pkt_len)
{
	unsigned char buffer[MAXLEN];
	int n;
	char *call;
	unsigned char cmd;
	static char imei[20];
	static char last_aprs[200];	// last aprs_head

	if (pkt_len > MAXLEN - 10)
		if (debug) {
			fprintf(stderr, "too long packet, len=%d\n", pkt_len);
			exit(0);
		}
	n = Readn(c_fd, buffer + 3, 1);
	if (n != 1)
		exit(0);
	cmd = buffer[3];
	if (cmd == 0x17) {

	}
	n = Readn(c_fd, buffer + 4, pkt_len + 1);
	if (n != pkt_len + 1)
		exit(0);
	if (debug) {
		fprintf(stderr, "gps_7878: 0x78 0x78 len=%d\n", pkt_len);
		dump_pkt(buffer + 3, n);
	}
	if (cmd == 0x01) {	// login command
		if (pkt_len < 10) {
			if (debug)
				fprintf(stderr, "login cmd, pkt_len should be >=10, but now is %d\n", pkt_len);
			exit(0);
		}
		memcpy(imei, buffer + 4, 8);
		if (debug) {
			fprintf(stderr, "IMEI: ");
			int i;
			for (i = 0; i < 8; i++) {
				fprintf(stderr, "%02X", *(imei + i));
			}
			fprintf(stderr, "\n");
		}
		buffer[0] = buffer[1] = 0x78;
		buffer[2] = 1;
		buffer[3] = 1;
		buffer[4] = 0x0d;
		buffer[5] = 0x0a;
		Write(c_fd, buffer, 6);
		return;
	}
	if (cmd == 0x10) {	// GPS infomation
		int stanum = buffer[10];

		int status = (buffer[20] >> 4) & 1;
		if (debug) {
			fprintf(stderr, "GPS status: %d\n", status);
		}
		static time_t last_tm;
		time_t now_tm;
		now_tm = time(NULL);
		if (now_tm - last_tm < 3) {
			if (debug)
				fprintf(stderr, "packet interval < 3, skip\n");
			return;
		}
		last_tm = now_tm;

		call = imei_call((unsigned char *)imei);
		if (call[0] == 0)
			return;
		n = sprintf(last_aprs, "%s>GT7878,TCPIP*:=", call);
		float l;
		l = ((buffer[11] * 256 + buffer[12]) * 256 + buffer[13]) * 256 + buffer[14];
		l = l / 30000;
		if (debug)
			fprintf(stderr, "%f\n", l);
		n += sprintf(last_aprs + n, "%02d%05.2f%c/", (int)(l / 60), l - 60 * ((int)(l / 60)), (buffer[20] & 4) == 0 ? 'S' : 'N');

		l = ((buffer[15] * 256 + buffer[16]) * 256 + buffer[17]) * 256 + buffer[18];
		l = l / 30000;
		if (debug)
			fprintf(stderr, "%f\n", l);
		n += sprintf(last_aprs + n, "%03d%05.2f%c>", (int)(l / 60), l - 60 * ((int)(l / 60)), (buffer[20] & 8) == 0 ? 'E' : 'W');
		n += sprintf(last_aprs + n, "%03d/%03d", (buffer[20] & 0x3) * 256 + buffer[21], (int)(buffer[19] * 0.62));

		char abuf[MAXLEN];

		n = sprintf(abuf, "%s IMEI:", last_aprs);
		int i;
		for (i = 6; i < 8; i++)
			n += sprintf(abuf + n, "%02X", *(imei + i));
		n += sprintf(abuf + n, "\r\n");
		if (debug)
			fprintf(stderr, "APRS: %s\n", abuf);
		if (strstr(abuf, "GT2UN-9") == 0)	// imei_call
			sendudp(abuf, n, "127.0.0.1", 14580);
		else {
			sendudp(abuf, n, "127.0.0.1", 14582);
			sendudp(abuf, n, "127.0.0.1", 14583);
		}
		time_t timep;
		struct tm *p;
		time(&timep);
		p = localtime(&timep);
		buffer[0] = buffer[1] = 0x78;
		buffer[2] = 0;
		buffer[3] = 0x10;
		buffer[4] = 1900 + p->tm_year - 2000;
		buffer[5] = (1 + p->tm_mon);
		buffer[6] = p->tm_mday;
		buffer[7] = p->tm_hour;
		buffer[8] = p->tm_min;
		buffer[9] = p->tm_sec;
		buffer[10] = 0x0d;
		buffer[11] = 0x0a;
		Write(c_fd, buffer, 12);
		return;
	}
	if (cmd == 0x08) {	// keep live
		return;
/*
		buffer[0] = buffer[1] = 0x67;
		buffer[2] = cmd;
		buffer[3] = 0;
		buffer[4] = 2;
		Write(c_fd, buffer, 7);
		return;
*/
	}
}

void Process(int c_fd)
{
	unsigned char buffer[MAXLEN];
	int n;
	int optval;
	socklen_t optlen = sizeof(optval);
	optval = 200;
	Setsockopt(c_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
	optval = 3;
	Setsockopt(c_fd, SOL_TCP, TCP_KEEPCNT, &optval, optlen);
	optval = 120;
	Setsockopt(c_fd, SOL_TCP, TCP_KEEPIDLE, &optval, optlen);
	optval = 2;
	Setsockopt(c_fd, SOL_TCP, TCP_KEEPINTVL, &optval, optlen);

	while (1) {
		n = Readn(c_fd, buffer, 3);
		if (n <= 0) {
			exit(0);
		}
		if ((buffer[0] == 0x67) && (buffer[1] == 0x67)) {	// gumi devices
			process_gumi(c_fd, buffer[2]);
			continue;
		}
		if ((buffer[0] == 0x78) && (buffer[1] == 0x78)) {	// 0x78 0x78
			process_7878(c_fd, buffer[2]);
			continue;
		}

		n += Readn(c_fd, buffer + 3, buffer[2] + 2);
		if (debug)
			dump_pkt(buffer, n);
		buffer[n] = 0;
		if ((n >= 15) && (buffer[0] == 0x68) && (buffer[1] == 0x68)
		    && (buffer[15] == 0x1a)) {	// heart beat 
			snprintf(last_status, 100,
				 "电压:%d GSM信号:%d 卫星数:%d/%d %s",
				 buffer[3], buffer[4], buffer[17], buffer[2] - 15,
				 buffer[16] == 0 ? "未定位" : (buffer[16] == 1 ? "实时GPS" : (buffer[16] == 2 ? "差分GPS" : "未知")));
			if (debug)
				fprintf(stderr, "status: %s\n", last_status);
			err_msg("keeplive from %02X%02X%02X%02X%02X%02X%02X%02X",
				buffer[5], buffer[6], buffer[7], buffer[8], 
				buffer[9], buffer[10], buffer[11], buffer[12]);
			buffer[0] = 0x54;
			buffer[1] = 0x68;
			buffer[2] = 0x1A;
			buffer[3] = 0x0d;
			buffer[4] = 0x0a;
			Write(c_fd, buffer, 5);
			if (debug) {
				fprintf(stderr, "heart beat packet\n");
				fprintf(stderr, "send back heart beat\n");
			}
			continue;
		}
		if ((n == 42) && (buffer[0] == 0x68) && (buffer[1] == 0x68)
		    && (buffer[2] == 0x25) &&
//                      (buffer[3]==0x0) && (buffer[4]==0x0) && 
		    (buffer[15] == 0x10)) {	// gps status 
			if (debug)
				fprintf(stderr, "GPS status packet\n");
			processaprs(buffer, n);
			continue;
		}
		if (debug)
			fprintf(stderr, "unknow packet\n");
		err_msg("unknow packet: %02X%02X\n",buffer[0],buffer[1]);
	}
}

void usage()
{
	printf("\ngt02 v1.0 - gt02 to aprs by james@ustc.edu.cn\n");
	printf("\ngt02 [ -d ] [ -p port ] default port is 8821\n\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	int c;
	int listen_fd;
	int c_fd;
	int llen;
	char *listen_port = "8821";
	while ((c = getopt(argc, argv, "dp:")) != EOF)
		switch (c) {
		case 'd':
			debug = 1;
			break;
		case 'p':
			listen_port = optarg;
			break;
		default:
			usage();
			exit(0);
		}

	signal(SIGCHLD, SIG_IGN);

	if (debug == 0)
		daemon_init("gt02", LOG_DAEMON);

	listen_fd = Tcp_listen("0.0.0.0", listen_port, (socklen_t *) & llen);

	while (1) {
		struct sockaddr sa;
		int slen;
		slen = sizeof(sa);
		c_fd = Accept(listen_fd, &sa, (socklen_t *) & slen);
		if (debug) {
			fprintf(stderr, "get connection:\n");
			Process(c_fd);
			fprintf(stderr, "connection lost\n");
		} else {
			if (Fork() == 0) {
				Close(listen_fd);
				Process(c_fd);
			}
		}
		Close(c_fd);
	}
}
