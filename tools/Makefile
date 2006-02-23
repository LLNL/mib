#*****************************************************************************\
#*  $Id: Makefile,v 1.9 2005/11/30 20:24:57 auselton Exp $
#*****************************************************************************
#*  Copyright (C) 2001-2002 The Regents of the University of California.
#*  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
#*  Written by Andrew Uselton <uselton2@llnl.gov>
#*  UCRL-CODE-2006-xxx.
#*  
#*  This file is part of Mib, an MPI-based parallel I/O benchamrk
#*  For details, see <http://www.llnl.gov/linux/mib/>.
#*  
#*  Mib is free software; you can redistribute it and/or modify it under
#*  the terms of the GNU General Public License as published by the Free
#*  Software Foundation; either version 2 of the License, or (at your option)
#*  any later version.
#*  
#*  Mib is distributed in the hope that it will be useful, but WITHOUT 
#*  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
#*  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
#*  for more details.
#*  
#*  You should have received a copy of the GNU General Public License along
#*  with Mib; if not, write to the Free Software Foundation, Inc.,
#*  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
#*****************************************************************************/

MAKE=    /usr/bin/make

# These are for BGL
CC = mpgcc
CCFLAGS = -g -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

# These are for Gauss
# To find the compiler use "export PATH=/usr/local/intel/compiler90_64_023/bin:$PATH"
#CC = mpiicc
#CCFLAGS = -g -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

# These are for ALC
# To find the compiler use "export PATH=/usr/local/intel/compiler90_031/bin:$PATH"
#CC = mpicc
#CCFLAGS = -g -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

LDFLAGS =
LIBDIRS = 
LIBS = 
INCDIR = 
DEBUG = # Put 'DEBUG="-DNDEBUG=1"' on the command line if you want 
        # debugging off

# I'd prefer getting a dependence between source files and headers
HEADERS = miberr.h mpi_wrap.h sys_wrap.h options.h mib_timer.h
SOURCES = mib.c mpi_wrap.c sys_wrap.c miberr.c options.c mib_timer.c
OBJECTS = $(SOURCES:.c=.o)

all: 
	@ echo "Nothing to make in the tools directory"


.c.o:
	$(CC) $(CCFLAGS) $(DEBUG) -c $<  $(INCDIR)

clean: 
	rm -f *.o *~
