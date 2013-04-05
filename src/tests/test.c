#include "zhelpers.h"
#include "zhelpers.h"
#include "czmq.h"
#include <sys/inotify.h>
#include <zhelpers.h>
#include "ip.h"
#include <hash_ring.h>
#include<glib.h>

#define WORKER_SOCKET "inproc://#1"
#define THREAD_COUNT 1

#ifdef PRODUCTION
	#define RECEIVE_ADDR "tcp://" FILE_HOST_IP_ADDR ":"  PROXY_FLUSH_PORT
	#define DISPATCHER_NOTIFY_ADDR "tcp://" REGISTRAR_IP_ADDR ":" DISPATCHER_NOTIFY_PORT

#else
	#define RECEIVE_ADDR "ipc:///tmp/" PROXY_FLUSH_PORT
	#define DISPATCHER_NOTIFY_ADDR "ipc:///tmp/" DISPATCHER_NOTIFY_PORT
#endif

typedef struct publisher_list_struct{
	hash_ring_t *publisher_ring;
	GHashTable *publisher_socks;
}publishers_info;

void parse_notifications(char *buff, ssize_t len, void* dispatch_socket);
void dispatch (char* update, int size, void* dispatch_socket);
static void parser_thread(void *args, zctx_t* ctx, void *pipe);
void create_parser_threads(int nthreads, zctx_t *ctx, publishers_info *publisher_ring);
static void publisher_updater(void *args, zctx_t *ctx, void *pipe);

#include "czmq.h"
#include <sys/inotify.h>
#include <zhelpers.h>
#include "ip.h"
#include <hash_ring.h>
#include <glib.h>
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


int main (int argc, char *argv [])
{
       	zctx_t *ctx = zctx_new ();
	struct timeval tvBegin, tvEnd, tvDiff;
	publishers_info *pub_interface = malloc(sizeof(publishers_info));

	pub_interface->publisher_socks = g_hash_table_new(g_str_hash, g_str_equal);
	
	void *dispatch_socket = NULL;
	create_socket(ctx, dispatch_socket, ZMQ_PUB, SOCK_CONNECT, "tcp://192.168.1.2:3000");
	g_hash_table_insert(pub_interface->publisher_socks,"tcp://192.168.1.2:3000", dispatch_socket);
	int i=0;


	gettimeofday(&tvBegin, NULL);
	timeval_print(&tvBegin);


	for(i=0;i<10000000;i++){
		void* ret =  g_hash_table_lookup(pub_interface->publisher_socks, "tcp://192.168.1.2:3000");
		if(ret == NULL)
			fprintf(stderr, "asdasd");
	}
	gettimeofday(&tvEnd, NULL);
	timeval_print(&tvEnd);

	// diff
	timeval_subtract(&tvDiff, &tvEnd, &tvBegin);
	printf("%ld.%06ld\n", tvDiff.tv_sec, tvDiff.tv_usec);
	
	g_hash_table_destroy(pub_interface->publisher_socks);

	zctx_destroy (&ctx);
	return 0;
}

