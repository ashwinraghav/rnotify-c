#ifndef IP_HEADER_INCLUDED
#define IP_HEADER_INCLUDED
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>

char* get_ip_address();

char* get_ip_address()
{
	struct ifaddrs *ifaddr, *ifa;
	int family, s;
	char *host = malloc(NI_MAXHOST);

	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	/* Walk through linked list, maintaining head pointer so we
	   can free list later */

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;

		/* Display interface name and family (including symbolic
		   form of the latter for the common families) */

		/* For an AF_INET* interface address, display the address */

		if ((g_strcmp0(ifa->ifa_name, "lo") != 0)&&(family == AF_INET)) {
			s = getnameinfo(ifa->ifa_addr,
					(family == AF_INET) ? sizeof(struct sockaddr_in) :
					sizeof(struct sockaddr_in6),
					host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if (s != 0) {
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
				exit(EXIT_FAILURE);
			}
			freeifaddrs(ifaddr);
			return(host);
		}
	}
	return "";
}
#endif
