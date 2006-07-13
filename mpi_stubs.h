/*****************************************************************************\
 *  $Id: mpi_stubs.h,v 1.5 2005/11/30 20:24:57 auselton Exp $
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

/* 
 *   In the absense of MPI, and the mpi.h header in particular, some 
 * constants need to be defined just so the code will compile.  In each
 * case the acual facilities won't be used because they are guarded by
 * the "USE_MPI" condition.
 */
typedef int MPI_Comm;
typedef int MPI_Op;
typedef int MPI_Datatype;
typedef int MPI_Group;
typedef int MPI_Errhandler;
typedef int MPI_Status;
/* MPI_Comm objects */
#define MPI_COMM_WORLD ((MPI_Comm)1)
#define MPI_COMM_NULL ((MPI_Comm)0)
/* MPI_Datatype objects */
#define MPI_INT    ((MPI_Datatype)1)
#define MPI_DOUBLE ((MPI_Datatype)2)
#define MPI_CHAR ((MPI_Datatype)3)
/* MPI_Op objects */
#define MPI_MAX ((MPI_Op)1)
#define MPI_MIN ((MPI_Op)2)
#define MPI_SUM ((MPI_Op)3)
/* MPI_Status objects */
#define MPI_SUCCESS ((MPI_Status)1)
