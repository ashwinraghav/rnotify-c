#include "rnot.h"
#include "zhelpers.h"
#ifdef PRODUCTION
	#define SUB_SOCK "tcp://localhost:6001"
#else
	#define SUB_SOCK "ipc:///tmp/6001"
#endif

static void check_notifications(const struct inotify_event* const pevent, void *args)
{
	rnot *rn = (rnot*)(args);
	char action[81+FILENAME_MAX] = {0};
	if ((pevent->mask & IN_OPEN) && (strcmp("foo", pevent->name) == 0)){
		fprintf(stderr, "\nGot the file");
		int elapsed_time = (int) (s_clock() - rn->start_time);
		char *elapsed_time_string = malloc(sizeof(char) * 100);
		sprintf(elapsed_time_string, "%d", elapsed_time);
		fprintf(stderr, "\nelapsed time = %s", elapsed_time_string);
		_send_string(rn->result_collect_socket, elapsed_time_string,strlen(elapsed_time_string));
		free(elapsed_time_string);
	}

}

static void test_channel_listener(void* args, zctx_t* ctx, void *pipe){
	while(true){
		rnot* rn = (rnot*)(args);
		int len;
		char *content = _recv_buff(rn->tester, &len);
		rn->start_time = s_clock();
	}
}
static void join_tester(void* result_sock){
	_send_string(result_sock, "foo", strlen("foo"));
}

int main (void)
{	
	const rnot* const rn = rnotify_init();	
	rsubscribe(rn, "/localtmp/dump/1");
	rsubscribe(rn, "/localtmp/dump/2");
	rsubscribe(rn, "/localtmp/dump/3");
	rsubscribe(rn, "/localtmp/dump/4");
	
	//join_tester(); 

	zthread_fork(rn->ctx, test_channel_listener, (void*)rn);
	
	start_listener(rn, check_notifications, (void*) rn);
	return 0;
}
