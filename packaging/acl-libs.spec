%define release_id 0

Summary:        The powerful c/c++ library
Name:           acl-libs
Version:        3.6.4
Release:        %{release_id}
Group:          System/Libs
License:        LGPL3.0,IBM
URL:            https://github.com/acl-dev/
Packager:       Zhang Qiang <155281969@qq.com>, Wang Haibin <634648088@qq.com>
BuildRoot:      %{_tmppath}/%{name}-%{version}-root
Source:         http://example.com/%{name}-%{version}.tar.gz

%description

One advanced C/C++ library for Linux/Mac/FreeBSD/Solaris(x86)/Windows/Android/IOS https://acl-dev.cn/.

%prep
%setup -q

%build
make -j 4 build_one
make -j 4 -C lib_fiber

%install

make packinstall  DESTDIR=$RPM_BUILD_ROOT
make -C lib_fiber packinstall  DESTDIR=$RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT/opt/soft/services/

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
# TODO: should be renamed
%{_includedir}/acl-lib/acl
%{_includedir}/acl-lib/acl_cpp
%{_includedir}/acl-lib/fiber
%{_includedir}/acl-lib/protocol
/usr/lib/libacl_all.a
/usr/lib/libfiber.a
/usr/lib/libfiber_cpp.a

%changelog

* Sat Jul 26 2025 shuxin.zheng shuxin.zheng@qq.com 3.6.4-0-20250726.12
- Bugfix: Fixed bugs in fiber mutex module;
- Feature: Add SSL for redis_client_pipeline.

* Fri Apr 25 2025 shuxin.zheng shuxin.zheng@qq.com 3.6.3-0-20250425.15
- Feature: Add fiber pool module;
- Feature: Support DMA IO;
- Feature: Support Arm64ec for Windows;
- Optimize: Optimize hooked epoll module in fiber mode;
- Optimize: Optimize fiber sem;
- Optimize: Optimize support c++11;
- Bugfix: Fiexed bugs in fiber_mutex & fiber_cond.

* Sat Nov 02 2024 shuxin.zheng shuxin.zheng@qq.com 3.6.2-0-20241102.10
- Feature: Fiber supports io-ruing;
- Feature: Fiber hooks setsockopt to improve IO timeout performance;
- Feature: Fiber supports Qt-UI on windows;
- Feature: Acl supports vc2022 on windows;
- Feature: Acl supports HarmonyOS;
- Feature: Acl supports being built by COSMOCC;
- Optimize: Connection pool has been optimized very much;
- More small features been added and more bugs been corrected.

* Fri Oct 20 2023 shuxin.zheng shuxin.zheng@qq.com 3.6.1-6-20231020.18
- feature: Add support for udp multicast;
- bugfix: Fixed one dead loop bugin mqtt_connect.cpp;
- workaroud: Add prefix on avl API to avoid API conlision;
- optimize: Optimize acl_htable module;
- feature: Add some supports for c++11/c++17;
- feature: Add proc_pidpath() on MacOS.

* Tue Jun 13 2023 shuxin.zheng shuxin.zheng@qq.com 3.6.1-4-20230613.10
- feature: add fiber_sbox2 class;
- optimize: optimize fiber_sbox class;
- bugfix: Fixed bug in acl_get_ifaddrs on windows.

* Tue Jun 06 2023 shuxin.zheng shuxin.zheng@qq.com 3.6.1-3-20230606.11
- bugfix: fixed some bugs in mqtt module;
- bugfix: redis_client_pipeline shouldn't use stack objects in sharing stack mode;
- performance: optimize redis_builder's performance for removing redis objectsx;
- feature: fiber_sem using async mode;
- feature: add fiber_tbox2 for transfering objects such add shared_ptr;
- feature: sendmmsg/recvmmsg were hooked in fiber IO module;
- feature: service templates in the mode of fiber or thread can bind wildcad addresses.

* Wed Mar 22 2023 shuxin.zheng shuxin.zheng@qq.com 3.6.1-2-20230322.15
- bugfix: fixed one bug in fiber_mutex module where some resouce collision maybe happen.

* Tue Mar 21 2023 shuxin.zheng shuxin.zheng@qq.com 3.6.1-1-20230321.10
- optimize: optimize fiber's performance by reducing the number of calling getting time;
- optimize: optimize fiber's performance by using clock_gettimeofday replacing gettimeofday;
- optimize: reduce objects' size of HTTP module by using COW(copy on write).

* Wed Mar 08 2023 shuxin.zheng shuxin.zheng@qq.com 3.6.1-0-20230308.11
- feature: SSL module in acl including Openssl and MbedTLS supports multiple certificates in server side.
- feature: support MbedTLS 3.3.0 version.
- optimize: optimize memory usage.

