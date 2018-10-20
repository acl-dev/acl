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

namespace acl
{

WebSocketServlet::WebSocketServlet(socket_stream* stream, session* session)
: HttpServlet(stream, session)
, max_msg_len_(100 * 1024*1024)
, ws_(NULL)
, buffer_(NULL)
, wpos_(0)
, opcode_(0)
{
}

WebSocketServlet::WebSocketServlet(socket_stream* stream, 
	const char* memcache_addr)
: HttpServlet(stream, memcache_addr)
, max_msg_len_(100 * 1024 * 1024)
, ws_(NULL)
, buffer_(NULL)
, wpos_(0)
, opcode_(0)
{
}

WebSocketServlet::WebSocketServlet(void)
: max_msg_len_(100 * 1024 * 1024)
, ws_(NULL)
, buffer_(NULL)
, wpos_(0)
, opcode_(0)
{
}

WebSocketServlet::~WebSocketServlet(void)
{
	delete ws_;
	if (buffer_) {
		acl_myfree(buffer_);
	}
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
	if (len > max_msg_len_) {
		logger_error("payload too large error: %llu", len);
		return false;
	}

	if (len > 0) {
		if (!buffer_) {
			buffer_ = (char*) acl_mymalloc((size_t) len + 1);
		} else {
			//frame not finish.
			buffer_ = (char*) acl_myrealloc(buffer_,
				wpos_ + (size_t) len + 1);
		}

		if (ws_->read_frame_data(buffer_ + wpos_, (size_t) len) < 0) {
			acl_myfree(buffer_);
			buffer_ = NULL;
			wpos_   = 0;
			return false;
		}
		wpos_ += (int) len;
		buffer_[wpos_] = '\0';
	}

	int  opcode = ws_->get_frame_opcode();
	bool ret    = false;

	if (ws_->get_frame_fin() == false) {
		//safe opcode.
		if(opcode != FRAME_CONTINUATION) {
			opcode_ = opcode;
		}
		return true;
	}

	//frame is finish callback frame.
	if (opcode == FRAME_CONTINUATION) {
		opcode = opcode_;
	}

	switch (opcode) {
	case FRAME_PING:
		ret = onPing(buffer_, wpos_);
		break;
	case FRAME_PONG:
		ret = onPong(buffer_, wpos_);
		break;
	case FRAME_CLOSE:
		onClose();
		break;
	case FRAME_TEXT:
	case FRAME_BINARY:
		ret = onMessage(buffer_, wpos_,
			ws_->get_frame_opcode() == FRAME_TEXT);
		break;
	default:
		logger_error("unknown websocket Frame opcode error: %d", 
			ws_->get_frame_opcode());
		break;
	}

	if (buffer_) {
		acl_myfree(buffer_);
		buffer_ = NULL;
		wpos_   = 0;
	}

	return ret;
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

bool WebSocketServlet::doWebsocket(HttpServletRequest&, HttpServletResponse&)
{
	acl_assert(!ws_);
	ws_ = NEW websocket(*getStream());
	return true;
}

} // namespace acl
