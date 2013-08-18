#include "lib_acl.h"
#include "lib_protocol.h"

#include <stdio.h>
#include <string.h>
#include <string.h>
#include <assert.h>

#include "probe.h"

/* forward declare */
static void timer_fn(int event_type, void *context);

static void timer_retry(ACL_AIO *aio, PROBE_SERVER *server)
{
	if (server->stream != NULL) {
		acl_vstream_close(server->stream);
		server->stream = NULL;
	}

	if (server->respond != NULL) {
		http_res_free(server->respond);
		server->respond = NULL;
		server->hdr_res = NULL;
	} else if (server->hdr_res != NULL) {
		http_hdr_res_free(server->hdr_res);
		server->hdr_res = NULL;
	}
	
	(void) acl_aio_request_timer(aio, timer_fn, (void *) server, server->probe_inter);
}

static void msg_info(PROBE_SERVER *server, const char *fmt, ...)
{
	const char *myname = "msg_info";
	va_list ap;
	static char buf[4096];

	if (server == NULL)
		acl_msg_fatal("%s: server null", myname);
	if (fmt == NULL)
		acl_msg_fatal("%s: fmt null", myname);

	va_start(ap, fmt);
	acl_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	server->time_end = time(NULL);
	server->time_total_cost = server->time_end - server->time_begin;

	if (server->logfp != NULL) {
		char fmtstr[128];

		acl_logtime_fmt(fmtstr, sizeof(fmtstr));
		acl_vstream_fprintf(server->logfp,
			"%s: <%s> addr(%s), url(%s), time(%ld), content_length(%d), reply_status(%d), %s\n",
			fmtstr,
			(server->warn_time > 0 && server->time_total_cost >= server->warn_time) ? "WARN" : "INFO",
			server->addr, server->url, server->time_total_cost,
			server->hdr_res->hdr.chunked ? -1 : server->hdr_res->hdr.content_length,
			server->hdr_res->reply_status, buf);
	} else
		acl_msg_info("<%s> addr(%s), url(%s), time(%ld), content_length(%d), reply_status(%d), %s",
			(server->warn_time > 0 && server->time_total_cost >= server->warn_time) ? "WARN" : "INFO",
			server->addr, server->url, server->time_total_cost,
			server->hdr_res->hdr.chunked ? -1 : server->hdr_res->hdr.content_length,
			server->hdr_res->reply_status, buf);
}

static void msg_error(PROBE_SERVER *server, const char *fmt, ...)
{
	const char *myname = "msg_error";
	va_list ap;
	static char buf[4096];

	if (server == NULL)
		acl_msg_fatal("%s: server null", myname);
	if (fmt == NULL)
		acl_msg_fatal("%s: fmt null", myname);

	va_start(ap, fmt);
	acl_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	server->time_end = time(NULL);
	server->time_total_cost = server->time_end - server->time_begin;

	if (server->logfp != NULL) {
		char fmtstr[128];

		acl_logtime_fmt(fmtstr, sizeof(fmtstr));
		acl_vstream_fprintf(server->logfp,
			"%s, %s: <%s> addr(%s), url(%s), time(%ld), %s\n",
			myname, fmtstr,
			(server->warn_time > 0 && server->time_total_cost >= server->warn_time) ? "WARN" : "INFO",
			server->addr, server->url, server->time_total_cost, buf);
	} else
		acl_msg_error("%s <%s> addr(%s), url(%s), time(%ld), %s",
			myname,
			(server->warn_time > 0 && server->time_total_cost >= server->warn_time) ? "WARN" : "INFO",
			server->addr, server->url, server->time_total_cost, buf);
}

static int get_body_ready(int status, const char *data, int dlen, void *arg)
{
	const char *myname = "get_body_ready";
	PROBE_SERVER *server = (PROBE_SERVER *) arg;
	ACL_AIO *aio = server->aio;

#if 0
	if (var_probe_debug_fp && status !=HTTP_STATUS_OK && dlen > 0) {
		acl_vstream_writen(var_probe_debug_fp, data, dlen);
	}
#else
	data = data;
	dlen = dlen;

	acl_msg_info("%s: len(%d), status=%d", myname, dlen, status);
#endif

	if (status == HTTP_CHAT_OK) {
		msg_info(server, "%s(%d), get body over, body_len(%d)",
				myname, __LINE__, dlen);
		timer_retry(aio, server);
	} else if (status >= HTTP_CHAT_ERR_IO) {
		msg_error(server, "%s(%d), get body error(io), status(%d)",
			myname, __LINE__, status);
		timer_retry(aio, server);
		return (-1);
	}

	return (0);
}

