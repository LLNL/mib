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

void command_line(int *argcp, char **argvp[]);
Options *read_options();
void write_log();
void read_log();
void close_log();
void log_it(char *fmt, char *str);
void Free_Opts();
void usage( void );
void show_keys(Options *opts);

#define check_flags(mask)  (opts->flags & mask)
#define verbosity(mask)    (opts->verbosity & mask)
#define check_cl(mask)     (cl_opts_mask & mask)

