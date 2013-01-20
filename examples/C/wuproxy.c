//  Weather proxy device

#include "zhelpers.h"
#include "czmq.h"
#define PROXY_SUB_SOCK "tcp://localhost:6500"
#define PROXY_PUB_SOCK "tcp://*:6001"


static void listener_thread (void *args, zctx_t *ctx, void *pipe);

static void publisher_thread (void *args, zctx_t *ctx, void *pipe);

static void publisher_thread (void *args, zctx_t *ctx, void *pipe)
{
	void *publisher = zsocket_new (ctx, ZMQ_PUB);
	zsocket_bind (publisher, "tcp://*:6500");

	while (!zctx_interrupted) {
		char string [10];
		sprintf (string, "%c-%05d", randof (10) + 'A', randof (100000));
		if (zstr_send (publisher, string) == -1)
			break;              //  Interrupted
		zclock_sleep (100);     //  Wait for 1/10th second
	}
}
static void listener_thread (void *args, zctx_t *ctx, void *pipe)
{
	//  Print everything that arrives on pipe
	while (true) {
		zframe_t *frame = zframe_recv (pipe);
		if (!frame)
			break;              //  Interrupted
		zframe_print (frame, NULL);
		zframe_destroy (&frame);
	}
}

int main (void)
{
	void *context = zctx_new ();
	printf("Proxy starting to work\n");

	//  This is where the weather server sits
	void *frontend = zsocket_new (context, ZMQ_XSUB);
	int rc = zsocket_connect (frontend, PROXY_SUB_SOCK);
	assert (rc == 0);
	void *backend = zsocket_new (context, ZMQ_XPUB);
	rc = zsocket_bind(backend, PROXY_PUB_SOCK);
	assert (rc == 0);
	
	//zthread_fork (context, publisher_thread, NULL);
	//zsocket_set_subscribe (frontend, "");

	//assert (rc == 0);

	//  Initialize poll set
	/*zmq_pollitem_t items [] = {
		{ frontend, 0, ZMQ_POLLIN, 0 },
		{ backend,  0, ZMQ_POLLIN, 0 }
	};*/
 	void *listener = zthread_fork (context, listener_thread, NULL);
	zmq_proxy (frontend, backend, listener);
	
	//  Switch messages between sockets
	/*while (1) {
		zmq_msg_t message;
		int more;           //  Multipart detection

		zmq_poll (items, 2, -1);
		printf("Proxy received something\n");
		if (items [0].revents & ZMQ_POLLIN) {
			while (1) {
				//  Process all parts of the message
				zmq_msg_init (&message);
				zmq_msg_recv (&message, frontend, 0);
				size_t more_size = sizeof (more);
				zmq_getsockopt (frontend, ZMQ_RCVMORE, &more, &more_size);
				zmq_msg_send (&message, backend, more? ZMQ_SNDMORE: 0);
				zmq_msg_close (&message);
				if (!more)
					break;      //  Last message part
			}
		}
		if (items [1].revents & ZMQ_POLLIN) {
			while (1) {
				//  Process all parts of the message
				zmq_msg_init (&message);
				zmq_msg_recv (&message, backend, 0);
				size_t more_size = sizeof (more);
				zmq_getsockopt (backend, ZMQ_RCVMORE, &more, &more_size);
				zmq_msg_send (&message, frontend, more? ZMQ_SNDMORE: 0);
				zmq_msg_close (&message);
				if (!more)
					break;      //  Last message part
			}
		}
	}
	*/



	//  Run the proxy until the user interrupts us
	//zmq_proxy (frontend, backend, NULL);

	zmq_close (frontend);
	zmq_close (backend);
	zmq_ctx_destroy (context);
	return 0;
}
