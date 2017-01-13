.PHONEY = all clean cl rb rebuild

all:
	@(cd fiber1; make)
	@(cd fiber2; make)
#	@(cd fiber3; make)
	@(cd fiber4; make)
#	@(cd client2; make)
#	@(cd server2; make)
	@(cd bio_client; make)
	@(cd bio_server; make)

cl clean:
	@(cd fiber1; make clean)
	@(cd fiber2; make clean)
	@(cd fiber3; make clean)
	@(cd fiber4; make clean)
	@(cd server2; make clean)
	@(cd client2; make clean)
	@(cd bio_client; make clean)
	@(cd bio_server; make clean)

rb rebuild: cl all
