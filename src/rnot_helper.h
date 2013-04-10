
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
#include <sys/time.h>
#include <math.h>

#define REPLICATION_FACTOR 10
#define PRODUCTION
#define REGISTER_PUBLISHER_SANITY_CHECK "1756372"
#define REGISTER_DISPATCHER_SANITY_CHECK "6577392"
#define REGISTER_FILE_OBJECT_SANITY_CHECK "63540274"
#define REGISTER_OK "hfjjdhfnc8"

#define PROXY_SUBSCRIBE_PORT "2000"
#define ACCEPT_PORT "*"
#define DISPATCHER_NOTIFY_PORT "4000"
#define PUBLISH_PORT "*"
#define REGISTER_PORT "6000"
#define PROXY_FLUSH_PORT "7000"
#define INITIATE_PORT "8500"
#define COLLECT_PORT "8700"

#define SOCK_BIND 17273
#define SOCK_CONNECT 276346

#define REGISTRAR_IP_ADDR "10.155.233.145"
#define FILE_HOST_IP_ADDR "10.155.233.145"

#ifdef PRODUCTION
	#define REGISTRATION_ADDR "tcp://" REGISTRAR_IP_ADDR ":" REGISTER_PORT
	#define TEST_INITIATE_ADDR "tcp://" REGISTRAR_IP_ADDR ":" INITIATE_PORT
	#define TEST_COLLECT_ADDR "tcp://" REGISTRAR_IP_ADDR ":" COLLECT_PORT
#else
	#define REGISTRATION_ADDR "ipc:///tmp/" REGISTER_PORT
	#define TEST_INITIATE_ADDR "ipc:///tmp/" INITIATE_PORT
	#define TEST_COLLECT_ADDR "ipc:///tmp/" COLLECT_PORT
#endif

//error check - print if erroneous
#define CHECK(x) do { \
	int retval = (x); \
	if (retval < 0) { \
		fprintf(stderr, "Runtime error: %s returned %s at %s:%d", #x, strerror(errno), __FILE__, __LINE__); \
	} \
} while (0)
static int64_t my_clock (void)
{
#if (defined (__WINDOWS__))
	SYSTEMTIME st;
	GetSystemTime (&st);
	return (int64_t) st.wSecond * 1000 + st.wMilliseconds;
#else
	struct timeval tv;
	gettimeofday (&tv, NULL);
	return (int64_t) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}
typedef struct rnot_struct{
	zctx_t *ctx;
	GHashTable* publisher_socks;
	int64_t start_time;
	void* subscriber, *tester, *result_collect_socket;
}rnot;

static char* to_c_string(char * str, int str_len, int size);
static void multi_part_send(void* const socket, zframe_t **content, int size)
{
	int i=0;
	int code = ZFRAME_REUSE + ZFRAME_MORE;
	for(i = 0; i < size; i++){
		if(i == size -1)
			code = 0;
		zframe_send(&content[i], socket, code);
	}
}

static void notify(void *socket, const struct inotify_event* const pevent)
{
	char* const filter = malloc(10);
	sprintf(filter, "%d", pevent->wd);

	zframe_t **content = malloc(sizeof(zframe_t*) * 2);
	int serial_length = sizeof(struct inotify_event) + pevent->len;

	assert(content[0] = zframe_new(filter, strlen(filter)));
	assert(content[1] = zframe_new((char*)pevent, serial_length));

	multi_part_send(socket, content, 2);	

	free(filter);
	free(content[0]);
	free(content[1]);
	free(content);
}

static char** multi_part_receive(void* const socket, int* const size)
{
	zmsg_t *msg = zmsg_recv (socket); 
	int i =0;

	*size = zmsg_size(msg);
	char **ret = (char**) malloc((*size) * sizeof(char*));	
	
	for(i = 0; i < *size; i++){
		
		assert(ret[i] = zmsg_popstr(msg));
	}	
	
        zmsg_destroy (&msg);
	
	return(ret);
}


static char** _register(void* const socket, const char* const registration_body, const char* const registration_type, int *response_size){
	zframe_t **content = malloc(sizeof(zframe_t*) * 2);
	
	assert(content[0] = zframe_new(registration_type, strlen(registration_type)));
	assert(content[1] = zframe_new(registration_body, strlen(registration_body)));
	
	multi_part_send(socket, content, 2);

	free(content[0]);
	free(content[1]);
	free(content);

	char ** registration_response = multi_part_receive(socket, response_size);
	return registration_response;
}

static int _send(void* const socket, const char** const string, size_t len) {
	zframe_t **content = malloc(sizeof(zframe_t*) * len);
	int i = 0;

	for(i = 0;i<len;i++){
		assert(content[i] = zframe_new(string[i], strlen(string[i])));
	}
	
	multi_part_send(socket, content, len);
	for(i = 0;i<len;i++){
		free(content[i]);
	}

	free(content);
	return (len);
}

static char* _recv_buff(void* const socket, int* size) {
	int rc; zmsg_t *msg ; zframe_t *frame;

	assert(msg = zmsg_recv (socket));
	assert(frame = zmsg_pop(msg));

	char *string = zframe_strdup (frame);
	*size = zframe_size(frame);

	zframe_destroy(&frame);
	free(frame);
	zmsg_destroy (&msg);

	return (string);
}

static int _send_buff(void* const socket, const char* const string, size_t len) {
	
	char ** buff = malloc(sizeof(char*) * 1);
	buff[0] = malloc(sizeof(char) * (len + 1));
	memcpy(buff[0], string, len);
	buff[0][len] = 0;
	_send(socket, (const char** const )buff, 1);
	free(buff);
	free(buff[0]);
	return len;	
}

