/*****************************************************************************\
 *  $Id: mkfls.c,v 1.2 2005/11/30 20:24:57 auselton Exp $
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
 *  UCRL-CODE-2006-xxx.
 *  
 *  This file is part of Mib, an MPI-based parallel I/O benchamrk
 *  For details, see <http://www.llnl.gov/linux/mib/>.
 *  
 *  Mib is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *  
 *  Mib is distributed in the hope that it will be useful, but WITHOUT 
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
 *  for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with Mib; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/


/*
#include <getopt.h>
#include <mpi.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h> * malloc *
#include <stdarg.h> * va_start *
#include <assert.h>
#include <math.h>
*/
#include <stdio.h>      /* FILE, fopen, etc. */
#include <sys/types.h>  /* open, etc */
#include <sys/stat.h>   /* open, etc */
#include <fcntl.h>      /* open, etc */
#include <unistd.h>     /* unlink, ssize_t */
#include <errno.h>
#include "mib.h"
#include "mpi_wrap.h"
#include "options.h"
#include "mib_timer.h"
#include "sys_wrap.h"
#include "mib_timer.h"

void init_filenames();
Results *init_results();
void write_test(Results *results);
char *fill_buff();
void init_status(char *str);
void status(double time);
void read_test(Results *results);
Results *data_reduction(Results *results);
void report(Results *results);

extern Options *opts;

int
main( int argc, char *argv[] )
{
  Results *results;
  Results *reduced;
  int size;
  int rank;

  mpi_init( &argc, &argv );
  mpi_comm_size(MPI_COMM_WORLD, &size );
  mpi_comm_rank(MPI_COMM_WORLD, &rank );
  opts = read_options(argc, argv, rank, size);
  init_filenames();
  /*
   *   use the documented ioctls here to create files with suitably
   * smoothly distributed allocations.
   */
  Free_Opts();
  mpi_finalize();
}

void
init_filenames()
{
  int read_rank = (opts->rank + (opts->size/2)) % opts->size;

  snprintf(opts->write_target, MAX_BUF, "%s/mibData.%08d", opts->testdir, opts->rank);
  snprintf(opts->read_target, MAX_BUF, "%s/mibData.%08d", opts->testdir, read_rank);
}


