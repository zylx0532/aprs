void sendudp(char *buf, int len, char *host, int port)
{
	struct sockaddr_in si_other;
	int s, slen = sizeof(si_other);
	int l;

	if (debug)
		fprintf(stderr, "send to %s, ret=", host);
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		fprintf(stderr, "socket error");
		return;
	}
	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port);
	if (inet_aton(host, &si_other.sin_addr) == 0) {
		fprintf(stderr, "inet_aton() failed\n");
		close(s);
		return;
	}
	l = sendto(s, buf, len, 0, (const struct sockaddr *)&si_other, slen);
	if (debug)
		fprintf(stderr, "%d\n", l);
	close(s);
}
