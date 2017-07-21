.PHONEY = all cl clean rb rebuild

all:
	@(cd master_ctl; make)
	@(cd master_ctld; make)

cl clean:
	@(cd master_ctl; make clean)
	@(cd master_ctld; make clean)

rb rebuild: cl all
