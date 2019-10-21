#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLEN 4096
int debug =1;
#include "util.c"

testutil(char *str)
{
	char *s;
	int h, slen;
	aprspacket_high_to_low(str,strlen(str),&h,&s,&slen);
	printf("old str=%s\n",str);
	printf("low str=%s\n",s);
	if(h) {
		printf("High, change to trans\n");
		aprspacket_gps_to_trans(s,strlen(s),&s,&slen);
		printf("tra str=%s\n",s);
	}
	printf("\n");
}
int main (int argc, char **argv) {
	testutil("user BH1IZK-3 pass 23208 vers iGnss.cc A1 V1 SV20181228\r\nBH1IZK-3>APA1V1,WIDE1-1:!4004.55N/11617.86E&APRS 144.640MHz@1200bps\r\n");
	testutil("BG6CQ-9>AP51WX:,3134.10123N/12020.10456E_000/003g006t064r000p000h71b10090,086,119,138,057,078,091,000051WS5 23.8V");
	testutil("BG6CQ-9>APRS,TCPIP*:!/>Ez#l,q=> PulseModem A");
	exit(0);
}
