#include "zhelpers.h"
#include "czmq.h"
#include <ip.h>

#ifdef PRODUCTION
	#define REGISTRATION_ADDR "tcp://localhost:" REGISTER_PORT
	#define BACKEND_ADDR "tcp://*:" DISPATCH_PORT
	#define FRONTEND_ADDR "tcp://*:" PUBLISH_PORT
#else
	#define REGISTRATION_ADDR "ipc:///tmp/" REGISTER_PORT
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
		zframe_print (frame, NULL);
		zframe_destroy (&frame);
	}
}

static void self_register(zctx_t *ctx){
	void *register_sock = create_socket(ctx, ZMQ_PUSH, SOCK_CONNECT, REGISTRATION_ADDR);
	char *my_ip_address = get_ip_address();
	
	two_phase_register(register_sock, my_ip_address, REGISTER_PUBLISHER_SANITY_CHECK);
	
	free(my_ip_address);

}

int main (void)
{
	zctx_t *ctx = zctx_new ();
	self_register(ctx);
	int io_threads = 4;
	zctx_set_iothreads (ctx,  io_threads);	
	
	void *subscriber = create_socket(ctx, ZMQ_XSUB, SOCK_BIND, BACKEND_ADDR);
	void *publisher = create_socket(ctx, ZMQ_XPUB, SOCK_BIND, FRONTEND_ADDR);

	void *listener = zthread_fork (ctx, listener_thread, NULL);
	zmq_proxy (subscriber, publisher, listener);
	puts (" interrupted");
	zctx_destroy (&ctx);
	return 0;
}
