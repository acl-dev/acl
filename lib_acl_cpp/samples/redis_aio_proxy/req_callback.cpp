#include "stdafx.h"
#include "req_callback.h"
#include "redis_parse.h"

extern acl::redis_client_cluster __manager;
extern redisCommand redisCommandTable[];
const char flags[] = {'-', '-', '-', '+', ':', '$', '*'};

req_callback::req_callback(acl::aio_socket_stream* conn)
    : conn_(conn)
{

}

req_callback::~req_callback()
{

}

static acl::string& result_to_string(const acl::redis_result* result, acl::string& out)
{
    acl::redis_result_t type = result->get_type();
    if (type != acl::REDIS_RESULT_ARRAY)
    {
        acl::string buf;
        int len = result->argv_to_string(buf);
        if (len <= 0) {
            buf = "-1";
        }
        out += flags[type];
        if (type == acl::REDIS_RESULT_STRING) {
            out += len;
            out += "\r\n";
        }
        out += buf;
        out += "\r\n";
        return out;
    }

    size_t size;
    const acl::redis_result** children = result->get_children(&size);
    if (children == NULL) {
        return out;
    } else {
        out += flags[type];
        out += size;
        out += "\r\n";
    }

    for (size_t i = 0; i < size; i++)
    {
        const acl::redis_result* rr = children[i];
        if (rr != NULL) {
            result_to_string(rr, out);
        }
    }

    return out;
}

bool req_callback::read_callback(char* data, int len)
{
    struct redis_parse_t redis_parse;
    unsigned size = len;
    int parse_start = 0;
    char *p = NULL;

    redis_parse_init(&redis_parse, REDIS_REQUEST, (char *const *)&data, &size);
    do {
        redis_parse_request(&redis_parse);
        if (redis_parse.rs.over) {
            int id = redisCommandTable[redis_parse.rs.command_id].firstkey;
            redis_proxy_->hash_slot(data + redis_parse.rs.field[id].offset, redis_parse.rs.field[id].len);
            redis_proxy_->build_request(data + parse_start, redis_parse.rs.dosize);
            const acl::redis_result* result = redis_proxy_->run();
            if (result) {
                acl::string outstr;
                result_to_string(result, outstr);
                conn_->write(outstr.c_str(), outstr.length());
            } else {
                //服务器返回未知错误
                const char * error = redis_errno_description(2);
                conn_->write(error, strlen(error));
            }
            parse_start = parse_start + redis_parse.rs.dosize;
            p = data + parse_start;
            size = len - parse_start;
            redis_parse_init(&redis_parse, REDIS_REQUEST, (char *const *)&p, &size);
        } else {
            //返回解释错误
            const char * error = redis_errno_description(redis_parse.rs.error);
            conn_->write(error, strlen(error));

            //继续解释，找到起点
            parse_start = parse_start + redis_parse.rs.dosize;
            p = strchr(data + parse_start, '*');
            if (p) {
                parse_start = p - data;
                size = len - parse_start;
                redis_parse_init(&redis_parse, REDIS_REQUEST, (char *const *)&p, &size);
            } else {
                return false;
            }
        }
    } while (parse_start < len);

    return true;
}

void req_callback::close_callback()
{
    /*
        logger("disconnect from %s, fd: %d", conn_->get_peer(),
            conn_->sock_handle());
    */
    // 必须在此处删除该动态分配的回调类对象以防止内存泄露
    delete redis_proxy_;
    delete this;
}


void req_callback::start()
{
    redis_proxy_ = new acl::redis_proxy(&__manager, __manager.size());
    // 注册异步流的读回调过程
    conn_->add_read_callback(this);

    // 注册异步流的写回调过程
    conn_->add_write_callback(this);

    // 注册异步流的关闭回调过程
    conn_->add_close_callback(this);

    // 注册异步流的超时回调过程
    conn_->add_timeout_callback(this);

    //redis_.set_cluster(&__manager,__manager.size());

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