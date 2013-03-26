#include "zhelpers.h"
#include "czmq.h"
#include<sys/inotify.h>


#ifdef PRODUCTION
	#define SUB_SOCK "tcp://localhost:6001"
#else
	#define SUB_SOCK "ipc:///tmp/6001"
#endif

void rsubscribe(char* file_path, ){

}