* Thu Jan 19 2023 shuxin.zheng shuxin.zheng@qq.com 3.6.0-4-20230119.15
- feature: add fiber's stack backtrace
- feature: add fiber mutex dead lock checking
- feature: hook fcntl API on Linux for Rust user

* Thu Dec 29 2022 shuxin.zheng shuxin.zheng@qq.com 3.6.0-3-20221229.17
- optimize: use AVL to handle the global timer to improve the timer's performance
- optimize: fiber_cond use the global timer module
- optimize: optimize fiber_mutex
- optimize: implement a new sendfile for io-uring
- test: add unit_test module for fiber module

* Fri Dec 16 2022 shuxin.zheng shuxin.zheng@qq.com 3.6.0-2-20221216.14
- bugfix: use syscall(SYS_gettid) other than gettid() for CentOS OS.

* Fri Dec 16 2022 shuxin.zheng shuxin.zheng@qq.com 3.6.0-1-20221216.13
- feature: one fd can be opered by multiple fibers in io_uring mode
- bugfix: fixed one bug in fiber_mutex.c

* Wed Dec 01 2022 shuxin.zheng shuxin.zheng@qq.com 3.6.0-0-20221201.20
- feature: fiber module supports io_uring
- optimize&feature: add the new fiber lock named as fiber_mutex
- some other optimize and bugs fixed

* Wed Sep 19 2022 shuxin.zheng shuxin.zheng@qq.com 3.5.3-23-20220919.10
- optimize: limit the minmal thread stack size to 4M when creating new thread
	on Win64 to avoid stack crash.

* Wed Sep 14 2022 shuxin.zheng shuxin.zheng@qq.com 3.5.3-22-20220914.14
- bugfix: when using SO_REUSEPORT, acl_threads_server.cpp should support
	binding unix domain socket.

* Wed Aug 31 2022 shuxin.zheng shuxin.zheng@qq.com 3.5.3-21-20220821.15
- feature: Openssl has been supported in acl ssl module
- workaroud: when IO timeout be set 0, which will not block the IO process,
	but the time before, 0 mean the blocking the IO process.
- workaroud: in aio module, it's the default that the connection will be closed
	after the data in sending buffer been sent.

* Mon Aug 15 2022 shuxin.zheng shuxin.zheng@qq.com 3.5.3-20-20220815.15
- bugfix: don't use max timeout when the timeout is -1 in poll.c

* Wed Aug 03 2022 shuxin.zheng shuxin.zheng@qq.com 3.5.3-19-20220802.10
- bugfix: fixed bugs in fiber_server.cpp when using shared stack mode in sighup fiber.

* Tue Jul 26 2022 shuxin.zheng shuxin.zheng@qq.com 3.5.3-18-20220726.16
- feature: optimize master service template

* Mon Jul 25 2022 shuxin.zheng shuxin.zheng@qq.com 3.5.3-17-20220725.17
- feature: the fiber service template supports SO_REUSEPORT option

* Wed Jul 13 2022 shuxin.zheng shuxin.zheng@qq.com 3.5.3-16-20220713.17
- just release rpm package for CentOS8.x

* Sat Jul 09 2022 shuxin.zheng shuxin.zheng@qq.com 3.5.3-15-20220709.17
- bugfix: fixed one crash bug that redis_command::request_buf_ will be NULL
  when calling redis_pubsub::get_message() first without calling
  redis_pubsub::subscribe() before.

* Wed Jun 15 2022 shuxin.zheng shuxin.zheng@qq.com 3.5.3-14-20220615.15
- bugfix: fixed one bug in redis_client_pipeline.cpp about handling redirect message.

* Tue Jun 07 2022 shuxin.zheng shuxin.zheng@qq.com 3.5.3-13-20220607.20
- bugfix: fixed one urgent bug in redis_client_cluster.hpp for memory overflow.

* Wed Jun 01 2022 shuxin.zheng shuxin.zheng@qq.com 3.5.3-12-20220601.20
- optimize iocp of fiber module
- fixed bugs in fiber_io.c
- optimize fiber on Windows when parsing hosts file
- optimize wizard for generating SSL service
- transfer open flags from lib_acl_cpp to lib_acl

* Sun Apr 24 2022 shuxin.zheng shuxin.zheng@qq.com 3.5.3-11-20220422.17
- fixed bugs in app/redis_tools/redis_builder

