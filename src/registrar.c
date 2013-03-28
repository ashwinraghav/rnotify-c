#include "zhelpers.h"
#include "czmq.h"
#include <hash_ring.h>
#include<sys/inotify.h>

//thread to accept new publisher registrations
static void accept_registrations(void *args, zctx_t *ctx, void *pipe)
{
	hash_ring_t *publishers = hash_ring_create(REPLICATION_FACTOR, HASH_FUNCTION_SHA1);
	hash_ring_t *dispatchers = hash_ring_create(REPLICATION_FACTOR, HASH_FUNCTION_SHA1);

	void *sub_recv_sock = create_socket(ctx, ZMQ_PULL, SOCK_BIND, REGISTRATION_ADDR);
	while(true){
		int len;
		char **registration = two_part_receive(sub_recv_sock, &len);
		// 0 th element - sanity checks; 1st element - IP address

		if(strcmp(registration[0], REGISTER_PUBLISHER_SANITY_CHECK) == 0){
			hash_ring_add_node(publishers, (uint8_t*)registration[1], strlen(registration[1]));
			printf("\n\nReceived registration for publisher %s", registration[1]);
		}
		else if(strcmp(registration[0], REGISTER_DISPATCHER_SANITY_CHECK) == 0){
			hash_ring_add_node(dispatchers, (uint8_t*)registration[1], strlen(registration[1]));
			printf("\n\nReceived registration for dispatcher %s", registration[1]);
		}
		
		//hash_ring_print(publishers);
		free(registration[0]);
		free(registration[1]);
	}
	hash_ring_free(publishers);
}

int main (){
	zctx_t *ctx = zctx_new();
	accept_registrations(NULL, ctx, NULL);
	return 0;
}
// Unclear on whether the char arrays passed to the hashing ring are freed
