## acl 库的编译及使用

* acl 库的功能参见文章[acl介绍](http://zsxxsz.iteye.com/blog/576517) 及 [acl工程](https://github.com/acl-dev/acl/)，本文主要讲述如何编译和使用 acl 库。
 
* acl 库其实包含七个库：lib_acl （基础库）、lib_protocol（http 和 icmp 协议库）、lib_acl_cpp（封装了 lib_acl 和 lib_protocol 两个 C 库的 C++ 版本封装，更是增加了丰富的功能）、lib_fiber（功能强大的网络协程库，使开发者可以象使用 Go 语言一样快速编写基于协程的高并发服务应用）、lib_dict（封装了 bdb, cdb, Tokyo Cabinet 库的用于字典查询的库）、lib_tls（封装了openssl部分功能的库，主要用于 lib_acl 的 ssl 加密传输）以及 lib_rpc（封装了 Google 的 protobuf 的库）。其中，笔者用的最多还是 lib_acl 、lib_protocol 以及 lib_acl_cpp（目前用得最多的库）三个库，所以本文主要介绍这三个库的编译与使用。
* acl 库目前支持 Linux、Solaris、FreeBSD、MacOS、IOS、Android 和 Windows 平台，欢迎读者将 acl 库移植至其它平台。
 
* [一、Linux/UNIX 平台上编译](#一linuxunix-平台上编译)
    * [1、编译静态库](#1编译静态库)
        * [1.1、编译 libacl.a 库](#11编译-libacla-库)
        * [1.2、编译 libprotocol.a 库](#12编译-libprotocola-库)
        * [1.3、编译 libacl_cpp.a 库](#13编译-libacl_cppa-库)
    * [2、编译动态库](#2编译动态库)
    * [3、编译成单一库](#3编译成单一库)
    * [4、使用 cmake 编译](#4使用-cmake-编译)
* [二、Windows 平台](#二windows-平台)
    * [1 编译（vc2003/vc2008/vc2010/vc2012）](#1-编译)
    * [2 使用](#2-使用)
* [三、 注意事项](#三注意事项)

### 一、Linux/UNIX 平台上编译
需要使用 gcc/g++编译器及 gnu make 命令。
### 1、编译静态库
#### 1.1、编译 libacl.a 库
使用 gcc 编译：进入 lib_acl 目录，直接运行 make，正常情况下便可在 lib 目录下生成 libacl.a 静态库，用户在使用 libacl.a 编写自己的程序时，需要在自己的 Makefile 文件中添加如下选项：
##### 1.1.1、编译选项：-I 指定 libacl.a 头文件所在目录(头文件在 lib_acl/include 目录下）
##### 1.1.2、链接选项：-L 指定 libacl.a 所在目录，-lacl 指定需要链接 libacl.a 库
##### 1.1.3、用户需要在自己的源程序中包含 lib_acl 的头文件，如下：

    #include "lib_acl.h"

简单的 Makefile 示例：  

~~~doc
test: main.o
	gcc -o test main.o -lacl -lz -lpthread -ldl
main.o: main.c
	gcc -O3 -Wall -c main.c -I./lib_acl/include 
~~~~

#### 1.2、编译 libprotocol.a 库
使用 gcc 编译：进入 lib_protocol 目录，直接运行 make，正常情况下便可以在 lib 目录下生成 lib_protocol.a 静态库，用户在使用 lib_protocol.a 编写自己的程序时，需要在自己的 Makefile 文件中添加如下选项：
##### 1.2.1、编译选项：-I 指定 lib_protocol.a 头文件目录（在 lib_protocol/include 目录下）
##### 1.2.2、链接选项：-L 指定 lib_protocol.a 所在目录，-L 指定 lib_acl.a 所在目录，-l_protocol -l_acl
##### 1.2.3、用户需要在自己的应用程序中包含 lib_protocol 头文件，如下：

    #include "lib_protocol.h"

简单的 Makefile 示例：  

~~~doc
test: main.o
	gcc -o test main.o -lprotocol -lacl -lz -lpthread -ldl
main.o: main.c
	gcc -O3 -Wall -c main.c -I./lib_acl/include -I./lib_protocol/include 
~~~

#### 1.3、编译 libacl_cpp.a 库
使用 g++ 编译器： 进入 lib_acl_cpp 目录，运行 make static 编译 libacl_cpp.a 静态库，便可 lib 目录下生成 libacl_cpp.a，用户在使用 libacl_cpp.a 编写程序时，需要在自己的 Makefile 文件中添加如下选项：
##### 1.3.1、编译选项：-I 指定 libacl_cpp.a 头文件目录（在 lib_acl_cpp/include 目录下）
##### 1.3.2、链接选项：-L
指定 libacl_cpp.a 所在目录，-L 指定 libprotocol.a 所在目录，-L 指定 libacl.a 目录。如：-L./lib_acl_cpp/lib -lacl_cpp -L./lib_protocol/lib -lprotocol -L./lib_acl/lib -lacl，一定要注意这三个库的依赖关系：libprotocol.a 依赖于 libacl.a，libacl_cpp.a 依赖于 libprotocol.a 及 lib_acl.a，在使用 g++ 进行编译时必须保证库的依赖顺序，被依赖的库总是应放在后面，否则在链接程序时会报函数不存在。
##### 1.3.3、用户需要在自己的应用中包含 lib_acl.hpp 头文件，如下：

      #include "acl_cpp/lib_acl.hpp"

简单的 Makefile 示例：  

~~~doc
test: main.o
	g++ -o test main.o -lacl_cpp -lprotocol -lacl -lz -lpthread -ldl
main.o: main.cpp
	g++ -O3 -Wall -c main.cpp -I./lib_acl_cpp/include -I./lib_acl/include -I./lib_protocol/include 
~~~

如果程序仅用到了 libacl_cpp.a 库中类及函数，则只需要包含 libacl_cpp.a 的头文件即可，至于该库所依赖的 libprotocol.a 及 libacl.a 库的头文件已经做了特殊隐含处理，不必显式包含。
 
### 2、编译动态库
     
 编译 libacl.so, libprotocol.so, libacl_cpp.so 的方式与编译静态库的方式有所不同，需要分别进入三个目录执行： make shared rpath=${lib_path}，其中 shared 表示需要编译动态库，${lib_path} 需要用实际的目标路径替换，比如：make shared rpath=/opt/acl/lib，则会将动态库编译好后存放于 /opt/acl/lib 目录，编译这三个库的顺序为：libacl.so --> libprotocol.so --> libacl_cpp.so。  
另外，在编译 libacl_cpp.so 时，还需要提前编译在 resource 目录下的 polarssl 库，编译完后再编译 libacl_cpp.so 同时需要指定 polarssl.lib 库所在的路径；如果不需要 SSL 通讯方式，则需要打开 lib_acl_cpp/Makefile 文件，去年编译选项：-DHAS_POLARSSL。  
 
应用程序在使用这三个动态库时的头文件的包含方式与静态库的相同，连接动态库的方式与静态库类似，只是将 .a 替换成 .so 即可，同时也要求包含的顺序与上述静态库相同。如：
- a、编译时的头文件包含方式：-I/opt/acl/include/acl -I/opt/acl/include/protocol -II/opt/acl/include
- b、连接时的库文件包含方式：-L/opt/acl/lib -lacl_cpp -lprotocol -lacl
- c、运行时的库文件加载方式：-Wl,-rpath,/opt/acl/lib，该参数指定程序运行时需要加载三个动态库的位置在 /opt/acl/lib 目录下。
 
### 3、编译成单一库
为了方便使用，还提供了将以上三个库合成一个库的方法，在 acl 库目录下运行：make build_one，则会生成统一库：libacl_all.a 及 libacl_all.so，该库包含了 libacl，lib_protocol，lib_acl_cpp 三个库。则编写的 Makefile 就更为简单：
~~~
test: main.o
	g++ -o test main.o -lacl_all -lz -lpthread -ldl
main.o: main.cpp
	g++ -O3 -Wall -c main.cpp -I./lib_acl_cpp/include -I./lib_acl/include -I./lib_protocol/include 
~~~
### 4、使用 cmake 编译
除以上使用 make 工具对 acl 库进行编译外，同时提供了使用 cmake 工具对 acl 库进行编译，编译过程非常简单：  
~~~
$mkdir build
$cd build
$cmake ..
$make
~~~
### 二、Windows 平台
### 1 编译
 
在 acl 项目的根目录下，多个 VC 的工程文件，用户可根据自身需要打开工程文件：acl_cpp_vc2003.sln，acl_cpp_vc2008.sln，acl_cpp_vc2010.sln，acl_cpp_vc2012.sln（最早也支持VC6）。用户可以选择编译 lib_acl、lib_protocol、lib_acl_cpp 的静态库调试版、静态库发布版、动态库调试版以及动态库发布版，编译完成后，会在 acl\dist\lib\win32 目录生成的静态库有：  
lib_acl_vc20xxd.lib、lib_acl_vc20xx.lib、lib_protocol_vc20xxd.lib 和 lib_protocol_vc20xx.lib；  
生成的与动态库相关的文件有：lib_acl_d.dll/lib_acl_d.lib，lib_acl.dll/lib_acl.lib，lib_protocol_d.dll/lib_protocol_d.lib，lib_protocol.dll/lib_protocol.lib，lib_acl_cpp_d.dll/lib_acl_cpp_d.lib，lib_acl_cpp.dll/lib_acl_cpp.lib。
 
### 2 使用
 
- a）在 win32 平台下使用 lib_acl 和 lib_protocol 静态库时，只需要在包含目录中添加
lib_acl/include、lib_protocol/include 以及 lib_acl_cpp/include
所在的路径，在链接时指定静态库路径及静态库名称。
- b）在 win32 平台下使用 lib_acl 的动态库时，不仅要做与 a） 中所指定的操作，而且需要在预处理器定义中添加：ACL_DLL；在使用 lib_protocol 的动态库，需要在预处理器定义中添加：HTTP_DLL 和 ICMP_DLL；在使用 lib_acl_cpp 的动态库时，需要在预处理器定义中添加：ACL_CPP_DLL。
 
### 三、注意事项
因为 lib_acl 是最基础的库，而 lib_protocol 依赖于 lib_acl，lib_acl_cpp 依赖于 lib_protocol 和 lib_acl，所在生成动态库时，需要注意生成顺序，编译顺序为：lib_acl，lib_protocol，lib_acl_cpp。
 
- 个人微博：http://weibo.com/zsxxsz/
- acl 下载：https://sourceforge.net/p/acl/
- github 地址：https://github.com/acl-dev/acl/
- osc git 地址：https://git.oschina.net/acl-dev/acl
- QQ 群：242722074
