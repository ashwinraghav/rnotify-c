#include "zhelpers.h"
#include "czmq.h"
#include "unistd.h"
#include<sys/inotify.h>


#ifdef PRODUCTION
	#define SUB_SOCK "tcp://localhost:6001"
	#define SUBSCRIBE_TO_SOCK "tcp://localhost:5558"
#else
	#define SUB_SOCK "ipc:///tmp/6001"
	#define SUBSCRIBE_TO_SOCK "ipc:///tmp/5558"
#endif


void listener(void *args){
	zctx_t *ctx= (zctx_t*)args;
	void *subscriber = create_socket(ctx, ZMQ_SUB, SOCK_CONNECT, SUB_SOCK);
	char* filter="";
	int count = 0;
	zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE, filter, strlen (filter));

	while(true)
	{
		int size;
		char *string = two_part_receive(subscriber, &size);
		//parse_notifications(string, size, &count);
		free (string);
	}
	zctx_destroy (&ctx);
}

void rsubscribe(char* file_path){
	printf("\nSubscribing");
	printf("\nSubscribing");
	zctx_t *ctx = zctx_new();
	void *sub_send_socket = create_socket(ctx, ZMQ_REQ, SOCK_CONNECT, SUBSCRIBE_TO_SOCK);
	safe_send(sub_send_socket, file_path, strlen(file_path)+1);
	int size;
	char *content_received = safe_recv(sub_send_socket, &size);
	printf("\n added watch %s", content_received);
}

