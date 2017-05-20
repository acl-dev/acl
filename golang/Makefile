GOPATH := $(CURDIR)

all: echo web

echo:
	@go build echo

web:
	@go build web

cl clean:
	@rm -f echo web

rb rebuild: cl all
