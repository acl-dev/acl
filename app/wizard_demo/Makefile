.PHONY = all clean
all:
	@(cd httpd_upload; make)
	@(cd httpd_download; make)

clean:
	@(cd httpd_upload; make clean)
	@(cd httpd_download; make clean)

rb rebuild: clean all
