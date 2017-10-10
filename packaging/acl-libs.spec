Summary:        The powerful c/c++ library and server framework
Name:           acl-libs
Version:        3.3.0
Release:        30
Group:          System/Libs
License:        IBM
URL:            http://cdnlog-web.qiyi.domain
Packager:       Zhang Qiang <qiangzhang@qiyi.com>
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
%clean
rm -rf %{buildroot}


%post -n acl-master
/sbin/chkconfig --add master

%preun -n acl-master
if [ "$1" = "0" ]; then
    service master stop >/dev/null 2>&1 ||:
    /sbin/chkconfig --del master
fi

%postun -n acl-master
if [ "$1" -ge "1" ]; then
    # TODO: upgrade should be support
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
* Sat Oct 10 2018 zhengshuxin@qiyi.com 3.3.0-30-20171010.14
- fiber_mutex: fixed bugs

* Sat Oct 10 2018 zhengshuxin@qiyi.com 3.3.0-29-20171010.11
- fiber_mutex: fixed one bug when thread_safe is true

* Sat Oct 07 2018 zhengshuxin@qiyi.com 3.3.0-28-20171007.23
- add event_mutex in lib_acl_cpp

* Sat Sep 29 2018 zhengshuxin@qiyi.com 3.3.0-27-20170929.23
- fiber_mutex: when blocked by thread mutex, the current fiber will be swapout

* Sat Sep 29 2018 zhengshuxin@qiyi.com 3.3.0-26-20170929.18
- version: upgrade version to 3.3.0-26

* Sat Sep 29 2018 zhengshuxin@qiyi.com 3.3.0-25-20170929.17
- version: upgrade version to 3.3.0-25

* Sat Sep 28 2018 zhengshuxin@qiyi.com 3.3.0-23-20170928.12
- valgrind: free global objects so valgrind no reporting error when process exiting

* Sat Sep 23 2018 zhengshuxin@qiyi.com 3.3.0-22-20170923.19
- fiber: hook mkdir/stat/lstat/fstat 

* Fri Sep 22 2018 zhengshuxin@qiyi.com 3.3.0-21-20170922.18
- acl_master: restructure web managing module.

* Thu Sep 21 2018 zhengshuxin@qiyi.com 3.3.0-20-20170921.16
- rpm: add version to 3.3.0-20

* Thu Sep 21 2018 zhengshuxin@qiyi.com 3.3.0-19-20170921.16
- bitmap: some method maybe collision with some macro on some OS

* Thu Sep 21 2018 zhengshuxin@qiyi.com 3.3.0-18-20170921.15
- event: fixed bug in events timer

* Wed Sep 20 2018 zhengshuxin@qiyi.com 3.3.0-17-20170920.17
- fiber: hook_net.c supports epoll_create1 API

* Wed Sep 20 2018 zhengshuxin@qiyi.com 3.3.0-16-20170920.17
- add one trying for checking fd's type

- Just increase version
* Tue Sep 19 2017 zhengshuxin@qiyi.com 3.3.0-15-20170919.18
- Just increase version

* Tue Sep 19 2017 zhengshuxin@qiyi.com 3.3.0-14-20170919.17
- Fixed one bug in acl_udp_server.c when sending status to acl_master

* Tue Sep 19 2017 zhengshuxin@qiyi.com 3.3.0-13-20170919.13
- Add ci support for gitlab
- Add timer trigger
