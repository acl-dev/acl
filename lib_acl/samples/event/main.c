#include <stdio.h>
#include <stdlib.h>
#include "lib_acl.h"

typedef struct {
	ACL_VSTREAM *in;
	ACL_VSTRING *buf;
	ACL_EVENT *event;
} STREAM_IN;

static void trigger_event2(int event_type acl_unused,
	ACL_EVENT *event acl_unused, void *context acl_unused)
{
	printf("%s: please input: \r\n", __FUNCTION__);
}

static void trigger_event1(int event_type acl_unused,
	ACL_EVENT *event acl_unused, void *context acl_unused)
{
	printf("%s: please input: \r\n", __FUNCTION__);
	acl_event_cancel_timer(event, trigger_event2, NULL);
}

static int __stop_event = 0;

static STREAM_IN *create_stream(ACL_EVENT *eventp)
{
	STREAM_IN *in = (STREAM_IN*) acl_mymalloc(sizeof(STREAM_IN));

	/* 用标准输入做为输入流 */
	in->in    = acl_vstream_fhopen(fileno(stdin), 0);
	in->buf   = acl_vstring_alloc(256);
	in->event = eventp;
	return in;
}

static void free_stream(STREAM_IN *in)
{
	acl_vstream_free(in->in);
	acl_vstring_free(in->buf);
	acl_myfree(in);
}

static void read_callback(int event_type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *stream, void *context)
{
	STREAM_IN *in = (STREAM_IN *) context;
	int   ready, ret;

	acl_assert(in->in == stream);
	acl_assert(in->event == event);

	/* 尝试从输入流中读取一行数据，并且要求去掉尾部的回车换行符 */
	ret = acl_vstream_gets_nonl_peek(stream, in->buf, &ready);
	if (ret == ACL_VSTREAM_EOF) {
		acl_event_disable_read(event, stream);
		printf(">>>>gets error\r\n");
		free_stream(in);
		__stop_event = 1;
	} else if (ready) {
#define	STR	acl_vstring_str

		/* 如果读到完整的一行数据，则显示之 */
		printf(">>>>gets one line: %s\r\n", STR(in->buf));

		if (strcasecmp(STR(in->buf), "QUIT") == 0) {
			__stop_event = 1;
			free_stream(in);
		} else {
			ACL_VSTRING_RESET(in->buf); /* 清空缓冲区 */
		}
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		"	-M [printf time meter]\r\n"
		"	-t select|poll|kernel [default: select]\r\n"
		"	-s delay_sec [defaut: 1]\r\n"
		"	-u delay_usec [default: 0]\r\n"
		"	-m timer_delay [default: 100 microsecond, disable timer if < 0]\r\n"
		"	-k [if timer keep on, default: 1]\r\n", procname);
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	ACL_EVENT *eventp;
	STREAM_IN *in;
	char  event_type[64];
	int   ch, delay_sec = 1, delay_usec = 0;
	int   timer_delay = 100, timer_keep = 0;
	int   meter = 0;

	/* 初始化 acl 库 */
	acl_lib_init();

	/* 将错误日志输出至标准输出 */
	acl_msg_stdout_enable(1);

	event_type[0] = 0;

	while ((ch = getopt(argc, argv, "ht:s:u:m:kM")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 't':
			ACL_SAFE_STRNCPY(event_type, optarg, sizeof(event_type));
			break;
		case 's':
			delay_sec = atoi(optarg);
			if (delay_sec < 0) {
				delay_sec = 0;
			}
			break;
		case 'u':
			delay_usec = atoi(optarg);
			if (delay_usec < 0) {
				delay_usec = 0;
			}
			break;
		case 'm':
			timer_delay = atoi(optarg);
			break;
		case 'k':
			timer_keep = 1;
			break;
		case 'M':
			meter = 1;
			break;
		default:
			break;
		}
	}

	/* 创建非阻塞事件引擎 */
	if (strcasecmp(event_type, "kernel") == 0) {
		eventp = acl_event_new_kernel(delay_sec, delay_usec);
	} else if (strcasecmp(event_type, "poll") == 0) {
		eventp = acl_event_new_poll(delay_sec, delay_usec);
	} else {
		eventp = acl_event_new_select(delay_sec, delay_usec);
	}

	if (timer_delay >= 0) {
		acl_event_request_timer(eventp, trigger_event1, NULL,
			timer_delay, timer_keep);
		acl_event_request_timer(eventp, trigger_event2, NULL,
			timer_delay, timer_keep);
	}

	/* 创建输入流对象 */
	in = create_stream(eventp);

	/* 将标准输入置入异步 IO 监听事件中 */
	acl_event_enable_read(eventp, in->in, 0, read_callback, in);

	printf("begin wait input from standard in\r\n");

	/* 进行事件循环中 */
	while (!__stop_event) {
		if (meter) {
			ACL_METER_TIME("begin event");
		}
		acl_event_loop(eventp);
		if (meter) {
			ACL_METER_TIME("end event");
		}
	}

	/* 释放事件引擎 */
	acl_event_free(eventp);

	printf("event stopped!\r\n");
	return 0;
}
