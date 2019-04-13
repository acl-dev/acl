# Acl 网络协程框架编程指南
<!-- vim-markdown-toc GFM -->

* [摘要](#摘要)
* [一、概述](#一概述)
* [二、简单示例](#二简单示例)
* [三、编译安装](#三编译安装)
* [四、使用多核](#四使用多核)
* [五、多核同步](#五多核同步)
* [六、消息传递](#六消息传递)
* [七、HOOK API](#七HOOK-API)
* [八、域名解析](#八域名解析)
* [九、使第三方网络库协程化](#九使第三方网络库协程化)
* [十、Windows界面编程协程化](#十Windows界面编程协程化)

<!-- vim-markdown-toc -->

## 摘要
本文主要讲述Acl网络协程框架的使用，从协程的应用场景出发，以一个简单的协程示例开始，然后逐步深入到Acl网络协程的各个使用场景及使用细节，以及需要避免的“坑”，希望能给大家带来实践上的帮助。
## 一、概述
讲到协程，大家必然会提到 Go 语言，毕竟是 Go 语言把协程的概念及使用实践普及的；但协程并不是一个新概念，我印象中在九十年代就出现了，当时一位同事还说微软推出了纤程（基本一个意思），可以创建成午上万个纤程，不必象线程那样只能创建较少的线程数量，但当时也没明白创建这么多纤程有啥用，只不过是一个上下文的快速切换协同而已。所以自己在写网络高并发服务时，主要还是以非阻塞方式来实现。
其实，Go 语言的作者之一 Russ Cox 早在 2002 年左右就用 C 实现了一个简单的基于协程的网络通信模型 -- libtask，但其只是一个简单的网络协程原型，还远达不到实践的要求。自从 Go 语言兴起后，很多基于 C/C++ 开发的协程库也多了起来，其中 Acl 协程库便是其中之一。
Acl 工程地址：https://github.com/acl-dev/acl
Acl 协程地址：https://github.com/acl-dev/acl/tree/master/lib_fiber

## 二、简单示例
下面为一个使用 Acl 库编写的简单线程的例子：    

```c++
#include <acl-lib/acl_cpp/lib_acl.hpp>

class mythread : public acl::thread
{
public:
	mythread(void) {}
	~mythread(void) {}
private:
	// 实现基类中纯虚方法，当线程启动时该方法将被回调
	// @override
	void* run(void) {
		for (int i = 0; i < 10; i++) {
			printf("thread-%lu: running ...\r\n", acl::thread::self());
		}
		return NULL;
	}
};

int main(void)
{
	std::vector<acl::thread*> threads;
	for (int i = 0; i < 10; i++) {
		acl::thread* thr = new mythread;
		threads.push_back(thr);
		thr->start();  // 启动线程
	}
	
	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		(*it)->wait();  // 等待线程退出
		delete *it;
	}
	return 0;	
}
```

上面线程例子非常简单，接着再给一个简单的协程的例子：    

```c++
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include <acl-lib/fiber/libfiber.hpp>

class myfiber : public acl::fiber
{
public:
	myfiber(void) {}
	~myfiber(void) {}
private:
	// 重现基类纯虚方法，当调用 fiber::start 时，该方法将被调用
	// @override
	void run(void) {
		for (int i = 0; i < 10; i++) {
			printf("hello world! the fiber is %d\r\n", acl::fiber::self());
			acl::fiber::yield();  // 让出CPU运行权给其它协程
		}
	}
};

int main(void)
{
	std::vector<acl::fiber*> fibers;
	for (int i = 0; i < 10; i++) {
		acl::fiber* fb = new myfiber;
		fibers.push_back(fb);
		fb->start();  // 启动一个协程
	}
	
	acl::fiber::schedule();  // 启用协程调度器
	
	for (std::vector<acl::fiber*>::iterator it = fibers.begin();
		it != fibers.end(); ++it) {
		delete *it;
	}
}
```

上面示例演示了协程的创建、启动及运行的过程，与前一个线程的例子非常相似，（简单实用是 Acl 库的目标之一）。
**协程调度其实是应用层面多个协程之间通过上下文切换形成的协作过程，如果一个协程库仅是实现了上下文切换，其实并不具备太多实用价值，当与网络事件绑定后，其价值才会显现出来**。下面一个简单的使用协程的网络服务程序：    

```c++
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include <acl-lib/fiber/libfiber.hpp>

// 客户端协程处理类，用来回显客户发送的内容，每一个客户端连接绑定一个独立的协程
class fiber_echo : public acl::fiber
{
public:
	fiber_echo(acl::socket_stream* conn) : conn_(conn) {}
private:
	acl::socket_stream* conn_;
	~fiber_echo(void) { delete conn_; }
	// @override
	void run(void) {
		char buf[8192];
		while (true) {
			// 从客户端读取数据（第三个参数为false表示不必填满整个缓冲区才返回）
			int ret = conn_->read(buf, sizeof(buf), false);
			if (ret == -1) {
				break;
			}
			// 向客户端写入读到的数据
			if (conn_->write(buf, ret) != ret) {
				break;
			}
		}
		delete this; // 自销毁动态创建的协程对象
	}
};

// 独立的协程过程，接收客户端连接，并将接收的连接与新创建的协程进行绑定
class fiber_listen : public acl::fiber
{
public:
	fiber_listen(acl::server_socket& listener) : listener_(listener) {}
private:
	acl::server_socket& listener_;
	~fiber_listen(void) {}
	// @override
	void run(void) {
		while (true) {
			acl::socket_stream* conn = listener_.accept();  // 等待客户端连接
			if (conn == NULL) {
				printf("accept failed: %s\r\n", acl::last_serror());
				break;
			}
			// 创建并启动单独的协程处理客户端连接
			acl::fiber* fb = new fiber_echo(conn);
			// 启动独立的客户端处理协程
			fb->start();
		}
		delete this;
	}
};

int main(void)
{
	const char* addr = "127.0.0.1:8800";
	acl::server_socket listener;
	// 监听本地地址
	if (listener.open(addr) == false) {
		printf("listen %s error %s\r\n", addr, acl::last_serror());
		return 1;
	}

	// 创建并启动独立的监听协程，接受客户端连接
	acl::fiber* fb = new fiber_listen(listener);
	fb->start();

	// 启动协程调度器
	acl::fiber::schedule();
	return 0;
}
```

这是一个简单的支持回显功能的网络协程服务器，可以很容易修改成线程模式。使用线程或线程处理网络通信都可以采用**顺序思维**模式，不必象非阻塞网络编程那样复杂，但使用协程的最大好处可以创建大量的协程来处理网络连接，而要创建大量的线程显示是不现实的（线程数非常多时，会导致操作系统的调度能力下降）。**如果你的网络服务应用不需要支持大并发，使用协程的意义就没那么大了**。
## 三、编译安装
在编译前，需要先从 github  https://github.com/acl-dev/acl  下载源码，国内用户可以选择从 gitee  https://gitee.com/acl-dev/acl  下载源码。
### 3.1、Linux/Unix 平台上编译安装
在 Linux/Unix 平台上的编译非常简单，可以选择使用 make 方式或 cmake 方式进行编译。  
- **make 方式编译：**
在 acl 项目根目录下运行：**make && make packinstall**，则会自动执行编译与安装过程，安装目录默认为系统目录：libacl_all.a, libfiber_cpp.a, libfiber.a 将被拷贝至 /usr/lib/ 目录，头文件将被拷贝至 /usr/include/acl-lib/ 目录。
- **cmake 方式编译：**
在 acl 项目根目录下创建 build 目录，然后：**cd build && cmake .. && make**
- **将 acl 库加入至你的工程**（以 make 方式为例）
先在代码中加入头文件包含项：    

```c++
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include <acl-lib/fiber/libfiber.hpp>
```

然后修改你的 Makefile 文件，示例如下：    

```Makefile
mytest: mytest.cpp
	g++ -o mytest mytest.cpp -lfiber_cpp -lacl_all -lfiber -ldl -lpthread -lz
```

**注意在 Makefile 中各个库的依赖顺序：** libfiber_cpp.a 依赖于 libacl_all.a 和 libfiber.a，其中 libacl_all.a 为 acl 的基础库，libfiber.a 为 C 语言协程库（其不依赖于 libacl_all.a），libfiber_cpp.a 用 C++ 语言封装了 libfiber.a，且使用了 libacl_all.a 中的一些功能。 
### 3.2、Windows 平台上编译
在 Windows 平台的编译也非常简单，可以用 vc2008/2010/2012/2013/2015/2017 打开相应的工程文件进行编译，如：可以用 vc2012 打开 acl_cpp_vc2012.sln 工程进行编译。
### 3.3 Mac 平台上编译
除可以使用 Unix 统一方式（命令行方式）编译外，还可以用 Xcode 打开工程文件进行编译。
### 3.4 Android 平台上编译
目前可以使用 Android Studio3.x 打开 acl\android\acl 目录下的工程文件进行编译。
### 3.5 使用 MinGW 编译
如果想要在 Windows 平台上编译 Unix 平台上的软件，可以借用 MinGW 套件进行编译，为此 Acl 库还提供了此种编译方式，但一般不建议用户使用这种编译方式，一方面是执行效率低，另一方面可能会存在某些不兼容问题。
### 3.6 小结
为了保证 Acl 工程无障碍使用，本人在编译 Acl 库方面下了很大功夫，支持几乎在所有平台上使用原生编译环境进行编译使用，真正达到了一键编译。甚至为了避免因依赖第三方库而导致的编译问题（如：有的模块需要 zlib 库，有的需要 polassl 库，有的需要 mysql/postgresql/sqlite 库），将这些依赖第三方库的模块都写成动态加载第三方库的方式，毕竟不是所有人都需要这些第三方库所提供的功能。
## 四、使用多核
Acl 协程的调度过程是基于单CPU的（虽然也可以修改成多核调度，但考虑到很多原因，最终还是采用了单核调度模式），即创建一个线程，所创建的所有协程都在这个线程空间中运行。为了使用多核，充分使用CPU资源，可以创建多个线程（也可以创建多个进程），每个线程为一个独立的协程运行容器，各个线程之间的协程相互隔离，互不影响。
下面先修改一下上面的例子，改成多线程的协程方式：    


```c++
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include <acl-lib/fiber/libfiber.hpp>

// 客户端协程处理类，用来回显客户发送的内容，每一个客户端连接绑定一个独立的协程
class fiber_echo : public acl::fiber
{
public:
	fiber_echo(acl::socket_stream* conn) : conn_(conn) {}
private:
	acl::socket_stream* conn_;
	~fiber_echo(void) { delete conn_; }
	// @override
	void run(void) {
		char buf[8192];
		while (true) {
			int ret = conn_->read(buf, sizeof(buf), false);
			if (ret == -1) {
				break;
			}
			if (conn_->write(buf, ret) != ret) {
				break;
			}
		}
		delete this; // 自销毁动态创建的协程对象
	}
};

// 独立的协程过程，接收客户端连接，并将接收的连接与新创建的协程进行绑定
class fiber_listen : public acl::fiber
{
public:
	fiber_listen(acl::server_socket& listener) : listener_(listener) {}
private:
	acl::server_socket& listener_;
	~fiber_listen(void) {}
	// @override
	void run(void) {
		while (true) {
			acl::socket_stream* conn = listener_.accept();  // 等待客户端连接
			if (conn == NULL) {
				printf("accept failed: %s\r\n", acl::last_serror());
				break;
			}
			// 创建并启动单独的协程处理客户端连接
			acl::fiber* fb = new fiber_echo(conn);
			fb->start();
		}
		delete this;
	}
};

// 独立的线程调度类
class thread_server : public acl::thread
{
public:
	thread_server(acl::server_socket& listener) : listener_(listener) {}
	~thread_server(void) {}
private:
	acl::server_socket& listener_;
	// @override
	void* run(void) {
		// 创建并启动独立的监听协程，接受客户端连接
		acl::fiber* fb = new fiber_listen(listener);
		fb->start();
		// 启动协程调度器
		acl::fiber::schedule(); // 内部处于死循环过程
		return NULL;
	}
};

int main(void)
{
	const char* addr = "127.0.0.1:8800";
	acl::server_socket listener;
	// 监听本地地址
	if (listener.open(addr) == false) {
		printf("listen %s error %s\r\n", addr, acl::last_serror());
		return 1;
	}

	std::vector<acl::thread*> threads;
	// 创建多个独立的线程对象，每个线程启用独立的协程调度过程
	for (int i = 0; i < 4; i++) {
		acl::thread* thr = thread_server(listener);
		threads.push_back(thr);
		thr->start();
	}
	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		(*it)->wait();
		delete *it;
	}
	return 0;
}
```

经过修改，上面的例子即可以支持大并发，又可以使用多核。
## 五、多核同步
上面的例子中涉及到了通过创建多线程使用多核的过程，但肯定会有人问，在多个线程中的协程之间如果想要共享某个资源怎么办？Acl 协程库提供了可以跨线程使用同步原语：线程协程事件同步及条件变量。
首先介绍一下事件同步对象类：acl::fiber_event，该类提供了三个方法：    

```C++
class fiber_event {
	...

	/**
	 * 等待事件锁
	 * @return {bool} 返回 true 表示加锁成功，否则表示内部出错
	 */
	bool wait(void);

	/**
	 * 尝试等待事件锁
	 * @return {bool} 返回 true 表示加锁成功，否则表示锁正在被占用
	 */
	bool trywait(void);

	/**
	 * 事件锁拥有者释放事件锁并通知等待者
	 * @return {bool} 返回 true 表示通知成功，否则表示内部出错
	 */
	bool notify(void);
	...
};
```

下面给出一个例子，看看在多个线程中的协程之间如何进行互斥的：    

```c++
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include <acl-lib/fiber/libfiber.hpp>

class myfiber : public acl::fiber
{
public:
	myfiber(acl::fiber_event& lock, int& count): lock_(lock), count_(count) {}
private:
	~myfiber(void) {}
	// @override
	void run(void) {
		for (int i = 0; i < 100; i++) {
			lock_.wait();
			count_++;
			lock_.notify();
			//acl::fiber::delay(1);  // 本协程休息1毫秒
		}
		delete this;
	}
private:
	acl::fiber_event& lock_;
	int& count_;
};

class mythread : public acl::thread
{
public:
	mythread(acl::fiber_event& lock, int& count): lock_(lock), count_(count) {}
	~mythread(void) {}
private:
	// @override
	void* run(void) {
		for (int i = 0; i < 100; i++) {
			acl::fiber* fb = new myfiber(lock_, count_);
			fb->start();
		}
		acl::fiber::schedule();
		return NULL;
	}
private:
	acl::fiber_event& lock_;
	int& count_;
};

int main(void)
{
	acl::fiber_event lock;  // 可以用在多个线程之间、各个线程中的协程之间的同步过程
	int count = 0;
	std::vector<acl::thread*> threads;
	for (int i = 0; i < 4; i++) {
		acl::thread* thr = new mythread(lock, count);
		threads.push_back(thr);
		thr->start();
	}
	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		(*it)->wait();
		delete *it;
	}

	printf("all over, count=%d\r\n", count);
	return 1;
}
```

acl::fiber_event 常被用在多个线程中的协程之间的同步，当然也可以用在多个线程之间的同步，这在很大程度弥补了 Acl 协程框架在使用多核上的不足。
## 六、消息传递
通过组合 acl::fiber_event（协程事件）和 acl::fiber_cond（协程条件变量），实现了协程间进行消息传递的功能模块：acl::fiber_tbox，fiber_tbox 不仅可以用在同一线程内的协程之间传递消息，而且还可以用在不同线程中的协程之间，不同线程之间，线程与协程之间传递消息。fiber_tbox 为模板类，因而可以传递各种类型对象。以下给出一个示例：     

```c++
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include <acl-lib/fiber/libfiber.hpp>

class myobj
{
public:
	myobj(void) : i_(0) {}
	~myobj(void) {}
	void set(int i) {
		i_ = i;
	}
	void test(void) {
		printf("hello world, i=%d\r\n", i_);
	}
private:
	int i_;
};

// 消费者协程，从消息管道中读取消息
class fiber_consumer : public acl::fiber
{
public:
	fiber_consumer(acl::fiber_tbox<myobj>& box) : box_(box) {}
private:
	~fiber_consumer(void) {}
private:
	acl::fiber_tbox<myobj>& box_;
	// @override
	void run(void) {
		while (true) {
			myobj* o = box_.pop();
			// 如果读到空消息，则结束
			if (o == NULL) {
				break;
			}
			o->test();
			delete o;
		}
		delete this;
	}
};

// 生产者协程，向消息管道中放置消息
class fiber_producer : public acl::fiber
{
public:
	fiber_producer(acl::fiber_tbox<myobj>& box) : box_(box) {}
private:
	~fiber_producer(void) {}
private:
	acl::fiber_tbox<myobj>& box_;
	// @override
	void run(void) {
		for (int i = 0; i < 10; i++) {
			myobj* o = new myobj;
			o->set(i);
			// 向消息管道中放置消息
			box_.push(o);
		}
		// 放置空消息至消息管道中，从而通知消费者协程结束
		box_.push(NULL);
		delete this;
	}
};

int main(void)
{
	acl::fiber_tbox<myobj> box;
	// 创建并启动消费者协程
	acl::fiber* consumer = new fiber_consumer(box);
	consumer->start();
	// 创建并启动生产者协程
	acl::fiber* producer = new fiber_producer(box);
	producer->start();
	// 启动协程调度器
	acl::fiber::schedule();
	return 0; 
}
```

上面例子展示了同一线程中的两个协程之间的消息传递过程，因为 acl::fiber_tbox 是可以跨线程的，所以它的更大价值是用在多个线程中的不同协程之间进行消息传递。    

```c++
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include <acl-lib/fiber/libfiber.hpp>

class myobj
{
public:
	myobj(void) : i_(0) {}
	~myobj(void) {}
	void set(int i) {
		i_ = i;
	}
	void test(void) {
		printf("hello world, i=%d\r\n", i_);
	}
private:
	int i_;
};

// 消费者协程，从消息管道中读取消息
class fiber_consumer : public acl::fiber
{
public:
	fiber_consumer(acl::fiber_tbox<myobj>& box) : box_(box) {}
private:
	~fiber_consumer(void) {}
private:
	acl::fiber_tbox<myobj>& box_;
	// @override
	void run(void) {
		while (true) {
			myobj* o = box_.pop();
			// 如果读到空消息，则结束
			if (o == NULL) {
				break;
			}
			o->test();
			delete o;
		}
		delete this;
	}
};

// 生产者线程，向消息管道中放置消息
class thread_producer : public acl::thread
{
public:
	thread_producer(acl::fiber_tbox<myobj>& box) : box_(box) {}
	~thread_producer(void) {}
private:
	acl::fiber_tbox<myobj>& box_;
	void* run(void) {
		for (int i = 0; i < 10; i++) {
			myobj* o = new myobj;
			o->set(i);
			box_.push(o);
		}
		box_.push(NULL);
		return NULL;
	}
};

int main(void)
{
	acl::fiber_tbox<myobj> box;
	// 创建并启动消费者协程
	acl::fiber* consumer = new fiber_consumer(box);
	consumer->start();
	// 创建并启动生产者线程
	acl::thread* producer = new thread_producer(box);
	producer->start();
	// 启动协程调度器
	acl::fiber::schedule();
	// schedule() 过程返回后，表示该协程调度器结束。
	// 等待生产者线程退出
	producer->wait();
	delete producer;
	return 0; 
}
```

在该示例中，生产者为一个独立的线程，消费者为另一个线程中的协程，二者通过 acl::fiber_tbox 进行消息通信。但**有一点需要注意**，fiber_tbox 一般可用在“单生产者-单消费者或多生产者-单消费者”的应用场景中，不应用在多消费者的场景中，虽然用在多个消费者场景时不会造成消费丢失或内存崩溃，但当消费者数量较多时却有可能出现惊群现象，所以应避免将一个 acl::fiber_tbox 用在大量的多消费者场景中。
下面再给一个应用场景的例子，也是我们平时经常会遇到的。    

```c++
#include <unistd.h>
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include <acl-lib/fiber/libfiber.hpp>

class mythread : public acl::thread
{
public:
	mythread(acl::fiber_tbox<int>& box) :box_(box) {}
	~mythread(void) {}
private:
	acl::fiber_tbox<int>& box_;
	// @override
	void* run(void) {
		int i;
		for (i = 0; i < 5; i++) {
			/* 假设这是一个比较耗时的操作*/
			printf("sleep one second\r\n");
			sleep(1);
		}
		int* n = new int(i);
		// 将计算结果通过消息管道传递给等待者协程
		box_.push(n);
		return NULL; 
	}
};

class myfiber : public acl::fiber
{
public:
	myfiber(void) {}
	~myfiber(void) {}
private:
	// @override
	void run(void) {
		acl::fiber_tbox<int> box;
		mythread thread(box);
		thread.set_detachable(true);
		thread.start();  // 启动独立的线程计算耗时运算
		int* n = box.pop();  // 等待计算线程返回运算结果，仅会阻塞当前协程
		printf("n is %d\r\n", *n);
		delete n;
	}
};

int main(void)
{
	myfiber fb;
	fb.start();
	acl::fiber::schedule();
	return 0;
}
```

协程一般用在网络高并发环境中，但协程并不是万金油，协程并不适合计算密集型应用，因为线程才是操作系统的最小调度单元，而协程不是，所以当遇到一些比较耗时的运算时，为了不阻塞当前协程所在的协程调度器，应将该耗时运算过程中抛给独立的线程去处理，然后通过 acl::fiber_tbox 等待线程的运算结果。
## 七、HOOK API
为了使现有的很多网络应用和网络库在尽量不修改的情况下协程化，Acl 协程库 Hook 了很多与 IO 和网络通信相关的系统 API，目前已经 Hook 的系统 API 有：    

| 内容项 |API|
|--|--|
|网络相关|socket/listen/accept/connect  |
|IO相关|read/readv/recv/recvfrom/recvmsg/write/writev/send/sendto/sendmsg/sendfile64|
|域名相关|gethostbyname(_r)/getaddrinfo/freeaddrinfo|
|事件相关|select/poll/epoll_create/ epoll_ctl/epoll_wait|
|其它|close/sleep|

## 八、域名解析
使用协程方式编写网络通信程序，域名解析是不能绕过的，记得有一个协程库说为了支持域名解析，甚至看了相关实现代码，然后说通过 Hook _poll API 就可以了，实际上这并不是通用的做法，至少在我的环境里通过 Hook _poll API 是没用的，所以最稳妥的做法还是应该将 DNS 查询协议实现了，在 acl 的协程库中，域名解析模块实际是集成了第三方 DNS 库，参见：https://github.com/wahern/dns  ， 毕竟，实现一个较为完整的 DNS 解析库还是比较麻烦的。 

## 九、使第三方网络库协程化
通常网络通信库都是阻塞式的，因为非阻塞式的通信库的通用性不高（使用各自的事件引擎，很难达到应用层的使用一致性），如果把这些第三方通信库（如：mysql 客户端库，Acl 中的 Redis 库）使用协程所提供的 IO 及网络  API 重写一遍则工作量太大，不太现实，好在 Acl 协程库 Hook 了很多系统 API，从而使阻塞式的网络通信库协程化变得简单。所谓网络库协程化就是使这些网络库可以应用在协程环境中，从而可以很容易编写出支持高并发的网络程序。
先写一个将 Acl Redis 客户端库协程化的例子：    

```c++
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include <acl-lib/fiber/libfiber.hpp>

class fiber_redis : public acl::fiber
{
public:
	fiber_redis(acl::redis_client_cluster& cluster) : cluster_(cluster) {}
private:
	~fiber_redis(void) {}
private:
	acl::redis_client_cluster& cluster_;
	// @override
	void run(void) {
		const char* key = "hash-key";
		for (int i = 0; i < 100; i++) {
			acl::redis cmd(&cluster_);
			acl::string name, val;
			name.format("hash-name-%d", i);
			val.format("hash-val-%d", i);
			if (cmd.hset(key, name, val) == -1) {
				printf("hset error: %s, key=%s, name=%s\r\n",
					cmd.result_error(), key, name.c_str());
				break;
			}
		}
		delete this;
	}
};

int main(void)
{
	const char* redis_addr = "127.0.0.1:6379";
	acl::redis_client_cluster cluster;
	cluster.set(redis_addr, 0);
	for (int i = 0; i < 100; i++) {
		acl::fiber* fb = new fiber_redis(cluster);
		fb->start();
	}
	acl::fiber::schedule();
	return 0;
}
```

读者可以尝试将上面的代码拷贝到自己机器上，编译后运行一下。另外，这个例子是只有一个线程，所以会发现 acl::redis_client_cluster 的使用方式和在线程下是一样的。如果将 acl::redis_client_cluster 在多个线程调度器上共享会怎样？还是有一点区别，如下：    

```c++
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include <acl-lib/fiber/libfiber.hpp>

// 每个协程共享相同的 cluster 对象，向 redis-server 中添加数据
class fiber_redis : public acl::fiber
{
public:
	fiber_redis(acl::redis_client_cluster& cluster) : cluster_(cluster) {}
private:
	~fiber_redis(void) {}
private:
	acl::redis_client_cluster& cluster_;
	// @override
	void run(void) {
		const char* key = "hash-key";
		for (int i = 0; i < 100; i++) {
			acl::redis cmd(&cluster_);
			acl::string name, val;
			name.format("hash-name-%d", i);
			val.format("hash-val-%d", i);
			if (cmd.hset(key, name, val) == -1) {
				printf("hset error: %s, key=%s, name=%s\r\n",
					cmd.result_error(), key, name.c_str());
				break;
			}
		}
		delete this;
	}
};

// 每个线程运行一个独立的协程调度器
class mythread : public acl::thread
{
public:
	mythread(acl::redis_client_cluster& cluster) : cluster_(cluster) {}
	~mythread(void) {}
private:
	acl::redis_client_cluster& cluster_;
	// @override
	void* run(void) {
		for (int i = 0; i < 100; i++) {
			acl::fiber* fb = new fiber_redis(cluster_	);
			fb->start();
		}
		acl::fiber::schedule();
		return NULL;
	}
};

int main(void)
{
	const char* redis_addr = "127.0.0.1:6379";
	acl::redis_client_cluster cluster;
	cluster.set(redis_addr, 0);
	cluster.bind_thread(true);

	// 创建多个线程，共享 redis 集群连接池管理对象：cluster，即所有线程中的
	// 所有协程共享同一个 cluster 集群管理对象
	std::vector<acl::thread*> threads;
	for (int i = 0; i < 4; i++) {
		acl::thread* thr = new mythread(cluster);
		threads.push_back(thr);
		thr->start();
	}
	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		(*it)->wait();
		delete *it;
	}
	return 0;
}
```

在这个多线程多协程环境里使用 acl::redis_client_cluster 对象时与前面的一个例子有所不同，在这里调用了：**cluster.bind_thread(true);** 
为何要这样做？原因是 Acl Redis 的协程调度器是单线程工作模式，网络套接字句柄在协程环境里不能跨线程使用，当调用 bind_thread(true) 后，Acl 连接池管理对象会自动给每个线程分配一个连接池对象，每个线程内的所有协程共享这个绑定于本线程的连接池对象。
## 十、Windows界面编程协程化
在Windows下写过界面程序的程序员都经历过使通信模块与界面结合的痛苦过程，因为 Windows 界面过程是基于 win32 消息引擎驱动的，所以在编写通信模块时一般有两个选择：要么使用 Windows 提供的异步非阻塞 API，要么把通信模块放在独立于界面的单独线程中然后通过窗口消息将结果通知窗口界面过程。
Acl 协程库的事件引擎支持 win32 消息引擎，所以很容易将界面过程的通信过程协程化，采用这种方式，一方面程序员依然可以采用顺序编程方式，另一方面通信协程与界面过程运行于相同的线程空间，则二者在相互访问对方的成员对象时不必加锁，从而使编写通信过程变得更加简单。
下面以一个简单的对话框为例说明界面网络通信协程化过程：    
1. 首先使用向导程序生成一个对话框界面程序，需要指定支持 socket 通信；
2. 然后在 OnInitDialog() 方法尾部添加如下代码：    

```c++
	// 设置协程调度的事件引擎，同时将协程调度设为自动启动模式
	acl::fiber::init(acl::FIBER_EVENT_T_WMSG, true);
	// HOOK ACL 库中的网络 IO 过程
	acl::fiber::acl_io_hook();
```

3. 创建一个按钮，并使其绑定一个事件方法，如：OnBnClickedListen，然后在这个方法里添加一些代码：    

```c++
	// 创建一个协程用来监听指定地址，接收客户端连接请求
	m_fiberListen = new CFiberListener("127.0.0.1:8800");
	// 启动监听协程
	m_fiberListen->start();
```

4. 实现步骤 3 中指定的监听协程类    

```c++
class CFiberListener : public acl::fiber
{
public:
	CFiberListener(const char* addr) : m_addr(addr) {}
private:
	~CFiberListener(void) {}
private:
	acl::string m_addr;
	acl::server_socket m_listener;
	// @override
	void run(void) {
		// 绑定并监听指定的本地地址
		if (m_listener.open(m_addr) == false) {
			return;
		}
		while (true) {
			// 等待客户端连接
			acl::socket_stream* conn = m_listener.accept();
			if (conn == NULL) {
				break;
			}
			// 创建独立的协程处理该客户端的请求
			acl::fiber* fb = new CFiberClient(conn);
			fb->start(); // 启动客户端处理协程
		}
		delete this;
	}
};
```

5. 实现步骤 4 中指定的客户端响应协程类    

```c++
class CFiberClient : public acl::fiber
{
public:
	CFiberClient(acl::socket_stream* conn) : m_conn(conn) {}
private:
	~CFiberClient(void) { delete m_conn; }
private:
	acl::socket_stream* m_conn;
	// @override
	void run(void) {
		char buf[8192];
		while (true) {
			// 从客户端读取数据
			int ret = m_conn->read(buf, sizeof(buf), false);
			if (ret == -1) {
				break;
			}
			// 将读到的数据回写给客户端
			if (m_conn->write(buf, ret) != ret) {
				break;
			}
		}
		delete this;
	}
};
```

通过以上步骤就可为 win32 界面程序添加基于协程模式的通信模块，上面的两个协程类的处理过程都是“死循环”的，而且又与界面过程同处同一线程运行空间，却为何却不会阻塞界面消息过程呢？其原因就是当通信协程类对象在遇到网络 IO 阻塞时，会自动将自己挂起，将线程的运行权交给其它协程或界面过程。原理就是这么简单，但内部实现还有点复杂度的，感兴趣的可以看看 Acl 协程库的实现源码(https://github.com/acl-dev/acl/tree/master/lib_fiber/ )。
此外，上面示例的完整代码请参考：https://github.com/acl-dev/acl/tree/master/lib_fiber/samples/WinEchod  。

