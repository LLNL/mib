/*****************************************************************************\
 *  $Id: options.c,v 1.13 2006/01/09 16:26:39 auselton Exp $
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
#include <stdlib.h>
#include <string.h>     /* strncpy */
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include "sys_wrap.h"
#include "miberr.h"
#include "options.h"
#include "mpi_wrap.h"

Options *Init_Opts(char *buf, int rank, int size);
void show_keys(Options *opts);

BOOL set_cluster(char *v, Options *opts);
BOOL set_cns(char *v, Options *opts);
BOOL set_ions(char *v, Options *opts);
BOOL set_testdir(char *v, Options *opts);
BOOL set_call_limit(char *v, Options *opts);
BOOL set_call_size(char *v, Options *opts);
BOOL set_time_limit(char *v, Options *opts);
BOOL set_tasks(char *v, Options *opts);
BOOL set_read_only(char *v, Options *opts);
BOOL set_write_only(char *v, Options *opts);
BOOL set_iterations(char *v, Options *opts);
BOOL set_pause(char *v, Options *opts);
BOOL set_progress(char *v, Options *opts);
BOOL set_profiles(char *v, Options *opts);
BOOL set_verbosity(char *v, Options *opts);
void lowercase(char *v);
BOOL kv_pair(char *buf, char **k, char **v);
BOOL set_key(char *k, char *v, Options *opts);

Options *opts;

void
command_line(int argc, char *argv[], char *opt_path)
{
  int opt;
  char *opt_str = "d:h";

  *opt_path = '\0';
  while( (opt = getopt(argc, argv, opt_str)) != -1)
    {
      switch(opt)
	{
	case 'd' : 
	  snprintf(opt_path, MAX_BUF, "%s", optarg); 
	  break;
	case 'h' :  
	default : usage(); break;
	}
    }
}

Options *
read_options(char *opt_path, int rank, int size)
{
  int opt;
  char *opt_str = "d:h";
  Options *opts;
  
  ASSERT(opt_path != NULL);
  opts = Init_Opts(opt_path, rank, size);
  show_keys(opts);
  return(opts);
}

void
usage( void )
{
  if (opts->rank == opts->base)
    {
      printf("usage: mib {options}\n");
      printf("       -d <log_dir>        :  parameters file should be in this dir\n");
      printf("       -f <tasks per node> :  Ratio of tasks to I/O nodes\n");
      printf("       -h                  :  This message\n");
    }
  exit(0);
}

Options *
Init_Opts(char *opt_path, int rank, int size)
{
  Options *opts;
  char options[MAX_BUF];
  char buf[MAX_BUF];
  char *b;
  char *k;
  char *v;
  FILE *ofp;
  int ret;
  int len = 0;
  int send;

  ASSERT(opt_path != NULL);
  opts = (Options *)Malloc(sizeof(Options));
  opts->cluster[0] = '\0';
  opts->cns = size;
  opts->ions = 1024;
  opts->log_dir[0] = '\0';
  opts->write_log[0] = '\0';
  opts->read_log[0] = '\0';
  opts->lfd = NULL;
  opts->testdir[0] = '\0';
  opts->call_limit = 512;
  opts->call_size = 524288;
  opts->time_limit = 100;
  opts->tasks = size;
  opts->rank = rank;
  opts->size = size;
  opts->base = 0;
  opts->last_write_call = 0;
  opts->last_read_call = 0;
  opts->comm = MPI_COMM_WORLD;
  opts->write_target[0] = '\0';
  opts->read_target[0] = '\0';
  opts->write_only = 0;
  opts->read_only = 0;
  opts->iterations = 1;
  opts->pause = 0;
  opts->progress = 0;
  opts->profiles = 0;
  if(opts->rank == opts->base)
    {
      if (*opt_path != '\0')
	{
	  strncpy(opts->log_dir, opt_path, MAX_BUF);
	  snprintf(options, MAX_BUF, "%s/options", opts->log_dir);
	  ofp = Fopen(options, "r"); 
	  b = Fgets(buf, MAX_BUF, ofp);
	  while( b != NULL)
	    {
	      if (kv_pair(buf, &k, &v))
		if(!set_key(k, v, opts))
		  FAIL();
	      b = Fgets(buf, MAX_BUF, ofp);
	    }
	  fclose(ofp);
	  if( (opts->write_only == 1)  && (opts->read_only == 1) )
	    {
	      opts->write_only = 0;
	      opts->read_only = 0;
	    }
	  if ( (ret = snprintf(opts->write_log, MAX_BUF, "%s/write.syscall.aves", opts->log_dir)) < 0)
	    FAIL();
	  if ( (ret = snprintf(opts->read_log, MAX_BUF, "%s/read.syscall.aves", opts->log_dir)) < 0)
	    FAIL();
	}
      else
	{
	  strncpy(opts->log_dir, ".", MAX_BUF);
	  strncpy(opts->testdir, ".", MAX_BUF);
	}
      while(opts->testdir[len++]);
    }

  mpi_bcast(&(opts->cns), 1, MPI_INT, opts->base, opts->comm);
  mpi_bcast(&(opts->ions), 1, MPI_INT, opts->base, opts->comm);
  mpi_bcast(&(len), 1, MPI_INT, opts->base, opts->comm);
  mpi_bcast(opts->testdir, len, MPI_CHAR, opts->base, opts->comm);
  mpi_bcast(&(opts->call_limit), 1, MPI_INT, opts->base, opts->comm);
  mpi_bcast(&(opts->call_size), 1, MPI_LONG_LONG, opts->base, opts->comm);
  mpi_bcast(&(opts->time_limit), 1, MPI_INT, opts->base, opts->comm);
  mpi_bcast(&(opts->tasks), 1, MPI_INT, opts->base, opts->comm);
  mpi_bcast(&(opts->write_only), 1, MPI_INT, opts->base, opts->comm);
  mpi_bcast(&(opts->read_only), 1, MPI_INT, opts->base, opts->comm);
  mpi_bcast(&(opts->iterations), 1, MPI_INT, opts->base, opts->comm);
  mpi_bcast(&(opts->pause), 1, MPI_INT, opts->base, opts->comm);
  /* Not sure if I really need these */
  mpi_bcast(&(opts->progress), 1, MPI_INT, opts->base, opts->comm);
  mpi_bcast(&(opts->profiles), 1, MPI_INT, opts->base, opts->comm);
  mpi_bcast(&(opts->verbosity), 1, MPI_INT, opts->base, opts->comm);
  
  return(opts);
}

