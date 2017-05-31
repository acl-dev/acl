#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/http/WebSocketServlet.hpp"
#endif

namespace acl
{
	WebSocketServlet::WebSocketServlet(socket_stream* stream, session* session)
		:HttpServlet(stream, session),
		max_msg_len_(100 * 1024*1024),
		ws_(NULL)
	{

	}

	WebSocketServlet::WebSocketServlet(
		socket_stream* stream, 
		const char* memcache_addr )
		:HttpServlet(stream, memcache_addr),
		max_msg_len_(100 * 1024 * 1024),
		ws_(NULL)
	{

	}

	WebSocketServlet::WebSocketServlet()
		:max_msg_len_(100 * 1024 * 1024),
		ws_(NULL)
	{

	}

	WebSocketServlet::~WebSocketServlet()
	{
		if (ws_)
			delete ws_;
		ws_ = NULL;
	}

	bool WebSocketServlet::doRun()
	{
		if (!ws_)
		{
			bool ret = HttpServlet::doRun();
			//websocket upgrade ok.
			//maybe http without keepalive ,
			//return false,framework will close this connection.
			if (ws_)
				return true;
			return ret;
		}

		if (ws_->read_frame_head() == false)
			return false;

		unsigned long long len = ws_->get_frame_payload_len();
		if (len > max_msg_len_)
		{
			logger_error("payload too large error: %llu", len);
			return false;
		}

		if (len > 0)
		{
			if (!recv_buffer_)
			{
				recv_buffer_ = (char*)acl_mymalloc((size_t)len + 1);
			}
			else
			{
				//frame not finish.
				recv_buffer_ = (char*)acl_myrealloc(recv_buffer_, write_pos_ + (size_t)len + 1);
			}
			
			if (ws_->read_frame_data(recv_buffer_+ write_pos_, len) < 0)
			{
				write_pos_ = 0;
				acl_myfree(recv_buffer_);
				recv_buffer_ = NULL;
				return false;
			}
			write_pos_ += len;
			recv_buffer_[write_pos_] = '\0';
		}
		int opcode = ws_->get_frame_opcode();
		bool ret = false;

		if (ws_->get_frame_fin() == false)
		{
			//safe opcode.
			if(opcode != FRAME_CONTINUATION)
				opcode_ = opcode;
			return true;
		}

		//frame is finish callback frame.
		if (opcode == FRAME_CONTINUATION)
		{
			opcode = opcode_;
		}

		switch (opcode)
		{
		case FRAME_PING:
			 ret = on_ping(recv_buffer_, write_pos_);
			 break;
		case FRAME_PONG:
			ret = on_pong(recv_buffer_, write_pos_);
			break;
		case FRAME_CLOSE:
			on_close();
			break;
		case FRAME_TEXT:
		case FRAME_BINARY:
			{
				ret = on_message(recv_buffer_, write_pos_,
					ws_->get_frame_opcode() == FRAME_TEXT);
				break;
			}
		default:
			logger_error("unknown websocket Frame opcode error: %d", 
				ws_->get_frame_opcode());
		}
		if (recv_buffer_)
		{
			acl_myfree(recv_buffer_);
			recv_buffer_ = NULL;
			write_pos_ = 0;
		}
			
		return ret;
	}


	bool WebSocketServlet::send_binary(const char *recv_buffer_, int len)
	{
		ws_->set_frame_opcode(FRAME_BINARY);
		ws_->set_frame_fin(true);
		ws_->set_frame_payload_len(len);
		return ws_->send_frame_data(recv_buffer_, len);
	}

	bool WebSocketServlet::send_text(const char *recv_buffer_fer)
	{
		ws_->set_frame_opcode(FRAME_TEXT);
		ws_->set_frame_fin(true);
		ws_->set_frame_payload_len(strlen(recv_buffer_fer));
		return ws_->send_frame_data(recv_buffer_fer, strlen(recv_buffer_fer));
	}

	bool WebSocketServlet::send_pong(const char *text)
	{
		size_t len = !text ? 0 : strlen(text);
		ws_->set_frame_opcode(FRAME_PONG);
		ws_->set_frame_fin(true);
		ws_->set_frame_payload_len(len);
		return ws_->send_frame_data(text, len);
	}

	bool WebSocketServlet::send_ping(const char *text)
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
		ws_ = new websocket(*getStream());
		return true;
	}

	void WebSocketServlet::on_close()
	{
	}
}