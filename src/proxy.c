#include "zhelpers.h"
#include "czmq.h"
#include "unistd.h"

#define BUFF_SIZE ((sizeof(struct inotify_event)+FILENAME_MAX)*2000)
#define PROXY_WORKER_SOCKET "inproc://#323"
#define PROXY_THREAD_COUNT 10

#ifdef PRODUCTION
	#define FLUSH_ADDR "tcp://*:" PROXY_FLUSH_PORT
	#define SUBSCRIPTION_RECV_ADDR "tcp://*:" PROXY_SUBSCRIBE_PORT
#else	
	#define FLUSH_ADDR "ipc:///tmp/" PROXY_FLUSH_PORT
	#define SUBSCRIPTION_RECV_ADDR "ipc:///tmp/" PROXY_SUBSCRIBE_PORT
#endif

//void start_proxy(int fd, void* publisher);
static void start_proxy(void *args, zctx_t* ctx, void *pipe);
void handle_error (int error);
void flush (char* update, int len, void* publisher);
void flusher(void* publisher, void* work_receiver_socket);

static int s_interrupted = 0;

static void subscription_receiver(void* args, zctx_t* ctx, void *pipe){
	void *sub_recv_socket = create_socket(ctx, ZMQ_REP, SOCK_BIND, SUBSCRIPTION_RECV_ADDR);
	int wd, size, *fd = (int*) args;
	fprintf(stderr, "\nStarting thread");

	while(true){
		char* const file_name = _recv_buff(sub_recv_socket, &size);
		char* const registration_id = register_notification(*fd, file_name);
		_send_string(sub_recv_socket, registration_id, strlen(registration_id));
		free(registration_id);
		free(file_name);
	}
}

int main ()
{
        int fd, wd;
	zctx_t *ctx = zctx_new ();
	assert(ctx);
	
	int io_threads = 15;
	zctx_set_iothreads (ctx,  io_threads);
	
	CHECK(fd = inotify_init());

	void *publisher = create_socket(ctx, ZMQ_PUSH, SOCK_BIND, FLUSH_ADDR);
	void *work_receiver_socket = create_socket(ctx, ZMQ_PULL, SOCK_BIND, PROXY_WORKER_SOCKET);
	sleep(1);

 
	zthread_fork(ctx, subscription_receiver, (void*)(&fd));

	int i = 0;
	for(i = 0; i < PROXY_THREAD_COUNT;i++){
		zthread_fork(ctx, start_proxy, (void*)(&fd));
	}
	flusher(publisher, work_receiver_socket);
	close(fd);
	zctx_destroy (&ctx);
	return 0;
}
void flusher(void* publisher, void* work_receiver_socket){
	
	while(true){
		int size;
		char *buff = _recv_buff(work_receiver_socket, &size);
		flush(buff, size, publisher);
		free(buff);
	}
	
}

void flush (char* update, int len, void* publisher)
{
	int sent_size = _send_string(publisher, update, len);
	printf("Sent Length is %d \n", sent_size);
}

static void start_proxy(void *args, zctx_t* ctx, void *pipe){

	void *work_sender_socket = create_socket(ctx, ZMQ_PUSH, SOCK_CONNECT, PROXY_WORKER_SOCKET);
	int *fd = (int*) args;
	char *buff = malloc(BUFF_SIZE);
	while(1){
		ssize_t len;
		len = read (*fd, buff, BUFF_SIZE);
		_send_string(work_sender_socket, buff, len);
	}
	free(buff);
}


