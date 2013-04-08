#ifndef RNOT_HEADER_INCLUDED
#define RNOT_HEADER_INCLUDED
#include "zhelpers.h"
#include "czmq.h"
#include "unistd.h"
#include <glib.h>
#include<sys/inotify.h>

const rnot* rnotify_init()
{
	rnot *rn = malloc(sizeof(rnot));
	rn->ctx = zctx_new();
	rn->publisher_socks = g_hash_table_new(g_str_hash, g_str_equal);
	
	rn->subscriber = create_socket(rn->ctx, ZMQ_REQ, SOCK_CONNECT, REGISTRATION_ADDR);
	
	rn->tester = create_socket(rn->ctx, ZMQ_SUB, SOCK_CONNECT, TEST_INITIATE_ADDR);
	zmq_setsockopt(rn->tester, ZMQ_SUBSCRIBE, "", strlen(""));
	
	rn->result_collect_socket = create_socket(rn->ctx, ZMQ_PUSH, SOCK_CONNECT, TEST_COLLECT_ADDR);
	return (rn);
}


//call does not return until registration is complete
void rsubscribe(const rnot* const rn, char* const file_path){

	fprintf(stderr, "\nSubscribing");
	int size;
	char ** registration_response = _register(rn->subscriber, file_path, REGISTER_FILE_OBJECT_SANITY_CHECK, &size);
	fprintf(stderr, "\n added watch with id %s", registration_response[1]);

	int i=0;
	for(i = 2; i < size; i++){
		fprintf(stderr, "\n%s is a publisher", registration_response[i]);
		void* sock = g_hash_table_lookup(rn->publisher_socks, registration_response[i]);

		if(sock == NULL){
			fprintf(stderr, "\ncreating new sock");
			sock = create_socket(rn->ctx, ZMQ_SUB, SOCK_CONNECT, registration_response[i]);
			void *psock = create_socket(rn->ctx, ZMQ_SUB, SOCK_CONNECT, registration_response[i]);
		
			g_hash_table_insert(rn->publisher_socks, registration_response[i], sock);
		}
		zsocket_set_subscribe(sock, registration_response[1]);
		
	}	
	free(registration_response[0]);
	free(registration_response[1]);
	free(registration_response);

	//Map file name to regId
		
	//Set filter with the regId
	//nothing freed yet
}

void start_listener(const rnot* const rn, void (*handler)(const struct inotify_event* const p, void *args), void *args){
	int i = 0;
	int table_size = g_hash_table_size(rn->publisher_socks);
	zmq_pollitem_t poll_items[table_size];
	GSList *list= NULL;

	GList * iterator = g_hash_table_get_keys(rn->publisher_socks);
	while(iterator != NULL){
		zmq_pollitem_t poll_item = {g_hash_table_lookup(rn->publisher_socks, iterator->data),
			0, ZMQ_POLLIN, 0};

		poll_items[i] = poll_item;

		iterator = iterator->next;
		i++;
	}
	int count = 0;
	while(true){	
		zmq_poll(poll_items, table_size, -1);
		for(i = 0; i < table_size; i++){
			if(poll_items[i].revents && ZMQ_POLLIN){
				int len;
				char **buff = multi_part_receive(poll_items[i].socket, &len);
				ssize_t current_pos = 0;
				int buff_len = strlen(buff[1]);
				while (current_pos < buff_len) {
					struct inotify_event *pevent = (struct inotify_event *)&buff[1][current_pos];
					(*handler)(pevent, args);
					count += 1;
					//fprintf(stderr, "\ncount = %d", count);
					current_pos += sizeof(struct inotify_event) + pevent->len;
				}
				free(buff[0]);
				free(buff[1]);
				free(buff);
			}
		}
	}

}
//caveats
//1. The Hash Table's Values are on the heap and never freed
//2. Contents of the rn are never freed
#endif
