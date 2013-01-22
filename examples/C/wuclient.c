//  Weather update client
//  Connects SUB socket to tcp://localhost:5556
//  Collects weather updates and finds avg temp in zipcode

#include "zhelpers.h"
#include<sys/inotify.h>

#define RECEIVE_SOCKET "tcp://localhost:5556"
#define DISPATCH_SOCKET "tcp://localhost:6500"

void parse_notifications(char *buff, ssize_t len, void* dispatch_socket);
static char* safe_recv_from_server (void *socket, int *size);
static int safe_send_to_proxy (void *socket, char *string, size_t len);
int dispatch (char* update, int size, void* dispatch_socket);
void output_notifications(struct inotify_event *pevent, char * action);

int main (int argc, char *argv [])
{
	//  Socket to talk to server
	printf ("Collecting updates from weather server...\n");
	void *context = zmq_ctx_new ();
	void *subscriber = zmq_socket (context, ZMQ_PULL);
	zsocket_set_hwm(subscriber, 100000); 	
	int rc = zmq_connect (subscriber, RECEIVE_SOCKET);
	assert (rc == 0);


	void *dispatch_socket = zmq_socket (context, ZMQ_PUB);
	zsocket_set_hwm(dispatch_socket, 100000); 	
	rc = zmq_connect(dispatch_socket, DISPATCH_SOCKET);
	assert(rc == 0);

	while(1)
	{
		int size;
		char *string = safe_recv_from_server (subscriber, &size);
		parse_notifications(string, size, dispatch_socket);
		free (string);
	}

	zmq_close (subscriber);
	zmq_close(dispatch_socket);
	zmq_ctx_destroy (context);
	return 0;
}
int dispatch(char* update, int len, void* dispatch_socket)
{
	//len = 10;

	//char string[10];
	//printf("arriving here \n");
	//sprintf (string, "A-asd\0");
	int sent_size = safe_send_to_proxy(dispatch_socket, update, len);
	//if(zstr_send(dispatch_socket, string) == -1){
	//	printf("The error is %s \n",zmq_strerror (errno));
	//}
	//printf("Something sent on wire \n");
	//printf("dispatching Length is %d \n", sent_size);
	return 0;

}
void parse_notifications(char *buff, ssize_t len, void* dispatch_socket)
{
	ssize_t i = 0;
	char action[81+FILENAME_MAX] = {0};
	int wd = -1;
	int start_position = i;

	while (i < len) {
		struct inotify_event *pevent = (struct inotify_event *)&buff[i];

		if (pevent->len){
			if((wd == -1) || (wd != pevent->wd)){
				if(wd != -1)
				{
					printf("dispatching\n");
					int serial_length = i - start_position;
					dispatch(&buff[start_position], serial_length, dispatch_socket);
					//dispatch(&buff[i], serial_length, dispatch_socket);
					//sizeof(struct inotify_event) + pevent->len;
				}
				wd = pevent->wd;
				start_position = i;
			}

		}
		i += sizeof(struct inotify_event) + pevent->len;
		output_notifications(pevent,action);

	}
	if(i != start_position){
		printf("dispatching\n");
		int serial_length = i - start_position;
		dispatch(&buff[start_position], serial_length, dispatch_socket);
	}

}

void output_notifications(struct inotify_event *pevent, char * action)
{
	if(pevent->len)
	{
		strcpy (action, pevent->name);
	}
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
	printf ("wd=%d mask=%d cookie=%d len=%d dir=%s\n",pevent->wd, pevent->mask, pevent->cookie, pevent->len,  (pevent->mask & IN_ISDIR)?"yes":"no");

	if (pevent->len) 
		printf ("name=%s\n", pevent->name);


	printf ("%s\n", action);
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
//char *serialized_event=malloc(serial_length + 1);

//memcpy (serialized_event, &buff[i], serial_length );
//serialized_event[serial_length]='\0';
//dispatch(serialized_event, serial_length, dispatch_socket);	
