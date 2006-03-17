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

void get_SLURM_env();
void show_SLURM_env();

/* 
 * These are the SLURM environment values available to a task.
 * I got them with the command:
 srun -N8 -n16 -l  bash -c set | grep SLURM
 * The task 0 environment variables:
 00: SLURM_CPUS_ON_NODE=2
 00: SLURM_CPUS_PER_TASK=1
 00: SLURM_CPU_BIND_LIST=
 00: SLURM_CPU_BIND_TYPE=
 00: SLURM_CPU_BIND_VERBOSE=quiet
 00: SLURM_JOBID=66371
 00: SLURM_LABELIO=1
 00: SLURM_LAUNCH_NODE_IPADDR=192.168.17.198
 00: SLURM_LOCALID=0
 00: SLURM_NNODES=8
 00: SLURM_NODEID=0
 00: SLURM_NODELIST='adev[2,8-14]'
 00: SLURM_NPROCS=16
 00: SLURM_PROCID=0
 00: SLURM_SRUN_COMM_HOST=adevi
 00: SLURM_SRUN_COMM_PORT=3071
 00: SLURM_STEPID=0
 00: SLURM_TASKS_PER_NODE='2(x8)'
 00: SLURM_TASK_PID=9026
 00: SLURM_UMASK=0022
 *
 */
#define MAX_SLURM_BUF 100

typedef struct SLURM_Struct {
  int use_SLURM;
  int CPUS_ON_NODE;
  int CPUS_PER_TASK;
  char CPU_BIND_LIST[MAX_SLURM_BUF];
  char CPU_BIND_TYPE[MAX_SLURM_BUF];
  char CPU_BIND_VERBOSE[MAX_SLURM_BUF];
  int  JOBID;
  char LAUNCH_NODE_IPADDR[MAX_SLURM_BUF];
  int  LOCALID;
  int  NNODES;
  int  NODEID;
  char NODELIST[MAX_SLURM_BUF];
  int  NPROCS;
  int  PROCID;
  char SRUN_COMM_HOST[MAX_SLURM_BUF];
  int  SRUN_COMM_PORT;
  int  STEPID;
  char TASKS_PER_NODE[MAX_SLURM_BUF];
  int  TASK_PID;
  char UMASK[MAX_SLURM_BUF];
}SLURM;

