#include "zhelpers.h"
#include "czmq.h"
#include<sys/inotify.h>

#define WORKER_SOCKET "inproc://#1"
#define THREAD_COUNT 50

#ifdef PRODUCTION
	#define RECEIVE_SOCKET "tcp://localhost:5556"
	#define DISPATCH_SOCKET "tcp://localhost:6500"
#else
	#define RECEIVE_SOCKET "ipc:///tmp/5556"
	#define DISPATCH_SOCKET "ipc:///tmp/6500"
#endif

void parse_notifications(char *buff, ssize_t len, void* dispatch_socket);
void dispatch (char* update, int size, void* dispatch_socket);
static void parser_thread(void *args, zctx_t* ctx, void *pipe);
void create_parser_threads(int nthreads, zctx_t *ctx);

int main (int argc, char *argv [])
{
        zctx_t *ctx = zctx_new ();
	
	void *subscriber = create_socket(ctx, ZMQ_PULL, SOCK_CONNECT, RECEIVE_SOCKET);
	void *worker = create_socket(ctx, ZMQ_PUSH, SOCK_BIND, WORKER_SOCKET);
	
	create_parser_threads(THREAD_COUNT, ctx);

	while(true)
	{
		int size;
		char *string = safe_recv(subscriber, &size);
		parse_notifications(string, size,  worker);
		free (string);
	}
	zctx_destroy (&ctx);
	return 0;
}

void create_parser_threads(int nthreads, zctx_t *ctx)
{
	for (nthreads=0; nthreads < 50; nthreads++)
	{
		zthread_fork(ctx, parser_thread, NULL);
	}

}

void dispatch(char* update, int len, void* dispatch_socket)
{
	int sent_size = safe_send(dispatch_socket, update, len);
}

static void parser_thread(void *args, zctx_t* ctx, void *pipe){
	void *dispatch_socket = create_socket(ctx, ZMQ_PUB, SOCK_CONNECT, DISPATCH_SOCKET);
	void *work_receiver_socket = create_socket(ctx, ZMQ_PULL, SOCK_CONNECT, WORKER_SOCKET);

	while(true)
	{
		int size;
		char *buff = safe_recv(work_receiver_socket, &size);

		ssize_t i = 0;
		char action[81+FILENAME_MAX] = {0};
		while (i < size) {
			struct inotify_event *pevent = (struct inotify_event *)&buff[i];

			if (pevent->len){
				int serial_length = sizeof(struct inotify_event) + pevent->len;
				dispatch(&buff[i], serial_length, dispatch_socket);
			}
			print_notifications(pevent);
			i += sizeof(struct inotify_event) + pevent->len;
		}
		free (buff);
	}
}

void parse_notifications(char *buff, ssize_t len, void* worker)
{	
	zmq_msg_t message;
	zmq_msg_init_size (&message, len);
	memcpy (zmq_msg_data (&message), buff, len);
	CHECK(zmq_msg_send (&message, worker, 0));
	zmq_msg_close (&message);
}

