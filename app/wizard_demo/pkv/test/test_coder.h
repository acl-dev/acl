//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#pragma once

namespace pkv {

bool test_redis_parse(const char* filepath);
bool test_redis_parse_once(const char* filepath);
bool test_redis_parse_stream(const char* filepath);

bool test_redis_build();
size_t redis_build_bench(size_t max);
size_t redis_parse_bench(const char* filepath, size_t max);

} // namespace pkv
