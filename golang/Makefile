GOPATH := $(CURDIR)

all: echo web test

echo:
	@go build echo

web:
	@go build web

test:
	@go build test

cl clean:
	@rm -f echo web test

rb rebuild: cl all
