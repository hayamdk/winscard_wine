PROGRAM = winscard.dll
OBJS = mymalloc.o winscard.o
SPECS = winscard.spec

WINECC := winegcc
CC := gcc

ARCH = -m32
#ARCH =
CFLAGS = $(ARCH) -Wall -fPIC -O2 -I /usr/include/PCSC
LDFLAGS = $(ARCH) -shared -ldl

$(PROGRAM): $(OBJS) $(SPECS)
	$(WINECC) $(OBJS) $(SPECS) $(LDFLAGS) -o $(PROGRAM).so
	mv $(PROGRAM).so $(PROGRAM)

SUFFIXES: .o .c

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: clean

clean:
	rm -f $(PROGRAM) $(OBJS)
