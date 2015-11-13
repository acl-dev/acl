# acl redis -- one C++ redis lib based on acl

## descrption
The acl redis is one C++ redis lib, which is one part of acl project. There're 12 redis CLASS and 150+ redis commands included in the acl redis. The acl redis provides all functions for each redis command including  including STRING, HASH, LIST, SET, ZSET, HLL, PUBSUB, TRANSACTION, SCRIPT, CONNECTION, SERVER. And the new cluster redis3.0's new commands(MOVE, ASK) are also implemented in the acl redis.
The header files of acl redis are in lib_acl_cpp\include\acl_cpp\redis; the source code files are in lib_acl_cpp\src\redis; and the redis samples are in lib_acl_cpp\samples\redis.

## compile
Because acl redis lib is a part of lib_acl_cpp lib, and lib_acl_cpp depend lib_acl and lib_protocol, you should compile lib_acl and lib_protocol libs first, and compile lib_acl_cpp lib. After you've compiled lib_acl_cpp lib, the redis lib is also compiled OK.

### compile on UNIX/LINUX
- 1 compile `lib_acl.a`: Enter into *lib_acl* path and type make, the lib_acl.a will be compiled
- 2 compile `lib_protocol.a`: Enter into *lib_protocol* path and type make, the lib_protocol.a will be compiled
- 3 compile `lib_acl_cpp.a`: Enter into *lib_acl_cpp* path and type make, the lib_acl_cpp.a will be compiled
- 4 compile redis samples: Enter into lib_acl_cpp\samples\redis and type make, all the redis samples(including redis_cluster, redis_connection, redis_hash, redis_hyperloglog, redis_key, redis_lib, redis_manager, redis_pool, redis_pubsub, redis_server, redis_set, redis_string, redis_trans, redis_zset, redis_zset_pool, redis_client_cluster) will be compiled.

### compile on WINDOWS
You can use `VC2003`, `VC2008`, `VC2010`, `VC2012` to build all acl libs including acl redis lib in lib_acl_cpp module when you open the acl projects(acl_cpp_vc2003.sln, acl_cpp_vc2008.sln, acl_cpp_vc2010.sln, acl_cpp_vc2012.sln). You should build lib_acl first, and second build lib_protocol, and third build lib_acl_cpp, and at last build all the acl samples including redis samples.

## write some samples using acl redis lib
### simple example for redis STRING and redis KEY:

```c++
#include <stdlib.h>
#include <stdio.h>
#include "acl_cpp/lib_acl.hpp"

static void test_redis_string(acl::redis_string& cmd, const char* key)
{
	acl::string val("test_value");

	// call redis-server: SET key value
	if (cmd.set(key, val.c_str()) == false)
	{
		printf("redis set error\r\n");
		return;
	}

	// clear the string buf space
	val.clear();

	// reset the redis command object for reusing it
	cmd.clear();

	// call redis-server: GET key
	if (cmd.get(key, val) == false)
		printf("get key error\r\n");
}

static void test_redis_key(acl::redis_key& cmd, const char* key)
{
	if (cmd.exists(key) == false)
		printf("key not exists\r\n");
	else
		printf("key exists\r\n");
}

int main()
{
	// init socket module for windows
	acl::acl_cpp_init();

	const char* redis_addr = "127.0.0.1:6379";
	int conn_timeout = 10, rw_timeout = 10;

	// the redis client connection
	acl::redis_client conn(redis_addr, conn_timeout, rw_timeout);

	const char* key = "test_key";

	// test redis STRING command
	// bind redis_string command with redis connection
	acl::redis_string cmd_string(&conn);
	test_redis_string(cmd_string, key);

	// test redis KEY command with the same redis connection
	acl::redis_key cmd_key(&conn);
	test_redis_key(cmd_key, key);
}
```
### redis client cluster example for redis3.0
```c++
int main(void)
{
	// init socket module for windows
	acl::acl_cpp_init();

	const char* redis_addr = "127.0.0.1:6379";
	int conn_timeout = 10, rw_timeout = 10, max_conns = 100;

	// declare redis cluster ojbect
	acl::redis_client_cluster cluster;
	cluster.set(redis_addr, max_conns, conn_timeout, rw_timeout);

	// redis operation command
	acl::redis_string cmd_string;
	acl::redis_key cmd_key;

	// bind redis command with redis cluster
	cmd_string.set_cluster(&cluster, max_conns);
	cmd_key.set_cluster(&cluster, max_conns);

	const char* key = "test_key";

	// call redis server
	test_redis_string(cmd_string, key);
	test_redis_key(cmd_key, key);
}
```
The redis cluster support caching the redis hash-slot in client for performance, and can dynamic add redis server nodes in running.


