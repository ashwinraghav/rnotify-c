//  Weather proxy device

#include "zhelpers.h"
#include "czmq.h"
#define BACKEND "tcp://localhost:6500"
#define FRONTEND "tcp://*:6001"


static void listener_thread (void *args, zctx_t *ctx, void *pipe);


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
	zctx_t *ctx = zctx_new ();
	//zthread_fork (ctx, publisher_thread, NULL);

	void *subscriber = zsocket_new (ctx, ZMQ_XSUB);
	zsocket_bind (subscriber, "tcp://*:6500");
	void *publisher = zsocket_new (ctx, ZMQ_XPUB);
	zsocket_bind (publisher, "tcp://*:6001");

	void *listener = zthread_fork (ctx, listener_thread, NULL);
	zmq_proxy (subscriber, publisher, listener);

	puts (" interrupted");
	//  Tell attached threads to exit
	zctx_destroy (&ctx);
	/*zctx_t *ctx = zctx_new ();
	  printf("Proxy starting to work\n");

	  void *frontend = zsocket_new (ctx, ZMQ_XSUB);
	  int rc = zsocket_bind(frontend, FRONTEND);

	  if(rc != 0){
		printf("1. The error is %s \n",strerror (errno));
	}

	void *backend = zsocket_new (ctx, ZMQ_XPUB);
	rc = zsocket_connect(backend, BACKEND);

	void *listener = zthread_fork (ctx, listener_thread, NULL);
	rc = zmq_proxy (frontend, backend, listener);
	*/
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

	//zmq_close (frontend);
	//zmq_close (backend);
	return 0;
}