* Wed Apr 13 2022 shuxin.zheng shuxin.zheng@qq.com 3.5.3-10-20220413.15
- fixed one dead lock process in event timer module
- fiber has more improvement: shared stack, close fd safety, IO timer with higth performance

* Wed Feb 23 2022 shuxin.zheng@qq.com 3.5.3-7-20220223.11
- optimize: optimize fiber module

* Wed Nov 17 2021 shuxin.zheng@qq.com 3.5.3-4-20211117.11
- bugfix: fixed one crash bug in http_aclient::handle_res_hdr()
- optimize: add more functions in aio module

* Wed Oct 27 2021 shuxin.zheng@qq.com 3.5.3-3-20211027.17
- fixed one crashed bug in acl_inet_connect.c.

* Thu Oct 21 2021 shuxin.zheng@qq.com 3.5.3-2-20211021.17
- bugfix: fixed one corrupt bug occasionally in acl_udp_server.c

* Thu Oct 14 2021 shuxin.zheng@qq.com 3.5.3-1-20211014.09
- bugfix: don't call getaddrinfo when binding a numric IP addr
- feature: add udp_monitor_netlink configure in acl_udp_server.c
- bugfix: optimize and fix bugs in iocp event in fiber module
- bugfix: optimize and fix bugs in windows GUI message in fiber module
- feature: DNS module in fiber module  can work on windows
- feature: add API hook for windows platform by adding detrours module from MS open source
- more: more features and bugs fixed

* Thu Jun 24 2021 shuxin.zheng@qq.com 3.5.3-0-20210624.16
- feature: mqtt module has been proved ok
- optimize dns module for supporting IPV6, etc
- fixed some bugs

* Mon Jan 11 2021 shuxin.zheng@qq.com 3.5.2-0-20210111.18
- feature: redis module has supported pipeline with very high performance!

* Thu Dec 17 2020 shuxin.zheng@qq.com 3.5.1-11-20201117.09
- feature: add more API for db_sqlite module by Vhdsih

* Wed Oct 28 2020 shuxin.zheng@qq.com 3.5.1-10-20201028.16
- fixed bugs and optimize operation in acl::string

* Tue Oct 27 2020 shuxin.zheng@qq.com 3.5.1-9-20201027.15
- rewrite rfc1035 protocol for DNS module.

* Thu Aug 13 2020 shuxin.zheng@qq.com 3.5.1-8-20200813.11
- fixed bug in events_timer.c/events_timer_thr.c

* Tue Aug 11 2020 shuxin.zheng@qq.com 3.5.1-7-20200811.17
- fixed one important bug in event_timer.c/event_timer_thr.c: when one timer
  cancel another timer, if the other timer has been in schedule queue, the
  memory will be corrupted and process will be crashed.

* Wed Aug 05 2020 shuxin.zheng@qq.com 3.5.1-6-20200805.11
- add some function in redis module for supporting binary data
- rewrite and optimize redis module
- optimize fiber module for checking if a fd is socket/pipe/fifo type
- optimize for mobile development

* Sat Jun 27 2020 shuxin.zheng@qq.com 3.5.1-5-20200627.14
- release 3.5.1-5 version
- add memory checking module for c++
- add http module written by c++11 in libfiber
- don't fatal in event_limit() for IOS14.beta

* Fri Jun 05 2020 shuxin.zheng@qq.com 3.5.1-4-20200605.18
- release 3.5.1-4 version
- add master_ctl to rpm package
- master_ctl using unix domain socket as the channel to acl_master

* Mon Jun 01 2020 shuxin.zheng@qq.com 3.5.1-3-20200601.16
- support "application/x-gzip" from http response header in http_client.cpp

* Thu May 28 2020 shuxin.zheng@qq.com 3.5.1-2-20200528.16
- release acl3.5.1-2 release new version for edge schedule

* Sun Jan 12 2020 shuxin.zheng@qq.com 3.5.1-1-20200112.22
- release acl3.5.1-1 for making ssl method more easily to use.

* Thu Jan 09 2020 shuxin.zheng@qq.com 3.5.1-0-20200109.09
- release acl 3.5.1 version

* Thu Jan 09 2020 shuxin.zheng@qq.com 3.5.0-12-20200109.09
- many optimization in acl
- acl-master start up with keep mask

* Thu Sep 19 2019 shuxin.zheng@qq.com 3.5.0-11-20190919.10
- optimize: many optimizing for decrease libs' size for mobile platom
- bugfix: fixed one bug in event_epoll.c of fiber lib

* Thu Aug 22 2019 shuxin.zheng@qq.com 3.5.0-10-20190822.11
- release 3.5.0-10 for bugfix

