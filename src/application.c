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
		free(content);
		fprintf(stderr, "\nreceived start");
		rn->start_time = s_clock();
	}
}
static void join_tester(void* result_sock){
	_send_string(result_sock, "foo", strlen("foo"));
}

void  destroy(gpointer data){
	free((void*) data);
}
int main (int argc, char *argv[])
{	
	srand(time(NULL));
	if(argc < 2){
		fprintf(stderr, "\n Only %d arguments were given", argc);
		exit(EXIT_FAILURE);
	}

	const rnot* const rn = rnotify_init();
	
	int subscription_count = atoi(argv[1]);
	int i = 0;
	char *base_dir = "/localtmp/dump/";

	GHashTable* subscribed_files = g_hash_table_new_full(g_str_hash, g_str_equal, destroy, NULL);

	for(i=0;i<subscription_count;i++){
		char* existing_subscription = NULL;
		while(true){
			char *file_name = malloc(sizeof(char) * (strlen(base_dir) + 2));

			char * dir_num = malloc(10);
			sprintf(dir_num, "%d",  (rand()%4) + 1);

			strcat(strcpy(file_name, base_dir),dir_num);
			file_name[sizeof(char) * (strlen(base_dir) + 1)] = 0;
			existing_subscription = g_hash_table_lookup(subscribed_files, file_name);

			if(existing_subscription == NULL){
				rsubscribe(rn, file_name);
				g_hash_table_insert(subscribed_files, file_name, file_name);
				free(dir_num);
				break;
			}else{
				free(file_name);
				free(dir_num);
			}
				
		}
		
	}

	zthread_fork(rn->ctx, test_channel_listener, (void*)rn);
	
	start_listener(rn, check_notifications, (void*) rn);
	return 0;
}
