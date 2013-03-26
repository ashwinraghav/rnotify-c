#include "rnot.h"

#ifdef PRODUCTION
	#define SUB_SOCK "tcp://localhost:6001"
#else
	#define SUB_SOCK "ipc:///tmp/6001"
#endif

void parse_notifications(char *buff, ssize_t len, int *count);

int main (void)
{	
	rsubscribe("/localtmp/dump/1");
	return 0;
}
void parse_notifications(char *buff, ssize_t len, int* count)
{
	ssize_t i = 0;
        while (i < len) {
                struct inotify_event *pevent = (struct inotify_event *)&buff[i];
		print_notifications(pevent);
		*count = *count + 1;
		printf("Count = %d \n", *count);
                i += sizeof(struct inotify_event) + pevent->len;
        }

}

