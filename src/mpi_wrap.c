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
#include "slurm.h"

/*
 * Some of the error handling might be improved if some MPI calls don't
 * have to FAIL and MPI_Abort but can defer their complaint to the 
 * next barrier.
 * The USE_MPI guard might be accompanied by an error in many cases, since
 * there is no good way of emulating the behavior without. 
 */

/* 
 *   the code is organized so that it will (at least try to) run even
 *   without MPI.
 */
int use_mpi = NO_MPI;

extern Options *opts;
extern SLURM *slurm;

void
mpi_abort(MPI_Comm comm, int errorcode)
{
#ifndef MPI_NOT_INSTALLED
  if( USE_MPI )
    {
      MPI_Abort(comm, errorcode);
    }
  else
#endif
    {
      exit(errorcode);
    }
}

void
mpi_allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int rc = 0;
  int size = 1;

#ifndef MPI_NOT_INSTALLED
  if(USE_MPI)
    {
      if((rc = MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm)) != MPI_SUCCESS)
	FAIL();
    }
  else
#endif
    {
      switch (datatype)
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
mpi_barrier(MPI_Comm comm)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;
  
  /*
   *   This code no longer checks for delayed error reports.  I'll
   * want to reintroduce that once I get error handling on a more
   * solid footing.
   */
  if(USE_MPI)
    if((rc = MPI_Barrier(comm)) != MPI_SUCCESS)
      FAIL();
#endif
}

void
mpi_bcast(void *buffer, int count, MPI_Datatype datatype, 
	 int root, MPI_Comm comm)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;

  if(USE_MPI)
    if((rc = MPI_Bcast(buffer, count, datatype, root, comm)) != MPI_SUCCESS)
      FAIL();
#endif
}

void
mpi_comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;

  if(USE_MPI)
    if((rc = MPI_Comm_create(comm, group, newcomm)) != MPI_SUCCESS)
      FAIL();
#endif
}

void
mpi_comm_free(MPI_Comm *comm)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;

  if(USE_MPI)
    if((rc = MPI_Comm_free(comm)) != MPI_SUCCESS)
      FAIL();
#endif
}

void
mpi_comm_group(MPI_Comm comm, MPI_Group *group)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;

  if(USE_MPI)
    if((rc = MPI_Comm_group(comm, group)) != MPI_SUCCESS)
      FAIL();
#endif
}

void
mpi_comm_rank(MPI_Comm comm, int *rankp)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;

  if(USE_MPI)
    {
      if((rc = MPI_Comm_rank(comm, rankp)) != MPI_SUCCESS)
	FAIL();
    }
#endif
}

void
mpi_comm_size(MPI_Comm comm, int *sizep)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;

  if(USE_MPI)
    {
      if((rc = MPI_Comm_size(comm, sizep)) != MPI_SUCCESS)
	FAIL();
    }
#endif
}

void
mpi_comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;

  if(USE_MPI)
    if((rc = MPI_Comm_split(comm, color, key, newcomm)) != MPI_SUCCESS)
      FAIL();
#endif
}

void 
mpi_errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;

  if(USE_MPI)
    {
      if((rc = MPI_Errhandler_set(comm, errhandler)) != MPI_SUCCESS)
	FAIL();
    }
#endif
}

void
mpi_finalize(void)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;

  if(USE_MPI)
    {
      if((rc = MPI_Finalize()) != MPI_SUCCESS)
	FAIL();
    }
#endif
}

void
mpi_gather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;

  if(USE_MPI)
    if((rc = MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm)) != MPI_SUCCESS)
      FAIL();
#endif
}

void
mpi_group_free(MPI_Group *group)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;

  if(USE_MPI)
    if((rc = MPI_Group_free(group)) != MPI_SUCCESS)
      FAIL();
#endif
}

void
mpi_group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;

  if(USE_MPI)
    if((rc = MPI_Group_range_incl(group, n, ranges, newgroup)) != MPI_SUCCESS)
      FAIL();
#endif
}

void
mpi_init(int *argcp, char ***argvp)
{
  int rc = 0;

  if( use_mpi == FORCE_NO_MPI )
    {
      conditional_report(SHOW_ENVIRONMENT, "Command line forbade the use of MPI\n");
      return;
    }
  if( ! slurm->use_SLURM )
    {
      conditional_report(SHOW_ENVIRONMENT, "No SLURM, no MPI\n");
      return;
    }
#ifndef MPI_NOT_INSTALLED
  if((rc = MPI_Init(argcp, argvp)) == MPI_SUCCESS)
    {
      conditional_report(SHOW_ENVIRONMENT, "Using MPI\n");
      use_mpi = YES_MPI;
    }
  else
#endif
    {
      conditional_report(SHOW_ENVIRONMENT, "No MPI - MPI_Init failed, attempting to proceed without\n");
      return;
    }
}

void
mpi_recv(void *buf, int count, MPI_Datatype datatype, 
	 int source, int tag, MPI_Comm comm, MPI_Status *status)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;

  if(USE_MPI)
    if((rc = MPI_Recv(buf, count, datatype, source, tag, comm, status)) != MPI_SUCCESS)
      FAIL();
#endif
}

void
mpi_reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  int rc = 0;
  int size = 1;

#ifndef MPI_NOT_INSTALLED
  if(USE_MPI)
    {
      if((rc = MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm)) != MPI_SUCCESS)
	FAIL();
    }
  else
#endif
    {
      switch (datatype)
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
mpi_send(void *buffer, int count, MPI_Datatype datatype, 
	 int dest, int tag, MPI_Comm comm)
{
#ifndef MPI_NOT_INSTALLED
  int rc = 0;

  if(USE_MPI)
    if((rc = MPI_Send(buffer, count, datatype, dest, tag, comm)) != MPI_SUCCESS)
      FAIL();
#endif
}

double 
mpi_wtime(void)
{
  double now;

#ifndef MPI_NOT_INSTALLED
  if(USE_MPI)
    {
      return(MPI_Wtime());
    }
  else
#endif
    {
      now = time(NULL);
      return(now);
    }
}
