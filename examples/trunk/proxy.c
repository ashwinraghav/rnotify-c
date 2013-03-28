#include "zhelpers.h"
#include "czmq.h"
#include "unistd.h"

#define BUFF_SIZE ((sizeof(struct inotify_event)+FILENAME_MAX)*2000)

#ifdef PRODUCTION
	#define SEND_SOCK "tcp://*:" PROXY_FLUSH_PORT
	#define SUB_RECV_SOCK "tcp://*:" PROXY_SUBSCRIBE_PORT
#else	
	#define SEND_SOCK "ipc:///tmp/" PROXY_FLUSH_PORT
	#define SUB_RECV_SOCK "ipc:///tmp/" PROXY_SUBSCRIBE_PORT
#endif

void start_proxy(int fd, void* publisher);
void handle_error (int error);
void flush (char* update, int len, void* publisher);

static void subscription_receiver(void* args, zctx_t* ctx, void *pipe){
	void *sub_recv_socket = create_socket(ctx, ZMQ_REP, SOCK_BIND, SUB_RECV_SOCK);
	int wd, size, *fd = (int*) args;
	printf("\nStarting thread");
	while(true){
		char *file_name = safe_recv(sub_recv_socket, &size);

		char *registration_id = register_notification(*fd, file_name);
		
		safe_send(sub_recv_socket, registration_id, size);
		free(registration_id);
		free(file_name);
	}
	
}

int main ()
{
        int fd, wd;
	zctx_t *ctx = zctx_new ();
	assert(ctx);
	void *publisher = create_socket(ctx, ZMQ_PUSH, SOCK_BIND, SEND_SOCK);
	
	CHECK(fd = inotify_init()); 
	zthread_fork(ctx, subscription_receiver, (void*)(&fd));

	start_proxy(fd, publisher);

	zctx_destroy (&ctx);
	return 0;
}

void flush (char* update, int len, void* publisher)
{
	int sent_size = safe_send(publisher, update, len);
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


