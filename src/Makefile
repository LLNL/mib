MAKE	= /usr/bin/make
MPICC	= /usr/bin/mpicc
CC	= /usr/bin/gcc

CFLAGS += -Wextra -g -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

HEADERS = miberr.h mpi_wrap.h sys_wrap.h options.h mib_timer.h slurm.h 
SOURCES = mib.c mpi_wrap.c sys_wrap.c miberr.c options.c mib_timer.c slurm.c
OBJECTS = $(SOURCES:.c=.o)

all:  mib mib-serial doc tools

mib: $(SOURCES) $(HEADERS)
	$(MPICC) $(CFLAGS) $(SOURCES) -o $@
mib-serial: $(SOURCES) $(HEADERS)
	$(CC) -DMPI_NOT_INSTALLED $(CFLAGS) $(SOURCES) -o $@

.PHONY = doc tools
doc: 
	$(MAKE) -C doc 

tools:
	$(MAKE) -C tools

clean: 
	$(MAKE) -C doc clean
	$(MAKE) -C tools clean
	rm -f *.o mib mib-serial *~
