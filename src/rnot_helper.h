
#ifndef RNOT_HELPER_HEADER_INCLUDED
#define RNOT_HELPER_HEADER_INCLUDED

#include <sys/inotify.h>
#include <zmq.h>
#include <czmq.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <uuid/uuid.h>
#include "ip.h"


#define REPLICATION_FACTOR 3

#define REGISTER_PUBLISHER_SANITY_CHECK "1756372"
#define REGISTER_DISPATCHER_SANITY_CHECK "6577392"

#define PROXY_FLUSH_PORT "1000"
#define PROXY_SUBSCRIBE_PORT "2000"
#define DISPATCH_PORT "3000"
#define DISPATCHER_NOTIFY_PORT "4000"
#define PUBLISH_PORT "5000"
#define REGISTER_PORT "6000"

#define SOCK_BIND 17273
#define SOCK_CONNECT 276346

#ifdef PRODUCTION
	#define REGISTRATION_ADDR "tcp://*:" REGISTER_PORT
#else
	#define REGISTRATION_ADDR "ipc:///tmp/" REGISTER_PORT
#endif

//error check - print if erroneous
#define CHECK(x) do { \
	int retval = (x); \
	if (retval < 0) { \
		fprintf(stderr, "Runtime error: %s returned %s at %s:%d", #x, strerror(errno), __FILE__, __LINE__); \
	} \
} while (0)


static void two_phase_notify(void *socket, struct inotify_event *pevent)
{
	int serial_length = sizeof(struct inotify_event) + pevent->len;
	char *filter = malloc(10);

	//create a frame with wd as the filter
	sprintf(filter, "%d", pevent->wd);

	fprintf(stderr, "Publishing with the filters %s", pevent->name);

	zframe_t *content_frame, *filter_frame = zframe_new(filter, strlen(filter));
	assert(content_frame = zframe_new ((char*)(pevent), serial_length));

	//send the filter frame as the first part
	zframe_send (&filter_frame, socket, ZFRAME_REUSE + ZFRAME_MORE);

	//send the content frame as the second part.
	//Will be received only by relevant
	//subscribers
	zframe_send (&content_frame, socket, 0);

	free(filter);
}

static void two_phase_register(void *socket, char *ip, char *registration_type){
	zframe_t *frame1, *frame2;

	assert(frame1 = zframe_new(registration_type, 
			strlen(registration_type)));

	assert(frame2 = zframe_new (ip, strlen(ip)));

	//send the registration frame followed by ip
	zframe_send (&frame1, socket, ZFRAME_REUSE + ZFRAME_MORE);
	zframe_send (&frame2, socket, 0);
}

static char** two_part_receive(void *socket, int *size)
{
	zframe_t *part1, *part2;
	char **ret = (char**) malloc(2*sizeof(char*));	

	//receive the first part containing the message filter
	zmsg_t *msg = zmsg_recv (socket); 
	
	assert(part1 = zmsg_pop (msg));
	assert(part2 = zmsg_pop (msg));
        
	ret[0] = zframe_strdup(part1);
	ret[1] = zframe_strdup(part2);

	*size = zframe_size(part1);
	
	zframe_destroy(&part1);
	zframe_destroy(&part2);
        zmsg_destroy (&msg);
	
	return(ret);
}

static int safe_send(void *socket, char *string, size_t len) {
	int rc; zmsg_t *msg ; zframe_t *frame;;
	
	assert (msg = zmsg_new());
	assert(frame = zframe_new (string, len));
	
	zmsg_push (msg, frame);
	
	assert(zmsg_size (msg) == 1); assert (zmsg_content_size (msg) == len);
	assert(zmsg_send (&msg, socket) == 0); assert (msg == NULL);
	return (len);
}

static char* safe_recv(void *socket, int *size) {
	int rc; zmsg_t *msg ; zframe_t *frame;
	
	assert(msg = zmsg_recv (socket));
	assert(frame = zmsg_pop(msg));

	char *string = zframe_strdup (frame);
	*size = zframe_size(frame);
	
	zframe_destroy(&frame);
	zmsg_destroy (&msg);
	
	return (string);
}

static void print_notifications(struct inotify_event *pevent)
{

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


	fprintf (stderr, "wd=%d mask=%d cookie=%d len=%d dir=%s\n",pevent->wd, pevent->mask, pevent->cookie, pevent->len,  (pevent->mask & IN_ISDIR)?"yes":"no");

	if (pevent->len) 
		fprintf (stderr, "name=%s\n", pevent->name);
}

static void print_error (int error)
{
	fprintf (stderr, "Error: %s\n", strerror(error));

}

static void* create_socket(zctx_t *ctx, int type, int mode, char* address)
{
	void *sock = zsocket_new (ctx, type);
	zsocket_set_hwm(sock, 100000);
	if(mode == SOCK_BIND) 	
		zsocket_bind(sock, address);
	else if (mode == SOCK_CONNECT)
		zsocket_connect(sock, address);

	return sock;
}

static char* register_notification(int fd, char* file_name){
	int wd;
	char *str = malloc(10);
	CHECK(wd = inotify_add_watch (fd, file_name, IN_ALL_EVENTS)); 
	printf("\n added watch for %s", file_name);
	sprintf(str, "%d", wd);
	return str;
}

static void self_register(zctx_t *ctx, char* registration_type){
	void *register_sock = create_socket(ctx, ZMQ_PUSH, SOCK_CONNECT, REGISTRATION_ADDR);
	char *my_ip_address = (char*) get_ip_address();
	two_phase_register(register_sock, my_ip_address, registration_type);
	free(my_ip_address);
}
#endif
