/*****************************************************************************\
 *  $Id: options.h,v 1.11 2006/01/09 16:26:39 auselton Exp $
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
#ifndef BOOL_DEF
typedef enum {FALSE, TRUE} BOOL;
#define BOOL_DEF
#endif

#define MAX_BUF 100

/* defines for the Options_Struc "flags" field */
#define DEFAULTS      0
#define NEW           (1 << 0)
#define REMOVE        (1 << 1)
#define READ_ONLY     (1 << 2)
#define WRITE_ONLY    (1 << 3)
#define RANDOM_READS  (1 << 4)
#define FORCE         (1 << 5)
#define DIRECTIO      (1 << 6)

/*
 * Defines for the Options_Struct "verbosity. " field
 */
#define QUIET                     (1 << 0)        
#define SHOW_SIGNON               (1 << 1)
#define SHOW_HEADERS              (1 << 2)
#define SHOW_ENVIRONMENT          (1 << 3)
#define SHOW_PROGRESS             (1 << 4)
#define SHOW_INTERMEDIATE_VALUES  (1 << 5)
#define SHOW_ALL                  (~0)

#define flag_set(flag, mask)  (flag & mask)

typedef struct Options_Struct {
  /* Path and filename for optional system call profiles tables */
  /* The one for writes gets ".write" appended and for reads ".read"  */
  char *profiles;
  /* This required argument gives the path to where the I/O will be directed */
  /* The file name in that directory for task <i> is "mibData.<i>" */
  char *testdir;
  /* Issue no more than this many system calls (per phase, write and read) */
  int call_limit;     
  /* System calls use a buffer of this size */
  long long call_size;
  /* After this much time issue no new system calls (per phase, write and read) */
  /* Since it may tak a long time to return from a system call and check for */
  /* this condition, this limit does not stop the test precisely at the time_limit */
  int time_limit;    
  /* For the optional random reads, seek with this size "stride" */
  long long granularity;
  /* see the flags and verbosity defines above */
  int flags;
  int verbosity;
}Options;

Options *command_line(int *argcp, char **argvp[]);
void Free_Opts();
void usage( void );
void show_details();


