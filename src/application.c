#include "rnot.h"

#ifdef PRODUCTION
	#define SUB_SOCK "tcp://localhost:6001"
#else
	#define SUB_SOCK "ipc:///tmp/6001"
#endif

int main (void)
{	
	const rnot* const rn = rnotify_init();	
	zmq_setsockopt (rn->subscriber, ZMQ_SUBSCRIBE, "", strlen (""));
	rsubscribe(rn, "/localtmp/dump/1");
	rsubscribe(rn, "/localtmp/dump/2");
	rsubscribe(rn, "/localtmp/dump/3");
	rsubscribe(rn, "/localtmp/dump/4");
	start_listener(rn, print_notifications);
	//cleanup
	return 0;
}
