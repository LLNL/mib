/*****************************************************************************\
 *  $Id: miberr.h,v 1.7 2006/01/09 16:26:39 auselton Exp $
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

#ifndef BOOL_DEF
typedef enum {FALSE, TRUE} BOOL;
#define BOOL_DEF
#endif

/* 
 *   An ASSERT is an event that will cause immediate termination.  It
 * is intended only for those places in the code where the condition
 * reveals a bug.  No user action can trigger this.
 */
#define ASSERT(cond) do {                                                    \
    if(!(cond)) {                                                            \
        fprintf(stderr, "ASSERTION: %s::%d (%s)\n",__FILE__,__LINE__,#cond); \
        fflush(stderr);                                                      \
        mpi_abort(MPI_COMM_WORLD, 1);                                        \
    }                                                                        \
} while (0)

/* 
 *   FAIL uses MPI_Abort to stop processing on all tasks immediately.
 * In contrast to ASSERT, FAIL is for conditions external to mib's
 * control that prevent it from continuing.
 */
#define FAIL() do {                                                          \
    fprintf(stderr, "FATAL: %s::%d\n", __FILE__, __LINE__);                  \
    fflush(stderr);                                                          \
    mpi_abort(MPI_COMM_WORLD, 1);                                            \
} while (0)
