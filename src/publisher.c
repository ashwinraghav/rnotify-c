#include "rnot_helper.h"
#include "czmq.h"

#ifdef PRODUCTION
	#define DYNAMIC_ADDR "tcp://*:*"
	#define BACKEND_ADDR "tcp://*:" ACCEPT_PORT
	#define FRONTEND_ADDR "tcp://*:" PUBLISH_PORT
#else
	#define BACKEND_ADDR "ipc:///tmp/" ACCEPT_PORT
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

	int io_threads = 4;
	zctx_set_iothreads (ctx,  io_threads);	
	
	void *subscriber, *publisher; 
	int accept_port = scan_and_bind_socket(ctx, &subscriber, ZMQ_XSUB, DYNAMIC_ADDR);
	int publish_port = scan_and_bind_socket(ctx, &publisher, ZMQ_XPUB, DYNAMIC_ADDR);
	

	char ** registration_response = register_publisher_service(ctx, accept_port, publish_port,  &size);

	free(registration_response);

	void *listener = zthread_fork (ctx, listener_thread, NULL);
	zmq_proxy (subscriber, publisher, listener);
	zctx_destroy (&ctx);
	return 0;
}
