obj-m := mydev.o

CC = arm-linux-gnueabihf-gcc

# KDIR = /lib/modules/$(shell uname -r)/build

# LIB = /usr/src/linux-headers-5.13.0-37-generic/lib

# INC = /usr/src/linux-headers-5.13.0-37-generic/include

CFLAG = --static

all: writer reader
	make -C ../  M=$(PWD) modules

clean:
	make -C ../  M=$(PWD) clean
	rm writer reader

mydev.o: lab4.c
	$(CC) -c -o $@ $< 

writer: writer.c
	$(CC) $(CFLAG) -o $@ $<

reader: reader.cpp
	$(CC) $(CFLAG) -o $@ $<
