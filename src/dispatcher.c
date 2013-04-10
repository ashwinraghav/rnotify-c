#include "zhelpers.h"
#include "czmq.h"
#include <sys/inotify.h>
#include <zhelpers.h>
#include "ip.h"
#include <hash_ring.h>
#include<glib.h>
#include <unistd.h>

#define WORKER_SOCKET "inproc://#1"
#define THREAD_COUNT 10

#ifdef PRODUCTION
	#define RECEIVE_ADDR "tcp://" FILE_HOST_IP_ADDR ":"  PROXY_FLUSH_PORT
	#define DISPATCHER_NOTIFY_ADDR "tcp://" REGISTRAR_IP_ADDR ":" DISPATCHER_NOTIFY_PORT

#else
	#define RECEIVE_ADDR "ipc:///tmp/" PROXY_FLUSH_PORT
	#define DISPATCHER_NOTIFY_ADDR "ipc:///tmp/" DISPATCHER_NOTIFY_PORT
#endif

typedef struct publisher_list_struct{
	hash_ring_t *publisher_ring;
	GHashTable *publisher_socks;
	GHashTable *publisher_socks_locks;
}publishers_info;

void parse_notifications(char *buff, ssize_t len, void* dispatch_socket);
void dispatch (char* update, int size, void* dispatch_socket);
static void parser_thread(void *args, zctx_t* ctx, void *pipe);
void create_parser_threads(int nthreads, zctx_t *ctx, publishers_info *publisher_ring);
static void publisher_updater(void *args, zctx_t *ctx, void *pipe);

GRWLock lock;

int main (int argc, char *argv [])
{
        zctx_t *ctx = zctx_new ();
	publishers_info *pub_interface = malloc(sizeof(publishers_info));

	pub_interface->publisher_ring = hash_ring_create(REPLICATION_FACTOR, HASH_FUNCTION_SHA1);
	pub_interface->publisher_socks = g_hash_table_new(g_str_hash, g_str_equal);
	pub_interface->publisher_socks_locks = g_hash_table_new(g_str_hash, g_str_equal);

	//self_register(ctx, REGISTER_DISPATCHER_SANITY_CHECK, "");

	void *subscriber = create_socket(ctx, ZMQ_PULL, SOCK_CONNECT, RECEIVE_ADDR);
	void *worker = create_socket(ctx, ZMQ_PUSH, SOCK_BIND, WORKER_SOCKET);
	
	create_parser_threads(THREAD_COUNT, ctx, pub_interface);
	
	zthread_fork(ctx, publisher_updater, pub_interface);
	while(true)
	{
		int size;
		char *string = _recv_buff(subscriber, &size);
		_send_string(worker, string, size);
		//fprintf(stderr, "\n Flush came in");
		free (string);
	}
	zctx_destroy (&ctx);
	hash_ring_free(pub_interface->publisher_ring);
	return 0;
}

void create_parser_threads(int nthreads, zctx_t *ctx, publishers_info *pub_interface)
{
	for (nthreads=0; nthreads < THREAD_COUNT; nthreads++)
	{
		zthread_fork(ctx, parser_thread, pub_interface);
	}

}

void dispatch(char* update, int len, void* dispatch_socket)
{
	int sent_size = _send_string(dispatch_socket, update, len);
}

static void publisher_updater(void *args, zctx_t *ctx, void *pipe){
	publishers_info *pub_interface = (struct publisher_list_struct*)args;
	void *publisher_updater_socket = create_socket(ctx, ZMQ_SUB, SOCK_CONNECT, DISPATCHER_NOTIFY_ADDR);
	zmq_setsockopt (publisher_updater_socket, ZMQ_SUBSCRIBE, "", strlen (""));
	while(true)
	{
		int size;
		char *publisher_addr = _recv_buff(publisher_updater_socket, &size);
		char *publisher_addr_dup  = to_c_string(publisher_addr, strlen(publisher_addr), strlen(publisher_addr)+1);

		hash_ring_add_node(pub_interface->publisher_ring, (uint8_t*)publisher_addr, strlen(publisher_addr));
		void *dispatch_socket = create_socket(ctx, ZMQ_PUB, SOCK_CONNECT, publisher_addr);
		g_rw_lock_writer_lock (&lock);

		g_hash_table_insert(pub_interface->publisher_socks, publisher_addr, dispatch_socket);

		GMutex m;
		g_mutex_init (&m);
		g_hash_table_insert(pub_interface->publisher_socks_locks, publisher_addr_dup, &m);

		g_rw_lock_writer_unlock (&lock);
		fprintf(stderr, "New Publisher was added at %s", publisher_addr);
		//free(publisher_addr);	
	}
}

//returns sock. Must unlock after usage. Otherwise the sock cannot be reused
static void* notify_random_publisher(struct inotify_event *pevent, publishers_info *pub_interface)
{
	char * instance_num = malloc(10);
	sprintf(instance_num, "%d",  (rand()%REPLICATION_FACTOR) + 1);

	char * publishable = to_c_string(pevent->name, pevent->len, (pevent->len + strlen(instance_num) + 1));
	strcat(publishable, instance_num);

	g_rw_lock_reader_lock (&lock);

	hash_ring_node_t *node = hash_ring_find_node(pub_interface->publisher_ring, (uint8_t*)publishable, strlen(publishable));
	char *publisher = to_c_string((char*)node->name, node->nameLen, node->nameLen + 1);
	void* ret =  g_hash_table_lookup(pub_interface->publisher_socks, publisher);
	void* mutex =  g_hash_table_lookup(pub_interface->publisher_socks_locks, publisher);

	g_rw_lock_reader_unlock (&lock);	
	free(publisher);
	free(publishable);
	free(instance_num);

	g_mutex_lock (mutex);
	notify(ret, pevent);
	g_mutex_unlock (mutex);
	
	return ret;

}
static void parser_thread(void *args, zctx_t* ctx, void *pipe){
	void *work_receiver_socket = create_socket(ctx, ZMQ_PULL, SOCK_CONNECT, WORKER_SOCKET);
	publishers_info *pub_interface = (publishers_info*)args;

	while(true)
	{
		int size;
		char *buff = _recv_buff(work_receiver_socket, &size);
		ssize_t i = 0;
		srand(time(NULL));
	
		while (i < size) {
			struct inotify_event *pevent = (struct inotify_event *)&buff[i];

			if (pevent->len)
			{
				notify_random_publisher(pevent, pub_interface);

			}
			//print_notifications(pevent);
			i += sizeof(struct inotify_event) + pevent->len;
		}
		free (buff);
	}
}

//random number generator does not seem uniform
//mallocing inside get random publisher too many times
