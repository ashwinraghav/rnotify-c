#include "zhelpers.h"
#include "zhelpers.h"
#include "czmq.h"
#include <sys/inotify.h>
#include <zhelpers.h>
#include "ip.h"
#include <hash_ring.h>
#include<glib.h>


int main(){
    zbeacon_t *node1 = zbeacon_new (5670);
    zbeacon_t *node2 = zbeacon_new (5670);
    zbeacon_t *node3 = zbeacon_new (5670);

    assert (*zbeacon_hostname (node1));
    assert (*zbeacon_hostname (node2));
    assert (*zbeacon_hostname (node3));

    zbeacon_set_interval (node1, 250);
    zbeacon_set_interval (node2, 250);
    zbeacon_set_interval (node3, 250);
    zbeacon_noecho (node1);
    zbeacon_publish (node1, (byte *) "NODE/1", 6);
    zbeacon_publish (node2, (byte *) "NODE/2", 6);
    zbeacon_publish (node3, (byte *) "GARBAGE", 7);
    zbeacon_subscribe (node1, (byte *) "NODE", 4);

    //  Poll on API pipe and on UDP socket
    zmq_pollitem_t pollitems [] = {
        { zbeacon_pipe (node1), 0, ZMQ_POLLIN, 0 },
        { zbeacon_pipe (node2), 0, ZMQ_POLLIN, 0 },
        { zbeacon_pipe (node3), 0, ZMQ_POLLIN, 0 }
    };
    long stop_at = zclock_time () + 1000;
    while (zclock_time () < stop_at) {
        long timeout = (long) (stop_at - zclock_time ());
        if (timeout < 0)
            timeout = 0;
        if (zmq_poll (pollitems, 3, timeout * ZMQ_POLL_MSEC) == -1)
            break;              //  Interrupted

        //  We cannot get messages on nodes 2 and 3
        assert ((pollitems [1].revents & ZMQ_POLLIN) == 0);
        assert ((pollitems [2].revents & ZMQ_POLLIN) == 0);

        //  If we get a message on node 1, it must be NODE/2
        if (pollitems [0].revents & ZMQ_POLLIN) {
            char *ipaddress = zstr_recv (zbeacon_pipe (node1));
            char *beacon = zstr_recv (zbeacon_pipe (node1));
            assert (streq (beacon, "NODE/2"));
            free (ipaddress);
            free (beacon);
        }
    }
    //  Stop listening
    zbeacon_unsubscribe (node1);

    //  Stop all node broadcasts
    zbeacon_silence (node1);
    zbeacon_silence (node2);
    zbeacon_silence (node3);

    //  Destroy the test nodes
    zbeacon_destroy (&node1);
    zbeacon_destroy (&node2); 
    zbeacon_destroy (&node3);
}
