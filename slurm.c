/*****************************************************************************\
 *  $Id: slurm.h 1.13 2006/01/09 16:26:39 auselton Exp $
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* strncpy */
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include "config.h"
#include "sys_wrap.h"
#include "slurm.h"

void _get_str_env(char *str, char *key);
int  _get_int_env(char *key);

SLURM   *slurm = NULL;

void
get_SLURM_env()
{
  slurm = (SLURM*)Malloc(sizeof(SLURM));
  slurm->use_SLURM = 1;
  slurm->CPUS_ON_NODE = _get_int_env("SLURM_CPUS_ON_NODE");
  slurm->CPUS_PER_TASK = _get_int_env("SLURM_CPUS_PER_TASK");
  _get_str_env(slurm->CPU_BIND_LIST, "SLURM_CPU_BIND_LIST");
  _get_str_env(slurm->CPU_BIND_TYPE, "SLURM_CPU_BIND_TYPE");
  _get_str_env(slurm->CPU_BIND_VERBOSE, "SLURM_CPU_BIND_VERBOSE");
  slurm->JOBID = _get_int_env("SLURM_JOBID");
  _get_str_env(slurm->LAUNCH_NODE_IPADDR, "SLURM_LAUNCH_NODE_IPADDR");
  slurm->LOCALID = _get_int_env("SLURM_LOCALID");
  slurm->NNODES = _get_int_env("SLURM_NNODES");
  slurm->NODEID = _get_int_env("SLURM_NODEID");
  _get_str_env(slurm->NODELIST, "SLURM_NODELIST");
  slurm->NPROCS = _get_int_env("SLURM_NPROCS");
  slurm->PROCID = _get_int_env("SLURM_PROCID");
  _get_str_env(slurm->SRUN_COMM_HOST, "SLURM_SRUN_COMM_HOST");
  slurm->SRUN_COMM_PORT = _get_int_env("SLURM_SRUN_COMM_PORT");
  slurm->STEPID = _get_int_env("SLURM_STEPID");
  _get_str_env(slurm->TASKS_PER_NODE, "SLURM_TASKS_PER_NODE");
  slurm->TASK_PID = _get_int_env("SLURM_TASK_PID");
  _get_str_env(slurm->UMASK, "SLURM_UMASK");
  if(slurm->NPROCS == 0)
    {
      slurm->use_SLURM = 0;
    }
  /* show_SLURM_env(slurm); */
}

void
show_SLURM_env()
{
  printf("use SLURM? %s\n", (slurm->use_SLURM ? "Yes" : "No"));
  if(slurm->use_SLURM)
    {
      printf("SLURM_CPUS_ON_NODE = %d\n", slurm->CPUS_ON_NODE);
      printf("SLURM_CPUS_PER_TASK = %d\n", slurm->CPUS_PER_TASK);
      printf("SLURM_CPU_BIND_LIST = %s\n", slurm->CPU_BIND_LIST);
      printf("SLURM_CPU_BIND_TYPE = %s\n", slurm->CPU_BIND_TYPE);
      printf("SLURM_CPU_BIND_VERBOSE = %s\n", slurm->CPU_BIND_VERBOSE);
      printf("SLURM_JOBID = %d\n", slurm->JOBID);
      printf("SLURM_LAUNCH_NODE_IPADDR = %s\n", slurm->LAUNCH_NODE_IPADDR);
      printf("SLURM_LOCALID = %d\n", slurm->LOCALID);
      printf("SLURM_NNODES = %d\n", slurm->NNODES);
      printf("SLURM_NODEID = %d\n", slurm->NODEID);
      printf("SLURM_NODELIST = %s\n", slurm->NODELIST);
      printf("SLURM_NPROCS = %d\n", slurm->NPROCS);
      printf("SLURM_PROCID = %d\n", slurm->PROCID);
      printf("SLURM_SRUN_COMM_HOST = %s\n", slurm->SRUN_COMM_HOST);
      printf("SLURM_SRUN_COMM_PORT = %d\n", slurm->SRUN_COMM_PORT);
      printf("SLURM_STEPID = %d\n", slurm->STEPID);
      printf("SLURM_TASKS_PER_NODE = %s\n", slurm->TASKS_PER_NODE);
      printf("SLURM_TASK_PID = %d\n", slurm->TASK_PID);
      printf("SLURM_UMASK = %s\n", slurm->UMASK);
    }
  fflush(stdout);
}

int
_get_int_env(char *key)
{
  char *val;

  if( (val = getenv(key)) != NULL )
    {
      /*      printf( "%s = %s\n", key, val);
	      fflush(stdout);*/
      return(atoi(val));
    }
  return(0);
}
  
void
_get_str_env(char *str, char *key)
{
  char *val;

  if( (val = getenv(key)) != NULL )
    {
      strncpy(str, val, MAX_SLURM_BUF);
    }
  else
    str[0] = '\0';
}
  

