# Acl redis - One C++ redis client library in Acl

## 0. Introduction
The redis module in Acl is a powerful redis client library with higth performance, rich interface and easy to use. There are more than 13 C++ classes and over 150 commands in Acl redis, including STRING, HASH, LIST, SET, ZSET, HyperLogLog, PUBSUB, STREAM, TRANSACTION, SCRIPT, CONNECTION, SERVER, etc. User using Acl redis doesn't need care about network comminucation, redis protocol, hash slots caching, etc., just like using C++ STL standard interface.

<hr>

* [0. Introduction](#0-introduction)
* [1. Building Acl redis](#1-building-acl-redis)
    * [1.1. Compiling on UNIX/LINUX](#11-compiling-on-unixlinux)
    * [1.2. Compiling on Windows](#12-compiling-on-windows)
* [2. Write some examples using Acl redis](#2-write-some-examples-using-acl-redis)
    * [2.1. Simple example for redis STRING and redis KEY](#21-simple-example-for-redis-string-and-redis-key)
    * [2.2. Redis client cluster example for redis3.0+](#22-redis-client-cluster-example-for-redis30)
    * [2.3. Using redis client cluster in multi-threads](#23-using-redis-client-cluster-in-multi-threads)
    * [2.4. Use redis pipeline in multi-threads](#24-use-redis-pipeline-in-multi-threads)
* [3. Add Acl redis to your projects](#3-add-acl-redis-to-your-projects)
    * [3.1. On UNIX/LINUX](#31-on-unixlinux)
    * [3.2. On Windows](#32-on-windows)
* [4. Reference](#4-reference)
* [5. About Acl](#5-about-acl)

<hr>

## 1. Building Acl redis
Acl redis is a part of lib_acl_cpp, so users only need to build the Acl project.

### 1.1. Compiling on UNIX/LINUX
- Enter the root directory of the Acl project and type **make**, **libacl_all.a** will be created later, which consists of three libraries: **libacl_cpp.a**, **lib_protocol.a** and **lib_acl.a**;
- Compile redis samples: Enter into lib_acl_cpp/samples/redis and type **make**, all the redis samples(including redis_cluster, redis_connection, redis_hash, redis_hyperloglog, redis_key, redis_lib, redis_manager, redis_pool, redis_pubsub, redis_server, redis_set, redis_string, redis_trans, redis_zset, redis_zset_pool, redis_client_cluster) will be compiled.

### 1.2. Compiling on Windows
Users can use `VS2003/VS2008/VS2010/VS2012/VS2015/VS2017/VS1019` to compile all Acl libraries by opening the Acl projects(acl_cpp_vc2003.sln, acl_cpp_vc2008.sln, acl_cpp_vc2010.sln, acl_cpp_vc2012.sln, acl_cpp_vc2015.sln, acl_cpp_vc2017.sln, acl_cpp_vc2019.sln) with the corresponding VS tools. Due to the dependency in Acl, lib_acl should be compiled first, then lib_protocol, and finally lib_acl_cpp.

## 2. Write some examples using Acl redis
### 2.1. Simple example for redis STRING and redis KEY:
```c++
#include <stdlib.h>
#include <stdio.h>
#include "acl_cpp/lib_acl.hpp"

static void test_redis_string(acl::redis& cmd, const char* key) {
	acl::string val("test_value");

	// call redis-server: SET key value
	if (!cmd.set(key, val.c_str())) {
		printf("redis set error\r\n");
		return;
	}

	// clear the string buf space
	val.clear();

	// reset the redis command object for reusing it
	cmd.clear();

	// call redis-server: GET key
	if (!cmd.get(key, val)) {
		printf("get key error\r\n");
	}
}

static void test_redis_key(acl::redis& cmd, const char* key) {
	if (cmd.exists(key)) {
		printf("key exists\r\n");
	} else {
		printf("key not exists\r\n");
	}
}

int main(void) {
	// init socket module for windows
	acl::acl_cpp_init();

	const char* redis_addr = "127.0.0.1:6379";
	int conn_timeout = 10, rw_timeout = 10;

	// the redis client connection
	acl::redis_client conn(redis_addr, conn_timeout, rw_timeout);

	const char* key = "test_key";

	// Bind redis_string command with redis connection.
	acl::redis cmd(&conn);
	test_redis_string(cmd, key);

	cmd.clear();  // Clear the temp memory.

	// Test redis KEY command with the same redis connection.
	test_redis_key(cmd, key);

	return 0;
}
```
### 2.2. Redis client cluster example for redis3.0+
```c++
#include <stdlib.h>
#include <stdio.h>
#include "acl_cpp/lib_acl.hpp"

int main(void) {
	// Init socket module for windows
	acl::acl_cpp_init();

	const char* redis_addr = "127.0.0.1:6379";
	int conn_timeout = 10, rw_timeout = 10, max_conns = 100;

	// Declare redis connection cluster ojbect.
	acl::redis_client_cluster cluster;
	cluster.set(redis_addr, max_conns, conn_timeout, rw_timeout);

	// Redis operation command.
	acl::redis cmd;

	// Bind redis command with redis connection cluster.
	cmd.set_cluster(&cluster);

	const char* key = "test_key";

	// Call redis server
	test_redis_string(cmd, key);
	cmd.clear();
	test_redis_key(cmd, key);

	return 0;
}
```
The redis cluster module of Acl supports caching the redis hash slots on client to improve performance, and can dynamically change local hash slots at runtime.

### 2.3. Using redis client cluster in multi-threads
```c++
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "acl_cpp/lib_acl.hpp"

static int __max_conns = 100;

static void* thread_main(void* arg) {
	acl::redis_client_cluster* cluster = (acl::redis_client_cluster*) arg;

	acl::redis cmd;
	cmd.set_cluster(cluster);

	const char* key = "test_key";

	for (int i = 0; i < 100000; i++) {
		test_redis_string(cmd, key);
		test_redis_key(cmd, key);
		cmd.clear(); // Clear temporary meory to avoid meory overflow.
	}

	return NULL;
}

int main(void) {
	// init socket module for windows
	acl::acl_cpp_init();

	const char* redis_addr = "127.0.0.1:6379";
	int conn_timeout = 10, rw_timeout = 10;

	// declare redis cluster ojbect
	acl::redis_client_cluster cluster;
	cluster.set(redis_addr, __max_conns, conn_timeout, rw_timeout);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	
	// create first thread
	pthread_t id1;
	pthread_create(&id1, &attr, thread_main, &cluster);

	// create second thread
	pthread_t id2;
	pthread_create(&id2, &attr, thread_main, &cluster);

	pthread_join(id1, NULL);
	pthread_join(id2, NULL);

	return 0;
}
```

### 2.4. Use redis pipeline in multi-threads
```c++
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "acl_cpp/lib_acl.hpp"

static void* thread_main(void* arg) {
	acl::redis_client_pipeline* pipeline = (acl::redis_client_pipeline*) arg;

	acl::redis cmd;
	cmd.set_pipeline(pipeline);

	acl::string key;

	for (int i = 0; i < 100000; i++) {
		key.format("test-key-%d", i);
		test_redis_string(cmd, key);
		test_redis_key(cmd, key);
	}

	return NULL;
}

int main(void) {
	// Init socket module for windows
	acl::acl_cpp_init();

	const char* redis_addr = "127.0.0.1:6379";

	// Declare redis pipeline ojbect
	acl::redis_client_pipeline pipeline(redis_addr);

	// Start the pipeline thread backend.
	pipeline.start_thread();

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	
	std::vector<pthread_t> threads;
	// start some threads to execute redis operations
	for (size_t i = 0; i < 100; i++) {
		pthread_t id;
		int ret = pthread_create(&id, &attr, thread_main, &pipeline);
		if (ret == 0) {
			threads.push_back(id);
		}
	}

	// wait for all threads to exit
	for (std::vector<pthread_t>::iterator it = threads.begin();
		it != threads.end(); ++it) {

		pthread_join(*it, NULL);
	}

	// stop the pipeline thread
	pipeline.stop_thread();
	return 0;
}
```

## 3. Add acl redis to your projects
Before using Acl redis, you should compile the three basic libraries. Enter the *lib_acl*, *lib_protocol*, *lib_acl_cpp*, and build the `lib_acl.a`, `lib_protocol.a` and `libacl_cpp.a`.
```compile
$cd lib_acl; make
$cd lib_protocol; make
$cd lib_acl_cpp; make
```

### 3.1. On UNIX/LINUX
You should add the following compilation options in your Makefile:
- **Compling options:** `-I` specify the lib_acl.hpp's parent path, for exmaple: `-I./lib_acl_cpp/include`;
- **Linking options:** Link with -L{path_to_acl_cpp} -lacl_cpp -L{path_to_protocol} -lprotocol -L{path_to_acl) -lacl ;
- **Reference:** lib_acl_cpp/samples/Makefile.in, lib_acl_cpp/samples/redis/redis/Makefile.

One Makefile as below:
```Makefile
ACL_PATH=./acl
CFLAGS = -c -g -W -O3 -Wall -Werror -Wshadow \
	-Wno-long-long -Wpointer-arith -D_REENTRANT \
	-D_POSIX_PTHREAD_SEMANTICS \
	-I $(ACL_PATH)/lib_acl_cpp/include
LDFLAGS = -L$(ACL_PATH)/lib_acl_cpp/lib -lacl_cpp \
	-L$(ACL_PATH)/lib_protocol/lib -lprotocol \
	-L$(ACL_PATH)/lib_acl/lib -lacl \
	-lpthread
test: main.o
	g++ -o main.o $(LDFLAGS)
main.o: main.cpp
	g++ $(CFLAGS) main.cpp -o main.o
```
### 3.2. On Windows
Open the VS projects, such as acl_cpp_vc2003.sln, acl_cpp_vc2008.sln, acl_cpp_vc2010.sln, acl_cpp_vc2012.sln, acl_cpp_vc2013.sln, acl_cpp_vc2015.sln, acl_cpp_vc2017.sln, or acl_cpp_vc2019.sln to  look at the redis samples project option setting.

## 4. Reference
- Acl redis headers: [Redis include files](../../include/acl_cpp/redis/)
- Acl redis source: [Redis source files](../../src/redis/)
- More examples: See examples in the current directory
## 5. About Acl
- See [About Acl](../../../README.md)
