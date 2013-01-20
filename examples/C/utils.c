#include "zhelpers.h"
#include <sys/inotify.h>
#include <utils.h>

static int safe_send (void *socket, char *string, size_t len) {
            zmq_msg_t message;
            zmq_msg_init_size (&message, len);
            memcpy (zmq_msg_data (&message), string, len);
            int size = zmq_msg_send (&message, socket, 0);
            zmq_msg_close (&message);
            return (size);
}
