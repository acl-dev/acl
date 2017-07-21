.PHONY = all clean rb rebuild

all:
	@(cd daemon; make)
	@(cd tools; make)

cl clean:
	@(cd daemon; make clean)
	@(cd tools; make clean)

rb rebuild: cl all
