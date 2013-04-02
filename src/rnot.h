#ifndef RNOT_HEADER_INCLUDED
#define RNOT_HEADER_INCLUDED
#include "zhelpers.h"
#include "czmq.h"
#include "unistd.h"
#include <glib.h>
#include<sys/inotify.h>


#ifdef PRODUCTION
	#define LISTEN_SOCK "tcp://localhost:" PUBLISH_PORT
#else
	#define LISTEN_SOCK "ipc:///tmp/" PUBLISH_PORT
#endif
typedef struct rnot_struct{
	zctx_t *ctx;
	GHashTable* hash;
	void* subscriber;
	void*listener;
}rnot;

const rnot* rnotify_init()
{
	rnot *rn = malloc(sizeof(rnot));
	rn->ctx = zctx_new();
	rn->hash = g_hash_table_new(g_str_hash, g_str_equal);
	
	rn->listener = create_socket(rn->ctx, ZMQ_SUB, SOCK_CONNECT, LISTEN_SOCK);
	rn->subscriber = create_socket(rn->ctx, ZMQ_REQ, SOCK_CONNECT, REGISTRATION_ADDR);

	return (rn);
}

//call does not return until registration is complete
void rsubscribe(const rnot* const rn, char* const file_path){

	fprintf(stderr, "\nSubscribing");
	int size;
	char ** registration_response = _register(rn->subscriber, file_path, REGISTER_FILE_OBJECT_SANITY_CHECK, &size);
	fprintf(stderr, "\n added watch with id %s", registration_response[1]);

		
	//Map file name to regId
	//g_hash_table_insert(rn->hash,file_path, registration_response);
		
	//Set filter with the regId
	zmq_setsockopt (rn->listener, ZMQ_SUBSCRIBE, "1", strlen ("1"));
	//nothing freed yet
}

//will start a new listener every time it is invoked. 
void start_listener(const rnot* const rn, void (*handler)(const struct inotify_event* const p)){
	int count = 0;
	
	while(true)
	{
		int len=0;
		char **buff = multi_part_receive(rn->listener, &len);
		ssize_t current_pos = 0;
		int buff_len = strlen(buff[1]);
		while (current_pos < buff_len) {
			struct inotify_event *pevent = (struct inotify_event *)&buff[1][current_pos];
			(*handler)(pevent);
			count = count + 1;
			printf("Count = %d \n", count);
			current_pos += sizeof(struct inotify_event) + pevent->len;
		}

		free (buff[0]);
		free (buff[1]);
	}
}

//caveats
//1. The Hash Table's Values are on the heap and never freed
//2. Contents of the rn are never freed
#endif
