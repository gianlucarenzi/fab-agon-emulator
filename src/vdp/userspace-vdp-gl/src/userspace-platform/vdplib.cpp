/*
 * Video Display Processor VDP Library for Agon Light (tm) computers
 * VDP Core, based on vdp-gl, FabGL library
 * 
 * With this library you can use any serial communication port to
 * AGON VDP to use it as external video graphics adapter, audio PWM and
 * keyboard controller (PS/2)
 *
 * Written by Gianluca Renzi <icjtqr@gmail.com> (C) 2023
 * 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <linux/serial.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <termios.h>
#include "serial.h"
#include "debug.h"
#include "ec_types.h"
#include "vdplib.h"

static int debuglevel = DBG_INFO;

#define TIMER_TICK        (50 * 1000L) /* 50msec TIMER RESOLUTION */

#define TIMEOUT_THREAD_MS    (1000)
#define TIMEOUT_MAIN_MS      (5000)

// Human date/time macros
#define USEC(a)       (a)
#define MSEC(a)       (USEC(a * 1000L))
#define SEC(a)        (MSEC(a * 1000L))
#define MIN(a)        (SEC(a * 60))
#define HOUR(a)       (MIN(a * 60))
#define DAY(a)        (HOUR(a * 24))

static pthread_mutex_t mutexLock;

typedef struct {
	uint32_t header;
	uint32_t len;
	uint32_t footer;
} t_signature;

typedef struct {
	int fd;
	int baudrate;
	int pre;
	int post;
} t_port;

#define BUFFER_SIZE (4096)

static void print_hex_ascii_line(const unsigned char *payload, int len, int offset)
{
	int i;
	int gap;
	const unsigned char *ch;

	// offset
	printR("%05x   ", offset);

	// hex
	ch = payload;
	for (i = 0; i < len; i++)
	{
		printR("%02x ", *ch);
		ch++;
		// print extra space after 8th byte for visual aid
		if (i == 7)
			printR(" ");
	}
	// print space to handle line less than 8 bytes
	if (len < 8)
	{
		printR(" ");
	}

	// fill hex gap with spaces if not full line
	if (len < 16)
	{
		gap = 16 - len;
		for (i = 0; i < gap; i++)
		{
			printR("   ");
		}
	}
	printR("   ");

	// ascii (if printable)
	ch = payload;
	for(i = 0; i < len; i++)
	{
		if (isprint(*ch))
		{
			printR("%c", *ch);
		}
		else
		{
			printR(".");
		}
		ch++;
	}
	printR("\n");
}

// print packet payload data (avoid printing binary data)
void print_payload(const char *func, const unsigned char *payload, int len, int dbglvl)
{
	int len_rem = len;
	int line_width = 16;           // number of bytes per line
	int line_len;
	int offset = 0;                // zero-based offset counter
	const unsigned char *ch = payload;

	// Stampiamo il payload se siamo >= DBG_VERBOSE oppure
	// in errore.
	if ((dbglvl >= DBG_VERBOSE) || (dbglvl == DBG_ERROR))
	{
		printR("Enter %p LEN: %d from %s\n", payload, len, func);

		if (len <= 0)
		{
			printR("No LEN. Exit\n");
			return;
		}

		// data fits on one line
		if (len <= line_width)
		{
			print_hex_ascii_line(ch, len, offset);
			printR("Small Line. Exiting\n");
			return;
		}

		// data spans multiple lines
		for (;;)
		{
			// compute current line length
			line_len = line_width % len_rem;
			// print line
			print_hex_ascii_line(ch, line_len, offset);
			// compute total remaining
			len_rem = len_rem - line_len;
			// shift pointer to remaining bytes to print
			ch = ch + line_len;
			// add offset
			offset = offset + line_width;
			// check if we have line width chars or less
			if (len_rem <= line_width)
			{
				// print last line and get out
				print_hex_ascii_line(ch, len_rem, offset);
				break;
			}
		}
		printR("Exit\n");
	}
}

static struct timeval startTimer;

static void timerstart(void)
{
	gettimeofday( &startTimer, NULL);
	DBG_N("TimerStart @ sec: %ld usec: %ld\n", startTimer.tv_sec,
		startTimer.tv_usec);
}

static int timerelapsed(int timeout)
{
	struct timeval now;
	DBG_N("Enter %ld - Timeout: %ld\n",
		startTimer.tv_sec, startTimer.tv_sec + timeout);
	// begin
	gettimeofday(&now, NULL);
	DBG_N("Now:     %ld\n", now.tv_sec);
	if ((now.tv_sec) > (startTimer.tv_sec + timeout)) {
		DBG_N("ELAPSED\n");
		return 1;
	}
	else {
		DBG_N("NOT ELAPSED\n");
		return 0;
	}
}

