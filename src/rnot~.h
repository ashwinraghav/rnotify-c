#ifndef RNOT_HEADER_INCLUDED
#define RNOT_HEADER_INCLUDED
#include "zhelpers.h"
#include "czmq.h"
#include "unistd.h"
#include <glib.h>
#include<sys/inotify.h>


#ifdef PRODUCTION
	#define LISTEN_ADDR "tcp://localhost:" PUBLISH_PORT
#else
	#define LISTEN_ADDR "ipc:///tmp/" PUBLISH_PORT
#endif
typedef struct rnot_struct{
	zctx_t *ctx;
	GHashTable* hash;
	GHashTable* publisher_socks;

	void* subscriber;
	void* listener;
}rnot;

int cast_notification(zloop_t *loop, zmq_pollitem_t *poller, void *arg);

const rnot* rnotify_init()
{
	rnot *rn = malloc(sizeof(rnot));
	rn->ctx = zctx_new();
	rn->hash = g_hash_table_new(g_str_hash, g_str_equal);
	rn->publisher_socks = g_hash_table_new(g_str_hash, g_str_equal);
	
	rn->listener = create_socket(rn->ctx, ZMQ_SUB, SOCK_CONNECT, LISTEN_ADDR);
	rn->subscriber = create_socket(rn->ctx, ZMQ_REQ, SOCK_CONNECT, REGISTRATION_ADDR);
	
	zsocket_set_subscribe(rn->listener, "");

	return (rn);
}

//call does not return until registration is complete
void rsubscribe(const rnot* const rn, char* const file_path, void (*handler)(const struct inotify_event* const p )){

	fprintf(stderr, "\nSubscribing");
	int size;
	char ** registration_response = _register(rn->subscriber, file_path, REGISTER_FILE_OBJECT_SANITY_CHECK, &size);
	free(registration_response[0]);
	free(registration_response[1]);
	fprintf(stderr, "\n added watch with id %s", registration_response[1]);

	int i=0;
	for(i = 2; i < size; i++){
		fprintf(stderr, "\n%s is a publisher", registration_response[i]);
		void* sock = g_hash_table_lookup(rn->publisher_socks, "tcp://192.168.1.2:5000");

		if(sock == NULL){
			fprintf(stderr, "\ncreating new sock");
			sock = create_socket(rn->ctx, ZMQ_SUB, SOCK_CONNECT, "tcp://192.168.1.2:5000");
			//void *psock = create_socket(rn->ctx, ZMQ_SUB, SOCK_CONNECT, registration_response[i]);
			
			g_hash_table_insert(rn->publisher_socks, "tcp://192.168.1.2:5000", sock);
		}
		zsocket_set_subscribe(sock, "");
		
	}	
	free(registration_response);

	//Map file name to regId
	//g_hash_table_insert(rn->hash,file_path, registration_response);
		
	//Set filter with the regId
	//zmq_setsockopt (rn->listener, ZMQ_SUBSCRIBE, "1", strlen ("1"));
	//nothing freed yet
}

void start_listener(const rnot* const rn){
	int i = 0;
	int table_size = g_hash_table_size(rn->publisher_socks);
	zmq_pollitem_t poll_items[table_size];
	GSList *list= NULL;

	GList * iterator = g_hash_table_get_keys(rn->publisher_socks);
	while(iterator != NULL){
		fprintf(stderr, "iterating");
		zmq_pollitem_t poll_item = {g_hash_table_lookup(rn->publisher_socks, iterator->data),
			0, ZMQ_POLLIN, 0};

		poll_items[i] = poll_item;

		fprintf(stderr, "appended");
		iterator = iterator->next;
		i++;
	}

	while(true){	
		//fprintf(stderr, "\nbefore poll %d", table_size);
		//zmq_poll(poll_items, table_size, -1);
		//fprintf(stderr, "\nafter_poll");
		//for(i = 0; i < table_size; i++){
		//	if(poll_items[i].revents && ZMQ_POLLIN){
				int len;
				char **buff = multi_part_receive(rn->listener, &len);
				ssize_t current_pos = 0;
				int buff_len = strlen(buff[1]);
				while (current_pos < buff_len) {
					struct inotify_event *pevent = (struct inotify_event *)&buff[1][current_pos];
					print_notifications(pevent);
					current_pos += sizeof(struct inotify_event) + pevent->len;
				}
				free(buff[0]);
				free(buff[1]);
				//free(buff);
			//}
		//}
	}

}
//caveats
//1. The Hash Table's Values are on the heap and never freed
//2. Contents of the rn are never freed
#endif
