[中文简体](README_CN.md)

# Acl -- One Advanced C/C++ Library for Unix/Windows

# 0. About Acl project
The Acl (Advanced C/C++ Library) project is powerful multi-platform network communication library and service framework, suppoting LINUX, WIN32, Solaris, FreeBSD, AndroidOS, iOS. Many applications written by Acl run on these devices with Linux, Windows, iPhone and Android and serve billions of users. There are some important modules in Acl project, including network communcation, server framework, application protocols, multiple coders, etc. The common protocols such as HTTP/SMTP/ICMP//MQTT/Redis/Memcached/Beanstalk/Handler Socket are implemented in Acl, and the codec library such as XML/JSON/MIME/BASE64/UUCODE/QPCODE/RFC2047/RFC1035, etc., are also included in Acl. Acl also provides unified abstract interface for popular databases such as Mysql, Postgresql, Sqlite. Using Acl library users can write database application more easily, quickly and safely.

Architecture diagram:
 ![Overall architecture diagram](res/img/architecture_en.png)

<hr>

* [1. The six most important modules](#1-the-six-most-important-modules)
    * [1.1. Basic network module](#11-basic-network-module)
    * [1.2. Coroutine](#12-coroutine)
    * [1.3. HTTP module](#13-http-module)
    * [1.4. Redis client](#14-redis-client)
    * [1.5. MQTT module](#15-mqtt-module)
    * [1.6. Server framework](#16-server-framework)

* [2. The other important modules](#2-the-other-important-modules)
    * [2.1. MIME module](#21-mime-module)
    * [2.2. Codec module](#22-codec-module)
    * [2.3. Database module](#23-database-module)

* [3. Platform support and compilation](#3-platform-support-and-compilation)
    * [3.1. Compile on different platforms](#31-compile-on-different-platforms)
    * [3.2. Precautions when compiling on Windows](#32-precautions-when-compiling-on-windows)

* [4. More about](#4-more-about)
    * [4.1. Samples](#41-samples)
    * [4.2. FAQ](#42-faq)
    * [4.3. Who are using acl?](#43-who-are-using-acl)
    * [4.4. License](#44-license)
    * [4.5. Reference](#45-reference)

<hr>

# 1. The six most important modules
As a C/C++ foundation library, Acl provides many useful functions for users to develop applications, including six important modules: Network, Coroutine, HTTP,  Redis client, MQTT, and Server framework.

## 1.1. Basic network module
- Stream processing module
This module is the most basic streaming communication module for the entire acl network communication. It not only supports network streaming, but also supports file streaming. It mainly supports:
  - Read data by line, compatible with \r\n under win32, and compatible with the end of \n under UNIX
  - Read data by line but require automatic removal of the trailing \n or \r\n
  - Read data as a string separator
  - Read the specified length of data
  - Try to read a row of data or try to read the specified length of data
  - Detect network IO status
  - Write a row of data
  - Write data by formatter, similar to fprintf
  - File stream location operation, similar to fseek
  - Write a set of data at once, similar to writev under unix
  - Truncate the file, similar to ftrunk
  - Get the file size
  - Get the current file stream pointer location, similar to ftell
  - Get the file size
  - Obtain the local address and remote address of the network stream

- Network operation module
  - This module mainly supports network server monitoring (supports TCP/UDP/UNIX domain socket), network client connection (supports TCP/UNIX domain socket), DNS domain name query and result cache (supports calling system gethostbyname/getaddrinfo function and direct Send DNS protocol two ways), socket (socket) operation and take the local network card and other functions.
- Non-blocking network flow
  - Support non-blocking mode connection, read (by row read, specified length read), write (write line, write specified length, write a set of data) and other operations.

- Common network application protocol library
  - It mainly supports common network application protocols, such as: HTTP, SMTP, ICMP, in which HTTP and ICMP modules implement blocking and non-blocking communication methods. In addition, the HTTP protocol supports server and client in C++ version of lib_acl_cpp. Two communication methods, when used as a server, support the interface usage similar to JAVA HttpServlet. When used as a client mode, it supports connection pool and cluster management. The module supports both cookie, session, and HTTP MIME file upload. Rich functions such as block transfer, automatic character set conversion, automatic decompression, and breakpoint resume.

- Common network communication library
  - Support memcached, beanstalk, handler socket client communication library, the communication library supports connection pool mode.

## 1.2. Coroutine
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
- For more information, see [Using Acl fiber](lib_fiber/README_en.md)

## 1.3. HTTP module
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

## 1.4. Redis client
The redis client module in Acl is powerful, high-performance and easy to use.
- Support Bitmap/String/Hash/List/Set/Sorted Set/PubSub/HyperLogLog/Geo/Script/Stream/Server/Cluster/etc.
- Provides stl-like C++ interface for each redis command
- Automaticaly cache and adapt the changing of hash slots of the redis cluster on the client
- Communication in single, cluster or pipeline mode
- Connection pool be used in signle or cluster mode
- High performance in cluster or pipline mode
- Same interface for single, cluster and pipeline modes
- Retry automatically for the reason of network error
- Can be used in the shared stack coroutine mode
- For more information, see [Using Acl redis client](lib_acl_cpp/samples/redis/README.md)

## 1.5. MQTT module
The MQTT 3.1.1 version has been implemented in Acl, which has a stream parser, so can be used indepedentily of any IO mode.
- Support MQTT 3.1.1 protocol: CONNECT/CONNACK/PUBLISH/PUBACK/PUBREC/PUBREL/PUBCOMP/SUBSCRIBE/SUBACK/UNSUBSCRIBE/UNSUBACK/PINGREQ/PINGRESP/DISCONNECT
- One class per command
- Stream parser can be used for any IO mode
- Data parsing separats from network communicationo
- Can be used on client and server sides
- For more information, see [Using Acl MQTT](lib_acl_cpp/samples/mqtt/README.md)

## 1.6. Server framework
The most important module in Acl is the server framework, which helps users quickly write back-end services, such as web services. Tools in app/wizard can help users generate one appropriate service code within several seconds. The server framework of Acl includes two parts, one is the services manager, the other is the service written by Acl service template. The services manager named acl_master in Acl comes from the famous Postfix, whose name is master, and acl_master has many extensions to master in order to be as one general services manager. There are six service templates in Acl that can be used to write application services, as below:
- **Process service:** One connection one process, the advantage is that the programming is simple, safe and stable, and the disadvantage is that the concurrency is too low;
- **Threads service:** Each process handles all client connections through a set of threads in the thread pool. The IO event trigger mode is used that a connection is bound to a thread only if it has readable data, and the thread will be released after processing the data. The service model's advantage is that it can handle a large number of client connections in one process with a small number of threads. Compare with the aio model, the programming is relatively simple;
- **Aio service:** Similar to nginx/squid/ircd, one thread can handle a large number of client connections in a non-blocking IO manner. The advantages of this model are high processing efficiency and low resource consumption, while the disadvantages are more complex programming;
- **Fiber service:** Although the non-blocking service model can obtain large concurrent processing capability, the programming complexity is high. The coroutine model combines the features of large concurrent processing and low programming complexity, enabling programmers to easily implement sequential IO programming;
- **UDP service:** The model is mainly a service model supporting UDP communication process;
- **Trigger service:** The model instance is mainly used to process the background service process of some scheduled tasks (similar to the system's crontab).

# 2. The other important modules

## 2.1. MIME module
MIME(Multipurpose Internet Mail Extensions) format is widely used in email application. MIME format is too important that it can be used not only for email applications, but also for Web applications. MIME RFC such as RFC2045/RFC2047/RFC822 has been implemented in Acl. Acl has a MIME data stream parser that is indepedent of the IO model, so it can be used by synchronous or asynchronous IO programs.

## 2.2. Codec module
Thare are some common codecs in Acl, such as Json, Xml, Base64, Url, etc., which are all stream parser and indepedent of IO communication. Json is very popular, so Acl also provides serialization/deserialization tools which can be used to transfer between Json data and C struct objects, which greatly improves the programming efficiency.

## 2.3. Database module
The unified database interface in Acl is designed to easily and safely operate thease well-known open source databases such as Mysql, Postgresq and SQLite. The SQL codec is designed to escape the charactors to avoid the DB SQL attacks. When users use Acl to write database applications, Acl will dynamically load the dynamic libraries of Mysql, Postgresql or SQLite. The advantage of dynamic loading is that users who don't need the database functionality don't care about it at all.

# 3. Platform support and compilation
## 3.1. Compile on different platforms
Acl project currently supports Linux, Windows, MacOS, FreeBSD, Solaris, Android, IOS.
- Linux/UNIX: The compiler is gcc, enter the lib_acl/lib_protocol/lib_acl_cpp directory directly in the terminal command line mode, run the make command.
- Windows: Can be compiled with VS2003/VS2008/VS2010/VS2012/VS2013/VS2015/VS2019. (If you need to compile with VS6/VS2005, you can refer to the compilation conditions of VS2003).
- MacOS: Compiled with xcode.
- Support for CMake cross-platform compilation

## 3.2. Precautions when compiling on Windows
There are a few things to keep in mind when using dynamic libraries in a WIN32 environment:
- When using the dynamic library of lib_acl, it needs to be predefined in the user's project: ACL_DLL;
- When using the HTTP library or ICMP library in the lib_protocol dynamic library, you need to predefine HTTP_DLL or ICMP_DLL in the project;
- When using the dynamic library of lib_acl_cpp, you need to predefine ACL_CPP_DLL in the project. If you use the VC2003 compiler environment, you need to predefine VC2003.
- When using the dynamic library of lib_dict, you need to predefine DICT_DLL in the project;
- When using a dynamic library of lib_tls, you need to predefine TLS_DLL in your project.
- Detailed compilation process, see: [Compilation and use of acl library](BUILD.md)

# 4. More about
## 4.1. Samples
There are a lot of examples in the acl library for reference, please refer to: [SAMPLES.md](SAMPLES.md)

## 4.2. FAQ
If you have some questions when using Acl, please see [FAQ.md](FAQ.md).

## 4.3. Who are using acl?
[![iqiyi](res/logo/logo_iqiyi.png)](http://www.iqiyi.com/)
[![263](res/logo/logo_263.png)](http://www.263.net/)
[![hexun](res/logo/logo_hexun.png)](http://www.hexun.com/)
[![v1](res/logo/logo_v1.png)](http://www.v1.cn/)
[![ksyun](res/logo/logo_ksyun.png)](https://www.ksyun.com/)
[![weibangong](res/logo/logo_weibangong.png)](https://www.weibangong.com/)
[![xianyou](res/logo/logo_xianyou.png)](http://www.i3game.com/)
[![foundao](res/logo/logo_foundao.png)](http://www.foundao.com/)

## 4.4. License
- LGPL-v3 license (see [LICENSE.txt](LICENSE.txt) in the acl project)

## 4.5. Reference
- WEB site: https://blog.csdn.net/zsxxsz
- Github:   https://github.com/acl-dev/acl
- Gitee:  https://gitee.com/acl-dev/acl
- Weibo:    http://weibo.com/zsxxsz
- QQ Group: ，693868497
