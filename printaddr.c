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
