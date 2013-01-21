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

	void *subscriber = zsocket_new (ctx, ZMQ_XSUB);
	zsocket_bind (subscriber, BACKEND);
	void *publisher = zsocket_new (ctx, ZMQ_XPUB);
	zsocket_bind (publisher, FRONTEND);

	void *listener = zthread_fork (ctx, listener_thread, NULL);
	zmq_proxy (subscriber, publisher, listener);

	puts (" interrupted");
	//  Tell attached threads to exit
	zctx_destroy (&ctx);
	return 0;
}
