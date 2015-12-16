.PHONY = all clean rb rebuild

all:
	@(cd http_logger; make)
	@(cd master_dispatch; make)
	@(cd redis_tools; make)
	@(cd wizard; make)
	@(cd wizard_demo; make)
clean:
	@(cd http_logger; make clean)
	@(cd master_dispatch; make clean)
	@(cd redis_tools; make clean)
	@(cd wizard; make clean)
	@(cd wizard_demo; make clean)