extern const char *fwBuild;

static void banner(void)
{
	/* La versione viene automaticamente generata ad ogni build.
	 * Nel momento di OK del software, viene committata e il build e'
	 * lanciato con il flag 'prod' che identifica che si vuole usare
	 * il file version.c presente (quindi committato)
	 */
	fprintf(stdout, "\n\n");
	fprintf(stdout, ANSI_BLUE "VDP LIBRARY FOR AGON VDP DEVICES" ANSI_RESET "\n");
	fprintf(stdout, ANSI_YELLOW );
	fprintf(stdout, "FWVER: %s", fwBuild);
	fprintf(stdout, ANSI_RESET "\n");
	fprintf(stdout, "\n\n");
}

static void signal_handle(int sig)
{
	char signame[8];

	switch( sig )
	{
		case SIGSEGV:
			sprintf(signame, "SIGSEGV");
			DBG_E("signal %s - %d caught\n", signame, sig);
			break;
		case SIGINT:
			sprintf(signame, "SIGINT");
			DBG_E("signal %s - %d caught\n", signame, sig);
			break;
		case SIGTERM:
			sprintf(signame, "SIGTERM");
			DBG_E("signal %s - %d caught\n", signame, sig);
			break;
		case SIGUSR1:
			sprintf(signame, "SIGUSR1");
			DBG_E("signal %s - %d caught\n", signame, sig);
			return;
			break; // NEVERREACHED
		case SIGUSR2:
			sprintf(signame, "SIGUSR2");
			DBG_E("signal %s - %d caught\n", signame, sig);
			return;
			break; // NEVERREACHED
		default:
			sprintf(signame, "UNKNOWN");
			DBG_E("signal %s - %d caught\n", signame, sig);
			break;
	}
	exit(sig);
}

static void version(const char * filename, const char *ver)
{
	char version[256];

	if (filename == NULL || ver == NULL)
		return;

	memset(version, 0, 256);

	sprintf(version, "mkdir -p /tmp/%s && echo %s > /tmp/%s/version",
			filename, ver, filename);
	if (system(version) != 0)
	{
		DBG_E("Error: %s gives error %d\n", version, errno);
	}
}

#define MAX_RETRY_COUNT (2)

static t_port * vdp_init_port(const char *device, const int baudrate)
{
	t_port * port = NULL;
	uint8_t vdp_tx[] = VDU_INIT;
	uint8_t vdp_rx[16];
	int rval;
	int vdp_init_done = 0;
	int retry=0;

	DBG_N("Entering with %s and %d baudrate\n", device, baudrate);

	// Arguments checking
	if (device == NULL)
	{
		DBG_E("No device passed\n");
		return NULL;
	}

	port = (t_port *) malloc(sizeof(t_port));
	if (port == NULL)
	{
		DBG_E("Memory error\n");
		return NULL;
	}

	port->fd = serial_device_init(device, baudrate, 0, 0);
	if (port->fd < 0)
	{
		DBG_E("Unable to initialize serial port for device %s\n", device);
		return NULL;
	}
	else
	{
		port->baudrate = baudrate;
		DBG_I("Serial Port File Handle: %d\n", port->fd);
	}

	/* Flush all data in buffers */
	DBG_N("FLUSH BUFFERS\n");
	serial_flush_rx(port->fd);
	serial_flush_tx(port->fd);

	return port;
}

#define WAIT_CHAR 150
// Running at 57600 1 char time is 174usec

int vdplib_send(int portfd, uint8_t b)
{
	int retval;
	unsigned char buf[1];
	buf[0] = (unsigned char) b;
	retval = serial_send_raw(portfd, buf, 1);
	usleep(WAIT_CHAR);
	return retval;
}

int vdplib_startup(void)
{
	int retval = -ENODEV;
	int baudrate = 57600;
	char device[1024];
	t_port * port = NULL;

	version("AGON-VDP-WRAPPER", fwBuild);
	banner();

	// Adesso posso istanziare l'handle dei segnali che utilizza il mutex
	signal(SIGSEGV, signal_handle);
	signal(SIGINT, signal_handle);
	signal(SIGTERM, signal_handle);
	signal(SIGUSR1, signal_handle);
	signal(SIGUSR2, signal_handle);

	sprintf(device, "/dev/ttyACM0");
	DBG_I("Using %s as VDP AGON device  @ BaudRate: %d ...\n", device, baudrate);

	port = vdp_init_port(device, baudrate); 
	if ( port == NULL)
	{
		DBG_E("Unable to communicate with the VDP AGON device\n");
	}
	else
	{
		DBG_I("VDP Ready\n");
		retval = port->fd;
	}
	DBG_N("Exit with: %d\n", retval);
	return retval;
}