* Wed Aug 21 2019 shuxin.zheng@qq.com 3.5.0-9-20190821.11
- release 3.5.0-9

* Thu Aug 08 2019 shuxin.zheng@qq.com 3.5.0-8-20190808.16
- bugfix: fixed one bug in the hooked API sleep in lib_fiber
- feature: url_coder permits the value of name is null or empty
- feature: http_aclient will check the resturned value by on_read_timeout

* Tue Aug 06 2019 shuxin.zheng@qq.com 3.5.0-7-20190806.14
- bugfix: the system API gettimeofday been hooked in lib_fiber wasn't accurate
- bugfix: init_log_mutex() in acl_mylog.c has one bug when process forks one child

* Fri Aug 02 2019 shuxin.zheng@qq.com 3.5.0-6-20190802.19
- bugfix: fixed bug in atomic_long for Windows
- bugfix: don't use PTHREAD_MUTEX_RECURSIVE macro as compiling condition in acl_mylog.c
- performance: improve the gettimeofday's performance in lib_fiber lib
- feature: don's use body_parse_ in HttpServletRequest

* Mon Jul 22 2019 shuxin.zheng@qq.com 3.5.0-5-20190722.15
- workaroud: format c++ code style
- compile: compiling with the minimal lib size for mobile client
- feature: add http_aclient supporting async HTTP+Websocket+SSL
- http module: more powerful and more feature supporting

* Mon May 20 2019 shuxin.zheng@qq.com 3.5.0-4-20190520.11
- optimize package size for Mobile by adding optional compiling

* Thu May 09 2019 shuxin.zheng@qq.com 3.5.0-3-20190509.13
- safety & feature: add nocopyable limit for many class to avoid potential problem

* Mon May 06 2019 shuxin.zheng@qq.com 3.5.0-2-20190506.09
- acl_vstream.c: fixed bug in acl_vstream_fflush() where wbuf_dlen should be set 0

* Tue Apr 30 2019 shuxin.zheng@qq.com 3.5.0-1-20190430.17
- tcp_keeper: add sync parameter to control if connect the given addr directly

* Sun Apr 28 2019 shuxin.zheng@qq.com 3.5.0-1-20190428.16
- tcp_keeper: fixed one bug in keeper_conn.cpp
- redis_zset: add ZPOPMIN/ZPOPMAX/BZPOPMIN/BZPOPMAX
- server_socket: remove one constructur method

* Sat Mar 09 2019 shuxin.zheng@qq.com 3.5.0-0-20190309.20
- release 3.5.0 version.

* Sun Feb 24 2019 shuxin.zheng@qq.com 3.4.1-48-20190224.22
- acl_mylog.c: remove thread mutex when writing log to local file or remote UDP server like syslog-ng

* Wed Feb 13 2019 shuxin.zheng@qq.com 3.4.1-47-20190213.11
- acl_scan_dir.c: the current dir's attribute can be got in scanning process

* Mon Feb 11 2019 shuxin.zheng@qq.com 3.4.1-46-20190211.11
- acl_scan_dir.c: continue to scan the next node when some error happends

* Fri Feb 01 2019 shuxin.zheng@qq.com 3.4.1-45-20190201.18
- scan_dir: can remove empty directories automatically.

* Mon Jan 21 2019 shuxin.zheng@qq.com 3.4.1-44-20190121.13
- connect_manager::check_idle: fixed one bug when pools_size is 0

* Thu Jan 17 2019 shuxin.zheng@qq.com 3.4.1-43-20190117.14
- tcp_keeper: don't set TCP SO_LINGER; tcp_keeper is ok now.

* Mon Jan 14 2019 shuxin.zheng@qq.com 3.4.1-42-20190114.14
- http_request::check_range: compatible some situation for the returned range value
- fiber/tcp_keeper: more improvement

* Wed Jan 02 2019 shuxin.zheng@qq.com 3.4.1-41-20190102.11
- fiber_tbox.hpp/tbox.hpp/mbox.hpp: set free_obj's default value to true

* Thu Dec 27 2018 shuxin.zheng@qq.com 3.4.1-40-20181227.11
- acl_fiber_event.c: fixed one bug in event_ferror

* Thu Dec 27 2018 shuxin.zheng@qq.com 3.4.1-39-20181227.10
- acl_fiber_cond.c: fixed bugs in acl_fiber_cond_timedwait/acl_fiber_cond_wait

* Tue Dec 25 2018 shuxin.zheng@qq.com 3.4.1-38-20181225.11
- acl_fiber_cond.c: fixed one bug in acl_fiber_cond_timedwait
- fiber_tbox: add bool return for push

