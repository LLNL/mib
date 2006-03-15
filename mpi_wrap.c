/*****************************************************************************\
 *  $Id: mpi_wrap.c,v 1.7 2006/01/09 16:26:39 auselton Exp $
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

#include <stdio.h>
#include <mpi.h>
#include "string.h"     /* for strncpy */
#include <dlfcn.h>      /* dynamic loader interface */
#include "mpi_wrap.h"
#include "miberr.h"
#include "options.h"
/*
 * Revisit these and decide which ones necessarily must FAIL, and wich
 * can defer to a barrier.
 */

int use_mpi = NO_MPI;
void *mpi_lib_handle;

extern Options *opts;

void
mpi_init(int *argcp, char ***argvp)
{
  char mpi_lib[] = "libmpi.so";
  int rc;
  int flag = RTLD_NOW;

  if( (use_mpi == FORCE_NO_MPI) || (mpi_lib_handle = dlopen(mpi_lib, flag)) == NULL)
    return;
  use_mpi = YES_MPI;
  if((rc = MPI_Init(argcp, argvp)) != MPI_SUCCESS)
    FAIL();
}

void
mpi_comm_size(MPI_Comm comm, int *sizep)
{
  int rc;

  if(USE_MPI)
    {
      if((rc = MPI_Comm_size(comm, sizep)) != MPI_SUCCESS)
	FAIL();
    }
  else
    *sizep = slurm->NPROCS;
}

void
mpi_comm_rank(MPI_Comm comm, int *rankp)
{
  int rc;

  if(USE_MPI)
    {
      if((rc = MPI_Comm_rank(comm, rankp)) != MPI_SUCCESS)
	FAIL();
    }
  else
    *rankp = slurm->PROCID;
}

/*MPI_Errhandler_set not planning on using this one yet*/
/*MPI_Error_string  */
/* MPI_Abort doesn't really need to be wrapped here */

void
mpi_finalize(void)
{
  int rc;

  if(USE_MPI)
    if((rc = MPI_Finalize()) != MPI_SUCCESS)
      FAIL();
  dlclose(mpi_lib_handle);
}

void
mpi_recv(void *buf, int count, MPI_Datatype datatype, 
	 int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  int rc;

  if(USE_MPI)
    if((rc = MPI_Recv(buf, count, datatype, source, tag, comm, status)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_send(void *buffer, int count, MPI_Datatype datatype, 
	 int dest, int tag, MPI_Comm comm)
{
  int rc;

  if(USE_MPI)
    if((rc = MPI_Send(buffer, count, datatype, dest, tag, comm)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_bcast(void *buffer, int count, MPI_Datatype datatype, 
	 int root, MPI_Comm comm)
{
  int rc;

  if(USE_MPI)
    if((rc = MPI_Bcast(buffer, count, datatype, root, comm)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  int rc;

  if(USE_MPI)
    {
      if((rc = MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm)) != MPI_SUCCESS)
	FAIL();
    }
  else
    {
      switch datatype
	{
	case MPI_INT :
	  size = 4;
	  break;
	case MPI_DOUBLE : 
	  size = 8;
	  break;
	case MPI_CHAR :
	default :
	  size = 1;
	  break;
	}
      size *= count;
      memcpy(recvbuf, sendbuf, size);
    }
}

void
mpi_comm_group(MPI_Comm comm, MPI_Group *group)
{
  int rc;

  if(USE_MPI)
    if((rc = MPI_Comm_group(comm, group)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup)
{
  int rc;

  if(USE_MPI)
    if((rc = MPI_Group_range_incl(group, n, ranges, newgroup)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
  int rc;

  if(USE_MPI)
    if((rc = MPI_Comm_create(comm, group, newcomm)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
  int rc;

  if(USE_MPI)
    if((rc = MPI_Comm_split(comm, color, key, newcomm)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_barrier(MPI_Comm comm)
{
  int rc;
  
  /*
   *   This code no longer checks for delayed error reports.  I'll
   * want to reintroduce that once I get error handling on a more
   * solid footing.
   */
  if(USE_MPI)
    if((rc = MPI_Barrier(comm)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_comm_free(MPI_Comm *comm)
{
  int rc;

  if(USE_MPI)
    if((rc = MPI_Comm_free(comm)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_group_free(MPI_Group *group)
{
  int rc;

  if(USE_MPI)
    if((rc = MPI_Group_free(group)) != MPI_SUCCESS)
      FAIL();
}

void
mpi_allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int rc;
  int size = 1;

  if(USE_MPI)
    {
      if((rc = MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm)) != MPI_SUCCESS)
	FAIL();
    }
  else
    {
      switch datatype
	{
	case MPI_INT :
	  size = 4;
	  break;
	case MPI_DOUBLE : 
	  size = 8;
	  break;
	case MPI_CHAR :
	default :
	  size = 1;
	  break;
	}
      size *= count;
      memcpy(recvbuf, sendbuf, size);
    }
}

void
mpi_gather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int rc;

  if(USE_MPI)
    if((rc = MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm)) != MPI_SUCCESS)
      FAIL();
}


