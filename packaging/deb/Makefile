ifndef $(PKG_NAME)
	PKG_NAME := acl-libs
endif

all: deb

deb:
	@chmod +x control
	@./control pack ${PKG_NAME}

clean cl:
	@rm -rf build
	@rm -f *.deb

