##########################################################
#
#            make PKG_NAME=acl-libs
#            make PKG_NAME=acl-master
#
#########################################################
ifndef $(PKG_NAME)
	PKG_NAME := acl-libs
endif 
PKG_VERSION = $(shell grep '^Version:' $(PKG_NAME).spec | awk '{print $$2}')

PWD = $(shell pwd)

all: libs

tar:
	@./control pack ${PKG_NAME}

libs: tar
	@mkdir -p {BUILD,BUILDROOT,SRPMS,SPECS}
	@rpmbuild --define "_topdir ${PWD}" --define "_sourcedir ${PWD}" --define "_rpmdir ${PWD}" -bb $(PKG_NAME).spec
	@echo Binary PRM for $(PKG_NAME) has been created under ${PWD}/`rpm --eval '%{_arch}'`

clean cl:
	@rm -rf $(PKG_NAME)-$(PKG_VERSION)
	@rm -rf *.tar.gz
	@rm -rf BUILD BUILDROOT SRPMS SPECS `rpm --eval '%{_arch}'`