* Sun Dec 16 2018 shuxin.zheng@qq.com 3.4.1-37-20181216.16
- connect_manager: support fiber in thread mode

* Thu Dec 06 2018 shuxin.zheng@qq.com 3.4.1-36-20181206.15
- acl_threads_server.c: fixed one crashed bug in client_wakeup
- fiber_tbox.hpp: make push more safety

* Thu Nov 29 2018 shuxin.zheng@qq.com 3.4.1-35-20181129.22
- fiber_event.c: fixed one bug in acl_fiber_event_notify.

* Thu Nov 29 2018 shuxin.zheng@qq.com 3.4.1-34-20181129.11
- fbase_event.c: restart IO when IO process is interrupted by EINTR

* Wed Nov 28 2018 shuxin.zheng@qq.com 3.4.1-33-20181128.16
- acl_udp_server.c: fixed one bug in server_binding() when binding failed
- lib_fiber: add fiber_cond.c

* Wed Nov 28 2018 shuxin.zheng@qq.com 3.4.1-32-20181128.09
- lib_fiber/c/src/fiber_event.c: fixed one bug for lock conlision

* Tue Nov 27 2018 shuxin.zheng@qq.com 3.4.1-31-20181127.14
- lib_fiber/c/src/fiber_event.c: fixed one bug

* Thu Nov 15 2018 shuxin.zheng@qq.com 3.4.1-30-20181115.15
- acl_master: add log info for remote control commands

* Tue Oct 23 2018 shuxin.zheng@qq.com 3.4.1-29-20181023.22
- bugfix: acl_udp_server.c can't bind multiple addrs
- feature: disable core when process exiting
- feature: core file size can be configured

* Wed Oct 17 2018 shuxin.zheng@qq.com 3.4.1-28-20181017.17
- bugfix: acl_master can't support UDP service on Centos whose version is below 7.x

* Thu Oct 11 2018 shuxin.zheng@qq.com 3.4.1-27-20181011.11
- release 3.4.1-27

* Fri Sep 28 2018 shuxin.zheng@qq.com 3.4.1-26-20180928.17
- bugfix: acl_inet_connect_ex of acl_inet_connect.c should support domain:port format

* Sat Sep 22 2018 shuxin.zheng@qq.com 3.4.1-25-20180922.13
- feature: The elements will be removed according LRU in acl_cache2.c
- feature: add flag to control is using SO_REUSEADDR when binding local addr
- bugfix: fixed one bug in acl_cache2_update of acl_cache2.c

* Thu Sep 20 2018 shuxin.zheng@qq.com 3.4.1-24-20180920.09
- bugfix: fixed bugs in lib_fiber for IPV6 supporting

* Wed Sep 19 2018 shuxin.zheng@qq.com 3.4.1-23-20180919.15
- bugfix: acl_ifconf_search in acl_ifconf.c can't handle some addrs patterns

* Sun Sep 16 2018 shuxin.zheng@qq.com 3.4.1-22-20180916.18
- bugfix: acl_ifconf.c and some modules can't justify some UNIX path that
  acl_master can't handle UNIX path like "master.sock" which hasn't '/' in it.

* Sun Sep 16 2018 shuxin.zheng@qq.com 3.4.1-21-20180916.21
- feature: support IPV6 OK!

* Sat Sep 08 2018 shuxin.zheng@qq.com 3.4.1-20-20180908.21
- fixed bugs in acl::string::begin_with API

* Sat Sep 08 2018 shuxin.zheng@qq.com 3.4.1-19-20180908.13
- fixed bugs in acl::string::begin_with API

* Thu Sep 06 2018 shuxin.zheng@qq.com 3.4.1-18-20180906.19
- release 3.4.1-18, prepare for adding IPV6 feature.

* Mon Aug 20 2018 shuxin.zheng@qq.com 3.4.1-17-20180820.11
- there's some comments error in token_tree

* Mon Aug 20 2018 shuxin.zheng@qq.com 3.4.1-16-20180820.10
- fixed one bug in token_tree's destructor

* Sun Aug 19 2018 shuxin.zheng@qq.com 3.4.1-15-20180819.15
- lib_acl_cpp: add token_tree class

* Tue Aug 07 2018 shuxin.zheng@qq.com 3.4.1-14-20180807.10
- optimize the storage size of ACL_VSTRING, ACL_VBUF and ACL_JSON_NODE

* Fri Aug 03 2018 shuxin.zheng@qq.com 3.4.1-13-20180803.11
- bugfix: json parse should not ignore string value begin with space

* Thu Aug 02 2018 shuxin.zheng@qq.com 3.4.1-12-20180802.22
- bugfix: json parser can't handle empty array object