void
Free_Opts()
{
  if(opts != NULL)
    free(opts);
}



BOOL
kv_pair(char *buf, char **k, char **v)
{
  char *eq;
  char *end;

  ASSERT(buf != NULL);
  *k = NULL;
  *v = NULL;

  eq = buf;
  while( (*eq != '#') && (*eq != '=') && (*eq != '\0') && (eq < buf + MAX_BUF - 1) ) eq++;
  if( (*eq == '#') || (*eq == '\0') || (eq == buf + MAX_BUF -1) ) return(FALSE);

  *k = buf;
  while( isspace(**k) && (*k < eq) ) (*k)++;
  if ( *k == eq) return(FALSE);

  end = *k + 1;
  while( ! isspace(*end) && (end < eq) ) end++;
  *end = '\0';

  end++;
  while( (*end != '#') && (*end != '\0') && (*end != '\n') && (end < buf + MAX_BUF - 1) ) end++;
  if ( (*end == '#') || (*end == '\n') )  end--;
 
  while( isspace(*end) ) end--;
  if (end == eq) return(FALSE);
  end++;
  *end = '\0';

  *v = eq + 1;
  while( isspace(**v) && (*v < end) ) (*v)++;
  return(TRUE);
}

BOOL
set_key(char *k, char *v, Options *opts)
{
  if(strncmp(k, "cluster", MAX_BUF) == 0)
    return(set_cluster(v, opts));
  if(strncmp(k, "cns", MAX_BUF) == 0)
    return(set_cns(v, opts));
  if(strncmp(k, "ions", MAX_BUF) == 0)
    return(set_ions(v, opts));
  if(strncmp(k, "testdir", MAX_BUF) == 0)
    return(set_testdir(v, opts));
  if(strncmp(k, "call_limit", MAX_BUF) == 0)
    return(set_call_limit(v, opts));
  if(strncmp(k, "call_size", MAX_BUF) == 0)
    return(set_call_size(v, opts));
  if(strncmp(k, "time_limit", MAX_BUF) == 0)
    return(set_time_limit(v, opts));
  if(strncmp(k, "tasks", MAX_BUF) == 0)
    return(set_tasks(v, opts));
  if(strncmp(k, "write_only", MAX_BUF) == 0)
    return(set_write_only(v, opts));
  if(strncmp(k, "read_only", MAX_BUF) == 0)
    return(set_read_only(v, opts));
  if(strncmp(k, "iterations", MAX_BUF) == 0)
    return(set_iterations(v, opts));
  if(strncmp(k, "pause", MAX_BUF) == 0)
    return(set_pause(v, opts));
  if(strncmp(k, "progress", MAX_BUF) == 0)
    return(set_progress(v, opts));
  if(strncmp(k, "profiles", MAX_BUF) == 0)
    return(set_profiles(v, opts));
  if(strncmp(k, "verbosity", MAX_BUF) == 0)
    return(set_verbosity(v, opts));
  return(FALSE);
}

