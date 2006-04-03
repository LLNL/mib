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

/* defines for the Options_Struc "flags" field */
#define DEFAULTS      0
#define NEW           (1 << 0)
#define REMOVE        (1 << 1)
#define READ_ONLY     (1 << 2)
#define WRITE_ONLY    (1 << 3)
#define RANDOM_READS  (1 << 4)
#define FORCE         (1 << 5)
#define check_flags(mask)  (opts->flags & mask)

/*
 * Defines for the Options_Struct "verbosity. " field
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
#define verbosity(mask)    (opts->verbosity & mask)

typedef struct Options_Struct {
  char *profiles;      /* = /home/auselton/testing/<date> */
  char *testdir;      /* = /p/gbtest/lustre-test/ior/smooth */
  int call_limit;     
  long long call_size;
  int time_limit;    
  long long granularity;
  int flags;
  int verbosity;
}Options;

void command_line(int *argcp, char **argvp[]);
void Free_Opts();
void usage( void );
void show_details();


