TARGET = build/x
LIBS = -lm -lz
CC = gcc
CFLAGS = -g -Werror -O2 -fno-strict-aliasing -Wall -D__DIR__=$(shell pwd) -fPIC

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst src/%.c, src/%.o, $(wildcard src/*.c))
ACTUAL = $(patsubst src/%.o, build/%.o, $(OBJECTS))
HEADERS = $(wildcard src/*.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $(subst src,build,$@)

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(ACTUAL) -Wall $(LIBS) -o $@

py: $(OBJECTS)
	ar rcs build/libxandri.a build/blob.o build/util.o
	-rm -rf build/lib.*
	-rm -rf build/temp.*
	python3 setup.py build
	cp build/lib*/_xandri* py_interface

clean:
	-rm -f *.o
	-rm -f $(TARGET)

test:
	rm -rf data
	ingest/cmds.sh
	./x serve
big:
	rm -rf data
	ingest/big.sh
	./x serve
test2:
	rm -rf data
	./x blob create ssi71
	./x blob create-index ssi71 time -b 4 ingest/index.bin
	./x blob query-index ssi71 time 15242 3600000 200
	./x blob create-key ssi71 altitude_barometer time -t float -b 4 ingest/alts.bin
	./x blob create-key ssi71 current_total time -t float -b 4 ingest/current.bin
	./x serve
open:
	cd src; vi -p  xandri.c util.h util.c 
