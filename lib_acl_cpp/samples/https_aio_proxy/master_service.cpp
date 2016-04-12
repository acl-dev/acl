#include "stdafx.h"
#include "req_callback.h"
#include "master_service.h"

//////////////////////////////////////////////////////////////////////////////

static char *var_cfg_backend_addr;
static char *var_cfg_request_file;
static char *var_cfg_respond_file;
static char *var_cfg_interface;
static int var_cfg_max_conns;
static int var_cfg_conn_timeout;
static int var_cfg_rw_timeout;

acl::master_str_tbl var_conf_str_tab[] = {
    { "https_backend_addr", "127.0.0.1:443", &var_cfg_backend_addr },
    { "https_request_file", "", &var_cfg_request_file },
    { "https_respond_file", "", &var_cfg_respond_file },
    { "https_interface", "/test/test_jqredis.php", &var_cfg_interface },    
    { 0, 0, 0 }
};

acl::master_bool_tbl var_conf_bool_tab[] = {

    { 0, 0, 0 }
};

acl::master_int_tbl var_conf_int_tab[] = {
    {"https_max_conns",100,&var_cfg_max_conns,0,0},
    {"https_conn_timeout",10,&var_cfg_conn_timeout,0,0},
    {"https_rw_timeout",10,&var_cfg_rw_timeout,0,0},
    { 0, 0 , 0 , 0, 0 }
};

static acl::polarssl_conf ssl_conf;
static acl::ofstream* __req_fp = NULL;
static acl::ofstream* __res_fp = NULL;
static acl::http_request_manager* __conn_manager = NULL;

//////////////////////////////////////////////////////////////////////////////

master_service::master_service()
{
}

master_service::~master_service()
{
}

bool master_service::on_accept(acl::aio_socket_stream* client)
{
    logger("connect from %s, fd %d", client->get_peer(true),client->sock_handle());

    req_callback* callback = new req_callback(__conn_manager,client,
        __req_fp, __res_fp);
    callback->start();
    return true;
}

void master_service::proc_on_init()
{
    if (var_cfg_request_file && *var_cfg_request_file)
    {
        __req_fp = new acl::ofstream;
        if (__req_fp->open_trunc(var_cfg_request_file) == false)
        {
            logger_error("open file %s error %s",
                var_cfg_request_file, acl::last_serror());
            delete __req_fp;
            __req_fp = NULL;
        }
    }

    if (var_cfg_respond_file && *var_cfg_respond_file)
    {
        __res_fp = new acl::ofstream;
        if (__res_fp->open_trunc(var_cfg_respond_file) == false)
        {
            logger_fatal("open file %s error %s",
                var_cfg_respond_file, acl::last_serror());
            delete __res_fp;
            __res_fp = NULL;
        }
    }
    __conn_manager = new acl::http_request_manager();
    __conn_manager->set_ssl(&ssl_conf);
    __conn_manager->init(var_cfg_backend_addr, var_cfg_backend_addr, var_cfg_max_conns, var_cfg_conn_timeout, var_cfg_rw_timeout);
}

void master_service::proc_on_exit()
{
    if (__req_fp)
        delete __req_fp;
    if (__res_fp)
        delete __res_fp;

    if (__conn_manager){
        // 销毁连接池
        delete __conn_manager;  
    }
}
