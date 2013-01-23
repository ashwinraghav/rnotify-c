//  Weather proxy device

#include "zhelpers.h"
#include "czmq.h"

#include<sys/inotify.h>
#define SUB_SOCK "tcp://localhost:6001"

void print_notifications(char *buff, ssize_t len, int *count);
static char* safe_recv_from_proxy (void *socket, int *size);

int main (void)
{
	zctx_t *ctx = zctx_new ();
	void *subscriber = zsocket_new (ctx, ZMQ_SUB);
	zsocket_set_hwm(subscriber, 100000); 	
	zsocket_connect (subscriber, SUB_SOCK);
	char* filter="";
	int count = 0;
	zmq_setsockopt (subscriber, ZMQ_SUBSCRIBE, filter, strlen (filter));

	while(1)
	{
		int size;
		char *string = safe_recv_from_proxy (subscriber, &size);
		//printf("receiving something \n");
		print_notifications(string, size, &count);
		free (string);
	}
	//zmq_close (subscriber);
	zctx_destroy (&ctx);
	return 0;
}
void print_notifications(char *buff, ssize_t len, int* count)
{
	ssize_t i = 0;
	char action[81+FILENAME_MAX] = {0};


        while (i < len) {
                struct inotify_event *pevent = (struct inotify_event *)&buff[i];
	

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
		*count = *count + 1;
		printf("Count = %d \n", *count);
                i += sizeof(struct inotify_event) + pevent->len;

        }

}

static char* safe_recv_from_proxy (void *socket, int *size) {
	zmq_msg_t message;
	zmq_msg_init (&message);
	*size = zmq_msg_recv (&message, socket, 0);
	//printf("Method thinks the length is %d", *size);
	if (*size == -1)
		return NULL;
	char *string = malloc (*size + 1);
	memcpy (string, zmq_msg_data (&message), *size);
	zmq_msg_close (&message);
	string [*size] = 0;
	return (string);
}
