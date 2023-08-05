base_path = ../../..
include ../../Makefile.in

PROG = json
#CFLAGS += -I/home/zsx/download/json/simdjson/include
CFLAGS += -DHAS_CJSON -DHAS_YYJSON -DHAS_SIMDJSON
EXTLIBS = -lyyjson -lsimdjson -lcjson -Wl,-rpath,/usr/local/lib
