//  Weather proxy device

#include "zhelpers.h"
#include "czmq.h"
#define BACKEND "tcp://*:6500"
#define FRONTEND "tcp://*:6001"


static void listener_thread (void *args, zctx_t *ctx, void *pipe);


static void listener_thread (void *args, zctx_t *ctx, void *pipe)
{
	//  Print everything that arrives on pipe
	while (true) {
		zframe_t *frame = zframe_recv (pipe);
		if (!frame)
			break;              //  Interrupted
		//zframe_print (frame, NULL);
		zframe_destroy (&frame);
	}
}

int main (void)
{
	zctx_t *ctx = zctx_new ();
        assert(ctx);	
	int io_threads = 4;
	//zmq_ctx_set (ctx, ZMQ_IO_THREADS, io_threads);
	zctx_set_iothreads (ctx,  io_threads);	
	//assert(zmq_ctx_get (ctx, ZMQ_IO_THREADS) == io_threads);
        printf("threads = %d\n", zmq_ctx_get(ctx, ZMQ_IO_THREADS));
	void *subscriber = zsocket_new (ctx, ZMQ_XSUB);
	zsocket_set_hwm(subscriber, 100000); 	
	zsocket_bind (subscriber, BACKEND);
	
	void *publisher = zsocket_new (ctx, ZMQ_XPUB);
	zsocket_set_hwm(publisher, 100000); 	
	zsocket_bind (publisher, FRONTEND);

	void *listener = zthread_fork (ctx, listener_thread, NULL);
	zmq_proxy (subscriber, publisher, listener);
	puts (" interrupted");
	zctx_destroy (&ctx);
	return 0;
}
