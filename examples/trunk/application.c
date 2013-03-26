#include "zhelpers.h"
#include "czmq.h"
#include<sys/inotify.h>

#ifdef PRODUCTION
	#define SUB_SOCK "tcp://localhost:6001"
#else
	#define SUB_SOCK "ipc:///tmp/6001"
#endif

void parse_notifications(char *buff, ssize_t len, int *count);

int main (void)
{
	zctx_t *ctx = zctx_new ();
	
	void *subscriber = create_socket(ctx, ZMQ_SUB, SOCK_CONNECT, SUB_SOCK);
	char* filter="";
	int count = 0;
	zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE, filter, strlen (filter));

	while(true)
	{
		int size;
		char *string = safe_recv(subscriber, &size);
		parse_notifications(string, size, &count);
		free (string);
	}
	zctx_destroy (&ctx);
	return 0;
}
void parse_notifications(char *buff, ssize_t len, int* count)
{
	ssize_t i = 0;
        while (i < len) {
                struct inotify_event *pevent = (struct inotify_event *)&buff[i];
		print_notifications(pevent);
		*count = *count + 1;
		printf("Count = %d \n", *count);
                i += sizeof(struct inotify_event) + pevent->len;
        }

}

