.PHONY: ketama_test clean

pwd:=$(shell pwd)
CFLAGS:=--std=c99 -lssl -lcrypto -O3 -g

all: test

test: ketama_test
	@cd t && ./test.sh
	@cd t && ./downed_server_test.sh

ketama_test:
	gcc $(pwd)/src/ketama.c $(pwd)/t/ketama_test.c $(CFLAGS) -o t/ketama_test -I$(pwd)/src/

clean:
	rm -f ketama_test
