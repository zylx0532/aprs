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

char table = '/';
char symbol = '>';

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
#include "printaddr.c"

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
			if (ibuf[strlen(ibuf) - 1] == '\r')
				ibuf[strlen(ibuf) - 1] = 0;
			if (memcmp(ibuf, call, 16) == 0) {
				char *p;
				fclose(fp);
				strncpy(call, ibuf + 17, 20);
				p = call;
				while (*p && (*p != ' '))
					p++;
				if (*p == ' ') {
					*p = 0;
					p++;
					if (*p && *(p + 1)) {
						table = *p;
						symbol = *(p + 1);
					} else {
						table = '/';
						symbol = '>';
					}
				} else {
					table = '/';
					symbol = '>';
				}
				return call;
			}
		}
		fclose(fp);
	}
	sprintf(call, "GT2UN-9");
	return call;
}

int process_6868(int c_fd, int len)
{
	char abuf[MAXLEN];
	unsigned char buf[MAXLEN];
	char *call;
	static time_t last_tm;
	time_t now_tm;
	int cmd;
	if (Readn(c_fd, buf + 3, len + 2) != len + 2)
		exit(0);
	if (debug)
		dump_pkt(buf + 3, len + 2);
	cmd = buf[15];

	if ((cmd == 0x1a) && (len >= 15)) {	// heart beat 
		snprintf(last_status, 100,
			 "Voltage:%d GSM Signal:%d Satellites:%d/%d %s",
			 buf[3], buf[4], buf[16] == 0 ? 0 : (buf[17] <= 12 ? buf[17] : 12), len - 15,
			 buf[16] == 0 ? "Unlocated" : (buf[16] == 1 ? "Realtime GPS" : (buf[16] == 2 ? "DGPS" : "Unknown")));
		if (debug)
			fprintf(stderr, "status: %s\n", last_status);
		err_msg("keeplive from %02X%02X%02X%02X%02X%02X%02X%02X", buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12]);
		buf[0] = 0x54;
		buf[1] = 0x68;
		buf[2] = 0x1A;
		buf[3] = 0x0d;
		buf[4] = 0x0a;
		Write(c_fd, buf, 5);
		if (debug)
			fprintf(stderr, "heart beat packet\n");
		return 1;
	}
	if ((cmd != 0x10) || (len != 0x25))
		return 0;

	now_tm = time(NULL);
	if (now_tm - last_tm < 3) {
		if (debug)
			fprintf(stderr, "packet interval < 3, skip\n");
		return 1;
	}
	last_tm = now_tm;

	call = imei_call(buf + 5);
	if (call[0] == 0)
		return 1;
	int n = 0, i;
	n = sprintf(abuf, "%s>GT02,TCPIP*:=", call);
	float l;
	l = (((buf[22] * 256 + buf[23]) * 256 + buf[24]) * 256 + buf[25]) / 30000.0;
	n += sprintf(abuf + n, "%02d%05.2f%c%c", (int)(l / 60), l - 60 * ((int)(l / 60)), (buf[39] & 2) == 0 ? 'S' : 'N', table);
	l = (((buf[26] * 256 + buf[27]) * 256 + buf[28]) * 256 + buf[29]) / 30000.0;
	n += sprintf(abuf + n, "%03d%05.2f%c%c", (int)(l / 60), l - 60 * ((int)(l / 60)), (buf[39] & 4) == 0 ? 'W' : 'E', symbol);
	n += sprintf(abuf + n, "%03d/%03d IMEI:", buf[31] * 256 + buf[32], (int)(buf[30] * 0.62));
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
	return 1;
}

