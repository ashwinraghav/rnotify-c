#include "zhelpers.h"
#include "czmq.h"
#include "unistd.h"

#define BUFF_SIZE ((sizeof(struct inotify_event)+FILENAME_MAX)*2000)

#ifdef PRODUCTION
	#define FLUSH_ADDR "tcp://*:" PROXY_FLUSH_PORT
	#define SUBSCRIPTION_RECV_ADDR "tcp://*:" PROXY_SUBSCRIBE_PORT
#else	
	#define FLUSH_ADDR "ipc:///tmp/" PROXY_FLUSH_PORT
	#define SUBSCRIPTION_RECV_ADDR "ipc:///tmp/" PROXY_SUBSCRIBE_PORT
#endif

void start_proxy(int fd, void* publisher);
void handle_error (int error);
void flush (char* update, int len, void* publisher);

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
	void *publisher = create_socket(ctx, ZMQ_PUSH, SOCK_BIND, FLUSH_ADDR);
	CHECK(fd = inotify_init()); 
	zthread_fork(ctx, subscription_receiver, (void*)(&fd));

	start_proxy(fd, publisher);
	close(fd);
	zctx_destroy (&ctx);
	return 0;
}


void flush (char* update, int len, void* publisher)
{
	int sent_size = _send_string(publisher, update, len);
	printf("Sent Length is %d \n", sent_size);
}


void start_proxy(int fd, void* publisher)
{
	char *buff = malloc(BUFF_SIZE);
	while(1){
		ssize_t len;
		len = read (fd, buff, BUFF_SIZE);
		flush(buff, len, publisher);
	}
	free(buff);
}


