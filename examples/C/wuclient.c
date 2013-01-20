//  Weather update client
//  Connects SUB socket to tcp://localhost:5556
//  Collects weather updates and finds avg temp in zipcode

#include "zhelpers.h"
#include<sys/inotify.h>

#define RECEIVE_SOCKET "tcp://localhost:5556"
#define DISPATCH_SOCKET "tcp://*:6500"

void parse_notifications(char *buff, ssize_t len);
static char* safe_recv_from_server (void *socket, int *size);
static int safe_send_to_proxy (void *socket, char *string, size_t len);
int dispatch (char* update, int size);

int main (int argc, char *argv [])
{
    //  Socket to talk to server
    printf ("Collecting updates from weather server...\n");
    void *context = zmq_ctx_new ();
    void *subscriber = zmq_socket (context, ZMQ_PULL);
    int rc = zmq_connect (subscriber, RECEIVE_SOCKET);
    assert (rc == 0);

    //  Subscribe to zipcode, default is NYC, 10001
    while(1)
    {
	int size;
        char *string = safe_recv_from_server (subscriber, &size);
        parse_notifications(string, size);
        free (string);
    }

    zmq_close (subscriber);
    zmq_ctx_destroy (context);
    return 0;
}
int dispatch(char* update, int len)
{
	len = 10;
    	void *context = zmq_ctx_new ();
	void *dispatch_socket = zmq_socket (context, ZMQ_PUB);
	
	int rc = zmq_bind(dispatch_socket, DISPATCH_SOCKET);
	assert(rc == 0);

	int sent_size = safe_send_to_proxy(dispatch_socket, "1234567890", len);
    	printf("dispatching Length is %d \n", sent_size);
	zmq_close(dispatch_socket);
	zmq_ctx_destroy(context);
	return 0;

}
void parse_notifications(char *buff, ssize_t len)
{
        ssize_t i = 0;
	char action[81+FILENAME_MAX] = {0};


        while (i < len) {
                struct inotify_event *pevent = (struct inotify_event *)&buff[i];
	
		dispatch(pevent, sizeof(struct inotify_event));	

                char action[81+FILENAME_MAX] = {0};

                if (pevent->len) 
                strcpy (action, pevent->name);
                else
                strcpy (action, "some random directory");

                if (pevent->mask & IN_ACCESS) 
                strcat(action, " was read");
                if (pevent->mask & IN_ATTRIB) 
                strcat(action, " Metadata changed");
                if (pevent->mask & IN_CLOSE_WRITE) 
                strcat(action, " opened for writing was closed");
                if (pevent->mask & IN_CLOSE_NOWRITE) 
                strcat(action, " not opened for writing was closed");
                if (pevent->mask & IN_CREATE) 
                strcat(action, " created in watched directory");
                if (pevent->mask & IN_DELETE) 
                strcat(action, " deleted from watched directory");
                if (pevent->mask & IN_DELETE_SELF) 
                strcat(action, "Watched file/directory was itself deleted");
                if (pevent->mask & IN_MODIFY) 
                strcat(action, " was modified");
                if (pevent->mask & IN_MOVE_SELF) 
                strcat(action, "Watched file/directory was itself moved");
                if (pevent->mask & IN_MOVED_FROM) 
                strcat(action, " moved out of watched directory");
                if (pevent->mask & IN_MOVED_TO) 
                strcat(action, " moved into watched directory");
                if (pevent->mask & IN_OPEN) 
                strcat(action, " was opened");

                
                //printf ("wd=%d mask=%d cookie=%d len=%d dir=%s\n",pevent->wd, pevent->mask, pevent->cookie, pevent->len,  (pevent->mask & IN_ISDIR)?"yes":"no");

                //if (pevent->len) 
		//	printf ("name=%s\n", pevent->name);
                

                //printf ("%s\n", action);

                i += sizeof(struct inotify_event) + pevent->len;

        }

}

static char* safe_recv_from_server (void *socket, int *size) {
	zmq_msg_t message;
	zmq_msg_init (&message);
	*size = zmq_msg_recv (&message, socket, 0);
	if (*size == -1)
		return NULL;
	char *string = malloc (*size + 1);
	memcpy (string, zmq_msg_data (&message), *size);
	zmq_msg_close (&message);
	string [*size] = 0;
	return (string);
}

static int safe_send_to_proxy(void *socket, char *string, size_t len) {
            zmq_msg_t message;
            zmq_msg_init_size (&message, len);
            memcpy (zmq_msg_data (&message), string, len);
            int rc = zmq_msg_send (&message, socket, 0);
	    assert(rc != -1);
            zmq_msg_close (&message);
            return (rc);
}
