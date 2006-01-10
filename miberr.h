/*****************************************************************************\
 *  $Id: miberr.h,v 1.7 2006/01/09 16:26:39 auselton Exp $
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
#include "mpi.h"

#ifndef BOOL_DEF
typedef enum {FALSE, TRUE} BOOL;
#define BOOL_DEF
#endif

#define ERR_BUF_LEN 128


/* 
 */

/* An ASSERT is an event that will cause immediate termination */
#define ASSERT(condition) do {                                           \
        if(!(condition)) {                                               \
          fprintf(stderr, "ASSERTION: %s (%d)\n", __FILE__, __LINE__);   \
          fflush(stderr);                                                \
          MPI_Abort(MPI_COMM_WORLD, 1);                                  \
        }                                                                \
} while (0)


/******************************************************************************/
/* FAIL uses MPI_Abort to stop processing on all tasks immediately */

#define FAIL() do {                                           \
        fprintf(stderr, "%s (%d)\n", __FILE__, __LINE__);     \
          fflush(stderr);                                                \
          MPI_Abort(MPI_COMM_WORLD, 1);                                  \
} while (0)


