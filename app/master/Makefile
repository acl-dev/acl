.PHONY = all clean rb rebuild install

all:
	@(cd daemon; make)
	@(cd tools; make)

cl clean:
	@(cd daemon; make clean)
	@(cd tools; make clean)

install:
	@(cd daemon; make install)
	@(cd tools/master_ctl; make install)
	@(cd tools/master_ctld; make install)
	@(cd tools/master_monitor; make install)

rb rebuild: cl all
