ifndef $(PKG_NAME)
	PKG_NAME := acl-libs
endif

all: pkg

pkg:
	@chmod +x control
	@./control pack ${PKG_NAME}

install-libs:
	sudo installer -pkg acl-libs-3.6.6-0-macos.pkg -target /

clean cl:
	@rm -rf build
	@rm -f *-macos.pkg

