PWD := $(shell pwd) 
CFLAGS ?=	-Wall -ggdb -W -I$(PWD)
CC ?= gcc
LDFLAGS ?=
PREFIX ?=	/usr/local/webbench
VERSION =1.5
TARGET := ImageMake


all:   TARGET
TARGET:  crc32.o imagemake.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) imagemake.o crc32.o

%.o : %.c 
	$(CC) $(CFLAGS) $(LDFLAGS) -c $<

clean:
	-rm -f *.o
	-rm -rf $(TARGET)

pack:
	./$(TARGET) -a app.bin -b boot.bin -o output.bin

.PHONY: clean alls pack