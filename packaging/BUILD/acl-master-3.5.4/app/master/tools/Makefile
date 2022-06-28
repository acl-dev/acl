.PHONEY = all cl clean rb rebuild

all:
	@(cd lib_global; make)
	@(cd master_ctl; make)
	@(cd master_ctld; make)
	@(cd master_dispatch; make)
	@(cd master_guard; make)
	@(cd service_guard; make)
	@(cd master_monitor; make)

cl clean:
	@(cd lib_global; make clean)
	@(cd master_ctl; make clean)
	@(cd master_ctld; make clean)
	@(cd master_dispatch; make clean)
	@(cd master_guard; make clean)
	@(cd service_guard; make clean)
	@(cd master_monitor; make clean)

rb rebuild: cl all
