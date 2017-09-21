Summary:        The powerful c/c++ library and server framework
Name:           acl-libs
Version:        3.3.0
Release:        18
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
