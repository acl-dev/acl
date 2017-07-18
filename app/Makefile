.PHONY = all clean rb rebuild

all:
	@(cd master/daemon; make)
	@(cd master_dispatch; make)
	@(cd wizard; make)
	@(cd redis_tools; make)
	@(cd http_logger; make)
	@(cd wizard_demo; make)
clean:
	@(cd master/daemon; make clean)
	@(cd master_dispatch; make clean)
	@(cd wizard; make clean)
	@(cd redis_tools; make clean)
	@(cd http_logger; make clean)
	@(cd wizard_demo; make clean)
