/*****************************************************************************\
 *  $id: mpi_wrap.c,v 1.7 2006/01/09 16:26:39 auselton Exp $
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* for strncpy */
#include <time.h>
#include <unistd.h>
#include "mpi_wrap.h"
#include "mib.h"        /* for conditional_report */
#include "miberr.h"
#include "sys_wrap.h"
#include "options.h"

/*
 * Some of the error handling might be improved if some MPI calls don't
 * have to FAIL and MPI_Abort but can defer their complaint to the 
 * next barrier.
 */

/* 
 *   the code is organized so that it will (at least try to) run even
 *   without MPI.
 */
extern Options *opts;

void
mpi_abort(MPI_Comm comm, int errorcode)
{
#ifdef WITH_MPI
  MPI_Abort(comm, errorcode);
#else
  exit(errorcode);
#endif
}

void
mpi_allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
#ifdef WITH_MPI
  if(MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm) != MPI_SUCCESS)
    FAIL();
#else
  switch (datatype)
  {
    case MPI_INT :
      memcpy(recvbuf, sendbuf, 4*count);
      break;
    case MPI_DOUBLE : 
      memcpy(recvbuf, sendbuf, 8*count);
      break;
    case MPI_CHAR :
    default :
      memcpy(recvbuf, sendbuf, 1*count);
      break;
    }
#endif
}

void
mpi_barrier(MPI_Comm comm)
{
#ifdef WITH_MPI
  /*
   *   This code no longer checks for delayed error reports.  I'll
   * want to reintroduce that once I get error handling on a more
   * solid footing.
   */
  if( MPI_Barrier(comm) != MPI_SUCCESS)
    FAIL();
#endif
}

void
mpi_bcast(void *buffer, int count, MPI_Datatype datatype, 
	 int root, MPI_Comm comm)
{
#ifdef WITH_MPI
  if(MPI_Bcast(buffer, count, datatype, root, comm) != MPI_SUCCESS)
    FAIL();
#endif
}

void
mpi_comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
#ifdef WITH_MPI
  if(MPI_Comm_create(comm, group, newcomm) != MPI_SUCCESS)
    FAIL();
#endif
}

void
mpi_comm_free(MPI_Comm *comm)
{
#ifdef WITH_MPI
  if(MPI_Comm_free(comm) != MPI_SUCCESS)
    FAIL();
#endif
}

void
mpi_comm_group(MPI_Comm comm, MPI_Group *group)
{
#ifdef WITH_MPI
  if(MPI_Comm_group(comm, group) != MPI_SUCCESS)
    FAIL();
#endif
}

void
mpi_comm_rank(MPI_Comm comm, int *rankp)
{
#ifdef WITH_MPI
  if(MPI_Comm_rank(comm, rankp) != MPI_SUCCESS)
    FAIL();
#else
  *rankp = 0;
#endif
}

void
mpi_comm_size(MPI_Comm comm, int *sizep)
{
#ifdef WITH_MPI
  if(MPI_Comm_size(comm, sizep) != MPI_SUCCESS)
    FAIL();
#else
  *sizep = 1;
#endif
}

void
mpi_comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
  if(MPI_Comm_split(comm, color, key, newcomm) != MPI_SUCCESS)
    FAIL();
}

void 
mpi_errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler)
{
#ifdef WITH_MPI
  if(MPI_Errhandler_set(comm, errhandler) != MPI_SUCCESS)
    FAIL();
#endif
}

void
mpi_finalize(void)
{
#ifdef WITH_MPI
  if(MPI_Finalize() != MPI_SUCCESS)
    FAIL();
#endif
}

void
mpi_gather(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
           void *recvbuf, int recvcount, MPI_Datatype recvtype, 
           int root, MPI_Comm comm)
{
#ifdef WITH_MPI
    if(MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, 
                  recvtype, root, comm) != MPI_SUCCESS)
      FAIL();
#endif
}

void
mpi_group_free(MPI_Group *group)
{
#ifdef WITH_MPI
  if(MPI_Group_free(group) != MPI_SUCCESS)
    FAIL();
#endif
}

void
mpi_group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup)
{
#ifdef WITH_MPI
  if( MPI_Group_range_incl(group, n, ranges, newgroup) != MPI_SUCCESS)
    FAIL();
#endif
}

void
mpi_init(int *argcp, char ***argvp)
{
#ifdef WITH_MPI
  if( MPI_Init(argcp, argvp) != MPI_SUCCESS)
    FAIL();
#endif
}

void
mpi_recv(void *buf, int count, MPI_Datatype datatype, 
	 int source, int tag, MPI_Comm comm, MPI_Status *status)
{
#ifdef WITH_MPI
  if(MPI_Recv(buf, count, datatype, source, tag, comm, status) != MPI_SUCCESS)
    FAIL();
#endif
}

void
mpi_reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
#ifdef WITH_MPI
  if(MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm) != MPI_SUCCESS)
    FAIL();
#else
  switch (datatype)
  {
    case MPI_INT :
      memcpy(recvbuf, sendbuf, 4*count);
      break;
    case MPI_DOUBLE : 
      memcpy(recvbuf, sendbuf, 8*count);
      break;
    case MPI_CHAR :
    default :
      memcpy(recvbuf, sendbuf, 1*count);
      break;
  }
#endif
}

void
mpi_send(void *buffer, int count, MPI_Datatype datatype, 
	 int dest, int tag, MPI_Comm comm)
{
#ifdef WITH_MPI
  if(MPI_Send(buffer, count, datatype, dest, tag, comm) != MPI_SUCCESS)
    FAIL();
#endif
}

double 
mpi_wtime(void)
{
#ifdef WITH_MPI
  return(MPI_Wtime());
#else
  return (double)time(NULL);
#endif
}
