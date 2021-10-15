XT_PRG_PREFIX = mipsel-linux-gnu-
CC = $(XT_PRG_PREFIX)gcc
LD = $(XT_PRG_PREFIX)ld


ifneq ($(wildcard /usr/bin/umps3),)
	UMPS3_DIR_PREFIX = /usr
else
	UMPS3_DIR_PREFIX = /usr/local
endif

UMPS3_DATA_DIR = $(UMPS3_DIR_PREFIX)/share/umps3
UMPS3_INCLUDE_DIR = $(UMPS3_DIR_PREFIX)/include/umps3
UPMS3_INCLUDE_DIR2 = $(UMPS3_DIR_PREFIX)/include

SRC_DIR = ../
PHASE1_DIR = $(SRC_DIR)/phase_1
PHASE2_DIR = $(SRC_DIR)/phase_2
PHASE3_DIR = $(SRC_DIR)/phase_3
TESTING_DIR = $(SRC_DIR)/resources
PROJ_INCLUDE = -I$(SRC_DIR) -I$(PHASE1_DIR) -I$(PHASE2_DIR) -I$(PHASE3_DIR)

CFLAGS_LANG = -ffreestanding
CFLAGS_MIPS = -mips1 -mabi=32 -mno-gpopt -G 0 -mno-abicalls -fno-pic -mfp32
CFLAGS = $(CFLAGS_LANG) $(CFLAGS_MIPS) -I$(UMPS3_INCLUDE_DIR) -I$(UMPS3_INCLUDE_DIR2) -Wall -O0


LDFLAGS = -G 0 -nostdlib -T $(UMPS3_DATA_DIR)/umpscore.ldscript


VPATH = $(UMPS3_DATA_DIR):$(SRC_DIR):$(PHASE1_DIR):$(PHASE2_DIR):$(PHASE3_DIR):$(TESTING_DIR)

.PHONY : all clean

all : kernel.core.umps

kernel.core.umps : kernel
	umps3-elf2umps -k $<

kernel : asl.o pcb.o initial.o scheduler.o crtso.o interrupts.o exceptions.o libumps.o pandos_libs.o vmSupport.o sysSupport.o initProc.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean :
	-rm -f *.o kernel kernel.*.umps ../resources/*.umps && clear

%.o : %.S
	$(CC) $(CFLAGS) -c -o $@ $<