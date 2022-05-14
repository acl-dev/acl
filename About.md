# 1. About Acl project
The Acl (full name is Advanced C/C++ Library) project is powerful multi-platform network communication library and service framework, suppoting LINUX, WIN32, Solaris, FreeBSD, AndroidOS, iOS. Many applications written by Acl run on these devices with Linux, Windows, iPhone and Android and serve billions of users. There are some important modules in Acl project, including network communcation, server framework, application protocols, multiple coders, etc. The common protocols such as HTTP/SMTP/ICMP//MQTT/Redis/Memcached/Beanstalk/Handler Socket are implemented in Acl, and the codec library such as XML/JSON/MIME/BASE64/UUCODE/QPCODE/RFC2047/RFC1035, etc., are also included in Acl. Acl also provides unified abstract interface for popular databases such as Mysql, Postgresql, Sqlite. Using Acl library users can write database application more easily, quickly and safely.

Architecture diagram:
 ![Overall architecture diagram](res/img/architecture_en.png)

# 2. The six important modules
As a C/C++ foundation library, Acl provides many useful functions for users to develop applications, including six important modules: HTTP, MIME, Redis client, Database interface, Coroutine, and Server framework.

## 2.1. HTTP module
Supports HTTP/1.1, can be used in client and server sides.

### 2.1.1. Client mode
- Connection pool mode
- Chunked block transfer
- Gzip decompression
- SSL encrypted
- Breakpoints transmission
- Setting/acquisition of cookies
- Websocket transmission
- Sync/Async mode
- ...

### 2.1.2 Server mode
- HttpServlet interface like Java
- Chunked block transfer
- Gzip compression
- SSL encrypted
- Breakpoints transmission
- Setting/acquisition of cookies
- Session managment
- HTTP MIME format
- Websocket transmission
- Sync/Async mode
- ...

# 2.2. Redis client
- Supports Bitmap/String/Hash/List/Set/Sorted Set/PubSub/HyperLogLog/Geo/Script/Stream/Server/Cluster/etc.
- Provides stl-like C++ interface for each redis command
- Communication in single, cluster or pipeline mode
- Connection pool be used in signle or cluster mode
- More information see lib_acl_cpp/samples/redis/README.md

# 2.3. Coroutine

# 2.4. Server framework

# 2.5. Database module

# 2.6. Mime module