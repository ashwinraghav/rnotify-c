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
zctx_t *ctx;

static void s_signal_handler (int signal_value)
{
	s_interrupted = 1;
}

static void s_catch_signals (void)
{
	struct sigaction action;
	action.sa_handler = s_signal_handler;
	action.sa_flags = 0;
	sigemptyset (&action.sa_mask);
	sigaction (SIGINT, &action, NULL);
	sigaction (SIGTERM, &action, NULL);
}
static void subscription_receiver(void* args, zctx_t* ctx, void *pipe){
}

int main ()
{
        int fd, wd;
	ctx = zctx_new ();
	assert(ctx);
	void *publisher = create_socket(ctx, ZMQ_PUSH, SOCK_BIND, FLUSH_ADDR);
	s_catch_signals();	
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
	while(!s_interrupted){
		ssize_t len;
		len = read (fd, buff, BUFF_SIZE);
		flush(buff, len, publisher);
	}
	free(buff);
}
