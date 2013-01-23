//  Weather update server
//  Binds PUB socket to tcp://*:5556
//  Publishes random weather updates

#include "zhelpers.h"
#include "czmq.h"
#include "unistd.h"
#include <sys/inotify.h>
#define BUFF_SIZE ((sizeof(struct inotify_event)+FILENAME_MAX)*1024)
#define SEND_SOCK "tcp://*:5556"
//#define SEND_SOCK "ipc:///tmp/com"

void get_event_at_server (int fd, void* publisher);
int send_message_to_dispatchers (char* update, int size, void* publisher);
void handle_error (int error);
static int safe_send (void *socket, char *string, size_t len);

int main ()
{
        int result;
        int fd;
        int wd;   /* watch descriptor */
	void *context = zmq_ctx_new ();
	void *publisher = zmq_socket (context, ZMQ_PUSH);
	zsocket_set_hwm(publisher, 100000); 	
	zmq_bind (publisher, SEND_SOCK);


        fd = inotify_init();
        if (fd < 0) {
                handle_error (errno);
                return 1;
        }
        
        wd = inotify_add_watch (fd, "/localtmp/dump/1", IN_ALL_EVENTS);
        wd = inotify_add_watch (fd, "/localtmp/dump/2", IN_ALL_EVENTS);
        wd = inotify_add_watch (fd, "/localtmp/dump/3", IN_ALL_EVENTS);
        wd = inotify_add_watch (fd, "/localtmp/dump/4", IN_ALL_EVENTS);
        if (wd < 0) {
                handle_error (errno);
                return 1;
        }

        while (1) {
                get_event_at_server(fd, publisher);
        }
	zmq_close (publisher);
	zmq_ctx_destroy (context);

        return 0;
}

int send_message_to_dispatchers (char* update, int len, void* publisher)
{
    int sent_size = safe_send(publisher, update, len);

    printf("Sent Length is %d \n", sent_size);

    //printf("sent something\n");
    return 0;
}

void get_event_at_server (int fd, void* publisher)
{
        ssize_t len, i = 0;
        char action[81+FILENAME_MAX] = {0};
        char buff[BUFF_SIZE] = {0};

        len = read (fd, buff, BUFF_SIZE);
        send_message_to_dispatchers(buff, len, publisher);

	while (i < len) {
		struct inotify_event *pevent = (struct inotify_event *)&buff[i];
		char action[81+FILENAME_MAX] = {0};

                if (pevent->len) 
                strcpy (action, pevent->name);
                else
                strcpy (action, "some random directory");

                if (pevent->mask & IN_Q_OVERFLOW)
                fprintf(stderr, "overflowingi*******************************\n");

               // printf ("wd=%d mask=%d cookie=%d len=%d dir=%s\n", pevent->wd, pevent->mask, pevent->cookie, pevent->len, (pevent->mask & IN_ISDIR)?"yes":"no");

  //              printf ("%s\n", action);

                i += sizeof(struct inotify_event) + pevent->len;

	}

}

void handle_error (int error)
{
        fprintf (stderr, "Error: %s\n", strerror(error));

}

static int safe_send (void *socket, char *string, size_t len) {
            zmq_msg_t message;
            zmq_msg_init_size (&message, len);
            memcpy (zmq_msg_data (&message), string, len);
            int size = zmq_msg_send (&message, socket, 0);
            zmq_msg_close (&message);
            return (size);
}
