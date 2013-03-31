#include <stdio.h>

#include <string.h> /* for strncpy */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#define REGISTER_PORT "6000"

#define REGISTRAR_IP_ADDR "asdfasdf"
#define REGISTRATION_ADDR "tcp://" REGISTRAR_IP_ADDR "asd"


int
main()
{
	fprintf(stderr, "%s", REGISTRATION_ADDR);
 return 0;
}