* Fri Jul 27 2018 shuxin.zheng@qq.com 3.4.1-11-20180727.16
- tbox: fixed one bug

* Fri Jul 27 2018 shuxin.zheng@qq.com 3.4.1-10-20180727.13
- tbox: support transfering NULL message

* Thu Jul 05 2018 shuxin.zheng@qq.com 3.4.1-9-20180705.14
- acl::fstream: add filelock methods

* Thu Jun 28 2018 shuxin.zheng@qq.com 3.4.1-8-20180628.21
- tbox: rewrite tbox with C++ template class

* Tue Jun 19 2018 shuxin.zheng@qq.com 3.4.1-7-20180619.18
- thread_cond: compiling error for wait overriding

* Fri Jun 15 2018 shuxin.zheng@qq.com 3.4.1-6-20180615.15
- thread_cond::wait add locked parameter for one locking condition

* Tue Jun 05 2018 shuxin.zheng@qq.com 3.4.1-5-20180605.14
- check_client: just choose one from on_refused and on_timeout to report connection status

* Tue Jun 05 2018 shuxin.zheng@qq.com 3.4.1-4-20180605.14
- connect_monitor: change params for on_refused and on_timeout methods

* Mon Jun 04 2018 shuxin.zheng@qq.com 3.4.1-3-20180604.18
- connect_monitor: add three callback for application to handle

* Thu May 17 2018 shuxin.zheng@qq.com 3.4.1-2-20180517.16
- fixed bugs in acl_write_wait.c

* Mon May 14 2018 shuxin.zheng@qq.com 3.4.1-1-20180514.15
- add new method in redis_client_cluster
- add check_idle in connect_manager

* Mon May 07 2018 shuxin.zheng@qq.com 3.4.1-0-20180507.14
- acl 3.4.1 released!

* Sun Apr 29 2018 shuxin.zheng@qq.com 3.4.0-0-20180429.20
- acl 3.4.0 released!

* Fri Apr 27 2018 shuxin.zheng@qq.com 3.3.0-90-20180427.27
- http: unsafe uri can be corrected

* Thu Apr 12 2018 shuxin.zheng@qq.com 3.3.0-89-20180412.20
- thread_cond::wait: don't save log when waiting timedout
- atomic: override constructur of atomic(const atomic&)

* Tue Mar 20 2018 shuxin.zheng@qq.com 3.3.0-88-20180320.10
- fixed one bug in thread_cond::wait there was one problem when computing timeout

* Thu Mar 15 2018 shuxin.zheng@qq.com 3.3.0-87-20180315.14
- move acl-master.json from acl-master to acl-tools

* Wed Mar 07 2018 shuxin.zheng@qq.com 3.3.0-86-20180307.13
- rm acl-tools from CI

* Wed Mar 07 2018 shuxin.zheng@qq.com 3.3.0-85-20180307.11
- rpm version

* Wed Mar 07 2018 shuxin.zheng@qq.com 3.3.0-84-20180307.11
- acl-tools rpm was removed from CI

* Mon Mar 05 2018 shuxin.zheng@qq.com 3.3.0-83-20180305.14
- lib_fiber: merge codes from libfiber

* Tue Feb 27 2018 shuxin.zheng@qq.com 3.3.0-82-20180227.14
- move tools-ctl from acl-master to acl-tools

* Tue Feb 27 2018 shuxin.zheng@qq.com 3.3.0-81-20180227.11
- master: version info can be get by web service

* Mon Feb 26 2018 shuxin.zheng@qq.com 3.3.0-80-20180226.13
- just upgrade version to 3.3.0-80 for CI

* Mon Feb 26 2018 shuxin.zheng@qq.com 3.3.0-79-20180226.11
- create tools-ctl for controling the master's tools

* Sat Feb 24 2018 shuxin.zheng@qq.com 3.3.0-78-20180224.09
- build rpm with three packages: acl-libs, acl-master, acl-tools, by wanghaibin

* Sat Feb 24 2018 shuxin.zheng@qq.com 3.3.0-77-20180224.09
- acl_unix_listen.c: won't fatal when binding UNIX addr error

* Sat Feb 24 2018 shuxin.zheng@qq.com 3.3.0-76-20180224.00
- acl_master: lock file should be before starting services

* Fri Feb 23 2018 shuxin.zheng@qq.com 3.3.0-75-20180223.23
- acl_master: when starting, lock the specified file avoiding starting more than once
- master_guard: listening master_guard.sock other master_guard.sock@unix
- master_ctld: listening master_ctld.sock

