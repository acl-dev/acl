include ./Makefile.in

PROG = json
CFLAGS += -DHAS_CJSON -DHAS_YYJSON -DHAS_SIMDJSON -DHAS_RAPIDJSON -DHAS_JTJSON
EXTLIBS = -lyyjson -lsimdjson -lcjson -Wl,-rpath,/usr/local/lib ./double-conversion.a