int process_gumi(int c_fd, unsigned char cmd)	//0x67 0x67
{
	unsigned char buf[MAXLEN];
	int n;
	int pkt_len;
	char *call;
	static unsigned char imei[20];
	static char last_aprs[200];	// last aprs_head

	n = Readn(c_fd, buf, 2);	// read packet len
	if (n != 2)
		exit(0);
	pkt_len = buf[0] * 256 + buf[1];
	if (pkt_len > MAXLEN - 10)
		if (debug) {
			fprintf(stderr, "too long packet, len=%d\n", pkt_len);
			exit(0);
		}
	n = Readn(c_fd, buf + 5, pkt_len);
	if (n != pkt_len) {
		err_msg("gumi: I want %, read %d\n", pkt_len, n);
		exit(0);
	}
	if (debug) {
		fprintf(stderr, "gumi: 0x67 0x67 cmd=%02X len=%d\n", cmd, pkt_len);
		dump_pkt(buf + 5, n);
	}
	if (cmd == 0x01) {	// login command
		if (pkt_len < 10) {
			if (debug)
				fprintf(stderr, "login cmd, pkt_len should be >=10, but now is %d\n", pkt_len);
			exit(0);
		}
		memcpy(imei, buf + 7, 8);
		if (debug) {
			fprintf(stderr, "IMEI: ");
			int i;
			for (i = 0; i < 8; i++) {
				fprintf(stderr, "%02X", *(imei + i));
			}
			fprintf(stderr, "\n");
		}
		err_msg("login from %02X%02X%02X%02X%02X%02X%02X%02X", imei[0], imei[1], imei[2], imei[3], imei[4], imei[5], imei[6], imei[7]);
		buf[0] = buf[1] = 0x67;
		buf[2] = 1;
		buf[3] = 0;
		buf[4] = 2;
		Write(c_fd, buf, 7);
		return 1;
	}
	if ((cmd == 0x02) || (cmd == 0x04) || (cmd == 0x05) || (cmd == 0x06)) {	// GPS infomation
		if (pkt_len < 25) {
			if (debug)
				fprintf(stderr, "GPS cmd, pkt_len should be >=25, but now is %d\n", pkt_len);
			exit(0);
		}

		int status = buf[7 + 24] & 1;
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
			goto drop_gps;
		}
		last_tm = now_tm;

		call = imei_call((unsigned char *)imei);
		if (call[0] == 0)
			goto drop_gps;
		n = sprintf(last_aprs, "%s>GUMI,TCPIP*:=", call);
		float l;
		l = ntohl(*((int *)(buf + 11))) / 30000.0;
		if (debug)
			fprintf(stderr, "%f\n", l);
		char d;
		if (l < 0) {
			l = -l;
			d = 'S';
		} else
			d = 'N';
		n += sprintf(last_aprs + n, "%02d%05.2f%c%c", (int)(l / 60), l - 60 * ((int)(l / 60)), d, table);

		l = ntohl(*((int *)(buf + 15))) / 30000.0;

		if (debug)
			fprintf(stderr, "%f\n", l);
		if (l < 0) {
			l = -l;
			d = 'W';
		} else
			d = 'E';
		n += sprintf(last_aprs + n, "%03d%05.2f%c%c", (int)(l / 60), l - 60 * ((int)(l / 60)), d, symbol);
		n += sprintf(last_aprs + n, "%03d/%03d", buf[20] * 256 + buf[21], (int)(buf[19] * 0.62));

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
 drop_gps:
		if (cmd == 0x02)
			return 1;
		if ((cmd == 0x04) || (cmd == 0x05)) {
			buf[0] = buf[1] = 0x67;
			buf[2] = cmd;
			buf[3] = 0;
			buf[4] = 2;
			Write(c_fd, buf, 7);
			return 1;
		}
		return 1;
	}
	if (cmd == 0x03) {	// keep live
		buf[0] = buf[1] = 0x67;
		buf[2] = cmd;
		buf[3] = 0;
		buf[4] = 2;
		err_msg("keep live");
		Write(c_fd, buf, 7);
		return 1;
	}
	if (cmd == 0x91) {	// unknow cmd, try empty respons
		buf[0] = buf[1] = 0x67;
		buf[2] = cmd;
		buf[3] = 0;
		buf[4] = 2;
		err_msg("try unknow cmd");
		Write(c_fd, buf, 7);
		return 1;
	}
	err_msg("cmd = %02X, len=%d, I do not know how to deal", cmd, pkt_len);
	return 0;
}

