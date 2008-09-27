CC	= /usr/bin/mpicc

# BG/L
#CC	= /usr/local/bin/mpgcc

CFLAGS += -Wall -g -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DWITH_MPI

HEADERS = miberr.h mpi_wrap.h sys_wrap.h options.h mib_timer.h 
SOURCES = mib.c mpi_wrap.c sys_wrap.c options.c mib_timer.c
OBJECTS = $(SOURCES:.c=.o)

all:  mib

mib: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

clean: 
	rm -f *.o mib *~

$(OBJECTS): $(HEADERS)
