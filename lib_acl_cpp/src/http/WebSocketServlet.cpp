/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Fu wangqin
 *   E-mail: "fuwangqin" <niukey@qq.com>
 * 
 * VERSION
 *   Wed 31 May 2017 02:37:14 PM CST
 */

#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/session/session.hpp"
#include "acl_cpp/http/websocket.hpp"
#include "acl_cpp/http/WebSocketServlet.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

WebSocketServlet::WebSocketServlet(socket_stream* stream, session* session)
: HttpServlet(stream, session)
, ws_(NULL)
, opcode_(0)
{
}

WebSocketServlet::WebSocketServlet(socket_stream* stream, 
	const char* memcache_addr)
: HttpServlet(stream, memcache_addr)
, ws_(NULL)
, opcode_(0)
{
}

WebSocketServlet::WebSocketServlet(void)
: ws_(NULL)
, opcode_(0)
{
}

WebSocketServlet::~WebSocketServlet(void)
{
	delete ws_;
}

bool WebSocketServlet::doRun(session& session, socket_stream* stream /* = NULL */)
{
	return HttpServlet::doRun(session, stream);
}

bool WebSocketServlet::doRun(const char* memcached_addr, socket_stream* stream)
{
	return HttpServlet::doRun(memcached_addr, stream);
}

bool WebSocketServlet::doRun(void)
{
	if (!ws_) {
		bool ret = HttpServlet::doRun();
		// websocket upgrade ok.
		// maybe http without keepalive ,
		// return false, framework will close this connection.
		if (ws_) {
			return true;
		}
		return ret;
	}

	if (ws_->read_frame_head() == false) {
		return false;
	}

	unsigned long long len = ws_->get_frame_payload_len();
	int  opcode = ws_->get_frame_opcode();
	bool ret    = false;

	bool finish = ws_->frame_is_fin();
	if (finish) {
		if (opcode == FRAME_CONTINUATION) {
			// restore the saved opcode
			opcode = opcode_;
		}
	} else if (opcode != FRAME_CONTINUATION) {
		// save opcode
		opcode_ = opcode;
	}

	switch (opcode) {
	case FRAME_PING:
		ret = onPing(len, finish);
		break;
	case FRAME_PONG:
		ret = onPong(len, finish);
		break;
	case FRAME_CLOSE:
		onClose();
		break;
	case FRAME_TEXT:
		ret = onMessage(len, true, finish);
		break;
	case FRAME_BINARY:
		ret = onMessage(len, false, finish);
		break;
	default:
		logger_error("unknown websocket frame opcode: %d", opcode);
		ret = false;
		break;
	}

	if (finish) {
		opcode_ = 0;
	}

	return ret;
}

int WebSocketServlet::readPayload(void* buf, size_t size)
{
	if (ws_ == NULL) {
		logger_error("ws_ NULL");
		return -1;
	}
	return ws_->read_frame_data(buf, size);
}

bool WebSocketServlet::sendBinary(const char *buffer, int len)
{
	ws_->set_frame_opcode(FRAME_BINARY);
	ws_->set_frame_fin(true);
	ws_->set_frame_payload_len(len);
	return ws_->send_frame_data(buffer, len);
}

bool WebSocketServlet::sendText(const char *buffer)
{
	ws_->set_frame_opcode(FRAME_TEXT);
	ws_->set_frame_fin(true);
	ws_->set_frame_payload_len(strlen(buffer));
	return ws_->send_frame_data(buffer, strlen(buffer));
}

bool WebSocketServlet::sendPong(const char *text)
{
	size_t len = !text ? 0 : strlen(text);
	ws_->set_frame_opcode(FRAME_PONG);
	ws_->set_frame_fin(true);
	ws_->set_frame_payload_len(len);
	return ws_->send_frame_data(text, len);
}

bool WebSocketServlet::sendPing(const char *text)
{
	size_t len = !text ? 0 : strlen(text);
	ws_->set_frame_opcode(FRAME_PING);
	ws_->set_frame_fin(true);
	ws_->set_frame_payload_len(len);
	return ws_->send_frame_data(text, len);
}

bool WebSocketServlet::doWebSocket(HttpServletRequest&, HttpServletResponse&)
{
	acl_assert(!ws_);
	ws_ = NEW websocket(*getStream());
	return true;
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
