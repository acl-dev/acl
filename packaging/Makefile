PKG_NAME = acl-libs
PKG_VERSION = $(shell grep '^Version:' $(PKG_NAME).spec | awk '{print $$2}')

PWD = $(shell pwd)


all: rpm

tar:
	@./control pack

rpm: tar
	@mkdir -p {BUILD,BUILDROOT,SRPMS,SPECS}
	@rpmbuild --define "_topdir ${PWD}" --define "_sourcedir ${PWD}" --define "_rpmdir ${PWD}" -bb $(PKG_NAME).spec
	@echo Binary PRM for $(PKG_NAME) has been created under ${PWD}/`rpm --eval '%{_arch}'`

clean cl:
	@rm -rf $(PKG_NAME)-$(PKG_VERSION)
	@rm -rf $(PKG_NAME)-$(PKG_VERSION).tar.gz
	@rm -rf BUILD BUILDROOT SRPMS SPECS `rpm --eval '%{_arch}'`
