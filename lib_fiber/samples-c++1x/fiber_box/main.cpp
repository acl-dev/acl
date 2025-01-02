#include "stdafx.h"
#include <getopt.h>
#include <cstdio>
#include <cstdlib>
#include <memory>

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
    printf("usage: %s -h [help]\r\n"
            " -n count\r\n"
            " -c fibers\r\n"
            " -t box_type[sbox2|tbox2]"
            " -b buf_count\r\n", procname);
}

int main(int argc, char *argv[])
{
    int  ch, n = 10, c = 1, buf = 0;
    std::string type = "sbox2";

    acl::acl_cpp_init();
    acl::log::stdout_open(true);

    while ((ch = getopt(argc, argv, "hn:t:c:b:")) > 0) {
        switch (ch) {
            case 'h':
                usage(argv[0]);
                return 0;
            case 'n':
                n = atoi(optarg);
                break;
            case 't':
                type = optarg;
                break;
            case 'c':
                c = atoi(optarg);
                break;
            case 'b':
		buf = atoi(optarg);
                break;
            default:
                break;
        }
    }

    std::shared_ptr<acl::box2<int>> box;

    if (type == "sbox2") {
        box = std::make_shared<acl::fiber_sbox2<int>>(buf);
    } else if (type == "tbox2") {
        box = std::make_shared<acl::fiber_tbox2<int>>();
    } else {
        printf("Unknow box type=%s\r\n", type.c_str());
        return 1;
    }

    struct timeval begin;
    gettimeofday(&begin, nullptr);

    long long int total = 0;

    for (int i = 0; i < c; i++) {
        go[box, n, &total] {
            for (int j = 0; j < n; j++) {
                int cnt;
                if (! box->pop(cnt, -1)) {
                    printf("pop failed!\r\n");
                    break;
                }
                total++;
            }
        };
    }

    for (int i = 0; i < c; i++) {
        go[box, n] {
            for (int j = 0; j < n; j++) {
                box->push(j, true);
            }
        };
    }

    acl::fiber::schedule();

    struct timeval end;
    gettimeofday(&end, nullptr);

    double tc = acl::stamp_sub(end, begin);
    double speed = (total * 1000) / tc;
    printf("total: %lld, time cost: %.2f ms, speed: %.2f qps\r\n", total, tc, speed);
	return 0;
}
