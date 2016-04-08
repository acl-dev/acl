#include "stdafx.h"
#include "req_callback.h"

req_callback::req_callback(acl::connect_manager* manager, acl::aio_socket_stream* conn,
	acl::ofstream* req_fp, acl::ofstream* res_fp)
: manager_(manager)
, conn_(conn)
, req_fp_(req_fp)
, res_fp_(res_fp)
{

}

req_callback::~req_callback()
{

}

bool req_callback::read_callback(char* data, int len)
{
	acl::string outstr;
	//set url
	char url[512];	
	char * var_cfg_backend_addr = *(var_conf_str_tab[0].target);
	ACL_ARGV *webserver = acl_argv_split(var_cfg_backend_addr, ":");
	char * domain = acl_strcasestr(data,webserver->argv[0]);
	acl_argv_free(webserver);
	if(domain==NULL){
		outstr.format("{\"ret\":0,\"rettype\":0,\"msg\":\"invalid url\",\"data\":\"\"}");
		conn_->write(outstr.c_str(), outstr.length());		
		logger_warn("url host is different with server,invalid url:%s\r\n",data);
		return false;
	}

	acl::safe_snprintf(url,sizeof(url),"%s",data);

	acl::http_request_pool* pool;
	acl::http_request* conn;

	pool = (acl::http_request_pool*) manager_->peek();

	if (pool == NULL)
	{
		outstr.format("{\"ret\":0,\"rettype\":1,\"msg\":\"peek connection pool failed\",\"data\":\"\"}");
		conn_->write(outstr.c_str(), outstr.length());		
		logger_warn("peek connection pool failed\r\n");
		return false;
	}
	
	conn = (acl::http_request*) pool->peek();
	if (conn == NULL)
	{
		outstr.format("{\"ret\":0,\"rettype\":2,\"msg\":\"peek connect failed from %s\",\"data\":\"\"}",pool->get_addr());
		conn_->write(outstr.c_str(), outstr.length());			
		logger_warn("peek connect failed from %s\r\n",pool->get_addr());
		return false;
	}
	// 需要对获得的连接重置状态，以清除上次请求过程的临时数据
	else
		conn->reset();


	// 创建 HTTP 请求头数据
	acl::http_header& header = conn->request_header();
	header.set_url(url)
		.set_keep_alive(true)
		.set_method(acl::HTTP_METHOD_GET)
		.accept_gzip(false);

	// 发送 HTTP 请求数据同时接收 HTTP 响应头
	if (conn->request(NULL, 0) == false)
	{
		pool->put(conn, false);
		return false;
	}

	char  buf[2048];
	int   ret, length = 0;
	int  size = 2048;
	char *p = buf;

	// 接收 HTTP 响应体数据
	while (true)
	{
		ret = conn->read_body(p, size);
		if (ret == 0)
			break;
		else if (ret < 0)
		{
			//错误日志
			pool->put(conn, false);
			outstr.format("{\"ret\":0,\"rettype\":3,\"msg\":\"read http body failed \",\"data\":\"\"}");
			conn_->write(outstr.c_str(), outstr.length());			
			logger_warn("read http body failed \r\n");			
			return false;
		}
		length += ret;
		p = p + ret;
		size = size - ret;
		if(size<=0){
			break;
		}
	}
	buf[length] = 0;
	pool->put(conn, true);
	outstr.format("{\"ret\":1,\"rettype\":0,\"msg\":\"\",\"data\":%s}",buf);
	conn_->write(outstr.c_str(), outstr.length());

	// 将数据写入本地请求文件
	if (req_fp_)
		req_fp_->write(data, len);

	// 将数据同时写入本地响应数据文件
	if (res_fp_)
		res_fp_->write(buf, length);

	return true;
}

void req_callback::close_callback()
{
	logger("disconnect from %s, fd: %d", conn_->get_peer(),
		conn_->sock_handle());

	// 必须在此处删除该动态分配的回调类对象以防止内存泄露  

	delete this;
}

void req_callback::start()
{
	// 注册异步流的读回调过程
	conn_->add_read_callback(this);

	// 注册异步流的写回调过程
	conn_->add_write_callback(this);

	// 注册异步流的关闭回调过程
	conn_->add_close_callback(this);

	// 注册异步流的超时回调过程
	conn_->add_timeout_callback(this);

    conn_->read();

}

acl::aio_socket_stream& req_callback::get_conn()
{
	acl_assert(conn_);
	return *conn_;
}


void req_callback::disconnect()
{
	if (conn_)
	{
		conn_->close();
	}
	else
		delete this;
}

