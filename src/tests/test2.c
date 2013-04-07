
#include "zhelpers.h"
#include "czmq.h"
#include <sys/inotify.h>
#include <zhelpers.h>
#include "ip.h"
#include <hash_ring.h>
#include<glib.h>
#include <unistd.h>
/* Return 1 if the difference is negative, otherwise 0.  */
int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1)
{
    long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
    result->tv_sec = diff / 1000000;
    result->tv_usec = diff % 1000000;

    return (diff<0);
}

void timeval_print(struct timeval *tv)
{
    char buffer[30];
    time_t curtime;

    printf("%ld.%06ld", tv->tv_sec, tv->tv_usec);
    curtime = tv->tv_sec;
    strftime(buffer, 30, "%m-%d-%Y  %T", localtime(&curtime));
    printf(" = %s.%06ld\n", buffer, tv->tv_usec);
}

int main(){

       	zctx_t *ctx = zctx_new ();
	zhash_t *hash = zhash_new ();
	struct timeval tvBegin, tvEnd, tvDiff;
	void *dispatch_socket = create_socket(ctx, ZMQ_PUB, SOCK_CONNECT, "tcp://192.168.1.2:3000");
	zhash_insert(hash,"tcp://192.168.1.2:3000", dispatch_socket);

	// begin
	gettimeofday(&tvBegin, NULL);
	timeval_print(&tvBegin);




	int i;
	for(i=0;i<10000000;i++){
		void * ret = zhash_lookup (hash,"tcp://192.168.1.2:3000");
		if(ret == NULL)
			fprintf(stderr, "asdasd");
	}

	gettimeofday(&tvEnd, NULL);
	timeval_print(&tvEnd);

	// diff
	timeval_subtract(&tvDiff, &tvEnd, &tvBegin);
	printf("%ld.%06ld\n", tvDiff.tv_sec, tvDiff.tv_usec);
	zhash_destroy (&hash);
	zctx_destroy (&ctx);
	return 0;
}
