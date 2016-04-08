#include "stdafx.h"
#include "master_service.h"

//////////////////////////////////////////////////////////////////////////////
// 配置内容项

static char *var_cfg_backend_addr;
static char *var_cfg_backend_pass;
static int var_cfg_redirect_max;
static int var_cfg_redirect_sleep;
static int var_cfg_max_conns;
static int var_cfg_conn_timeout;
static int var_cfg_rw_timeout;

static acl::master_str_tbl var_conf_str_tab[] = {
    { "redis_backend_addr", "10.105.17.224:6000", &var_cfg_backend_addr },
    { "redis_backend_pass", "", &var_cfg_backend_pass },
    { 0, 0, 0 }
};

static acl::master_bool_tbl var_conf_bool_tab[] = {
    { 0, 0, 0 }
};

static acl::master_int_tbl var_conf_int_tab[] = {
    {"redis_redirect_max",10,&var_cfg_redirect_max,0,0},
    {"redis_redirect_sleep",500,&var_cfg_redirect_sleep,0,0},
    {"redis_max_conns",100,&var_cfg_max_conns,0,0},
    {"redis_conn_timeout",10,&var_cfg_conn_timeout,0,0},
    {"redis_rw_timeout",10,&var_cfg_rw_timeout,0,0},
    { 0, 0 , 0 , 0, 0 }
};

acl::redis_client_cluster __manager;

void setRedis(){
    __manager.set_retry_inter(1);
    // 设置重定向的最大阀值，若重定向次数超过此阀值则报错
    if(var_conf_int_tab[0].target){
        __manager.set_redirect_max(*var_conf_int_tab[0].target);        
    }else{
        __manager.set_redirect_max(var_conf_int_tab[0].defval);
    }

    // 当重定向次数 >= 2 时每次再重定向此函数设置休息的时间(毫秒)
    if(var_conf_int_tab[1].target){
        __manager.set_redirect_sleep(*var_conf_int_tab[1].target);        
    }else{
        __manager.set_redirect_max(var_conf_int_tab[1].defval);
    }

    acl::string backend_addr;
    if(var_conf_str_tab[0].target){
        backend_addr = (*var_conf_str_tab[0].target);        
    }else{
        backend_addr = (*var_conf_str_tab[0].defval); 
    }
    const std::vector<acl::string>& token = backend_addr.split2(",; \t");

    int redis_max_conns = var_conf_int_tab[2].defval;
    if(var_conf_int_tab[2].target){
        redis_max_conns = *var_conf_int_tab[2].target;
    }
    int redis_conn_timeout = var_conf_int_tab[3].defval;
    if(var_conf_int_tab[3].target){
        redis_conn_timeout = *var_conf_int_tab[3].target;
    }
    int redis_rw_timeout = var_conf_int_tab[4].defval;
    if(var_conf_int_tab[4].target){
        redis_rw_timeout = *var_conf_int_tab[4].target;
    }
    __manager.set(backend_addr.c_str(), redis_max_conns, redis_conn_timeout, redis_rw_timeout);
    __manager.set_all_slot(token[0], redis_max_conns);
    if(var_conf_str_tab[1].target){
        __manager.set_password("default", *var_conf_str_tab[1].target);
    }
}

int main(int argc, char* argv[])
{
    char *pathname = NULL;
    ACL_FILE *fp = NULL;
    acl::master_conf *conf;

    // 初始化 acl 库
    acl::acl_cpp_init();

    master_service& ms = acl::singleton2<master_service>::get_instance();

    // 设置配置参数表
    /*
    ms.set_cfg_int(var_conf_int_tab);
    ms.set_cfg_str(var_conf_str_tab);
    ms.set_cfg_bool(var_conf_bool_tab);
    */
    conf = new acl::master_conf();
    conf->set_cfg_int(var_conf_int_tab);
    conf->set_cfg_str(var_conf_str_tab);
    conf->set_cfg_bool(var_conf_bool_tab);

    // 开始运行

    if (argc >= 2 && strcmp(argv[1], "alone") == 0)
    {
        acl::log::stdout_open(true);  // 日志输出至标准输出
        const char* addr = ":5300";
        logger("listen on: %s\r\n", addr);
        if (argc >= 3){
            logger("config path: %s", argv[2]);
            fp = acl_fopen((const char*)argv[2],"r");
            if(fp){
                acl_fclose(fp);
                conf->load(argv[2]);
                setRedis();
                delete conf;
                ms.run_alone(addr, argv[2], acl::ENGINE_SELECT);
            }else{
                logger_error("Invalid config path: %s", argv[2]);
            }
        }
        else{
            pathname = acl_concatenate(argv[0], ".cf", (char *) 0);
            logger("config path: %s", pathname);
            fp = acl_fopen((const char*)pathname,"r");
            if(fp){
                acl_fclose(fp);
                conf->load(pathname);
                acl_myfree(pathname);
                setRedis();
                delete conf;
                ms.run_alone(addr, NULL, acl::ENGINE_SELECT);  // 单独运行方式
            }else{
                logger_error("Invalid config path: %s", pathname);
            }                
        }
    }
    else
    {
#ifdef  WIN32
        acl::log::stdout_open(true);  // 日志输出至标准输出
        const char* addr = ":5300";
        logger("listen on: %s\r\n", addr);
        pathname = acl_concatenate(argv[0],".cf", (char *) 0);
        logger("config path: %s", pathname);
        fp = acl_fopen((const char*)pathname,"r");
        if(fp){
            acl_fclose(fp);
            conf->load(pathname);
            acl_myfree(pathname);
            setRedis();
            delete conf;
            ms.run_alone(addr, NULL, acl::ENGINE_SELECT);  // 单独运行方式
        }else{
            logger_error("Invalid config pathname: %s", pathname);
        }
#else
        if (argc >= 2 && strcmp(argv[1], "-f") == 0){
            fp = acl_fopen(argv[2],"r");
            if(fp){
                acl_fclose(fp);
                conf->load(argv[2]);
                setRedis();
                delete conf;      
                ms.run_daemon(argc, argv);  // acl_master 控制模式运行          
            }else{
                acl::log::stdout_open(true);  // 日志输出至标准输出
                logger_error("Invalid config pathname: %s", argv[2]);
            }
        }else{
                acl::log::stdout_open(true);  // 日志输出至标准输出
                logger_error("This program only run with alone or daemon mode");
        }
#endif
    }
    if(conf)
        delete conf;
    return 0;
}

