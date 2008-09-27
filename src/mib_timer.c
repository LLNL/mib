/*****************************************************************************\
 *  $Id: mib_timer.c,v 1.3 2005/11/30 20:24:57 auselton Exp $
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
 *  UCRL-CODE-222725
 *  
 *  This file is part of Mib, an MPI-based parallel I/O benchamrk
 *  For details, see <http://www.llnl.gov/linux/mib/>.
 *  
 *  Mib is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License (as published by the Free
 *  Software Foundation version 2, dated June 1991.
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
#include <stdio.h>
#include <time.h>
#include <string.h>     /* strncpy */
#include "mpi_wrap.h"
#include "miberr.h"
#include "options.h"
#include "mib_timer.h"

/*
 *   _zero is set for the task at the beginning of the test
 * immediately after a global barrier.  Since each task may have a
 * distinct notion of the time an all_reduce gets the least such value
 * and each node's difference from that minimum is noted as its _skew.
 * Henceforth, the current time for the task is measured relative to
 * (_zero - _skew).  This methodology does not account for any
 * possible drift, but casual observation has confirmend that drift is
 * not significant in the time scales contemplated.  A check for drift
 * would be easy to implement if desired.
 */
static double _zero;
static double _skew;
static int _initialized = 0;

extern char *version;

void
init_timer()
{
  double start;

  mpi_barrier(MPI_COMM_WORLD);
  start = mpi_wtime();
  mpi_allreduce(&start, &_zero, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
  _skew = start - _zero;
  _initialized = 1;
}

double
get_time()
{
  double now;

  ASSERT(_initialized == 1);
  now = mpi_wtime() - _zero - _skew;
  return(now);
}
