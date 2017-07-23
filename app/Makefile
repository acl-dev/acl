.PHONY = all clean rb rebuild

all:
	@(cd master; make)
	@(cd wizard; make)
	@(cd redis_tools; make)
	@(cd http_logger; make)
	@(cd wizard_demo; make)
cl clean:
	@(cd master; make clean)
	@(cd wizard; make clean)
	@(cd redis_tools; make clean)
	@(cd http_logger; make clean)
	@(cd wizard_demo; make clean)

rb rebuild: cl all
