#include "stdafx.h"
#include "status/StatusServlet.h"
#include "status/HttpServerRpc.h"

HttpServerRpc::HttpServerRpc(acl::aio_socket_stream* client)
: client_(client)
, keep_alive_(false)
{
}

HttpServerRpc::~HttpServerRpc()
{
}

// 鍦ㄥ瓙绾跨▼涓鐞�
void HttpServerRpc::rpc_run()
{
	// 鎵撳紑闃诲娴佸璞�
	acl::socket_stream stream;

	// 蹇呴』鐢� get_vstream() 鑾峰緱鐨� ACL_VSTREAM 娴佸璞″仛鍙傛暟
	// 鏉ユ墦寮€ stream 瀵硅薄锛屽洜涓哄湪 acl_cpp 鍜� acl 涓殑闃诲娴�
	// 鍜岄潪闃诲娴佹渶缁堥兘鏄熀浜� ACL_VSTREAM锛岃€� ACL_VSTREAM 娴�
	// 鍐呴儴缁存姢鐫€浜嗕竴涓/鍐欑紦鍐插尯锛屾墍浠ュ湪闀胯繛鎺ョ殑鏁版嵁澶勭悊涓紝
	// 蹇呴』姣忔灏� ACL_VSTREAM 鍋氫负鍐呴儴娴佺殑缂撳啿娴佹潵瀵瑰緟
	ACL_VSTREAM* vstream = client_->get_vstream();
	ACL_VSTREAM_SET_RWTIMO(vstream, var_cfg_rw_timeout);

	(void) stream.open(vstream);

	// 寮€濮嬪鐞嗚 HTTP 璇锋眰
	handle_http(stream);

	// 灏� ACL_VSTREAM 涓庨樆濉炴祦瀵硅薄瑙ｇ粦瀹氾紝杩欐牱鎵嶈兘淇濊瘉褰撻噴鏀鹃樆濉炴祦瀵硅薄鏃�
	// 涓嶄細鍏抽棴涓庤姹傝€呯殑杩炴帴锛屽洜涓鸿杩炴帴鏈韩鏄睘浜庨潪闃诲娴佸璞＄殑锛岄渶瑕侀噰
	// 鐢ㄥ紓姝ユ祦鍏抽棴鏂瑰紡杩涜鍏抽棴
	stream.unbind();
}

void HttpServerRpc::handle_http(acl::socket_stream& stream)
{
	StatusServlet servlet;
	acl::memcache_session session(var_cfg_session_addr);

	servlet.setLocalCharset("gb2312");

	// 鏄惁鍏佽涓庡鎴风涔嬮棿淇濇寔闀胯繛鎺�
	if (servlet.doRun(session, &stream) == true && servlet.keep_alive())
		keep_alive_ = true;
}

/////////////////////////////////////////////////////////////////////////////

// 鍦ㄤ富绾跨▼涓鐞�
void HttpServerRpc::rpc_onover()
{
	if (keep_alive_)
		client_->read_wait(var_cfg_rw_timeout);
	else
		client_->close();
}
