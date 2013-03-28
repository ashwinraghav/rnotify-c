#include "zhelpers.h"
#include "czmq.h"
#include <hash_ring.h>
#include<sys/inotify.h>

#ifdef PRODUCTION
	#define REGISTRATION_ADDR "tcp://*:" REGISTER_PORT
#else
	#define REGISTRATION_ADDR "ipc:///tmp/" REGISTER_PORT
#endif
//thread to accept new publisher registrations
static void accept_registrations(void *args, zctx_t *ctx, void *pipe)
{
	hash_ring_t *publishers = hash_ring_create(REPLICATION_FACTOR, HASH_FUNCTION_SHA1);

	void *sub_recv_sock = create_socket(ctx, ZMQ_PULL, SOCK_BIND, REGISTRATION_ADDR);
	while(true){
		int len;
		char **registration = two_part_receive(sub_recv_sock, &len);
		// 0 th element has the sanity checks
		// 1st element has the address

		if(strcmp(registration[0], REGISTER_PUBLISHER_SANITY_CHECK) == 0){
			hash_ring_add_node(publishers, (uint8_t*)registration[1], strlen(registration[1]));
			printf("\n\nReceived registration for publisher %s", registration[1]);
			//hash_ring_print(publishers);
		}
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