void
show_keys(Options *opts)
{
  ASSERT(opts != NULL);
  if ( (opts->rank == opts->base) && (opts->verbosity >= NORMAL) )
    {
      printf("cluster    = %s\n", opts->cluster);      
      printf("cns        = %d\n", opts->cns);
      printf("ions       = %d\n", opts->ions);
      printf("write_log  = %s\n", opts->write_log);
      printf("read_log   = %s\n", opts->read_log);
      printf("testdir    = %s\n", opts->testdir);
      printf("call_limit = %d\n", opts->call_limit);
      printf("call_size  = %lld\n", opts->call_size);
      printf("time_limit = %d\n", opts->time_limit);
      printf("tasks      = %d\n", opts->tasks);
      printf("write_only = %d\n", opts->write_only);
      printf("read_only  = %d\n", opts->read_only);
      printf("iterations = %d\n", opts->iterations);
      printf("pause      = %d\n", opts->pause);
      printf("progress   = %d\n", opts->progress);
      printf("profiles   = %d\n", opts->profiles);
      printf("verbosity  = %d\n", opts->verbosity);
    }
}

/* the interaction with the log files for system call timings */
/* is the exclusive province of the base task */
void
write_log()
{
  if(opts->rank == opts->base) 
    {
      if(opts->lfd != NULL) fclose(opts->lfd);
      opts->lfd = Fopen(opts->write_log, "w");
    }
}

void
read_log()
{
  if(opts->rank == opts->base) 
    {
      if(opts->lfd != NULL) fclose(opts->lfd);
      opts->lfd = Fopen(opts->read_log, "w");
    }
}

void
close_log()
{
  if(opts->rank == opts->base) 
    {
      if(opts->lfd != NULL) fclose(opts->lfd);
      opts->lfd = NULL;
    }
}

void
log_it(char *fmt, char *str)
{
  ASSERT(opts->rank == opts->base);
  if(opts->rank == opts->base)
    Fprintf(opts->lfd, fmt, str);
}
/* End of log functions */

BOOL
set_cluster(char *v, Options *opts)
{
  int ret;

  if ( (ret = snprintf(opts->cluster, MAX_BUF, "%s", v)) < 0)
    FAIL();
  return(TRUE);
}

BOOL
set_cns(char *v, Options *opts)
{
  opts->cns = atoi(v);
  return(TRUE);
}

BOOL
set_ions(char *v, Options *opts)
{
  opts->ions = atoi(v);
  return(TRUE);
}

BOOL
set_testdir(char *v, Options *opts)
{
  int ret;

  if ( (ret = snprintf(opts->testdir, MAX_BUF, "%s", v)) < 0)
    FAIL();
  return(TRUE);
}

BOOL
set_call_limit(char *v, Options *opts)
{
  opts->call_limit = atoi(v);
  return(TRUE);
}

BOOL
set_call_size(char *v, Options *opts)
{
  int ret;
  char c;

  ret = sscanf(v, "%lld %c", &(opts->call_size), &c);
  if (ret < 1) FAIL();
  if(ret > 1)
    {
      switch((int)c)
	{
	case 'k':
	case 'K': opts->call_size*= 1024;
	  break;
	case 'm':
	case 'M': opts->call_size*= 1024*1024;
	  break;
	default: break;
	}
    }	  
  return(TRUE);
}

BOOL
set_time_limit(char *v, Options *opts)
{
  opts->time_limit = atoi(v);
  return(TRUE);
}

BOOL
set_tasks(char *v, Options *opts)
{
  opts->tasks = atoi(v);
  return(TRUE);
}

BOOL
set_write_only(char *v, Options *opts)
{
  lowercase(v);
  if(strncmp(v, "true", MAX_BUF) == 0)
    opts->write_only = 1;
  else
    opts->write_only = 0;
  return(TRUE);
}

BOOL
set_read_only(char *v, Options *opts)
{
  lowercase(v);
  if(strncmp(v, "true", MAX_BUF) == 0)
    opts->read_only = 1;
  else
    opts->read_only = 0;
  return(TRUE);
}

BOOL
set_iterations(char *v, Options *opts)
{
  opts->iterations = atoi(v);
  return(TRUE);
}

BOOL
set_pause(char *v, Options *opts)
{
  lowercase(v);
  if(strncmp(v, "true", MAX_BUF) == 0)
    opts->pause = 1;
  else
    opts->pause = 0;
  return(TRUE);
}

BOOL
set_progress(char *v, Options *opts)
{
  lowercase(v);
  if(strncmp(v, "false", MAX_BUF) == 0)
    opts->progress = 0;
  else
    opts->progress = 1;
  return(TRUE);
}

BOOL
set_profiles(char *v, Options *opts)
{
  lowercase(v);
  if(strncmp(v, "false", MAX_BUF) == 0)
    opts->profiles = 0;
  else
    opts->profiles = 1;
  return(TRUE);
}

BOOL
set_verbosity(char *v, Options *opts)
{
  opts->verbosity = atoi(v);
  return(TRUE);
}

void
lowercase(char *v)
{
  while(*v)
    {
      if( (*v >= 'A') && (*v <= 'Z') )
	*v += 'a' - 'A';
      v++;
    }
}

