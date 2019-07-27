#pragma once

class HttpServerRpc : public acl::rpc_request
{
public:
	HttpServerRpc(acl::aio_socket_stream* client);
	~HttpServerRpc();

protected:
	// 鍩虹被 rpc_request 铏氬嚱鏁�

	// 瀛愮嚎绋嬪鐞嗗嚱鏁�
	void rpc_run();

	// 涓荤嚎绋嬪鐞嗚繃绋嬶紝鏀跺埌瀛愮嚎绋嬩换鍔″畬鎴愮殑娑堟伅
	void rpc_onover();

private:
	acl::aio_socket_stream* client_;
	bool keep_alive_;

	// 澶勭悊 HTTP 璇锋眰杩囩▼
	void handle_http(acl::socket_stream& stream);
};
