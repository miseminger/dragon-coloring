OBJECTS=main.o
DEVICE  = msp430g2553
INSTALL_DIR=/home/madeline/ti/gcc

GCC_DIR =  $(INSTALL_DIR)/bin
SUPPORT_FILE_DIRECTORY = $(INSTALL_DIR)/include

CC      = $(GCC_DIR)/msp430-elf-gcc
GDB     = $(GCC_DIR)/msp430-elf-gdb

CFLAGS = -I $(SUPPORT_FILE_DIRECTORY) -mmcu=$(DEVICE) -g -ggdb
LFLAGS = -L $(SUPPORT_FILE_DIRECTORY) -T $(DEVICE).ld

all: ${OBJECTS}
	$(CC) $(CFLAGS) $(LFLAGS) $? -o main.elf 

debug: all
	$(GDB) main.elf
