# acl disque -- one C++ disque lib based on acl

## descrption
The acl disque is one C++ disque lib, which is one part of acl project. There're disque commands included in the acl disque. The acl disque provides all functions for each disque command including ADDJOB, GETJOB, QLEN, QPEEK, SHOW, ACKJOB, FASTACK, INFO, HELLO, ENQUEUE, DEQUEUE, DELJOB.
The header files of acl disque are in lib_acl_cpp\include\acl_cpp\disque; the source code files are in lib_acl_cpp\src\disque; and the disque samples are in lib_acl_cpp\samples\disque.

## compile
Because acl disque lib is a part of lib_acl_cpp lib, and lib_acl_cpp depend lib_acl and lib_protocol, you should compile lib_acl and lib_protocol libs first, and compile lib_acl_cpp lib. After you've compiled lib_acl_cpp lib, the disque lib is also compiled OK.

### compile on UNIX/LINUX
- 1 compile lib_acl.a: Enter into lib_acl path and type make, the lib_acl.a will be compiled
- 2 compile lib_protocol.a: Enter into lib_protocol path and type make, the lib_protocol.a will be compiled
- 3 compile lib_acl_cpp.a: Enter into lib_acl_cpp path and type make, the lib_acl_cpp.a will be compiled
- 4 compile disque samples: Enter into lib_acl_cpp\samples\disque and type make, all the disque samples(including disque_client, disque_pool, disque_manager) will be compiled.

### compile on WINDOWS
You can use VC2003, VC2008, VC2010, VC2012 to build all acl libs including acl disque lib in lib_acl_cpp module when you open the acl projects(acl_cpp_vc2003.sln, acl_cpp_vc2008.sln, acl_cpp_vc2010.sln, acl_cpp_vc2012.sln). You should build lib_acl first, and second build lib_protocol, and third build lib_acl_cpp, and at last build all the acl samples including disque samples.

## write some samples using acl disque lib
### simple example for disque:

```c++
#include <stdlib.h>
#include <stdio.h>
#include "acl_cpp/lib_acl.hpp"

static void test_disque(acl::disque& cmd)
{
	const char* queue = "greeting";
	const char* job = "job";
	int timeout = 100;
	acl::disque_cond cond;

	cond.set_delay(1);
	cond.set_replicate(2);

	// call disque-server with ADDJOB
	const char* jobid = cmd.addjob(queue, job, timeout, &cond);
	if (jobid == NULL)
	{
		printf("disque addjob error\r\n");
		return;
	}

	// reset the disque command object for reusing it
	cmd.clear();

	// call disque-server with GETJOB
	std::vector<acl::string> queues;
	queues.push_back(queue);

	size_t count = 100;

	const std::vector<acl::disque_job*>* jobs = cmd.getjob(queues, timeout, count);
	if (jobs == NULL)
	{
		printf("get jobs error\r\n");
		return;
	}

	std::vector<acl::string> job_ids;
	std::vector<acl::disque_job*>::const_iterator cit;

	for (cit = jobs->begin(); cit != jobs->end(); ++cit)
	{
		jobid = (*cit)->get_id();
		if (*jobid)
			job_ids.push_back(jobid);
	}

	// reset the disque command object for reusing it
	cmd.clear();

	if (!job_ids.empty())
	{
		// call disque-server with ACKJOB
		int ret = cmd.ackjob(job_ids);
		if (ret < 0)
			printf("ackjob error: %s\r\n", cmd.result_error());
		else
			printf("ackjob ok, ret: %d\r\n", ret);
	}
}

int main()
{
	// init socket module for windows
	acl::acl_cpp_init();

	const char* disque_addr = "127.0.0.1:7711";
	int conn_timeout = 10, rw_timeout = 10;

	// the disque client connection
	acl::disque_client conn(disque_addr, conn_timeout, rw_timeout);

	// bind disque command with disque connection
	acl::disque cmd(&conn);
	test_disque(cmd);
}
```
### disque client cluster pool running in multi-threads
```c++
static int __max_conns = 100;

static void* thread_main(void* arg)
{
	acl::disque_client_cluster* cluster = (acl::disque_client_cluster*) arg;

	acl::disque cmd;
	cmd.set_cluster(cluster, __max_conns);

	for (int i = 0; i < 100000; i++)
		test_disque(cmd);

	return NULL;
}

int main(void)
{
	// init socket module for windows
	acl::acl_cpp_init();

	int conn_timeout = 10, rw_timeout = 10;

	// declare disque cluster ojbect
	acl::disque_client_cluster cluster(conn_timeout, rw_timeout);
	cluster.set("127.0.0.1:7711", __max_conns);
	cluster.set("127.0.0.1:7712", __max_conns);
	cluster.set("127.0.0.1:7713", __max_conns);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	
	// create first thread
	pthread_t id1;
	pthread_create(&id1, &attr, thread_main, &cluster);

	// create second thread
	pthread_t id2;
	pthread_create(&id2, &attr, thread_main, &cluster);

	pthread_join(&id1, NULL);
	pthread_join(&id2, NULL);

	return 0;
}
```

### add acl disque to your projects
Before you use the acl disque, you should compile the three base libraries which disque depending on. Enter the `lib_acl`, `lib_protocol`, `lib_acl_cpp`, and build the `lib_acl.a`, `lib_protocol.a` and `lib_acl_cpp.a`.
```compile
$cd lib_acl; make
$cd lib_protocol; make
$cd lib_acl_cpp; make
```

#### On UNIX/LINUX
In your Makefile, you should add below compiling flags:
-DLINUX2 for LINUX, -DFREEBSD for FreeBSD, -DMACOSX for MAXOS, -DSUNOS5 for Solaris X86;
-I path specify the lib_acl.hpp's parent path, for exmaple: -I./lib_acl_cpp/include, in the lib_acl_cpp/include path the acl_cpp path should be included;
At last, link with `-L{path_to_acl_cpp} -l_acl_cpp -L{path_to_protocol} -l_protocol -L{path_to_acl) -l_acl`
Of couse you can look at the Makefile.in in lib_acl_cpp\samples and Makfile in lib_acl_cpp\samples\disque\ to find the build conditions.
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
	gcc -o main.o $(LDFLAGS)
main.o: main.cpp
	gcc $(CFLAGS) main.cpp -o main.o
```
### On WIN32
Open acl_cpp_vc2003.sln/acl_cpp_vc2008.sln/acl_cpp_vc2010.sln/acl_cpp_vc2012.sln, and look at at the disque samples project option setting.

## reference
- disque include in acl: [disque include files](../../include/acl_cpp/disque/)
- disque src in acl: [disque source files](../../src/disque/)

## Authors
the acl disque lib was written by zsx, the lib is just one part of acl project which includes lib_acl(base C lib), lib_protocol(http/icmp/smtp C libs) and lib_acl_cpp(a wrapper of lib_acl and lib_protocol witch C++, including one more other useful libs).
