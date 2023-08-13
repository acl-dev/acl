#include "stdafx.h"
#include <signal.h>
#include "test/test_coder.h"
#include "test/test_db.h"
#include "master_service.h"

static void on_sigint(int) {
    master_service& ms = acl::singleton2<master_service>::get_instance();
    logger("---Begin to close db---");
    ms.close_db();
    logger("---Close db ok---");
    _exit(0);
}

static bool test_redis_coder(const char* file, size_t max) {
#if 1
    printf(">>>>>>>>Begin to test redis parsing<<<<<<<<<\r\n");
    if (!pkv::test_redis_parse(file)) {
        printf(">>>>>>>Test redis parsing Error<<<<<<<\r\n");
        return false;
    }
    printf(">>>>>>>Test redis parsing successfully<<<<<<<<<\r\n");

    printf("\r\n");

    printf(">>>>>>>>>Begin to test redis building<<<<<<<<\r\n");
    if (!pkv::test_redis_build()) {
        printf(">>>>>Test redis building Error<<<<<<<<\r\n");
        return false;
    }
    printf(">>>>>>>Test redis building successfully<<<<<<<\r\n");

#endif

    struct timeval begin, end;

    printf(">>>Begin to build benchmark\r\n");
    gettimeofday(&begin, NULL);
    size_t n = pkv::redis_build_bench(max);
    gettimeofday(&end, NULL);
    double cost = acl::stamp_sub(end, begin);
    double speed = (n * 1000) / (cost > 0 ? cost : 0.00001);
    printf(">>>Build ok, count=%zd, cost=%.2f, speed=%.2f\r\n", n, cost, speed);

    printf(">>>Begin to parse benchmark\r\n");
    gettimeofday(&begin, NULL);
    n = pkv::redis_parse_bench(file, max);
    gettimeofday(&end, NULL);
    cost = acl::stamp_sub(end, begin);
    speed = (n * 1000) / (cost > 0 ? cost : 0.00001);
    printf(">>>Parse ok, count=%zd, cost=%.2f, speed=%.2f\r\n", n, cost, speed);
    return true;
}

static void test_db(const char* dbpath, size_t max) {
    pkv::test_db::bench(dbpath, max);
}

int main(int argc, char *argv[]) {
    acl::acl_cpp_init();

    if (argc >= 2 && strcasecmp(argv[1], "test") == 0) {
        const char* file = "hash.txt";
        size_t max = 2000000;

        if (argc >= 3) {
            file = argv[2];
        }
        if (argc >= 4) {
            max = std::atoi(argv[3]);
        }
        test_redis_coder(file, max);
        test_db("./data/test", max);
        return 0;
    }

    master_service& ms = acl::singleton2<master_service>::get_instance();

    // 设置配置参数表
    ms.set_cfg_int(var_conf_int_tab);
    ms.set_cfg_int64(var_conf_int64_tab);
    ms.set_cfg_str(var_conf_str_tab);
    ms.set_cfg_bool(var_conf_bool_tab);

    if (argc == 1 || (argc >= 2 && strcasecmp(argv[1], "alone") == 0)) {
        signal(SIGINT, on_sigint);

        // 日志输出至标准输出
        acl::log::stdout_open(true);
        // 禁止生成 acl_master.log 日志
        acl::master_log_enable(false);

        const char* addr = nullptr;
        //printf("listen: %s\r\n", addr);
        ms.run_alone(addr, argc >= 3 ? argv[2] : nullptr);
    } else {
#if defined(_WIN32) || defined(_WIN64)
        const char* addr = "|8887";

		acl::log::stdout_open(true);
		printf("listen on: %s\r\n", addrs);
		ms.run_alone(addr, argc >= 3 ? argv[2] : NULL);
#else
        ms.run_daemon(argc, argv);
#endif
    }

    return 0;
}