int process_7878(int c_fd, unsigned char pkt_len)
{
	unsigned char buf[MAXLEN];
	int n;
	char *call;
	unsigned char cmd;
	static unsigned char imei[20];
	static char last_aprs[200];	// last aprs_head

	if (pkt_len > MAXLEN - 10)
		if (debug) {
			fprintf(stderr, "too long packet, len=%d\n", pkt_len);
			exit(0);
		}
	n = Readn(c_fd, buf + 3, 1);
	if (n != 1)
		exit(0);
	cmd = buf[3];
	if (debug)
		fprintf(stderr, "gpr_7878, cmd=%02X\n", cmd);

	if (cmd == 0x01) {	// login command
		n = Readn(c_fd, buf + 4, pkt_len - 2);	// 协议文本有错，实际的协议中，包长度是除了7878以外的所有字节，包含最后的0d0a
		if (n != pkt_len - 2)
			exit(0);
		if (debug) {
			fprintf(stderr, "gps_7878: 0x78 0x78 len=%d, login cmd=0x01\n", pkt_len);
			dump_pkt(buf + 3, n + 1);
		}
		if (pkt_len < 10) {
			if (debug)
				fprintf(stderr, "login cmd, pkt_len should be >=10, but now is %d\n", pkt_len);
			exit(0);
		}
		memcpy(imei, buf + 4, 8);
		if (debug) {
			fprintf(stderr, "IMEI: ");
			int i;
			for (i = 0; i < 8; i++) {
				fprintf(stderr, "%02X", *(imei + i));
			}
			fprintf(stderr, "\n");
		}
		buf[0] = buf[1] = 0x78;
		buf[2] = 1;
		buf[3] = 1;
		buf[4] = 0x0d;
		buf[5] = 0x0a;
		if (debug) {
			fprintf(stderr, "login cmd return, len=%d\n", 6);
			dump_pkt(buf, 6);
		}
		Write(c_fd, buf, 6);
		return 1;
	}
	if (cmd == 0x10) {	// GPS infomation
		n = Readn(c_fd, buf + 4, pkt_len + 2);
		if (n != pkt_len + 2)
			exit(0);
		if (debug) {
			fprintf(stderr, "gps_7878: 0x78 0x78 len=%d, gps information cmd=0x10\n", pkt_len);
			dump_pkt(buf + 3, n + 1);
		}
		int satnum = buf[10] & 0xf;

		int status = (buf[20] >> 4) & 1;
		if (debug) {
			fprintf(stderr, "GPS status: %d\n", status);
		}
		static time_t last_tm;
		time_t now_tm;
		now_tm = time(NULL);
		if (now_tm - last_tm < 3) {
			if (debug)
				fprintf(stderr, "packet interval < 3, skip\n");
			goto drop_gps;
		}
		last_tm = now_tm;

		call = imei_call((unsigned char *)imei);
		if (call[0] == 0)
			return 1;
		n = sprintf(last_aprs, "%s>GT7878,TCPIP*:=", call);
		float l;
		l = (((buf[11] * 256 + buf[12]) * 256 + buf[13]) * 256 + buf[14]) / 30000.0;
		n += sprintf(last_aprs + n, "%02d%05.2f%c%c", (int)(l / 60), l - 60 * ((int)(l / 60)), (buf[20] & 4) == 0 ? 'S' : 'N', table);

		l = (((buf[15] * 256 + buf[16]) * 256 + buf[17]) * 256 + buf[18]) / 30000.0;
		n += sprintf(last_aprs + n, "%03d%05.2f%c%c", (int)(l / 60), l - 60 * ((int)(l / 60)), (buf[20] & 8) == 0 ? 'E' : 'W', symbol);
		n += sprintf(last_aprs + n, "%03d/%03d", (buf[20] & 0x3) * 256 + buf[21], (int)(buf[19] * 0.62));

		char abuf[MAXLEN];

		n = sprintf(abuf, "%s IMEI:", last_aprs);
		int i;
		for (i = 6; i < 8; i++)
			n += sprintf(abuf + n, "%02X", *(imei + i));
		n += sprintf(abuf + n, "卫星数:%d\r\n", satnum);
		if (debug)
			fprintf(stderr, "APRS: %s\n", abuf);
		if (strstr(abuf, "GT2UN-9") == 0)	// imei_call
			sendudp(abuf, n, "127.0.0.1", 14580);
		else {
			sendudp(abuf, n, "127.0.0.1", 14582);
			sendudp(abuf, n, "127.0.0.1", 14583);
		}
 drop_gps:
		buf[0] = buf[1] = 0x78;
		buf[2] = 0;
		buf[3] = 0x10;
/*
		buf[4] = 1900 + p->tm_year - 2000;
		buf[5] = (1 + p->tm_mon);
		buf[6] = p->tm_mday;
		buf[7] = p->tm_hour;
		buf[8] = p->tm_min;
		buf[9] = p->tm_sec;
*/
		buf[10] = 0x0d;
		buf[11] = 0x0a;
		if (debug) {
			fprintf(stderr, "gps cmd return, len=%d\n", 12);
			dump_pkt(buf, 12);
		}
		Write(c_fd, buf, 12);
		return 1;
	}
	if (cmd == 0x08) {	// heart beat
		n = Readn(c_fd, buf + 4, pkt_len + 1);
		if (n != pkt_len + 1)
			exit(0);
		if (debug) {
			fprintf(stderr, "gps_7878: 0x78 0x78 len=%d, hart beat cmd=0x08\n", pkt_len);
			dump_pkt(buf + 3, n + 1);
		}
		return 1;
	}
	if (cmd == 0x13) {	// status   
		n = Readn(c_fd, buf + 4, pkt_len);
		if (n != pkt_len)
			exit(0);
		if (debug) {
			fprintf(stderr, "gps_7878: 0x78 0x78 len=%d, status cmd=0x13\n", pkt_len);
			dump_pkt(buf + 3, n + 1);
		}
		return 1;
	}
	if (cmd == 0x30) {	// sync time
		n = Readn(c_fd, buf + 4, pkt_len + 1);
		if (n != pkt_len + 1)
			exit(0);
		if (debug) {
			fprintf(stderr, "gps_7878: 0x78 0x78 len=%d, sync time cmd=0x30\n", pkt_len);
			dump_pkt(buf + 3, n + 1);
		}
		time_t timep;
		struct tm *p;
		time(&timep);
		p = gmtime(&timep);
		buf[0] = buf[1] = 0x78;
		buf[2] = 0x7;
		buf[3] = 0x30;
		buf[4] = (1900 + p->tm_year) / 256;
		buf[5] = (1900 + p->tm_year) % 256;
		buf[6] = (1 + p->tm_mon);
		buf[7] = p->tm_mday;
		buf[8] = p->tm_hour;
		buf[9] = p->tm_min;
		buf[10] = p->tm_sec;
		buf[11] = 0x0d;
		buf[12] = 0x0a;
		if (debug) {
			fprintf(stderr, "gps sync time return, len=%d\n", 13);
			dump_pkt(buf, 13);
		}
		Write(c_fd, buf, 13);
		return 1;
	}
	if (cmd == 0x57) {	// sync data    
		n = Readn(c_fd, buf + 4, pkt_len + 1);
		if (n != pkt_len + 1)
			exit(0);
		if (debug) {
			fprintf(stderr, "gps_7878: 0x78 0x78 len=%d, sync data cmd=0x57\n", pkt_len);
			dump_pkt(buf + 3, n + 1);
		}
//起始位2byte 包长度1byte 协议号1byte 上传间隔2byte 开关1byte 闹钟9byte        勿扰时间开关1byte 勿扰时间9byte      GPS 定时开关1byte GPS 定时时间4byte SOS 爸爸妈妈3 个号码 （长度不定，3B（";"）做分割符 结束位2byte
//        7878 1F           57            0060         01     000000 000000 000000 00              000000000000000000 00 00000000 3B3B3B0D0A
//         0 1  2            3             4 5          6      7 8 9 101112 131415 16              171819202122232425 26 27282930 3132333435  
//上传间隔：BCD 编码，0060，这个为60 秒

		buf[0] = buf[1] = 0x78;
		buf[2] = 0x1f;	//len
		buf[3] = 0x57;	//cmd
		buf[4] = 0x0;	//upload interval
		buf[5] = 0x5;	//upload interval seconds
		buf[6] = 0x1;	//switch
		for (n = 7; n <= 31; n++)
			buf[n] = 0;
		buf[31] = buf[32] = buf[33] = 0x3b;

		buf[34] = 0x0d;
		buf[35] = 0x0a;
		if (debug) {
			fprintf(stderr, "gps sync data return, len=%d\n", 36);
			dump_pkt(buf, 36);
		}

		Write(c_fd, buf, 36);
		return 1;
	}
	n = Readn(c_fd, buf + 4, pkt_len + 1);
	if (n != pkt_len + 1)
		exit(0);
	if (debug) {
		fprintf(stderr, "gps_7878: 0x78 0x78 len=%d\n", pkt_len);
		dump_pkt(buf + 3, n + 2);
	}

	return 0;
}

