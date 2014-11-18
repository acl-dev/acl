#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "aqueue.h"
#include "netio.h"
#include "pipeio.h"
#include "unixio.h"

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -n max_loop -m all|aqueue|netio|pipeio|unixio\r\n", procname);
}

int main(int argc, char *argv[])
{
	char  ch;
	int   max = 0;
	char  method[24];

	method[0] = 0;
	while ((ch = getopt(argc, argv, "hn:m:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'n':
			max = atoi(optarg);
			break;
		case 'm':
			snprintf(method, sizeof(method), "%s", optarg);
			break;
		default:
			break;
		}
	}

	if (method[0] == 0) {
		usage(argv[0]);
		return (0);
	}

	if (strcasecmp(method, "netio") == 0)
		netio_run(max);
	else if (strcasecmp(method, "aqueue") == 0)
		aqueue_run(max);
	else if (strcasecmp(method, "pipeio") == 0)
		pipeio_run(max);
	else if (strcasecmp(method, "unixio") == 0)
		unixio_run(max);
	else if (strcasecmp(method, "all") == 0) {
		netio_run(max);
		aqueue_run(max);
		pipeio_run(max);
		unixio_run(max);
	} else
		usage(argv[0]);
	return (0);
}
