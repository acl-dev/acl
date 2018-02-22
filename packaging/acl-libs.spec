Summary:        The powerful c/c++ library and server framework
Name:           acl-libs
Version:        3.3.0
Release:        74
Group:          System/Libs
License:        IBM
URL:            http://cdnlog-web.qiyi.domain
Packager:       Zhang Qiang <qiangzhang@qiyi.com>
Source2:        acl-master.json
BuildRoot:      %{_tmppath}/%{name}-%{version}-root
Source:         http://example.com/%{name}-%{version}.tar.gz

%define debug_package %{nil}
%description

One advanced C/C++ library for Linux/Mac/FreeBSD/Solaris(x86)/Windows/Android/IOS http://zsxxsz.iteye.com/.


%package -n acl-master
Summary: acl master framework
License: IBM
Group: System Environment/Tools
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description -n acl-master
acl master framework


%prep
%setup -q

%build

make build_one -j 32
make -C lib_fiber

%install

make packinstall  DESTDIR=$RPM_BUILD_ROOT
make -C lib_fiber packinstall  DESTDIR=$RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT/opt/soft/services/

install -m 644 %{SOURCE2} $RPM_BUILD_ROOT/opt/soft/services/

%clean
rm -rf %{buildroot}


%post -n acl-master
/sbin/chkconfig --add master

%preun -n acl-master
if [ "$1" = "0" ]; then
    service master stop >/dev/null 2>&1 ||:
    /sbin/chkconfig --del master
    sleep 1
fi

%postun -n acl-master
if [ "$1" -ge "1" ]; then
    # TODO: upgrade should be support
    sleep 1
    service master masterrestart > /dev/null 2>&1 ||:
fi


%files
%defattr(-,root,root,-)
# TODO: should be renamed
%{_bindir}/acl_master
%{_includedir}/acl-lib/acl
%{_includedir}/acl-lib/acl_cpp
%{_includedir}/acl-lib/fiber
%{_includedir}/acl-lib/protocol
/usr/lib/libacl_all.a
/usr/lib/libfiber.a
/usr/lib/libfiber_cpp.a

%files -n acl-master
%defattr(-,root,root)
/opt/soft
/etc/init.d/master

%changelog

* Thu Feb 22 2018 zhengshuxin@qiyi.com 3.3.0-74-20180222.10
- master_ctld: add UNIX domain listening avoiding be blocked by iptables.
- gson: std::map object can also be optional in json serialization.

* Mon Feb 12 2018 zhengshuxin@qiyi.com 3.3.0-73-20180212.15
- master_ctld: support GET for checking port if service is aliving

* Mon Feb 12 2018 zhengshuxin@qiyi.com 3.3.0-71-20180212.14
- remove daemon from master's service

* Wed Jan 31 2018 zhengshuxin@qiyi.com 3.3.0-69-20180131.12
- master_guard: can't count fds on Cendos5.x
- master_monitor: can't get real version for "-v"
- acl_master: should override check_xxx when reloading service configure

* Tue Jan 30 2018 zhengshuxin@qiyi.com 3.3.0-68-20180130.09
- master_guard: invalid exiting status

* Mon Jan 29 2018 zhengshuxin@qiyi.com 3.3.0-67-20180129.18
- acl_master: when service hasn't master_notify_addr then using the default
  path as /opt/soft/acl-master/var/public/monitor.sock
- acl_master: when service crashed, one message will be delivered without
  master_notify_recipients.

* Mon Jan 29 2018 zhengshuxin@qiyi.com 3.3.0-66-20180129.17
- service_guard: support tcp service by using acl::tcp_ipc class
- master_guard: using acl::tcp_ipc when using tcp connection

* Thu Jan 25 2018 zhengshuxin@qiyi.com 3.3.0-65-20180125.12
- master_ctld: add default checking items.

* Thu Jan 25 2018 zhengshuxin@qiyi.com 3.3.0-64-20180125.11
- master_guard/service_guard: support memory usage checking

* Wed Jan 24 2018 zhengshuxin@qiyi.com 3.3.0-63-20180124.23
- master tools can support new protocols

* Wed Jan 24 2018 zhengshuxin@qiyi.com 3.3.0-62-20180124.16
- add master_monitor tool of acl_master

* Tue Jan 23 2018 zhengshuxin@qiyi.com 3.3.0-61-20180123.19
- epoll_event.c should handle EPOLLERR|EPOLLHUP events.

* Tue Jan 23 2018 zhengshuxin@qiyi.com 3.3.0-60-20180123.16
- master & master tools: support version manager

* Fri Jan 19 2018 zhengshuxin@qiyi.com 3.3.0-59-20180119.10
- fiber: remove valgrind debug by default in Makefile of fiber

* Fri Jan 19 2018 zhengshuxin@qiyi.com 3.3.0-58-20180119.10
- rpm shell: don't use systemd

* Tue Jan 16 2018 zhengshuxin@qiyi.com 3.3.0-57-20180116.14
- fiber_cpp: remove FIBER_API in including headers

* Tue Jan 16 2018 zhengshuxin@qiyi.com 3.3.0-56-20180116.14
- increase version for building rpm

* Tue Jan 16 2018 zhengshuxin@qiyi.com 3.3.0-55-20180116.11
- acl_master: don't transfer parsed addrs to children
- acl_udp_server: fixed one bug which can't feel the changing of network IP

* Fri Jan 05 2018 zhengshuxin@qiyi.com 3.3.0-54-20180105.13
- acl_master: fixed one bug in masetr_api.cpp for checking command path, so
just using ACL_MASETR_SERV::path, and ACL_MASETR_SERV::command was removed.

* Thu Jan 04 2018 zhengshuxin@qiyi.com 3.3.0-53-20180104.13
- add service_guard tool for master

