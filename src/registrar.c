#include "zhelpers.h"
#include "czmq.h"
#include <hash_ring.h>
#include<sys/inotify.h>
#include<glib.h>

static char* get_address_from_ring_entry(hash_ring_node_t *cur, char *port)
{
	char *dispatcher_address = malloc(
				sizeof(char)*(
				strlen("tcp://") +
				cur->nameLen +
				strlen(port) + 
				strlen(":")
				)+1);
	
	char *host_name=malloc(cur->nameLen+1);
	memcpy(host_name, cur->name, cur->nameLen);
	host_name[cur->nameLen] = '\0';

	strcat(strcat(strcat(strcpy(dispatcher_address,"tcp://"), host_name), ":"), port);

	free(host_name);

	return dispatcher_address;
}

static void notify_all(hash_ring_t *ring, char* message, char* port){	
	if(ring == NULL) return;
	zctx_t *ctx = zctx_new();
	ll_t *tmp, *cur = ring->nodes;
	while(cur != NULL) {
		hash_ring_node_t *node = (hash_ring_node_t*)cur->data;
		char *dispatcher_address = get_address_from_ring_entry(node, port);
		fprintf(stderr, "\n%s is the dispatcher", dispatcher_address);
		//void *sock = create_socket(ctx, ZMQ_PUSH, SOCK_CONNECT, dispatcher_address);
		//safe_send(sock, message, strlen(message));
		free(dispatcher_address);
		cur = cur->next;
	}
	zctx_destroy (&ctx);
}

//thread to accept new publisher registrations
static void accept_registrations(void *args, zctx_t *ctx, void *pipe)
{
	hash_ring_t *publisher_ring = hash_ring_create(REPLICATION_FACTOR, HASH_FUNCTION_SHA1);
	hash_ring_t *dispatcher_ring = hash_ring_create(REPLICATION_FACTOR, HASH_FUNCTION_SHA1);

	void *sub_recv_sock = create_socket(ctx, ZMQ_PULL, SOCK_BIND, REGISTRATION_ADDR);
	while(true){
		int len;
		char **registration = two_part_receive(sub_recv_sock, &len);
		// 0 th element - sanity checks; 1st element - IP address

		if(strcmp(registration[0], REGISTER_PUBLISHER_SANITY_CHECK) == 0)
		{
			hash_ring_add_node(publisher_ring, (uint8_t*)registration[1], strlen(registration[1]));
			notify_all(dispatcher_ring, registration[1], DISPATCHER_NOTIFY_PORT);
			fprintf(stderr, "\n\nReceived registration for publisher %s", registration[1]);
		}
		else if(strcmp(registration[0], REGISTER_DISPATCHER_SANITY_CHECK) == 0)
		{
			hash_ring_add_node(dispatcher_ring, (uint8_t*)registration[1], strlen(registration[1]));
			fprintf(stderr, "\n\nReceived registration for dispatcher %s", registration[1]);
		}
		
		//hash_ring_print(publisher_ring);
		free(registration[0]);
		free(registration[1]);
	}
	hash_ring_free(publisher_ring);
	hash_ring_free(dispatcher_ring);
}

int main (){
	zctx_t *ctx = zctx_new();
	accept_registrations(NULL, ctx, NULL);
	zctx_destroy (&ctx);
	return 0;
}

//Unclear on whether the char arrays passed to the hashing ring are freed
