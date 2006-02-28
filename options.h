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
#ifndef NDEBUG
#define DEBUG(str) do {          \
  if(opts->rank == opts->base)   \
 {                               \
   printf(str);                  \
   fflush(stdout);               \
 }                               \
} while (0)
#else
#define DEBUG(str)
#endif

#ifndef BOOL_DEF
typedef enum {FALSE, TRUE} BOOL;
#define BOOL_DEF
#endif

#define MAX_BUF 100

typedef struct Options_Struct {
  char cluster[MAX_BUF];
  int new;
  int remove;
  int tasks;
  int nodes;
  char log_dir[MAX_BUF];      /* = /home/auselton/testing/<date> */
  char write_log[MAX_BUF];    /* = /home/auselton/testing/<date>/write.syscalls.ave */
  char read_log[MAX_BUF];     /* = /home/auselton/testing/<date>/read.syscalls.date */
  FILE *lfd;                  /* Currently active log file */
  char testdir[MAX_BUF];      /* = /p/gbtest/lustre-test/ior/smooth */
  int call_limit;       /* = 8 */
  long long call_size;  /* = 1m */
  int time_limit;     /* = 400 */
  int rank;
  int size;
  int base;
  int last_write_call;
  int last_read_call;
  MPI_Comm comm;
  char write_target[MAX_BUF];
  char read_target[MAX_BUF];
  int write_only;
  int read_only;
  int iterations;
  int pause;
  int progress;
  int profiles;
  int use_node_aves;
  int verbosity;
}Options;

#define QUIET   0
#define NORMAL  1
#define VERBOSE 2

void command_line(int argc, char *argv[], char *opt_path, int rank);
Options *read_options(char *opt_path, int rank, int size);
void write_log();
void read_log();
void close_log();
void log_it(char *fmt, char *str);
void Free_Opts();
void usage( void );