static void get_header_ready(int status, void *arg)
{
	const char *myname = "get_header_ready";
	PROBE_SERVER *server = (PROBE_SERVER *) arg;
	ACL_AIO *aio = server->aio;
	char buf[256];

	if (status != HTTP_CHAT_OK) {
		msg_error(server, "%s(%d), get header error", myname, __LINE__);
		timer_retry(aio, server);
		return;
	}

	if (http_hdr_res_parse(server->hdr_res) < 0) {
		msg_error(server, "%s(%d), parse hdr error", myname, __LINE__);
		timer_retry(aio, server);
		return;
	}

	snprintf(buf, sizeof(buf), "%d", server->hdr_res->reply_status);
	if (strstr(server->http_status_errors, buf) != NULL) {
		msg_error(server, "%s(%d), reply status(%s) error", myname, __LINE__, buf);
		if (var_probe_debug_fp)
			http_hdr_fprint(var_probe_debug_fp, &server->hdr_res->hdr, myname);
		timer_retry(aio, server);
		return;
	}

	if (server->hdr_res->hdr.content_length == 0) {
		msg_info(server, "%s(%d), get hdr over", myname, __LINE__);
		if (var_probe_debug_fp)
			http_hdr_fprint(var_probe_debug_fp, &server->hdr_res->hdr, myname);
		timer_retry(aio, server);
	} else {
		if (var_probe_debug_fp)
			http_hdr_fprint(var_probe_debug_fp, &server->hdr_res->hdr, myname);

		server->respond = http_res_new(server->hdr_res);
		http_res_body_get_ready(server->respond,
				aio,
				server->stream,
				server->rw_timeout,
				get_body_ready,
				server);
	}
}

static void write_notify_fn(int event_type,
		ACL_AIO *aio,
		ACL_VSTREAM *stream,
		void *context,
		const char *data,
		int dlen)
{
	const char *myname = "write_notify_fn";
	PROBE_SERVER *server = (PROBE_SERVER *) context;

	assert(stream == server->stream);
	data = data;
	dlen = dlen;

	if (event_type == ACL_EVENT_XCPT) {
		msg_error(server, "%s(%d), write error(%s)",
			myname, __LINE__, strerror(errno));
		timer_retry(aio, server);
		return;
	} else if (event_type == ACL_EVENT_RW_TIMEOUT) {
		msg_error(server, "%s(%d), write timeout", myname, __LINE__);
		timer_retry(aio, server);
		return;
	}

	server->hdr_res = http_hdr_res_new();
	http_hdr_res_get_ready(server->hdr_res,
			aio,
			server->stream,
			server->rw_timeout,
			get_header_ready,
			server);
}

static void connect_notify_fn(int event_type,
		ACL_AIO *aio,
		ACL_VSTREAM *stream,
		void *context)
{
	const char *myname = "connect_notify_fn";
	PROBE_SERVER *server = (PROBE_SERVER *) context;
	int   ret;

	if (event_type == ACL_EVENT_XCPT) {
		msg_error(server, "%s(%d), connect error(%s)",
				myname, __LINE__, strerror(errno));
		timer_retry(aio, server);
		return;
	} else if (event_type == ACL_EVENT_RW_TIMEOUT) {
		msg_error(server, "%s(%d), connect timeout", myname, __LINE__);
		timer_retry(aio, server);
		return;
	}

	server->stream = stream;

	ret = acl_aio_writen(aio,
			stream,
			server->rw_timeout,
			server->http_request_header,
			server->http_request_len,
			write_notify_fn,
			server);
	if (ret < 0) {
		msg_error(server, "%s(%d), write error(%s)",
				myname, __LINE__, strerror(errno));
		timer_retry(aio, server);
		return;
	}
}

static void timer_fn(int event_type, void *context)
{
	const char *myname = "timer_fn";
	PROBE_SERVER *server = (PROBE_SERVER *) context;
	ACL_AIO *aio = server->aio;
	int   ret;

	event_type = event_type;

	server->time_begin = time(NULL);

	ret = acl_aio_connect(aio, server->addr, server->connect_timeout,
				connect_notify_fn, server);
	if (ret < 0) {
		msg_error(server, "%s(%d), connect error(%s)",
				myname, __LINE__, strerror(errno));
		timer_retry(aio, server);
	}
}

void probe_run(int max_threads, int idle_limit)
{
	const char *myname = "run_init";
	PROBE_SERVER *server;
	ACL_AIO *aio;
	int   i;

	aio = acl_aio_create(max_threads, idle_limit);
	if (aio == NULL)
		acl_msg_fatal("%s: create aio error", myname);

	if (var_probe_server_link == NULL)
		acl_msg_fatal("%s(%d): var_probe_server_link null", myname, __LINE__);

	for (i = 0; i < acl_array_size(var_probe_server_link); i++) {
		server = (PROBE_SERVER *) acl_array_index(var_probe_server_link, i);
		server->aio = aio;
		timer_retry(aio, server);
	}

	while (1) {
		acl_aio_loop(aio);
	}
}
