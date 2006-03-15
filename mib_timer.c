/*****************************************************************************\
 *  $Id: mib_timer.c,v 1.3 2005/11/30 20:24:57 auselton Exp $
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
#include <mpi.h>
#include <stdio.h>
#include <time.h>
#include <string.h>     /* strncpy */
#include "mpi_wrap.h"
#include "options.h"
#include "mib_timer.h"
#include "miberr.h"

#define MAX_BUF 100

static double _zero;
static double _skew;
static int _initialized = 0;

extern char *version;
extern int use_mpi;

void
init_timer(int rank, char *signon)
{
  double start;
  char time_str[MAX_BUF];
  time_t t;
  char *p;

  t = time(NULL);
  if (USE_MPI)
    {
      mpi_barrier(MPI_COMM_WORLD);
      start = MPI_Wtime();
      mpi_allreduce(&start, &_zero, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
      _skew = start - _zero;
    }
  else
    {
      _zero = t;
      _skew = 0;
    }
  _initialized = 1;
  if ( rank == 0 )
    {
      strncpy(time_str, ctime(&t), MAX_BUF);
      p = time_str;
      while( (*p != '\0') && (*p != '\n') && (p - time_str < MAX_BUF) )p++;
      if( *p == '\n' ) *p = '\0';
      sprintf(signon, "\n\nmib-%s  %s\n\n", version, time_str);
    }
}

double
get_time()
{
  time_t t;
  double now;

  ASSERT(_initialized == 1);
  if(USE_MPI)
    now = MPI_Wtime() - _zero - _skew;
  else
    {
      t = time(NULL);
      now = t - _zero;
    }
  return(now);
}
