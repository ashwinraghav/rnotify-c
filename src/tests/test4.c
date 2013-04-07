#include "czmq.h"
static int s_timer_event (zloop_t *loop, zmq_pollitem_t *item, void *output)
{
    zstr_send (output, "PING");
    return 0;
}

static int s_socket_event (zloop_t *loop, zmq_pollitem_t *item, void *arg)
{
    //  Just end the reactor
    return -1;
}

void izloop_test (bool verbose)
{
    printf (" * zloop: ");
    int rc = 0;
    //  @selftest
    zctx_t *ctx = zctx_new ();
    assert (ctx);

    void *output = zsocket_new (ctx, ZMQ_PAIR);
    assert (output);
    zsocket_bind (output, "tcp://*:6565");
    void *input = zsocket_new (ctx, ZMQ_PAIR);
    assert (input);
    zsocket_connect (input, "tcp://localhost:6565");

    zloop_t *loop = zloop_new ();
    assert (loop);
    zloop_set_verbose (loop, verbose);

    //  After 10 msecs, send a ping message to output
    zloop_timer (loop, 10, 1, s_timer_event, output);
    fprintf(stderr, "pinged");
    //  When we get the ping message, end the reactor
    zmq_pollitem_t poll_input = { input, 0, ZMQ_POLLIN };
    rc = zloop_poller (loop, &poll_input, s_socket_event, NULL);
    assert (rc == 0);
    zloop_start (loop);

    zloop_destroy (&loop);
    assert (loop == NULL);

    zctx_destroy (&ctx);
    //  @end
    printf ("OK\n");
}




int main( void )
{
	izloop_test(true);
	return 0;
}