void Process(int c_fd)
{
	unsigned char buffer[MAXLEN];
	int n;
	int optval;
	int r;
	socklen_t optlen = sizeof(optval);
	optval = 1;
	Setsockopt(c_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
	optval = 3;
	Setsockopt(c_fd, SOL_TCP, TCP_KEEPCNT, &optval, optlen);
	optval = 240;
	Setsockopt(c_fd, SOL_TCP, TCP_KEEPIDLE, &optval, optlen);
	optval = 5;
	Setsockopt(c_fd, SOL_TCP, TCP_KEEPINTVL, &optval, optlen);

	while (1) {
		n = Readn(c_fd, buffer, 3);
		if (n != 3) {
			exit(0);
		}
		if (debug)
			fprintf(stderr, "packet first 3 bytes:: %02X%02X%02X\n", buffer[0], buffer[1], buffer[2]);

		if ((buffer[0] == 0x67) && (buffer[1] == 0x67))	// gumi devices
			r = process_gumi(c_fd, buffer[2]);
		if ((buffer[0] == 0x68) && (buffer[1] == 0x68))	// gt02a
			r = process_6868(c_fd, buffer[2]);
		if ((buffer[0] == 0x78) && (buffer[1] == 0x78))	// 0x78 0x78
			r = process_7878(c_fd, buffer[2]);
		if (r)
			continue;
		if (debug)
			fprintf(stderr, "unknow packet\n");
		err_msg("unknow packet: %02X%02X%02X", buffer[0], buffer[1], buffer[2]);
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
		err_msg("accept from %s", PrintAddr((struct sockaddr *)&sa));
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
