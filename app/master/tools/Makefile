.PHONEY = all cl clean rb rebuild

all:
	@(cd master_ctl; make)
	@(cd master_ctld; make)
	@(cd master_dispatch; make)

cl clean:
	@(cd master_ctl; make clean)
	@(cd master_ctld; make clean)
	@(cd master_dispatch; make clean)

rb rebuild: cl all