* Thu Feb 22 2018 shuxin.zheng@qq.com 3.3.0-74-20180222.10
- master_ctld: add UNIX domain listening avoiding be blocked by iptables.
- gson: std::map object can also be optional in json serialization.

* Mon Feb 12 2018 shuxin.zheng@qq.com 3.3.0-73-20180212.15
- master_ctld: support GET for checking port if service is aliving

* Mon Feb 12 2018 shuxin.zheng@qq.com 3.3.0-71-20180212.14
- remove daemon from master's service

* Wed Jan 31 2018 shuxin.zheng@qq.com 3.3.0-69-20180131.12
- master_guard: can't count fds on Cendos5.x
- master_monitor: can't get real version for "-v"
- acl_master: should override check_xxx when reloading service configure

* Tue Jan 30 2018 shuxin.zheng@qq.com 3.3.0-68-20180130.09
- master_guard: invalid exiting status

* Mon Jan 29 2018 shuxin.zheng@qq.com 3.3.0-67-20180129.18
- acl_master: when service hasn't master_notify_addr then using the default
  path as /opt/soft/acl-master/var/public/monitor.sock
- acl_master: when service crashed, one message will be delivered without
  master_notify_recipients.

* Mon Jan 29 2018 shuxin.zheng@qq.com 3.3.0-66-20180129.17
- service_guard: support tcp service by using acl::tcp_ipc class
- master_guard: using acl::tcp_ipc when using tcp connection

* Thu Jan 25 2018 shuxin.zheng@qq.com 3.3.0-65-20180125.12
- master_ctld: add default checking items.

* Thu Jan 25 2018 shuxin.zheng@qq.com 3.3.0-64-20180125.11
- master_guard/service_guard: support memory usage checking

* Wed Jan 24 2018 shuxin.zheng@qq.com 3.3.0-63-20180124.23
- master tools can support new protocols

* Wed Jan 24 2018 shuxin.zheng@qq.com 3.3.0-62-20180124.16
- add master_monitor tool of acl_master

* Tue Jan 23 2018 shuxin.zheng@qq.com 3.3.0-61-20180123.19
- epoll_event.c should handle EPOLLERR|EPOLLHUP events.

* Tue Jan 23 2018 shuxin.zheng@qq.com 3.3.0-60-20180123.16
- master & master tools: support version manager

* Fri Jan 19 2018 shuxin.zheng@qq.com 3.3.0-59-20180119.10
- fiber: remove valgrind debug by default in Makefile of fiber

* Fri Jan 19 2018 shuxin.zheng@qq.com 3.3.0-58-20180119.10
- rpm shell: don't use systemd

* Tue Jan 16 2018 shuxin.zheng@qq.com 3.3.0-57-20180116.14
- fiber_cpp: remove FIBER_API in including headers

* Tue Jan 16 2018 shuxin.zheng@qq.com 3.3.0-56-20180116.14
- increase version for building rpm

* Tue Jan 16 2018 shuxin.zheng@qq.com 3.3.0-55-20180116.11
- acl_master: don't transfer parsed addrs to children
- acl_udp_server: fixed one bug which can't feel the changing of network IP

* Fri Jan 05 2018 shuxin.zheng@qq.com 3.3.0-54-20180105.13
- acl_master: fixed one bug in masetr_api.cpp for checking command path, so
just using ACL_MASETR_SERV::path, and ACL_MASETR_SERV::command was removed.

* Thu Jan 04 2018 shuxin.zheng@qq.com 3.3.0-53-20180104.13
- add service_guard tool for master

* Wed Jan 03 2018 shuxin.zheng@qq.com 3.3.0-52-20180103.23
- one compile error on Centos6.4

* Wed Jan 03 2018 shuxin.zheng@qq.com 3.3.0-51-20180103.23
- add master_guard tool for master

* Tue Jan 02 2018 shuxin.zheng@qq.com 3.3.0-50-20180102.18
- acl_udp_server.c: can exit gracefully
- set ulimit -n in master's shell

* Thu Dec 28 2017 shuxin.zheng@qq.com 3.3.0-49-20171228.17
- fiber can support FreeBSD
- add replace param in http_header::add_entry

* Mon Dec 25 2017 shuxin.zheng@qq.com 3.3.0-48-20171226.16
- master: fixed one urgent bug in master which will make master crashed

* Fri Dec 22 2017 shuxin.zheng@qq.com 3.3.0-46-20171222.16
- fiber: fixed one bug in event_prepare of event.c

* Fri Dec 22 2017 shuxin.zheng@qq.com 3.3.0-45-20171222.16
- master restart service ok by remote command

