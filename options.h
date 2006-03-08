/*****************************************************************************\
 *  $Id: options.h,v 1.11 2006/01/09 16:26:39 auselton Exp $
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

#define MAX_BUF 100

#define DEFAULTS      0
#define NEW           (1 << 0)
#define REMOVE        (1 << 1)
#define READ_ONLY     (1 << 2)
#define WRITE_ONLY    (1 << 3)
#define PROFILES      (1 << 4)
#define USE_NODE_AVES (1 << 5)


typedef struct Options_Struct {
  char log_dir[MAX_BUF];      /* = /home/auselton/testing/<date> */
  char testdir[MAX_BUF];      /* = /p/gbtest/lustre-test/ior/smooth */
  int call_limit;     
  long long call_size;
  int time_limit;    
  int flags;
  int verbosity;
}Options;

typedef struct Mib_Struct {
  int nodes;
  int tasks;
  int rank;
  int size;
  int base;
  MPI_Comm comm;
}Mib;

/*
 *  I made "QUIET" have a non-zero value above the others so that 
 * the verbosity macro will return a TRUE for the SHOW_ALL level.
 */
#define SHOW_SIGNON               (1 << 0)
#define SHOW_HEADERS              (1 << 1)
#define SHOW_ENVIRONMENT          (1 << 2)
#define SHOW_PROGRESS             (1 << 3)
#define SHOW_INTERMEDIATE_VALUES  (1 << 4)
#define QUIET                     (1 << 5)
#define SHOW_ALL                  (~0)

#define CL_LOG_DIR                  (1 << 0)
#define CL_TESTDIR                  (1 << 1)
#define CL_CALL_LIMIT               (1 << 2)
#define CL_CALL_SIZE                (1 << 3)
#define CL_TIME_LIMIT               (1 << 4)
#define CL_NEW                      (1 << 5)
#define CL_REMOVE                   (1 << 6)
#define CL_WRITE_ONLY               (1 << 7)
#define CL_READ_ONLY                (1 << 8)
#define CL_PROFILES                 (1 << 9)
#define CL_USE_NODE_AVES            (1 << 10)
#define CL_SHOW_SIGNON              (1 << 11)
#define CL_SHOW_HEADERS             (1 << 12)
#define CL_SHOW_ENVIRONMENT         (1 << 13)
#define CL_SHOW_PROGRESS            (1 << 14)
#define CL_SHOW_INTERMEDIATE_VALUES (1 << 15)

void command_line(int argc, char *argv[], int rank);
Options *read_options(int rank, int size);
void write_log();
void read_log();
void close_log();
void log_it(char *fmt, char *str);
void Free_Opts();
void usage( void );

#define check_flags(mask)  (opts->flags & mask)
#define verbosity(mask)    (opts->verbosity & mask)
#define check_cl(mask)     (cl_opts_mask & mask)

/* 
 * These are the SLURM environment values available to a task.
 * I go them with the command:
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

typedef struct SLURM_Struct {
  int CPUS_ON_NODE;
  int CPUS_PER_TASK;
  char CPU_BIND_LIST[MAX_BUF];
  char CPU_BIND_TYPE[MAX_BUF];
  char CPU_BIND_VERBOSE[MAX_BUF];
  int  JOBID;
  char LAUNCH_NODE_IPADDR[MAX_BUF];
  int  LOCALID;
  int  NNODES;
  int  NODEID;
  char NODELIST[MAX_BUF];
  int  NPROCS;
  int  PROCID;
  char SRUN_COMM_HOST[MAX_BUF];
  int  SRUN_COMM_PORT;
  int  STEPID;
  char TASKS_PER_NODE[MAX_BUF];
  int  TASK_PID;
  char UMASK[MAX_BUF];
}SLURM;

