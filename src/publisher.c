#include "rnot_helper.h"
#include "czmq.h"

#ifdef PRODUCTION
	#define BACKEND_ADDR "tcp://*:" DISPATCH_PORT
	#define FRONTEND_ADDR "tcp://*:" PUBLISH_PORT
#else
	#define BACKEND_ADDR "ipc:///tmp/" DISPATCH_PORT
	#define FRONTEND_ADDR "ipc:///tmp/" PUBLISH_PORT
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
	int size;
	char ** registration_response = self_register(ctx, REGISTER_PUBLISHER_SANITY_CHECK, DISPATCH_PORT, &size);
	free(registration_response);

	int io_threads = 4;
	zctx_set_iothreads (ctx,  io_threads);	
	
	void *subscriber = create_socket(ctx, ZMQ_XSUB, SOCK_BIND, BACKEND_ADDR);
	void *publisher = create_socket(ctx, ZMQ_XPUB, SOCK_BIND, FRONTEND_ADDR);

	void *listener = zthread_fork (ctx, listener_thread, NULL);
	zmq_proxy (subscriber, publisher, listener);
	zctx_destroy (&ctx);
	return 0;
}
