# change application name here (executable output name)
TARGET=orch90.dll
 
# compiler
CC=gcc
# linker
LD=gcc
# debug
DEBUG=#-g
# optimisation
OPT=-O2
# warnings
WARN=#-Wall
# Include Path 
INCDIR=# -I /usr/include/boost/predef/os
# Obj Dir
ODIR = obj

PTHREAD=#-pthread

SDLINC=`sdl2-config --cflags`
AGARINC=`agar-config --cflags`
IDIR=$(INCDIR) $(SDLINC) $(AGARINC)
 
CCFLAGS=$(IDIR) $(DEBUG) $(OPT) $(WARN) -fPIC
 
SDLLIB=`sdl2-config --libs`
AGARLIB=`agar-config --libs`
LIBS=$(SDLLIB) $(AGARLIB) -

LDFLAGS=-shared $(PTHREAD) -s -Wl,--out-implib,liborch90.a,--subsystem,console

UNUSEDOBJS= 

_OBJS=orch90.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

orch90.dll: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@ 
    
$(ODIR)/orch90.o: orch90.c orch90.h
	$(CC) -c $(CCFLAGS) $< -o $@

#$(ODIR)/fileops.o: fileops.c fileops.h
#	$(CC) -c $(CCFLAGS) $< -o $@

clean:
	rm -f $(ODIR)/*.o $(TARGET) orch90.dll liborch90.a

install:
	cp orch90.dll ../CoCo/libs

test:
	.\testlib.exe orch90.dll ModuleName