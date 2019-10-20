// code from https://www.biaodianfu.com/coordinate-system.html
//
// Copyright (C) 1000 - 9999 Somebody Anonymous
// NO WARRANTY OR GUARANTEE
//

#include <math.h>

const double pi = 3.14159265358979324;

//
// Krasovsky 1940
//
// a = 6378245.0, 1/f = 298.3
// b = a * (1 - f)
// ee = (a^2 - b^2) / a^2;

const double a = 6378245.0;
const double ee = 0.00669342162296594323;

int outOfChina(double lat, double lon)
{
	if (lon < 72.004 || lon > 137.8347)
		return 1;
	if (lat < 0.8293 || lat > 55.8271)
		return 1;
	return 0;
}

double transformLat(double x, double y)
{
	double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * sqrt(abs(x));
	ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;
	ret += (20.0 * sin(y * pi) + 40.0 * sin(y / 3.0 * pi)) * 2.0 / 3.0;
	ret += (160.0 * sin(y / 12.0 * pi) + 320 * sin(y * pi / 30.0)) * 2.0 / 3.0;
	return ret;
}

double transformLon(double x, double y)
{
	double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * sqrt(abs(x));
	ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;
	ret += (20.0 * sin(x * pi) + 40.0 * sin(x / 3.0 * pi)) * 2.0 / 3.0;
	ret += (150.0 * sin(x / 12.0 * pi) + 300.0 * sin(x / 30.0 * pi)) * 2.0 / 3.0;
	return ret;
}

//
// World Geodetic System ==> Mars Geodetic System
void transform(double wgLat, double wgLon, double *mgLat, double *mgLon)
{
	if (outOfChina(wgLat, wgLon)) {
		*mgLat = wgLat;
		*mgLon = wgLon;
		return;
	}
	double dLat = transformLat(wgLon - 105.0, wgLat - 35.0);
	double dLon = transformLon(wgLon - 105.0, wgLat - 35.0);
	double radLat = wgLat / 180.0 * pi;
	double magic = sin(radLat);
	magic = 1 - ee * magic * magic;
	double sqrtMagic = sqrt(magic);
	dLat = (dLat * 180.0) / ((a * (1 - ee)) / (magic * sqrtMagic) * pi);
	dLon = (dLon * 180.0) / (a / sqrtMagic * cos(radLat) * pi);
	*mgLat = wgLat + dLat;
	*mgLon = wgLon + dLon;
}

// 这是普通的  BH4WAD-8>AP51G2:!3215.79N/11943.80E>000/000/A=000036QQ:241612172 12.2V 31.6C S11
// 这是高精度的BH4WAD-8>AP51G2:!3215.79123N/11943.80456E>000/000/A=000036QQ:241612172 12.2V 31.6C S11
// 对于高精度的，返回值 high_res 为1，low_res是对应的低精度packet

// 先查找:字符，然后看后面第9个字符是字母还是数字，如果是字母（上面是N）说明是之前的格式，如果是数字，说明是高精度格式

void aprspacket_high_to_low(char *buf, int len, int *high_res, char **low_res, int *low_len)
{
	static char low_res_buf[MAXLEN];
	char *p;
	if (debug)
		fprintf(stderr, "OLD APRS: %s\n", buf);
	*high_res = 0;
	*low_res = buf;
	*low_len = len;
	p = strchr(buf, ':');
	if (p && ((*(p + 1) == ',') || isdigit(*(p + 9)))) {
		*high_res = 1;
		memcpy(low_res_buf, buf, len);
		*low_len = len;
		p = low_res_buf + (p - buf);
		if (*(p + 1) == ',')
			*(p + 1) = '!';
		p = p + 9;	// now p point to 0 (3th char)
		memmove(p, p + 3, *low_len - (p - low_res_buf));
		p = p + 10;
		*low_len -= 3;
		memmove(p, p + 3, *low_len - (p - low_res_buf));
		*low_len -= 3;
		if (debug)
			fprintf(stderr, "new APRS: %s\n", low_res_buf);
		*low_res = low_res_buf;
		return;
	}
	return;
}

// 这是普通的BH4WAD-8>AP51G2:!3215.79N/11943.80E>000/000/A=000036QQ:241612172 12.2V 31.6C S11

void aprspacket_gps_to_trans(char *buf, int len, char **trans_res, int *trans_len)
{
	static char trans_res_buf[MAXLEN];
	char *p;
	double lat, lon, tlat, tlon, tmp;
	char tmp_str[MAXLEN];

	if (debug)
		fprintf(stderr, "OLD APRS: %s\n", buf);
	strncpy(trans_res_buf, buf, MAXLEN);
	*trans_res = trans_res_buf;
	*trans_len = len;
	p = strchr(trans_res_buf, ':');
	if ((p == NULL) || (*(p + 1) != '!') || (strlen(p) < 20))
		return;
	p = p + 2;
	memcpy(tmp_str, p, 2);
	tmp_str[2] = 0;
	sscanf(tmp_str, "%lf", &lat);
	memcpy(tmp_str, p + 2, 5);
	tmp_str[5] = 0;
	sscanf(tmp_str, "%lf", &tmp);
	lat = lat + tmp / 60.0;
	if (*(p + 7) == 'S')
		lat = -lat;
	memcpy(tmp_str, p + 9, 3);
	tmp_str[3] = 0;
	sscanf(tmp_str, "%lf", &lon);
	memcpy(tmp_str, p + 12, 5);
	tmp_str[5] = 0;
	sscanf(tmp_str, "%lf", &tmp);
	lon = lon + tmp / 60.0;
	if (*(p + 17) == 'W')
		lon = -lon;

	fprintf(stderr, "lat=%lf lon=%lf\n", lat, lon);

	transform(lat, lon, &tlat, &tlon);

	fprintf(stderr, "transed lat=%lf lon=%lf\n", tlat, tlon);

	sprintf(tmp_str, "%02d", (int)tlat);
	memcpy(p, tmp_str, 2);
	sprintf(tmp_str, "%02.03f", (tlat - floor(tlat)) * 60);
	memcpy(p + 2, tmp_str, 5);
	sprintf(tmp_str, "%03d", (int)tlon);
	memcpy(p + 9, tmp_str, 3);
	sprintf(tmp_str, "%02.03f", (tlon - floor(tlon)) * 60);
	memcpy(p + 12, tmp_str, 5);

	fprintf(stderr, "new APRS: %s\n", trans_res_buf);
	return;
}
