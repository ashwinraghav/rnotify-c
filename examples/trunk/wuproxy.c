#include "zhelpers.h"
#include "czmq.h"
#ifdef PRODUCTION
	#define BACKEND "tcp://*:6500"
	#define FRONTEND "tcp://*:6001"
#else
	#define BACKEND "ipc:///tmp/6500"
	#define FRONTEND "ipc:///tmp/6001"
#endif

static void listener_thread (void *args, zctx_t *ctx, void *pipe);


static void listener_thread (void *args, zctx_t *ctx, void *pipe)
{
	while (true) {
		zframe_t *frame = zframe_recv (pipe);
		if (!frame)
			break; //  Interrupted
		//zframe_print (frame, NULL);
		zframe_destroy (&frame);
	}
}

int main (void)
{
	zctx_t *ctx = zctx_new ();
	int io_threads = 4;
	//zmq_ctx_set (ctx, ZMQ_IO_THREADS, io_threads);
	zctx_set_iothreads (ctx,  io_threads);	
	
	void *subscriber = create_socket(ctx, ZMQ_XSUB, SOCK_BIND, BACKEND);
	void *publisher = create_socket(ctx, ZMQ_XPUB, SOCK_BIND, FRONTEND);

	void *listener = zthread_fork (ctx, listener_thread, NULL);
	zmq_proxy (subscriber, publisher, NULL);
	puts (" interrupted");
	zctx_destroy (&ctx);
	return 0;
}
