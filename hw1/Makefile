GCC = arm-unknown-linux-gnu-gcc
GNU_LIB_PATH = /opt/arm-unknown-linux-gnu/arm-unknown-linux-gnu/lib/
GNU_INC_PATH = /opt/arm-unknown-linux-gnu/arm-unknown-linux-gnu/include/
INC_PATH = /home/vbscript/microtime/linux/include/

CFLAG = -L $(GNU_LIB_PATH) -I $(GNU_INC_PATH) -I $(INC_PATH)

all: hw1

HW1: hw1.c
	$(GCC) -o $@ $(CFLAG) $<

clean:
	rm hw1
