#include <stdio.h>

#include <string.h> /* for strncpy */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdlib.h>
#define REGISTER_PORT "6000"

#define REGISTRAR_IP_ADDR "asdfasdf"
#define REGISTRATION_ADDR "tcp://" REGISTRAR_IP_ADDR "asd"


int
main()
{
double random_number = rand();
int i=0;
srand(time(NULL));
int r = rand();
printf("%d", r%10);
return 0;
}