* Wed Jan 03 2018 zhengshuxin@qiyi.com 3.3.0-52-20180103.23
- one compile error on Centos6.4

* Wed Jan 03 2018 zhengshuxin@qiyi.com 3.3.0-51-20180103.23
- add master_guard tool for master

* Tue Jan 02 2018 zhengshuxin@qiyi.com 3.3.0-50-20180102.18
- acl_udp_server.c: can exit gracefully
- set ulimit -n in master's shell

* Thu Dec 28 2017 zhengshuxin@qiyi.com 3.3.0-49-20171228.17
- fiber can support FreeBSD
- add replace param in http_header::add_entry

* Mon Dec 25 2017 zhengshuxin@qiyi.com 3.3.0-48-20171226.16
- master: fixed one urgent bug in master which will make master crashed

* Fri Dec 22 2017 zhengshuxin@qiyi.com 3.3.0-46-20171222.16
- fiber: fixed one bug in event_prepare of event.c

* Fri Dec 22 2017 zhengshuxin@qiyi.com 3.3.0-45-20171222.16
- master restart service ok by remote command

* Fri Dec 22 2017 zhengshuxin@qiyi.com 3.3.0-44-20171222.15
- fixed compiling error on gcc4.1

* Fri Dec 22 2017 zhengshuxin@qiyi.com 3.3.0-43-20171222.14
- fixed master's bug when reload services

* Thu Dec 15 2017 zhengshuxin@qiyi.com 3.3.0-42-20171215.11
- test multithreads writing to mbox

* Wed Dec 08 2017 zhengshuxin@qiyi.com 3.3.0-41-20171208.10
- fixed one compile error

* Wed Dec 08 2017 zhengshuxin@qiyi.com 3.3.0-40-20171208.09
- changed charset from gbk to utf-8 for all service's configure files

* Wed Dec 05 2017 zhengshuxin@qiyi.com 3.3.0-39-20171205.15
- master's configure main.cf: changed service_throttle_time from 60s to 10s

* Wed Dec 05 2017 zhengshuxin@qiyi.com 3.3.0-38-20171205.12
- master_ctld's configure was updated
- fixed some bugs in redis module for supporting binary data

* Wed Nov 28 2017 zhengshuxin@qiyi.com 3.3.0-37-20171128.12
- update package version

* Sat Nov 16 2017 zhengshuxin@qiyi.com 3.3.0-36-20171116.12
- compiling error on Centos6.x

* Sat Nov 16 2017 zhengshuxin@qiyi.com 3.3.0-35-20171116.11
- add master_ctl into rpm package

* Sat Nov 13 2017 zhengshuxin@qiyi.com 3.3.0-34-20171113.09
- acl_udp_server & master upgrade

* Sat Oct 27 2017 zhengshuxin@qiyi.com 3.3.0-33-20171027.14
- fixed one compiling error

* Sat Oct 27 2017 zhengshuxin@qiyi.com 3.3.0-32-20171027.13
- fixed one bug in fiber that int maybe overflow

* Sat Oct 16 2017 zhengshuxin@qiyi.com 3.3.0-31-20171016.10
- auto compiling fiber module for Linux

* Sat Oct 10 2017 zhengshuxin@qiyi.com 3.3.0-30-20171010.14
- fiber_mutex: fixed bugs

* Sat Oct 10 2017 zhengshuxin@qiyi.com 3.3.0-29-20171010.11
- fiber_mutex: fixed one bug when thread_safe is true

* Sat Oct 07 2017 zhengshuxin@qiyi.com 3.3.0-28-20171007.23
- add event_mutex in lib_acl_cpp

* Sat Sep 29 2017 zhengshuxin@qiyi.com 3.3.0-27-20170929.23
- fiber_mutex: when blocked by thread mutex, the current fiber will be swapout

* Sat Sep 29 2017 zhengshuxin@qiyi.com 3.3.0-26-20170929.18
- version: upgrade version to 3.3.0-26

* Sat Sep 29 2017 zhengshuxin@qiyi.com 3.3.0-25-20170929.17
- version: upgrade version to 3.3.0-25

* Sat Sep 28 2017 zhengshuxin@qiyi.com 3.3.0-23-20170928.12
- valgrind: free global objects so valgrind no reporting error when process exiting

* Sat Sep 23 2017 zhengshuxin@qiyi.com 3.3.0-22-20170923.19
- fiber: hook mkdir/stat/lstat/fstat 

* Fri Sep 22 2017 zhengshuxin@qiyi.com 3.3.0-21-20170922.18
- acl_master: restructure web managing module.

* Thu Sep 21 2017 zhengshuxin@qiyi.com 3.3.0-20-20170921.16
- rpm: add version to 3.3.0-20

* Thu Sep 21 2017 zhengshuxin@qiyi.com 3.3.0-19-20170921.16
- bitmap: some method maybe collision with some macro on some OS

* Thu Sep 21 2017 zhengshuxin@qiyi.com 3.3.0-18-20170921.15
- event: fixed bug in events timer

* Wed Sep 20 2017 zhengshuxin@qiyi.com 3.3.0-17-20170920.17
- fiber: hook_net.c supports epoll_create1 API

* Wed Sep 20 2017 zhengshuxin@qiyi.com 3.3.0-16-20170920.17
- add one trying for checking fd's type

- Just increase version
* Tue Sep 19 2017 zhengshuxin@qiyi.com 3.3.0-15-20170919.18
- Just increase version

* Tue Sep 19 2017 zhengshuxin@qiyi.com 3.3.0-14-20170919.17
- Fixed one bug in acl_udp_server.c when sending status to acl_master

* Tue Sep 19 2017 zhengshuxin@qiyi.com 3.3.0-13-20170919.13
- Add ci support for gitlab
- Add timer trigger
