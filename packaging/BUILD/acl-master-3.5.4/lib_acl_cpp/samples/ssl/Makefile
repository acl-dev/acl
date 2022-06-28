.PHONY = all clean

all:
	@(cd ssl_client; make)
	@(cd ssl_server; make)
	@(cd ssl_client2; make)
	@(cd ssl_server2; make)
	@(cd https_client; make)
	@(cd https_server; make)
	@(cd https_proxy; make)

clean:
	@(cd ssl_client; make clean)
	@(cd ssl_server; make clean)
	@(cd ssl_client2; make clean)
	@(cd ssl_server2; make clean)
	@(cd https_client; make clean)
	@(cd https_server; make clean)
	@(cd https_proxy; make clean)

rb rebuild: clean all
