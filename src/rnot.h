#ifndef RNOT_HEADER_INCLUDED
#define RNOT_HEADER_INCLUDED
#include "zhelpers.h"
#include "czmq.h"
#include "unistd.h"
#include <glib.h>
#include<sys/inotify.h>


#ifdef PRODUCTION
	#define LISTEN_SOCK "tcp://localhost:6001"
	#define SUBSCRIBE_TO_SOCK "tcp://localhost:5558"
#else
	#define LISTEN_SOCK "ipc:///tmp/6001"
	#define SUBSCRIBE_TO_SOCK "ipc:///tmp/5558"
#endif
typedef struct rnot_struct{
	zctx_t *ctx;
	GHashTable* hash;
	void *subscriber;
	void *listener;
}rnot;

rnot* rnotify_init()
{
	rnot *rn = malloc(sizeof(rnot));
	rn->ctx = zctx_new();
	rn->hash = g_hash_table_new(g_str_hash, g_str_equal);
	
	rn->listener = create_socket(rn->ctx, ZMQ_SUB, SOCK_CONNECT, LISTEN_SOCK);
	rn->subscriber = create_socket(rn->ctx, ZMQ_REQ, SOCK_CONNECT, SUBSCRIBE_TO_SOCK);

	return (rn);
}

//call does not return until registration is complete
void rsubscribe(rnot *rn, char* file_path){

	//Sends the Subscription to the file host as a REQ
	printf("\nSubscribing");
	safe_send(rn->subscriber, file_path, strlen(file_path));

	//Received the registration Id as a REP
	int size;
	char *registration_id = safe_recv(rn->subscriber, &size);
	printf("\n added watch with id %s", registration_id);
	
	//Map file name to regId
	g_hash_table_insert(rn->hash,file_path, registration_id);
	
	//Set filter with the regId
	zmq_setsockopt (rn->listener, ZMQ_SUBSCRIBE, registration_id, strlen (registration_id));

	free(registration_id);
}

//will start a new listener every time it is invoked. 
void start_listener(rnot *rn, void (*handler)(struct inotify_event*)){
	int count = 0;
	
	while(true)
	{
		int len=0;
		char **buff = two_part_receive(rn->listener, &len);
		ssize_t current_pos = 0;
		while (current_pos < len) {
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
