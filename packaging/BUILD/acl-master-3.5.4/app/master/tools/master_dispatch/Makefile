.PHONEY = all clean cl rebuild rb

all:
	@(cd server; make)
	@(cd manager; make)

clean cl:
	@(cd server; make clean)
	@(cd manager; make clean)

rebuild rb: cl all
