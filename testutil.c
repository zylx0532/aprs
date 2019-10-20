#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLEN 4096
int debug =1;
#include "util.c"

int main (int argc, char **argv) {

	char *s;
	int h, slen;
	char * str="user BH1IZK-3 pass 23208 vers iGnss.cc A1 V1 SV20181228\r\nBH1IZK-3>APA1V1,WIDE1-1:!4004.55N/11617.86E&APRS 144.640MHz@1200bps\r\n";
	aprspacket_high_to_low(str,57,&h,&s,&slen);
	
//	char *str="BG6CQ-9>AP51WX:,3134.10123N/12020.10456E_000/003g006t064r000p000h71b10090,086,119,138,057,078,091,000051WS5 23.8V";
//	aprspacket_high_to_low(str,70,&h,&s,&slen);
	printf("str=%s\n",str);
	printf("str=%s\n",s);
	exit(0);
}
