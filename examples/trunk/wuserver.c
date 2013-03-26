#include "zhelpers.h"
#include "czmq.h"
#include "unistd.h"

#define BUFF_SIZE ((sizeof(struct inotify_event)+FILENAME_MAX)*2000)

#ifdef PRODUCTION
	#define SEND_SOCK "tcp://*:5556"
#else	
	#define SEND_SOCK "ipc:///tmp/5556"
#endif

void flush_notifications(int fd, void* publisher);
void handle_error (int error);
void flush (char* update, int len, void* publisher);

int main ()
{
        int fd, wd;
	zctx_t *ctx = zctx_new ();
	assert(ctx);
	void *publisher = create_socket(ctx, ZMQ_PUSH, SOCK_BIND, SEND_SOCK);

	CHECK(fd = inotify_init()); 
	CHECK(inotify_add_watch (fd, "/localtmp/dump/1", IN_ALL_EVENTS));
	CHECK(inotify_add_watch (fd, "/localtmp/dump/2", IN_ALL_EVENTS));
	CHECK(inotify_add_watch (fd, "/localtmp/dump/3", IN_ALL_EVENTS));
	CHECK(wd = inotify_add_watch (fd, "/localtmp/dump/4", IN_ALL_EVENTS));

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