static int _send_string(void* const socket, const char* const string, size_t len) {
	
	//int size = zmq_send (socket, string, strlen (string), 0);
	
	zframe_t **frame = malloc(sizeof(zframe_t *) * 1);
	assert(frame[0] = zframe_new(string, len));

	multi_part_send(socket, frame, 1);

	free(frame[0]);
	free(frame);

	
	//free(content[1]);
	//free(content);


	//assert (msg = zmsg_new());
	//assert(frame = zframe_new (string, len));
	
	//zmsg_push (msg, frame);
	
	//assert(zmsg_size (msg) == 1); assert (zmsg_content_size (msg) == len);
	//assert(zmsg_send (&msg, socket) == 0); assert (msg == NULL);

	//free(frame);
	return (len);
}

static void print_notifications(const struct inotify_event* const pevent, void *args)
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
	if(strcmp("foo", pevent->name) == 0){
		fprintf(stderr, "\nGot the file");
	}

	//fprintf (stderr, "wd=%d mask=%d cookie=%d len=%d dir=%s\n",pevent->wd, pevent->mask, pevent->cookie, pevent->len,  (pevent->mask & IN_ISDIR)?"yes":"no");

	if(strcmp("foo", pevent->name) == 0){
		fprintf(stderr, "Foo File \n");
	}
//	if (pevent->len) 
//		fprintf (stderr, "name=%s\n", pevent->name);
}


static void print_error (int error)
{
	fprintf (stderr, "Error: %s\n", strerror(error));

}


static int scan_and_bind_socket(zctx_t* const ctx, void** sock, int type, const char* const address)
{
	*sock = zsocket_new (ctx, type);
	//zsocket_set_hwm(*sock, 100000);
	return zsocket_bind(*sock, address);
}

static void* create_socket(zctx_t* const ctx, int type, int mode, const char* const address)
{
	void* sock = zsocket_new (ctx, type);
	//zsocket_set_hwm(sock, 100000);
	if(mode == SOCK_BIND) 	
		zsocket_bind(sock, address);
	else if (mode == SOCK_CONNECT)
		zsocket_connect(sock, address);

	return sock;
}

static char* register_notification(int fd, const char* const file_name){
	int wd;
	char* const str = malloc(10);
	CHECK(wd = inotify_add_watch (fd, file_name, IN_ALL_EVENTS)); 
	fprintf(stderr, "\n added watch for %s with wd %d", file_name, wd);
	sprintf(str, "%d", wd);
	return str;
}

static char** register_publisher_service(zctx_t* const ctx, int accept_port, int publish_port, int * const response_size){

	char* const accept_port_string = malloc(10);
	char* const publish_port_string = malloc(10);

	sprintf(accept_port_string, "%d", accept_port);
	sprintf(publish_port_string, "%d", publish_port);


	void* const register_sock = create_socket(ctx, ZMQ_REQ, SOCK_CONNECT, REGISTRATION_ADDR);
	const char* const ip = (char*) get_ip_address();
	char* const ip_string = to_c_string("tcp://", strlen("tcp://") + 1, strlen("tcp://") + 1 + strlen(ip));
	strcat(ip_string, ip);

	zframe_t **content = malloc(sizeof(zframe_t*) * 4);
	
	assert(content[0] = zframe_new(REGISTER_PUBLISHER_SANITY_CHECK, strlen(REGISTER_PUBLISHER_SANITY_CHECK)));
	assert(content[1] = zframe_new(ip_string, strlen(ip_string)));
	assert(content[2] = zframe_new(accept_port_string, strlen(accept_port_string)));
	assert(content[3] = zframe_new(publish_port_string, strlen(publish_port_string)));
	
	multi_part_send(register_sock, content, 4);
	
	free(content[0]);
	free(content[1]);
	free(content[2]);
	free(content[3]);
	free(content);
	free((void*)ip);
	free((void*)ip_string);
	free(accept_port_string);
	free(publish_port_string);

	char ** registration_response = multi_part_receive(register_sock, response_size);
	return registration_response;
}




static char** self_register(zctx_t* const ctx, const char* const registration_type, const char* const my_port, int * const response_size){
	void* const register_sock = create_socket(ctx, ZMQ_REQ, SOCK_CONNECT, REGISTRATION_ADDR);
	const char* const my_ip_address = (char*) get_ip_address();
	char* const address = malloc(
				sizeof(char)*(
				strlen("tcp://") +
				strlen(my_ip_address) +
				strlen(my_port) + 
				strlen(":")
				)+1);
	
	strcat(strcat(strcat(strcpy(address,"tcp://"), my_ip_address), ":"), my_port);
	char ** registration_response = _register(register_sock, address, registration_type, response_size);

	free((void*) my_ip_address);
	free((void*) address);
	zsocket_destroy (ctx, register_sock);

	//free sock
	return registration_response;
}

//returns a new c string. Needs to be freed
static char* to_c_string(char * str, int str_len, int size)
{
	char* const c_string = malloc (sizeof(char) * (size) );
	memcpy(c_string, str, str_len);
	c_string[str_len] = '\0';
	return c_string;
}

static void* create_new_hash_table(){
	//zhash_t *hash = zhash_new ();
	return g_hash_table_new(g_str_hash, g_str_equal);
}

static void insert_into_table(void* table, char* key, void* value){
	zhash_insert((zhash_t*) table,key, value);
}

static void* table_lookup(void* table, char* key){
	return zhash_lookup (table, key);
}
#endif
