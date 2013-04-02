#include "zhelpers.h"
#include "czmq.h"
#include <hash_ring.h>
#include<sys/inotify.h>
#include<glib.h>

#ifdef PRODUCTION
	#define DISPATCHER_NOTIFY_ADDR "tcp://*:" DISPATCHER_NOTIFY_PORT
	#define ME "tcp://*:" REGISTER_PORT
	#define SUBSCRIBE_TO_SOCK "tcp://" FILE_HOST_IP_ADDR ":" PROXY_SUBSCRIBE_PORT
#else
	#define DISPATCHER_NOTIFY_ADDR "ipc:///tmp/" DISPATCHER_NOTIFY_PORT
	#define ME "ipc:///tmp/" REGISTER_PORT
	#define SUBSCRIBE_TO_SOCK "ipc:///tmp/" PROXY_SUBSCRIBE_PORT
#endif

static GHashTable * get_publishers_table(char * file_name, hash_ring_t* publisher_ring)
{
	char * instance_num = malloc(10);
	int i=0;
	GHashTable* publishers_table = g_hash_table_new(g_str_hash, g_str_equal);

	for(i = 0;i < REPLICATION_FACTOR;i++){
		char* instance_num = malloc(10);
		sprintf(instance_num, "%d",  (i));

		char * publishable = to_c_string(file_name, strlen(file_name), strlen(file_name) + strlen(instance_num) + 1);
		strcat(publishable, instance_num);

		hash_ring_node_t *node = hash_ring_find_node(publisher_ring, (uint8_t*)publishable, strlen(publishable) + 1);
		char *publisher = to_c_string((char*)node->name, node->nameLen, node->nameLen + 1);
		g_hash_table_insert(publishers_table, publisher, publisher);

		free(publishable);
		free(instance_num);
	}
	return publishers_table;	
}

//thread to accept new publisher registrations
static void accept_registrations(void *args, zctx_t *ctx, void *pipe)
{
	hash_ring_t *publisher_ring = hash_ring_create(REPLICATION_FACTOR, HASH_FUNCTION_SHA1);
	hash_ring_t *dispatcher_ring = hash_ring_create(REPLICATION_FACTOR, HASH_FUNCTION_SHA1);

	void *sub_recv_sock = create_socket(ctx, ZMQ_REP, SOCK_BIND, ME);
	void *dispatcher_notify_sock = create_socket(ctx, ZMQ_PUB, SOCK_BIND, DISPATCHER_NOTIFY_ADDR);
	void *file_subscriber = create_socket(ctx, ZMQ_REQ, SOCK_CONNECT, SUBSCRIBE_TO_SOCK);

	while(true){
		int n_parts;
		char **registration = multi_part_receive(sub_recv_sock, &n_parts);
		// 0 th element - sanity checks; 1st element - IP address


		if(strcmp(registration[0], REGISTER_PUBLISHER_SANITY_CHECK) == 0)
		{

			hash_ring_add_node(publisher_ring, (uint8_t*)registration[1], strlen(registration[1]));

			//respond success
			_send_string(sub_recv_sock, (const char* const) REGISTER_OK, strlen(REGISTER_OK));

			//notify dispatcher
			_send_string(dispatcher_notify_sock, registration[1], strlen(registration[1]));


			fprintf(stderr, "\n\nReceived registration for publisher %s", registration[1]);
		}
		else if(strcmp(registration[0], REGISTER_DISPATCHER_SANITY_CHECK) == 0)
		{
			//no response sent .. NEEDS FIX
			hash_ring_add_node(dispatcher_ring, (uint8_t*)registration[1], strlen(registration[1]));
			fprintf(stderr, "\n\nReceived registration for dispatcher %si\n", registration[1]);
		}
		else if(strcmp(registration[0], REGISTER_FILE_OBJECT_SANITY_CHECK) == 0){
			fprintf(stderr, "\n\n File added %s", registration[1]);
			_send_string(file_subscriber, registration[1], strlen(registration[1]));
			int id_len;
			char *registration_id = _recv_buff(file_subscriber, &id_len);

			GHashTable *publishers_table = get_publishers_table(registration[1], publisher_ring);
				
			int i = 1;
			char ** publishers_list = malloc(sizeof(char*) * (g_hash_table_size(publishers_table) + 1));
			publishers_list[0] = to_c_string(REGISTER_OK, strlen(REGISTER_OK), strlen(REGISTER_OK)+1);

			GList * iterator = g_hash_table_get_keys(publishers_table);
			while(iterator != NULL){
				publishers_list[i] = to_c_string((char*)iterator->data, 
							strlen(iterator->data), strlen(iterator->data) + 1);
				
				fprintf(stderr, "\n %s", publishers_list[i]);
				void * value = g_hash_table_lookup(publishers_table, iterator->data);
				GList * key = iterator;
				iterator = iterator->next;
				i++;
				//free(publishers_list[i]);
			}
			_send(sub_recv_sock,(const char **) publishers_list, i);
			int j = 0;
			for(j = 0; j < i; j ++){
				free(publishers_list[j]);
			}
			g_hash_table_destroy(publishers_table);
			g_list_free(iterator);
			free(publishers_list);
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

//Iterate over all dispatchers and notify
//Usage -- notify_all(dispatcher_ring, registration[1], DISPATCHER_NOTIFY_PORT);
//notify_all(dispatcher_ring, registration[1], DISPATCHER_NOTIFY_PORT);
static void notify_all(hash_ring_t *ring, char* message, char* port){	
	if(ring == NULL) return;
	zctx_t *ctx = zctx_new();
	ll_t *tmp, *cur = ring->nodes;
	while(cur != NULL) {
		hash_ring_node_t *node = (hash_ring_node_t*)cur->data;
		char *dispatcher_address = get_address_from_ring_entry(node, port);
		fprintf(stderr, "\n%s is the dispatcher", dispatcher_address);
		void *sock = create_socket(ctx, ZMQ_PUSH, SOCK_CONNECT, dispatcher_address);
		_send_string(sock, message, strlen(message));
		free(dispatcher_address);
		cur = cur->next;
	}
	zctx_destroy (&ctx);
}

//Unclear on whether the char arrays passed to the hashing ring are freed
