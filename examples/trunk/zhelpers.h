/*  =====================================================================
    zhelpers.h

    Helper header file for example applications.
    =====================================================================
 */

#ifndef __ZHELPERS_H_INCLUDED__
#define __ZHELPERS_H_INCLUDED__

//  Include a bunch of headers that we will need in the examples

#include <sys/inotify.h>
#include <zmq.h>
#include <czmq.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <uuid/uuid.h>

#define SOCK_BIND 17273
#define SOCK_CONNECT 276346

//error checking macro
#define CHECK(x) do { \
	int retval = (x); \
	if (retval < 0) { \
		fprintf(stderr, "Runtime error: %s returned %s at %s:%d", #x, strerror(errno), __FILE__, __LINE__); \
	} \
} while (0)

//  Version checking, and patch up missing constants to match 2.1
#if ZMQ_VERSION_MAJOR == 2
#   error "Please upgrade to ZeroMQ/3.2 for these examples"
#endif

//  Provide random number from 0..(num-1)
#if (defined (__WINDOWS__))
#   define randof(num)  (int) ((float) (num) * rand () / (RAND_MAX + 1.0))
#else
#   define randof(num)  (int) ((float) (num) * random () / (RAND_MAX + 1.0))
#endif


//  Receive 0MQ string from socket and convert into C string
//  Caller must free returned string. Returns NULL if the context
//  is being terminated.
static char *
s_recv (void *socket) {
	char buffer [256];
	int size = zmq_recv (socket, buffer, 255, 0);
	if (size == -1)
		return NULL;
	if (size > 255)
		size = 255;
	buffer [size] = 0;
	return strdup (buffer);
}

//  Convert C string to 0MQ string and send to socket
static int
s_send (void *socket, char *string) {
	int size = zmq_send (socket, string, strlen (string), 0);
	return size;
}

//  Sends string as 0MQ string, as multipart non-terminal
static int
s_sendmore (void *socket, char *string) {
	int size = zmq_send (socket, string, strlen (string), ZMQ_SNDMORE);
	return size;
}

//  Receives all message parts from socket, prints neatly
//

static void s_dump (void *socket)
{
	puts ("----------------------------------------");
	while (1) {
		//  Process all parts of the message
		zmq_msg_t message;
		zmq_msg_init (&message);
		int size = zmq_msg_recv (&message, socket, 0);

		//  Dump the message as text or binary
		char *data = zmq_msg_data (&message);
		int is_text = 1;
		int char_nbr;
		for (char_nbr = 0; char_nbr < size; char_nbr++)
			if ((unsigned char) data [char_nbr] < 32
					||  (unsigned char) data [char_nbr] > 127)
				is_text = 0;

		printf ("[%03d] ", size);
		for (char_nbr = 0; char_nbr < size; char_nbr++) {
			if (is_text)
				printf ("%c", data [char_nbr]);
			else
				printf ("%02X", (unsigned char) data [char_nbr]);
		}
		printf ("\n");

		int64_t more;           //  Multipart detection
		more = 0;
		size_t more_size = sizeof (more);
		zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
		zmq_msg_close (&message);
		if (!more)
			break;      //  Last message part
	}
}

//  Set simple random printable identity on socket
//
	static void
s_set_id (void *socket)
{
	char identity [10];
	sprintf (identity, "%04X-%04X", randof (0x10000), randof (0x10000));
	zmq_setsockopt (socket, ZMQ_IDENTITY, identity, strlen (identity));
}


//  Sleep for a number of milliseconds
	static void
s_sleep (int msecs)
{
#if (defined (__WINDOWS__))
	Sleep (msecs);
#else
	struct timespec t;
	t.tv_sec  =  msecs / 1000;
	t.tv_nsec = (msecs % 1000) * 1000000;
	nanosleep (&t, NULL);
#endif
}

//  Return current system clock as milliseconds
	static int64_t
s_clock (void)
{
#if (defined (__WINDOWS__))
	SYSTEMTIME st;
	GetSystemTime (&st);
	return (int64_t) st.wSecond * 1000 + st.wMilliseconds;
#else
	struct timeval tv;
	gettimeofday (&tv, NULL);
	return (int64_t) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

//  Print formatted string to stdout, prefixed by date/time and
//  terminated with a newline.

	static void
s_console (const char *format, ...)
{
	time_t curtime = time (NULL);
	struct tm *loctime = localtime (&curtime);
	char *formatted = malloc (20);
	strftime (formatted, 20, "%y-%m-%d %H:%M:%S ", loctime);
	printf ("%s", formatted);
	free (formatted);

	va_list argptr;
	va_start (argptr, format);
	vprintf (format, argptr);
	va_end (argptr);
	printf ("\n");
}

#endif  //  __ZHELPERS_H_INCLUDED__


static int safe_send(void *socket, char *string, size_t len) {
	int rc;
	zmq_msg_t message;
	zmq_msg_init_size (&message, len);
	memcpy (zmq_msg_data (&message), string, len);
	CHECK(rc = zmq_msg_send (&message, socket, 0));
	zmq_msg_close (&message);
	return (rc);
}

static char* safe_recv(void *socket, int *size) {
	zmq_msg_t message;
	zmq_msg_init (&message);
	*size = zmq_msg_recv (&message, socket, 0);
	if (*size == -1)
		return NULL;
	char *string = malloc (*size + 1);
	memcpy (string, zmq_msg_data (&message), *size);
	zmq_msg_close (&message);
	string [*size] = 0;
	return (string);
}

static void print_notifications(struct inotify_event *pevent)
{

	char action[81+FILENAME_MAX] = {0};

	if (pevent->len) 
		strcpy (action, pevent->name);
	else
		strcpy (action, "some random directory");

	if (pevent->mask & IN_ACCESS) 
		strcat(action, " was read");
	if (pevent->mask & IN_ATTRIB) 
		strcat(action, " Metadata changed");
	if (pevent->mask & IN_CLOSE_WRITE) 
		strcat(action, " opened for writing was closed");
	if (pevent->mask & IN_CLOSE_NOWRITE) 
		strcat(action, " not opened for writing was closed");
	if (pevent->mask & IN_CREATE) 
		strcat(action, " created in watched directory");
	if (pevent->mask & IN_DELETE) 
		strcat(action, " deleted from watched directory");
	if (pevent->mask & IN_DELETE_SELF) 
		strcat(action, "Watched file/directory was itself deleted");
	if (pevent->mask & IN_MODIFY) 
		strcat(action, " was modified");
	if (pevent->mask & IN_MOVE_SELF) 
		strcat(action, "Watched file/directory was itself moved");
	if (pevent->mask & IN_MOVED_FROM) 
		strcat(action, " moved out of watched directory");
	if (pevent->mask & IN_MOVED_TO) 
		strcat(action, " moved into watched directory");
	if (pevent->mask & IN_OPEN) 
		strcat(action, " was opened");


	printf ("wd=%d mask=%d cookie=%d len=%d dir=%s\n",pevent->wd, pevent->mask, pevent->cookie, pevent->len,  (pevent->mask & IN_ISDIR)?"yes":"no");

	if (pevent->len) 
		printf ("name=%s\n", pevent->name);


}

void print_error (int error)
{
	fprintf (stderr, "Error: %s\n", strerror(error));

}

void* create_socket(zctx_t *ctx, int type, int mode, char* address)
{
	void *sock = zsocket_new (ctx, type);
	zsocket_set_hwm(sock, 100000);
	if(mode == SOCK_BIND) 	
		zsocket_bind(sock, address);
	else if (mode == SOCK_CONNECT)
		zsocket_connect(sock, address);

	return sock;
}
