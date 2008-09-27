/*****************************************************************************\
 *  $Id: mib.h,v 1.7 2005/11/30 20:24:57 auselton Exp $
 *****************************************************************************
 *  Copyright (C) 2006 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
 *  UCRL-CODE-222725
 *  
 *  This file is part of Mib, an MPI-based parallel I/O benchamrk
 *  For details, see <http://www.llnl.gov/linux/mib/>.
 *  
 *  Mib is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License (as published by the Free
 *  Software Foundation) version 2, dated June 1991.
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

#ifdef DEBUG_CODE
#define DEBUG(str) do {          \
  conditional_report(SHOW_ALL, str);    \
} while (0)
#else
#define DEBUG(str)
#endif

#ifndef BOOL_DEF
typedef enum {FALSE, TRUE} BOOL;
#define BOOL_DEF
#endif

#define MIB_VERSION "1.9.8"

/*
 *   In normal use the rank, base, and size fields are the important ones.
 * Rank and size are the MPI values for the current task.  Base refers to
 * the task designated for sending info to stdout.  Nodes and tasks are
 * inferred from the SLURM environment if possible.  Tasks should equal size
 * if there are no subcommunicators (which there aren't in this version).
 * tasks/nodes may be useful in some circumstances, as when subcommunicaotrs 
 * reduce the amount of data to be sent to stdout.  this was done on BGL
 * though it is not implemented int he current version.
 */
typedef struct Mib_Struct {
  int rank;
  int size;
  int base;
  MPI_Comm comm;
}Mib;

/* 
 *   During a test all the timing and counting results are put in this
 * struct.  Some fields are relevant only to write or only to read tests.
 * Most apply to both.
 */
typedef struct Results_Struct {
  double  before_unlink;
  double  start_open;
  double  end_open;
  double  start;
  double *timings;
  int     num;
  double  end;
  double  transferred;
  int     short_transfers;
  double  finish_fsync;  /* Only used for writes */
  double  start_close;
  double  end_close;
  double  after_unlink;
  double  end_test;
}Results;

void base_report(char *fmt, ...);
void conditional_report(int verb, char *fmt, ...);