### another way to use acl redis easily
The acl::redis class inherits from all the other acl redis command class, which includes all the redis client commands. So you can use the acl::redis class just as you can do in all the redis-client-commands class.

```c++
#include <stdlib.h>
#include <stdio.h>
#include "acl_cpp/lib_acl.hpp"

static void test_redis_string(acl::redis& cmd, const char* key)
{
	acl::string val("test_value");

	// call redis-server: SET key value
	if (cmd.set(key, val.c_str()) == false)
	{
		printf("redis set error\r\n");
		return;
	}

	// clear the string buf space
	val.clear();

	// reset the redis command object for reusing it
	cmd.clear();

	// call redis-server: GET key
	if (cmd.get(key, val) == false)
		printf("get key error\r\n");
}

static void test_redis_key(acl::redis& cmd, const char* key)
{
	if (cmd.exists(key) == false)
		printf("key not exists\r\n");
	else
		printf("key exists\r\n");
}

int main(void)
{
	// init socket module for windows
	acl::acl_cpp_init();

	const char* redis_addr = "127.0.0.1:6379";
	int conn_timeout = 10, rw_timeout = 10, max_conns = 100;

	// declare redis cluster ojbect
	acl::redis_client_cluster cluster;
	cluster.set(redis_addr, max_conns, conn_timeout, rw_timeout);

	// redis operation command
	acl::redis cmd;

	// bind redis command with redis cluster
	cmd.set_cluster(&cluster, max_conns);

	const char* key = "test_key";

	// call redis server
	test_redis_string(cmd, key);
	test_redis_key(cmd, key);
}
```

### redis client cluster running in multi-threads
```c++
static int __max_conns = 100;

static void* thread_main(void* arg)
{
	acl::redis_client_cluster* cluster = (acl::redis_client_cluster*) arg;

	acl::redis cmd;
	cmd.set_cluster(cluster, __max_conns);

	const char* key = "test_key";

	for (int i = 0; i < 100000; i++)
	{
		test_redis_string(cmd, key);
		test_redis_key(cmd, key);
	}

	return NULL;
}

int main(void)
{
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

### add acl redis to your projects
Before you use the acl redis, you should compile the three base libraries which redis depending on. Enter the *lib_acl*, *lib_protocol*, *lib_acl_cpp*, and build the `lib_acl.a`, `lib_protocol.a` and `lib_acl_cpp.a`.
```compile
$cd lib_acl; make
$cd lib_protocol; make
$cd lib_acl_cpp; make
```

#### On UNIX/LINUX
In your Makefile, you should add below compiling flags:
-DLINUX2 for LINUX, -DFREEBSD for FreeBSD, -DMACOSX for MAXOS, -DSUNOS5 for Solaris X86;
-I path specify the lib_acl.hpp's parent path, for exmaple: -I./lib_acl_cpp/include, in the lib_acl_cpp/include path the acl_cpp path should be included;
At last, link with -L{path_to_acl_cpp} -l_acl_cpp -L{path_to_protocol} -l_protocol -L{path_to_acl) -l_acl
Of couse you can look at the Makefile.in in lib_acl_cpp\samples and Makfile in lib_acl_cpp\samples\redis\ to find the build conditions.
One Makefile as below:
```Makefile
CFLAGS = -c -g -W -O3 -Wall -Werror -Wshadow \
-Wno-long-long -Wpointer-arith -D_REENTRANT \
-D_POSIX_PTHREAD_SEMANTICS -DLINUX2 \
-I ./lib_acl_cpp/include
BASE_PATH=./acl
LDFLAGS = -L$(BASE_PATH)/lib_acl_cpp/lib -l_acl_cpp \
	-L$(BASE_PATH)/lib_protocol/lib -l_protocol \
	-L$(BASE_PATH)/lib_acl/lib -l_acl \
	-lpthread
test: main.o
	g++ -o main.o $(LDFLAGS)
main.o: main.cpp
	g++ $(CFLAGS) main.cpp -o main.o
```
### On WIN32
Open acl_cpp_vc2003.sln/acl_cpp_vc2008.sln/acl_cpp_vc2010.sln/acl_cpp_vc2012.sln, and look at at the redis samples project option setting.

## reference
- redis include in acl: [redis include files](../../include/acl_cpp/redis/)
- redis src in acl: [redis source files](../../src/redis/)

## Authors
the acl redis lib was written by zsx, the lib is just one part of acl project which includes lib_acl(base C lib), lib_protocol(http/icmp/smtp C libs) and lib_acl_cpp(a wrapper of lib_acl and lib_protocol witch C++, including one more other useful libs).
