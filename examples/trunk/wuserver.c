#include "zhelpers.h"
#include "czmq.h"
#include "unistd.h"

#define BUFF_SIZE ((sizeof(struct inotify_event)+FILENAME_MAX)*2000)

#ifdef PRODUCTION
	#define SEND_SOCK "tcp://*:5556"
	#define SUB_RECV_SOCK "tcp://*:5558"
#else	
	#define SEND_SOCK "ipc:///tmp/5556"
	#define SUB_RECV_SOCK "ipc:///tmp/5558"
#endif

void flush_notifications(int fd, void* publisher);
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
	//subscription_receiver(ctx, (void*)(&fd));

	while (1)
	{
		flush_notifications(fd, publisher);
	}

	zctx_destroy (&ctx);
	return 0;
}

void flush (char* update, int len, void* publisher)
{
	int sent_size = safe_send(publisher, update, len);
	printf("Sent Length is %d \n", sent_size);
}

void flush_notifications(int fd, void* publisher)
{
	ssize_t len;
	char action[81+FILENAME_MAX] = {0};
	char buff[BUFF_SIZE] = {0};
	len = read (fd, buff, BUFF_SIZE);
	flush(buff, len, publisher);
	//print_notifications(buff, len);
}


