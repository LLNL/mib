/*****************************************************************************\
 *  $Id: mpi_wrap.h,v 1.5 2005/11/30 20:24:57 auselton Exp $
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

#ifndef MPI_NOT_INSTALLED
#include <mpi.h>
#else
#include "mpi_stubs.h"
#endif

#ifndef BOOL_DEF
typedef enum {FALSE, TRUE} BOOL;
#define BOOL_DEF
#endif

/*
 *   All of these "wrapper" functions correspond directly to their MPI
 * equivalents, with one exceptions.  If the NO_MPI condition is set
 * then each one will avoid calling the corresponding MPI function and
 * will either just return, try to emulate it, or FAIL().  
 * This allows operation where there is no MPI.  The
 * wrappers should be avoided if there is no MPI and mib does so
 * mostly, but in some cases it relies on the "do nothing" or
 * emulation behavior.  mpi_wtime() will just return time() in the
 * absense of MPI.
 */
void mpi_abort(MPI_Comm comm, int errorcode);

void mpi_allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, 
		       MPI_Op op, MPI_Comm comm);

void mpi_barrier(MPI_Comm comm);

void mpi_bcast(void *buffer, int count, MPI_Datatype datatype, 
	  int root, MPI_Comm comm);

void mpi_comm_create(MPI_Comm comm, MPI_Group group,MPI_Comm *newcomm);

void mpi_comm_free(MPI_Comm *comm);

void mpi_comm_group(MPI_Comm comm, MPI_Group *group);

void mpi_comm_rank(MPI_Comm comm, int *rankp);

void mpi_comm_size(MPI_Comm comm, int *sizep);

void mpi_comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);

void mpi_errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler);

void mpi_finalize(void);

void mpi_gather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, 
		    int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
void mpi_group_free(MPI_Group *group);

void mpi_group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup);

void mpi_init(int *argcp, char ***argvp);

void mpi_recv(void *buf, int count, MPI_Datatype datatype,  
	 int source, int tag, MPI_Comm comm, MPI_Status *status);

void mpi_reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, 
		    MPI_Op op, int root, MPI_Comm comm);

void mpi_send(void *buffer, int count, MPI_Datatype datatype, 
	      int dest, int tag, MPI_Comm comm);

double mpi_wtime();