* Fri Dec 22 2017 shuxin.zheng@qq.com 3.3.0-44-20171222.15
- fixed compiling error on gcc4.1

* Fri Dec 22 2017 shuxin.zheng@qq.com 3.3.0-43-20171222.14
- fixed master's bug when reload services

* Fri Dec 15 2017 shuxin.zheng@qq.com 3.3.0-42-20171215.11
- test multithreads writing to mbox

* Fri Dec 08 2017 shuxin.zheng@qq.com 3.3.0-41-20171208.10
- fixed one compile error

* Fri Dec 08 2017 shuxin.zheng@qq.com 3.3.0-40-20171208.09
- changed charset from gbk to utf-8 for all service's configure files

* Tue Dec 05 2017 shuxin.zheng@qq.com 3.3.0-39-20171205.15
- master's configure main.cf: changed service_throttle_time from 60s to 10s

* Tue Dec 05 2017 shuxin.zheng@qq.com 3.3.0-38-20171205.12
- master_ctld's configure was updated
- fixed some bugs in redis module for supporting binary data

* Tue Nov 28 2017 shuxin.zheng@qq.com 3.3.0-37-20171128.12
- update package version

* Thu Nov 16 2017 shuxin.zheng@qq.com 3.3.0-36-20171116.12
- compiling error on Centos6.x

* Thu Nov 16 2017 shuxin.zheng@qq.com 3.3.0-35-20171116.11
- add master_ctl into rpm package

* Mon Nov 13 2017 shuxin.zheng@qq.com 3.3.0-34-20171113.09
- acl_udp_server & master upgrade

* Fri Oct 27 2017 shuxin.zheng@qq.com 3.3.0-33-20171027.14
- fixed one compiling error

* Fri Oct 27 2017 shuxin.zheng@qq.com 3.3.0-32-20171027.13
- fixed one bug in fiber that int maybe overflow

* Mon Oct 16 2017 shuxin.zheng@qq.com 3.3.0-31-20171016.10
- auto compiling fiber module for Linux

* Tue Oct 10 2017 shuxin.zheng@qq.com 3.3.0-30-20171010.14
- fiber_mutex: fixed bugs

* Tue Oct 10 2017 shuxin.zheng@qq.com 3.3.0-29-20171010.11
- fiber_mutex: fixed one bug when thread_safe is true

* Sat Oct 07 2017 shuxin.zheng@qq.com 3.3.0-28-20171007.23
- add event_mutex in lib_acl_cpp

* Fri Sep 29 2017 shuxin.zheng@qq.com 3.3.0-27-20170929.23
- fiber_mutex: when blocked by thread mutex, the current fiber will be swapout

* Fri Sep 29 2017 shuxin.zheng@qq.com 3.3.0-26-20170929.18
- version: upgrade version to 3.3.0-26

* Fri Sep 29 2017 shuxin.zheng@qq.com 3.3.0-25-20170929.17
- version: upgrade version to 3.3.0-25

* Thu Sep 28 2017 shuxin.zheng@qq.com 3.3.0-23-20170928.12
- valgrind: free global objects so valgrind no reporting error when process exiting

* Sat Sep 23 2017 shuxin.zheng@qq.com 3.3.0-22-20170923.19
- fiber: hook mkdir/stat/lstat/fstat 

* Fri Sep 22 2017 shuxin.zheng@qq.com 3.3.0-21-20170922.18
- acl_master: restructure web managing module.

* Thu Sep 21 2017 shuxin.zheng@qq.com 3.3.0-20-20170921.16
- rpm: add version to 3.3.0-20

* Thu Sep 21 2017 shuxin.zheng@qq.com 3.3.0-19-20170921.16
- bitmap: some method maybe collision with some macro on some OS

* Thu Sep 21 2017 shuxin.zheng@qq.com 3.3.0-18-20170921.15
- event: fixed bug in events timer

* Wed Sep 20 2017 shuxin.zheng@qq.com 3.3.0-17-20170920.17
- fiber: hook_net.c supports epoll_create1 API

* Wed Sep 20 2017 shuxin.zheng@qq.com 3.3.0-16-20170920.17
- add one trying for checking fd's type

- Just increase version
* Tue Sep 19 2017 shuxin.zheng@qq.com 3.3.0-15-20170919.18
- Just increase version

* Tue Sep 19 2017 shuxin.zheng@qq.com 3.3.0-14-20170919.17
- Fixed one bug in acl_udp_server.c when sending status to acl_master

* Tue Sep 19 2017 shuxin.zheng@qq.com 3.3.0-13-20170919.13
- Add ci support for gitlab
- Add timer trigger
