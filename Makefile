TARGET = x
LIBS = -lm -lz
CC = gcc
CFLAGS = -g -Werror -O2 -fno-strict-aliasing -Wall -D__DIR__=$(shell pwd)

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
OBJECTS += $(patsubst blob/%.c, blob/%.o, $(wildcard blob/*.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

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
