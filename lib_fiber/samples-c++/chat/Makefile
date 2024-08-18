.PHONEY = all clean cl rebuild rb

all:
	@(cd server; make)
	@(cd client; make)
	@(cd reader; make)
	@(cd writer; make)
	@(cd connect; make)
	@(cd demo; make)

clean cl:
	@(cd server; make clean)
	@(cd client; make clean)
	@(cd reader; make clean)
	@(cd writer; make clean)
	@(cd connect; make clean)
	@(cd demo; make clean)

rebuild rb: cl all
