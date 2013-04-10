#include "zhelpers.h"
#include "czmq.h"
#include <hash_ring.h>
#include<sys/inotify.h>
#include<glib.h>


#ifdef PRODUCTION
	#define INITIATE_ADDR "tcp://*:" INITIATE_PORT
	#define COLLECT_ADDR "tcp://*:" COLLECT_PORT
#else
	#define INITIATE_ADDR "ipc:///tmp/" INITIATE_PORT
	#define COLLECT_ADDR "ipc:///tmp/" COLLECT_PORT
#endif
void wait_for_joiners(void * result_collect_socket, int *client_count){
	int i=0;
	for(i = 0; i < *client_count; i++){
		int len;
		char *response = _recv_buff(result_collect_socket, &len);
		fprintf(stderr, "%s seconds", response);
		free(response);
	}
}
static void test_channel(void* args, zctx_t* ctx, void *pipe){
	int *client_count = (int*) (args);
	FILE *fr;
	void * touch_start_socket = create_socket(ctx, ZMQ_PUB, SOCK_BIND, INITIATE_ADDR);
	void * result_collect_socket = create_socket(ctx, ZMQ_PULL, SOCK_BIND, COLLECT_ADDR);
	char line[80];
	//sleep(15);

	//wait_for_joiners(result_collect_socket, client_count);
	int a;
	while(true){
		fprintf(stderr, "\nPress Enter to start");
		getchar();
		_send_string(touch_start_socket, "message", strlen("message"));

		sleep(3);
		fprintf(stderr, "coming");
		fr = fopen ("/localtmp/dump/1/foo", "rt");


		zmq_pollitem_t items [1];
		/* First item refers to ØMQ socket 'socket' */
		items[0].socket = result_collect_socket;
		items[0].events = ZMQ_POLLIN;



		int count = 0;
		float tot_time = 0;
		while(true){	
			zmq_poll(items, 1, 10000);
			if(items[0].revents && ZMQ_POLLIN){
				int len;
				char *response = _recv_buff(result_collect_socket, &len);
				float cur_time = ((float)atoi(response)-3000);
				tot_time += cur_time;
				fprintf(stderr, "\n%f milliseconds", cur_time);
				free(response);
				count++;
			}else{
				break;
			}
		}
		fprintf(stderr, "\n\n ***********Total Average Time = %f************", (tot_time/(count)));
		//getchar();
		//fgets(line, 80, fr);
		//fclose(fr);  	
	}	
}

int main (){
	zctx_t *ctx = zctx_new();
	int client_count = 10;
	//zthread_fork(ctx, test_channel, (void*)(&client_count));
	test_channel((void*)(&client_count), ctx, NULL);

	zctx_destroy (&ctx);
	return 0;
}

