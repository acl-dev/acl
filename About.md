# 1. About Acl project
The Acl (Advanced C/C++ Library) project is powerful multi-platform network communication library and service framework, suppoting LINUX, WIN32, Solaris, FreeBSD, AndroidOS, iOS. Many applications written by Acl run on these devices with Linux, Windows, iPhone and Android and serve billions of users. There are some important modules in Acl project, including network communcation, server framework, application protocols, multiple coders, etc. The common protocols such as HTTP/SMTP/ICMP//MQTT/Redis/Memcached/Beanstalk/Handler Socket are implemented in Acl, and the codec library such as XML/JSON/MIME/BASE64/UUCODE/QPCODE/RFC2047/RFC1035, etc., are also included in Acl. Acl also provides unified abstract interface for popular databases such as Mysql, Postgresql, Sqlite. Using Acl library users can write database application more easily, quickly and safely.

Architecture diagram:
 ![Overall architecture diagram](res/img/architecture_en.png)

# 2. The six most important modules
As a C/C++ foundation library, Acl provides many useful functions for users to develop applications, including six important modules: HTTP, MIME, Redis client, Database interface, Coroutine, and Server framework.

## 2.1. Coroutine
The coroutine module in Acl can be used on multiple platforms, and there are many engineering practices in some important projects.
- Run on Linux, MacOS, Windows, iOS and Android
- Support x86, Arm architecture
- Support select/poll/epoll/kqueue/iocp/win32 GUI message
- The DNS protocol has been implemented in acl coroutine, so DNS API can also be used in coroutine model
- Hook system IO API on Unix and Windows
  - Read API: read/readv/recv/recvfrom/recvmsg
  - Write API: write/writev/send/sendto/sendmsg/sendfile64
  - Socket API: socket/listen/accept/connect/setsockopt
  - event API: select/poll/epoll_create/epoll_ctl/epoll_wait
  - DNS API: gethostbyname/gethostbyname_r/getaddrinfo/freeaddrinfo
- Support shared stack mode to minimize memory usage
- Synchronization primitive
  - Coroutine mutex, semphore can be used between coroutines
  - Coroutine event can be used between coroutines and threads
- More information see [Using acl fiber](lib_fiber/README_en.md)

## 2.2. HTTP module
Supports HTTP/1.1, can be used in client and server sides.
- HttpServlet interface like Java(server side)
- Connection pool mode(client side)
- Chunked block transfer
- Gzip compression/decompression
- SSL encrypted
- Breakpoints transmission
- Setting/acquisition of cookies
- Session managment(server side)
- Websocket transmission
- HTTP MIME format
- Sync/Async mode
- ...

## 2.3. Redis client
The redis client module in Acl is powerful, high-performance and easy to use.
- Support Bitmap/String/Hash/List/Set/Sorted Set/PubSub/HyperLogLog/Geo/Script/Stream/Server/Cluster/etc.
- Provides stl-like C++ interface for each redis command
- Communication in single, cluster or pipeline mode
- Connection pool be used in signle or cluster mode
- More information see [Using acl redis client](lib_acl_cpp/samples/redis/README.md)

## 2.4. MQTT module

## 2.5. Database module
The unified database interface in Acl is designed to easily and safely operate thease well-known open source databases such as Mysql, Postgresq and SQLite. The SQL codec is designed to escape the charactors to avoid the DB SQL attacks. When users use Acl to write database applications, Acl will dynamically load the dynamic libraries of Mysql, Postgresql or SQLite. The advantage of dynamic loading is that users who don't need the database functionality don't care about it at all.

## 2.6. Server framework
The most important module in Acl is the server framework, which helps users quickly write back-end services, such as web services. Tools in app/wizard can help users generate one appropriate service code within several seconds. The server framework of Acl includes two parts, one is the services manager, the other is the service written by Acl service template. The services manager named acl_master in Acl comes from the famous Postfix, whose name is master, and acl_master has many extensions to master in order to be as one general services manager. There are six service templates in Acl that can be used to write application services, as below:
- **Process service:** One connection one process, the advantage is that the programming is simple, safe and stable, and the disadvantage is that the concurrency is too low;
- **Threads service:** Each process handles all client connections through a set of threads in the thread pool. The IO event trigger mode is used that a connection is bound to a thread only if it has readable data, and the thread will be released after processing the data. The service model's advantage is that it can handle a large number of client connections in one process with a small number of threads. Compare with the aio model, the programming is relatively simple;
- **Aio service:** Similar to nginx/squid/ircd, one thread can handle a large number of client connections in a non-blocking IO manner. The advantages of this model are high processing efficiency and low resource consumption, while the disadvantages are more complex programming;
- **Fiber service:** Although the non-blocking service model can obtain large concurrent processing capability, the programming complexity is high. The coroutine model combines the features of large concurrent processing and low programming complexity, enabling programmers to easily implement sequential IO programming;
- **UDP service:** The model is mainly a service model supporting UDP communication process;
- **Trigger service:** The model instance is mainly used to process the background service process of some scheduled tasks (similar to the system's crontab).

# 3. The other basic modules

## 3.1. MIME module
MIME(Multipurpose Internet Mail Extensions) format is widely used in email application. MIME format is too important that it can be used not only for email applications, but also for Web applications. MIME RFC such as RFC2045/RFC2047/RFC822 has been implemented in Acl. Acl has a MIME data stream parser that is indepedent of the IO model, so it can be used by synchronous or asynchronous IO programs.
