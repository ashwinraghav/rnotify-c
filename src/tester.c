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
		printf("Press Enter to start");
		getchar();
		_send_string(touch_start_socket, "message", strlen("message"));

		fprintf(stderr, "coming");
		sleep(5);
		fr = fopen ("/localtmp/dump/1/foo", "rt");

		//collect_reponses
		int i=0;
		float tot_time = 0;
		for(i = 0; i < *client_count; i++){
			int len;
			char *response = _recv_buff(result_collect_socket, &len);
			tot_time += (float)atoi(response);
			fprintf(stderr, "%s seconds", response);
			free(response);
		}
		fprintf(stderr, "\n\n Total Average Time = %f", tot_time/(*client_count));
		fgets(line, 80, fr);
		fclose(fr);  	
	}	
}

int main (){
	zctx_t *ctx = zctx_new();
	int client_count = 50;
	//zthread_fork(ctx, test_channel, (void*)(&client_count));
	test_channel((void*)(&client_count), ctx, NULL);

	zctx_destroy (&ctx);
	return 0;
}

