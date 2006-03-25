/*****************************************************************************\
 *  $Id: mpi_wrap.h,v 1.5 2005/11/30 20:24:57 auselton Exp $
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

#ifndef BOOL_DEF
typedef enum {FALSE, TRUE} BOOL;
#define BOOL_DEF
#endif

#define FORCE_NO_MPI (-1)
#define NO_MPI 0
#define YES_MPI 1
#define USE_MPI (use_mpi > NO_MPI)
#define DO_NOT_USE_MPI (use_mpi < YES_MPI)

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

void mpi_errhandler_set(MPI_Comm comm, MPI_Errhandler *errhandler);

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


