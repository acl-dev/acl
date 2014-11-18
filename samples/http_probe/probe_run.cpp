#include "stdafx.h"

#include <stdio.h>
#include <string.h>
#include <string.h>
#include <assert.h>

#include "probe.h"

/* forward declare */
static void timer_fn(int event_type, ACL_EVENT *event, void *context);

static void close_stream(PROBE_SERVER *server)
{
	if (server->stream != NULL) {
		ACL_ASTREAM *stream = server->stream;
		server->stream = NULL;
		acl_aio_iocp_close(stream);
	}
}

static void timer_retry(PROBE_SERVER *server)
{
	if (server->stream != NULL)
		close_stream(server);

	if (server->res != NULL) {
		http_res_free(server->res);
		server->res = NULL;
		server->hdr_res = NULL;
	} else if (server->hdr_res != NULL) {
		http_hdr_res_free(server->hdr_res);
		server->hdr_res = NULL;
	}
	
	acl_aio_request_timer(server->aio, timer_fn, server,
		server->probe_inter * 1000000, 0);
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
		acl_vstream_fprintf(server->logfp, "%s: <%s> addr(%s), url(%s),"
			" time(%ld), content_length(%lld), reply_status(%d), %s\n",
			fmtstr, (server->warn_time > 0 && server->time_total_cost
			       	>= server->warn_time) ? "WARN" : "INFO",
			server->addr, server->url, server->time_total_cost,
			server->hdr_res->hdr.chunked
				? -1 : server->hdr_res->hdr.content_length,
			server->hdr_res->reply_status, buf);
	} else
		acl_msg_info("<%s> addr(%s), url(%s), time(%ld), "
			"content_length(%lld), reply_status(%d), %s",
			(server->warn_time > 0 && server->time_total_cost
			 	>= server->warn_time) ? "WARN" : "INFO",
			server->addr, server->url, server->time_total_cost,
			server->hdr_res->hdr.chunked
				? -1 : server->hdr_res->hdr.content_length,
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
			myname, fmtstr, (server->warn_time > 0
				&& server->time_total_cost
					>= server->warn_time) ? "WARN" : "INFO",
			server->addr, server->url, server->time_total_cost, buf);
	} else
		acl_msg_error("%s <%s> addr(%s), url(%s), time(%ld), %s",
			myname, (server->warn_time > 0
				&& server->time_total_cost
					>= server->warn_time) ? "WARN" : "INFO",
			server->addr, server->url, server->time_total_cost, buf);
}

static int get_body_ready(int status, char *data acl_unused,
	int dlen acl_unused, void *arg)
{
	const char *myname = "get_body_ready";
	PROBE_SERVER *server = (PROBE_SERVER *) arg;

#if 0
	if (var_probe_debug_fp && status !=HTTP_STATUS_OK && dlen > 0) {
		acl_vstream_writen(var_probe_debug_fp, data, dlen);
	}
#else
	acl_msg_info("%s: len(%d), status=%d", myname, dlen, status);
#endif

	if (status == HTTP_CHAT_OK) {
		msg_info(server, "%s(%d), get body over, body_len(%d)",
			myname, __LINE__, dlen);

		close_stream(server);
		printf("---------------read over(%d) -----------------\r\n", dlen);
		timer_retry(server);
	} else if (status >= HTTP_CHAT_ERR_IO) {
		msg_error(server, "%s(%d), get body error(io), status(%d)",
			myname, __LINE__, status);

		close_stream(server);
		timer_retry(server);
		return -1;
	}

	return 0;
}

static int get_header_ready(int status, void *arg)
{
	const char *myname = "get_header_ready";
	PROBE_SERVER *server = (PROBE_SERVER *) arg;
	char buf[256];

	if (status != HTTP_CHAT_OK) {
		msg_error(server, "%s(%d), get header error", myname, __LINE__);
		close_stream(server);
		timer_retry(server);
		return -1;
	}

	if (http_hdr_res_parse(server->hdr_res) < 0) {
		msg_error(server, "%s(%d), parse hdr error", myname, __LINE__);
		close_stream(server);
		timer_retry(server);
		return -1;
	}

	snprintf(buf, sizeof(buf), "%d", server->hdr_res->reply_status);
	if (strstr(server->http_status_errors, buf) != NULL) {
		msg_error(server, "%s(%d), reply status(%s) error",
			myname, __LINE__, buf);
		if (var_probe_debug_fp)
			http_hdr_fprint(var_probe_debug_fp,
				&server->hdr_res->hdr, myname);
		close_stream(server);
		timer_retry(server);
		return -1;
	}

	if (server->hdr_res->hdr.content_length == 0) {
		msg_info(server, "%s(%d), get hdr over", myname, __LINE__);
		if (var_probe_debug_fp)
			http_hdr_fprint(var_probe_debug_fp,
				&server->hdr_res->hdr, myname);
		close_stream(server);
		timer_retry(server);
	} else {
		if (var_probe_debug_fp)
			http_hdr_fprint(var_probe_debug_fp,
				&server->hdr_res->hdr, myname);

		server->res = http_res_new(server->hdr_res);
		http_res_body_get_async(server->res, server->stream,
			get_body_ready, server, server->rw_timeout);
	}

	return 0;
}

static int write_callback(ACL_ASTREAM *stream, void *context)
{
	PROBE_SERVER *server = (PROBE_SERVER *) context;

	assert(stream == server->stream);
	server->hdr_res = http_hdr_res_new();
	printf("++++++++++++===begin get headr==========\r\n");
	http_hdr_res_get_async(server->hdr_res, stream,
		get_header_ready, server, server->rw_timeout);
	return 0;
}

static int connect_callback(ACL_ASTREAM *stream, void *context)
{
	PROBE_SERVER *server = (PROBE_SERVER *) context;

	acl_aio_add_write_hook(stream, write_callback, context);
	printf("======================write to server===============\r\n");
	acl_aio_writen(stream, server->http_request_header,
		server->http_request_len);
	printf("======================write to server ok===============\r\n");
	return 0;
}

static void timer_fn(int event_type acl_unused,
	ACL_EVENT *event acl_unused, void *context)
{
	const char *myname = "timer_fn";
	PROBE_SERVER *server = (PROBE_SERVER *) context;

	server->time_begin = time(NULL);

	server->stream = acl_aio_connect(server->aio, server->addr,
		server->connect_timeout);
	if (server->stream == NULL) {
		msg_error(server, "%s(%d), connect error(%s)",
			myname, __LINE__, strerror(errno));
		timer_retry(server);
	} else {
		acl_aio_add_connect_hook(server->stream,
			connect_callback, context);
		/*
		acl_aio_add_timeo_hook(server->stream, timeout_callback, ctx);
		acl_aio_add_close_hook(server->stream, closeed_callback, ctx);
		*/
	}
}

void probe_run()
{
	const char *myname = "run_init";
	PROBE_SERVER *server;
	ACL_AIO *aio;
	ACL_ITER iter;

	aio = acl_aio_create(ACL_EVENT_SELECT);

	if (var_probe_server_link == NULL)
		acl_msg_fatal("%s(%d): var_probe_server_link null",
			myname, __LINE__);

	acl_foreach(iter, var_probe_server_link) {
		server = (PROBE_SERVER*) iter.data;
		server->aio = aio;
		timer_retry(server);
	}

	while (1)
		acl_aio_loop(aio);
}
