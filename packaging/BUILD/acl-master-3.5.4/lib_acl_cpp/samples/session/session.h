#pragma once

// in memcache_session.cpp
bool test_memcache_session(const char* addr, int n);
void test_memcache_session_delay(const char* addr);

// in redis_session.cpp
bool test_redis_session(const char* addr, int n, int max_threads);
bool test_redis_session_attrs(const char* addr, int n);
void test_session_string(const char* addr);
