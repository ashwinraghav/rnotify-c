#ifndef UTILS_INCLUDED
#define UTILS_INCLUDED
static int safe_send (void *socket, char *string, size_t len);
#endif

#define BUFF_SIZE ((sizeof(struct inotify_event)+FILENAME_MAX)*1024)
