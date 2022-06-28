.PHONY = all clean

all:
	@(cd http; make)
	@(cd httpd; make)
	@(cd http_gate; make)
	@(cd http_probe; make)
	@(cd ping; make)
	@(cd smtp_client; make)

clean:
	@(cd http; make clean)
	@(cd httpd; make clean)
	@(cd http_gate; make clean)
	@(cd http_probe; make clean)
	@(cd ping; make clean)
	@(cd smtp_client; make clean)
