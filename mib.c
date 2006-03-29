/*****************************************************************************\
 *  $Id: mib.c,v 1.19 2006/01/09 16:26:39 auselton Exp $
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
/*
 *   Mib is intended to be as simple as possible (but no simpler).  It
 * is intended that the results be easily interpreted on the one hand.
 * On the other, it is intended that the code might be easily extended
 * to particular purposes.  Central to the program is a test that has
 * each task perform I/O, of a fixed size per system call, to a file
 * named in a conventional way.  It is presumed, but not required,
 * that the file is on a parallel file system. It is required that the
 * task writing a given file and the task reading it both see the same
 * file, and those tasks will be distinct.  Detailed timing data for
 * all the system calls is sent to a file (one for writes and one for
 * reads) after the I/O is complete.
 *   Stonewalling tests have all the tasks stop after the same amount
 * of time.  Non-stonewalling tests allow all task to complete a fixed
 * number of system calls before stopping.  Whichever citerion is 
 * satisfied first ends I/O from a task.  It is possible for system 
 * call limits and time limits to be chosen so that some tasks finish
 * due to one and other tasks due to the other.  I don't see how 
 * that could be desirable, but is allowed and no special note is made
 * if it does happen.
 *   One particular special purpose is already implemented here, not
 * particularly well.  It checks that sibling tasks (all performing I/O
 * from the same node) proceed through their system calls uniformly,
 * i.e. in lock step.  If they do not, the fact is noted, but no other
 * action is taken.  That one section of code could be removed without
 * loss of functionality, and others could be put in its place.
 */


#include <stdio.h>      /* FILE, fopen, etc. */
#include <sys/types.h>  /* open, etc */
#include <sys/stat.h>   /* open, etc */
#include <fcntl.h>      /* open, etc */
#include <unistd.h>     /* unlink, ssize_t */
#include <stdlib.h>     /* free */
#include <string.h>     /* strncpy */
#include <errno.h>
#include <time.h>
#include <stdarg.h>    /* varargs */
#include "config.h"
#include "mpi_wrap.h"
#include "mib.h"
#include "miberr.h"
#include "options.h"
#include "slurm.h"
#include "mib_timer.h"
#include "sys_wrap.h"

void sign_on();
Mib *Init_Mib(int rank, int size);
double write_test();
char *fill_buff();
void init_status(char *str);
void status(int calls, double time);
double read_test();
Results *reduce_results(Results *res);
void profiles(double *array, int count, char *profile_log_name);
void report(double write, double read);
void base_report(int verb, char *fmt, ...);

Mib  *mib = NULL;
char *version=MIB_VERSION;
char *arch=MIB_ARCH;

extern Options *opts;
extern SLURM   *slurm;
extern int use_mpi;

int
main( int argc, char *argv[] )
{
  /*
   */
  int size = 0;
  int rank;
  double read = 0;
  double write = 0;
  int bail;

  /* initialize the slurm struct */
  get_SLURM_env();
  /* initialize the opts struct */
  command_line(&argc, &argv);
  /* initialize MPI */
  mpi_init( &argc, &argv );
  mpi_comm_size(MPI_COMM_WORLD, &size );
  mpi_comm_rank(MPI_COMM_WORLD, &rank );
  /* initialize the timer, check for skew, get the signon timestamp */
  init_timer(rank);
  sign_on();
  /* initialze the mib struct */
  mib = Init_Mib(rank, size);
  if(verbosity(SHOW_ENVIRONMENT)) show_details();
  if( mib->comm == MPI_COMM_NULL )
    {
      mpi_barrier(MPI_COMM_WORLD);
    }
  else
    {
      DEBUG("Initializations complete.\n");
      if(! check_flags(READ_ONLY) )
	{
	  write = write_test();
	  base_report(SHOW_INTERMEDIATE_VALUES, "Aggregate write rate = %10.2f\n", write);
	}
      if(! check_flags(WRITE_ONLY) )
	{
	  read = read_test();
	  base_report(SHOW_INTERMEDIATE_VALUES, "Aggregate read rate = %10.2f\n", read);
	}
      report(write, read);
      Free_Opts();
      if( mib->comm != MPI_COMM_WORLD)
	mpi_comm_free(&(mib->comm));
      mpi_barrier(MPI_COMM_WORLD);
    }
  mpi_finalize();
  exit(0);
}

void
sign_on()
{
  time_t t;
  char signon[MAX_BUF];
  char time_str[MAX_BUF];
  char *p;

  t = time(NULL);
  strncpy(time_str, ctime(&t), MAX_BUF);
  p = time_str;
  while( (*p != '\0') && (*p != '\n') && (p - time_str < MAX_BUF) )p++;
  if( *p == '\n' ) *p = '\0';
  sprintf(signon, "\n\nmib-%s-%s  %s\n\n", version, arch, time_str);
  base_report(SHOW_SIGNON, signon);
}

Mib *
Init_Mib(int rank, int size)
{
  Mib *mib;

  mib = (Mib *)Malloc(sizeof(Mib));
  /* 
   *   If we don't even have a slurm environment then act like there's 
   * just one of us.  Get your rank off of the hostname and be sure to
   * set size = 1 and base the same as rank.
   */
  mib->nodes = slurm->use_SLURM ? slurm->NNODES : 1;
  mib->tasks = slurm->use_SLURM ? slurm->NPROCS : 1;
  if( mib->tasks == mib->nodes ) opts->flags &= ~USE_NODE_AVES;

  mib->rank =  USE_MPI ? rank : (slurm->use_SLURM ? slurm->PROCID : get_host_index());
  mib->size =  USE_MPI ? size : (slurm->use_SLURM ? slurm->NPROCS : 1 );
  mib->base =  USE_MPI ? 0 : (slurm->use_SLURM ? 0 : mib->rank );
  mib->comm = MPI_COMM_WORLD;
  return(mib);
}

int
get_host_index()
{
  int rc;
  char name[MAX_BUF];
  char *p = name;

  if((rc = gethostname(name, MAX_BUF)) == -1)
    {
      return(0);
    }

  while ( *p && ((*p < '0') || (*p > '9')) ) p++;
  if(*p) return(atoi(p));
  return(0);
}

double
write_test()
{
  /*
   *   The results array hold the timings of the various stages of the 
   * test as well as particulars, like how many system calls were actually
   * made in this task.  The "timings" field is an array of doubles that
   * will hold the return values from checking the time at the end of each
   * system call.  Wouldn't it be cool to get all sorts of other diagnostic
   * info at the end of each system call?
   */

  int call = 0;
  int last_call;
  char *buf;          /* what gets sent in the system call */
  int wf;             /* the file handle for the writes */
  int flag = O_WRONLY | O_CREAT;/* the flag value for the open */
  double time_limit;
  double time;
  double rate = 0;
  ssize_t xfer;
  Results *res;
  Results *red;       /* individual results are reduced into this struct */
  int ret;
  char write_target[MAX_BUF];
  int last_write_call;

  DEBUG("Starting write test.\n");
  res = (Results *)Malloc(sizeof(Results));
  res->timings = (double *)Malloc(opts->call_limit*opts->time_limit*sizeof(double));
  buf = fill_buff();
  snprintf(write_target, MAX_BUF, "%s/mibData.%08d", opts->testdir, mib->rank);
  /*
   *  This motif is repeated several times.  You get a barrier then an
   * action, or an action then a barrier, or both.  Timestamps are
   * taken at each step.  Here the file is being opened.  It better
   * already be there or mib will fail.  Us the "file-layout.sh"
   * utility.
   */
  mpi_barrier(mib->comm);
  res->before_unlink = get_time();
  if( check_flags(NEW) ) 
    {
      Unlink(write_target);
    }
  res->start_open = get_time();
  wf = Open(write_target, flag);
  res->end_open = get_time();
  mpi_barrier(mib->comm);

  /*
   *   This just intializes a few things for the loop
   */
  res->transferred = 0;
  res->short_transfers = 0;
  init_status("write phase");
  res->start = res->timings[call] = get_time();
  time_limit = res->timings[call] + opts->time_limit;

  /*
   *   This is the meat of the test.  The loop is terminated by the
   * first occurance of out-of-time or out-of-space.  It might be
   * interesting to see what happens if you barrier at ieach
   * iteration.  Haven't tried that.
   */
  do
    {
      call++;
      xfer = Write(wf, buf, opts->call_size);
      if (xfer < 0) printf("call %d wrote %d\n",call, xfer);
      res->transferred += xfer;
      if (xfer < opts->call_size) res->short_transfers++; 
      res->timings[call] = get_time();
      status(call, res->timings[call] - res->timings[0]);
    }
  while( (call < opts->call_limit) && (res->timings[call] < time_limit) );
  base_report(SHOW_PROGRESS, "\n");

  /*
   *   Some final data points after the loop.
   */
  res->num = call;
  res->end = res->timings[call];
  /*
   *   I don't really consider this fsync optional, but it is important to
   * isolate its effect.  As far as the tasks are concerned the test is
   * over, but the file system is still chewing.
   */
  Fsync(wf);
  res->finish_fsync = get_time();
  mpi_barrier(mib->comm);

  /*
   *   Now to close the files.  This version does not report anything
   * about file open and file close times, though it could if that
   * were of interest.
   */
  res->start_close = get_time();
  Close(wf);
  res->end_close = get_time();
  /* 
   * N.B. there is no optional unlink at the end of the writes
   * Note to self: What happens when this is a write_only test and files are to be
   * removed at the end?
   */
  res->after_unlink = get_time();
  mpi_barrier(mib->comm);
  res->end_test = get_time();
  free(buf);
  /*
   *   The test is over, now it's time to report out the results.
   */

  /*
   *  We want all the tasks to have valid data for the entire period
   *  of the test, even if they didn't record a time.  This fills in
   *  zeros as necessary.
   */
  last_call = call;
  mpi_allreduce(&(last_call), &(last_write_call), 1, MPI_INT, MPI_MAX, mib->comm);
  base_report(SHOW_INTERMEDIATE_VALUES, "Call %d was the last write recorded\n", last_write_call);
  while(call < last_write_call)
    {
      call++;
      res->timings[call] = 0;
    }
  /*
   *   Subsequent reports will be in MB rather than bytes.
   */
  res->transferred /= (1024*1024);
  base_report(SHOW_INTERMEDIATE_VALUES, 
	      "\nAfter %d calls and %f seconds\n", 
	      last_write_call, res->end_test - res->start_open);

  if(USE_MPI)
    {
      /*
       *   Red will end up with the aggregate amout of data transfered and the
       * earliest and latest values for the various timestamps.  From that it
       * is a simple calculation to get the aggregate data rate.
       */
      red = reduce_results(res);
      if(mib->rank == mib->base)
	{
	  time = red->finish_fsync - red->start;
	  if(time != 0)
	    rate = red->transferred/time;
	}
      base_report(SHOW_INTERMEDIATE_VALUES, "%f MB written in %f seconds\n", red->transferred, time);
      if ( red->short_transfers != 0 )
	base_report(SHOW_INTERMEDIATE_VALUES, "%d short writes\n", red->short_transfers);
      free(red);
      
      if( opts->profiles != NULL )
	{
	  /*
	   *   The job of reporting out details of the system call timings is
	   *   left to this function.  The info goes to the
	   *   write.syscalls.aves file in the log directory.  A summary of
	   *   the unevenness of the system calls, if any, is reported to
	   *   stdout.
	   */
	  DEBUG("Write table:\n");
	  profiles(res->timings, last_write_call + 1, "write");
	}
    }
  else
    {
      time = res->finish_fsync - res->start;
      if(time != 0)
	rate = res->transferred/time;
    }
  free(res->timings);
  free(res);
  DEBUG("Write complete.\n");
  return(rate);
}

Results *
reduce_results(Results *res)
{
  /*
   * The results I'm interested in are:
   * Min_{n} start_open
   * Max_{n} end_open
   * Min_{n} start
   * timings_{node, call}
   * Sum_{n} transferred
   * Max_{n} finish_fsync
   * Min_{n} start_close
   * Max_{n} end_close
   * All but the arrays can be brought to rank == base.
   * The arrays will need individual node subcommunicators, and then
   * a fancy subcommunicator to bring all the node arrays to the 
   * rank == base node.
   *  I can stuff all the easy ones into another Results struct.  I'll
   * have to do something different for the node arrays.  I'll fob that 
   * off to the profiles() routine.
   */
  Results *red;

  red = (Results *)Malloc(sizeof(Results));
  red->timings = NULL;

  mpi_reduce(&(res->before_unlink), &(red->before_unlink), 1, MPI_DOUBLE, 
	     MPI_MIN, mib->base, mib->comm);
  mpi_reduce(&(res->start_open), &(red->start_open), 1, MPI_DOUBLE, 
	     MPI_MIN, mib->base, mib->comm);
  mpi_reduce(&(res->end_open), &(red->end_open), 1, MPI_DOUBLE, 
	     MPI_MAX, mib->base, mib->comm);
  mpi_reduce(&(res->start), &(red->start), 1, MPI_DOUBLE, 
	     MPI_MIN, mib->base, mib->comm);
  mpi_reduce(&(res->transferred), &(red->transferred), 1, MPI_DOUBLE, 
	     MPI_SUM, mib->base, mib->comm);
  mpi_reduce(&(res->short_transfers), &(red->short_transfers), 1, MPI_INT, 
	     MPI_SUM, mib->base, mib->comm);
  mpi_reduce(&(res->end), &(red->end), 1, MPI_DOUBLE, 
	     MPI_MAX, mib->base, mib->comm);
  mpi_reduce(&(res->finish_fsync), &(red->finish_fsync), 1, MPI_DOUBLE, 
	     MPI_MAX, mib->base, mib->comm);
  mpi_reduce(&(res->start_close), &(red->start_close), 1, MPI_DOUBLE, 
	     MPI_MIN, mib->base, mib->comm);
  mpi_reduce(&(res->end_close), &(red->end_close), 1, MPI_DOUBLE, 
	     MPI_MAX, mib->base, mib->comm);
  mpi_reduce(&(res->after_unlink), &(red->after_unlink), 1, MPI_DOUBLE, 
	     MPI_MIN, mib->base, mib->comm);
  mpi_reduce(&(res->end_test), &(red->end_test), 1, MPI_DOUBLE, 
	     MPI_MAX, mib->base, mib->comm);
  return(red);
}


#define LOWER_BITS 44
char *
fill_buff()
{
  /*
   */
  char *buffer;
  unsigned long long count, hi, lo, lomask;
  unsigned long long *buf;


  buffer = (char *)Malloc(opts->call_size);
  buf  = (unsigned long long *)buffer;
  lomask = -1ULL >> (sizeof(unsigned long long)*8 - LOWER_BITS);
  hi = mib->rank;
  hi = hi << LOWER_BITS;
  for (count = 0; count < opts->call_size / sizeof(unsigned long long); count++) {
    lo = (count * sizeof(unsigned long long)) & lomask;
    buf[count] = hi | lo;
  }
  return(buffer);
}

#define EXPECTATION 50
static int progress;
static char pbuf[EXPECTATION];

void
init_status(char *str)
{
  /*
   *   If the options call for a progress meter then the following
   * expectation metric is sent to stdout.  An array of '*' characters
   * will provide the progress measure.
   */
  int i;

  progress = 0;
  for(i = 0; i < EXPECTATION - 1; i++)
    pbuf[i] = '*';
  pbuf[EXPECTATION - 1] = '\0';
  base_report(SHOW_PROGRESS, "\n%s\nshould last about this long---------------------->\n", str);
}


void
status(int calls, double time)
{
  /*
   *   When status is called zero, one, or more of the
   * EXPECTATION fractions of the total test may have
   * elapsed.  Figure out how many and print that many '*' 
   * characters.
   */
  int current_time  = (EXPECTATION*time)/opts->time_limit;
  int current_calls = (EXPECTATION*calls)/opts->call_limit;
  int current;
  int i;

  current = (current_time < current_calls) ? current_calls : current_time;
  if( (current > progress) && (current < EXPECTATION - 1) )
    {
      pbuf[current-progress] = '\0';

      base_report(SHOW_PROGRESS, pbuf);
      pbuf[current-progress] = '*';
      progress = current;
    }
  
}

double
read_test()
{
  /*
   *   This test proceeds identically with the write test except that
   * there is no fsync() call for reading.  Stick a zero in that 
   * spot and proceed as before.
   */
  int call = 0;
  int last_call;
  char *buf;
  int rf;
  ssize_t xfer;
  double time_limit;
  double time;
  double rate = 0;
  Results *red = NULL;
  Results *res = NULL;
  int ret;
  char read_target[MAX_BUF];
  int read_rank = ((mib->rank + (mib->tasks/2)) % mib->tasks) + mib->base;
  int last_read_call;
  off_t offset;
  double gran;
  struct stat stats;

  DEBUG("Starting read.\n");
  res = (Results *)Malloc(sizeof(Results));
  res->timings = (double *)Malloc(opts->call_limit*opts->time_limit*sizeof(double));
  buf = (char *)Malloc(opts->call_size);
  snprintf(read_target, MAX_BUF, "%s/mibData.%08d", opts->testdir, read_rank);

  /* 
   * N.B. there is no optional unlink at the beginning of the reads 
   * Note to self: What happens when this is a read_only test and there 
   * are no target files?
   */
  res->before_unlink = get_time();
  /*
   *   Open the target file.  
   */
  if( ! Exists(read_target) )
    {
      printf("No file: %s\n", read_target);
      fflush(stdout);
      FAIL();
    }
  mpi_barrier(mib->comm);
  res->start_open = get_time();
  rf = Open(read_target, O_RDONLY);
  res->end_open = get_time();
  /* 
   * If we're doing random reads then we sneek the fstat call in here 
   * A similar sort of check might be appropriate for 
   * stat.st_size <  opts->call_size
   * reagardless of whether we're random seeking or not. 
   */
  if( check_flags(RANDOM_READS) ) 
    {
      Fstat(rf, &stats);
      if (stats.st_size < opts->granularity)
	{
	  printf("In task %d, the file size (%d) is too small for the granularity (%ld)\n", 
		      mib->rank, stats.st_size, opts->granularity);
	  fflush(stdout);
	  if (USE_MPI) {FAIL();} else exit(1);
	}
      gran = stats.st_size/(opts->granularity*RAND_MAX);
    }
  mpi_barrier(mib->comm);

  /*
   *   Initialize some loop control variables.
   */
  res->transferred = 0;
  res->short_transfers = 0;
  init_status("read phase");
  res->start = res->timings[call] = get_time();
  time_limit = res->timings[call] + opts->time_limit;

  /*
   *   Loop until out of time or out of space, whichever occurs 
   * first.
   */
  do
    {
      call++;
      if( check_flags(RANDOM_READS) ) 
	{
	  offset = gran*rand();
	  Lseek(rf, offset, SEEK_SET);
	}
      xfer = Read(rf, buf, opts->call_size);
      res->transferred += xfer;
      if( xfer < opts->call_size )
	res->short_transfers++;
      res->timings[call] = get_time();
      status(call, res->timings[call] - res->timings[0]);
    }
  while( (call < opts->call_limit) && (res->timings[call] < time_limit) );
  base_report(SHOW_PROGRESS, "\n");

  /* 
   *   Note the time when done.
   */
  res->num = call;
  res->end = res->timings[call];
  res->finish_fsync = 0;  /* reads don't use this value */

  /*
   *   Close the file.
   */
  mpi_barrier(mib->comm);
  res->start_close = get_time();
  Close(rf);
  res->end_close = get_time();
  if( check_flags(REMOVE) ) 
    {
      Unlink(read_target);
    }
  res->after_unlink = get_time();
  mpi_barrier(mib->comm);
  res->end_test = get_time();
  free(buf);

  /*
   *   The test is over, now it's time to report out the results.
   */

  /*
   *  We want all the tasks to have valid data for the entire period
   *  of the test, even if they didn't record a time.  This fills in
   *  zeros as necessary.
   */
  last_call = call;
  mpi_allreduce(&(last_call), &(last_read_call), 1, MPI_INT, MPI_MAX, mib->comm);
  base_report(SHOW_INTERMEDIATE_VALUES, "Call %d was the last read recorded\n", last_read_call);
  while(call < last_read_call)
    {
      call++;
      res->timings[call] = 0;
    }
  /*
   *   Subsequent reports will be in MB rather than bytes.
   */
  res->transferred /= (1024*1024);
  base_report(SHOW_INTERMEDIATE_VALUES, 
	      "\nAfter %d calls and %f seconds\n", 
	      last_read_call, res->end_test - res->start_open);

  if(USE_MPI)
    {
      /*
       *   Red will end up with the aggregate amout of data transfered and the
       * earliest and latest values for the various timestamps.  From that it
       * is a simple calculation to get the aggregate data rate.
       */
      red = reduce_results(res);
      if(mib->rank == mib->base)
	{
	  time = red->end - red->start;
	  if(time != 0)
	    rate = red->transferred/time;
	}
      base_report(SHOW_INTERMEDIATE_VALUES, 
		  "%f MB read in %f seconds\n", red->transferred, time);
      if ( red->short_transfers != 0 )
	base_report(SHOW_INTERMEDIATE_VALUES, "%d short reads\n", red->short_transfers);
      free(red);
      
      if( opts->profiles != NULL )
	{
	  /*
	   *   The job of reporting out details of the system call timings is
	   *   left to this function.  The info goes to the
	   *   read.syscalls.aves file in the log directory.  A summary of
	   *   the unevenness of the system calls, if any, is reported to
	   *   stdout.
	   */
	  DEBUG("Read table:\n");
	  profiles(res->timings,  last_read_call + 1, "read");
	  DEBUG("Reductions complete.\n");
	}
    }
  else
    {
      time = res->end - res->start;
      if(time != 0)
	rate = res->transferred/time;      
    }
  free(res->timings);
  free(res);
  DEBUG("Read complete.\n");
  return(rate);
}

void
profiles(double *array,   int count, char *io_direction)
{
  /*
   *   Create subcommunicators for every group of 
   * sibling tasks.  Then each can do a reduction to get averages
   * for the node.  Release those subcommunicators.
   *   Create a subcommunicator out of the <base> from each of
   * the previous subcommunicators.  Send all the data to the 
   * base of this new subcommunicator (also the base of MPI_WORLD),
   * so that it can be sent to the NFS target directory.
   *   Keep in mind that this function requires the "mib->nodes"
   * configuration parameter to be set correctly.  If it is not 
   * then the computations are not going to be valid, and there 
   * may even be stability issues.  In particular, default operation
   * should insure that profiling is OFF.
   */
  int node, task;
  MPI_Comm node_comm;
  int node_base = 0;
  int node_group = 0;
  int call, gap, max_gap, min_gap, ave_gap;
  double *table;
  char *buffer;
      /*
       *   BUF_LIMIT is a hack.  I want enough space so that each
       * NODE can print one double.  I do this rather than having
       * <num_nodes> separate print statements.  That would be very 
       * slow.
       */
  int BUF_LIMIT;
  char *ip;
  double *node_min, *node_max, *node_ave;
  int node_size, node_rank;
  int ret;
  int i;
  int tasks_per_node = mib->tasks/mib->nodes;
  FILE *lfd;
  char profile_log_name[MAX_BUF];

  if(mib->rank == mib->base)
    {
      if ( (ret = snprintf(profile_log_name, MAX_BUF, "%s.%s", opts->profiles, io_direction)) < 0)
	FAIL();
      lfd = Fopen(profile_log_name, "w");
    }

  /*
   *   The <node, task> pair is unique to each task.  The mpi spli
   * creates a subcommunicator corresponding to the sibling tasks 
   * associated with each NODE.  The task = 0 task in each 
   * subcommunicator is the base to which the others send their data.
   */
  if ( ! check_flags(USE_NODE_AVES) )
    {
      node = mib->rank;
      task = mib->rank;
      BUF_LIMIT = 20*mib->tasks;
      buffer = Malloc(BUF_LIMIT);
      DEBUG("Table\n");
      base_report(SHOW_INTERMEDIATE_VALUES, 
		  "Table of %d tasks with up to %d system calls\n", mib->tasks, count);
      table = (double *)Malloc(mib->tasks*sizeof(double));
      for(call = 0; call < count; call++)
	{
	  mpi_gather(&(array[call]), 1, MPI_DOUBLE, table, 1, MPI_DOUBLE, mib->base, mib->comm);
	  if( mib->rank == mib->base )
	    {
	      ip = buffer;
	      for(i = 0; i < mib->tasks; i++)
		{
		  /* N.B.  the following can only have one double,
		     since varargs doesn't work.  */
		  ret = Snprintf(ip, BUF_LIMIT - (ip - buffer), "%f\t", table[i]);
		  ip += ret;
		  *ip = '\0';
		}
	      Fprintf(lfd, "%s\n", buffer);
	    }
	}
    }
  else
    {
      node = mib->rank/tasks_per_node;
      task = mib->rank % tasks_per_node;

      BUF_LIMIT = 20*mib->nodes;
      buffer = Malloc(BUF_LIMIT);
      DEBUG("About to execute the first split.\n");
      mpi_comm_split(mib->comm, node, task, &node_comm);
      
      /*
       *   The primary goal of this exercise is to get the avearge
       * behavior accross each NODE and send that array of averages 
       * to the MPI_WORLD base task for output to NFS.
       *   Prior to send the averages this code makes a quick check
       * to see that, within the set of sibling tasks, the timings are
       * in lock step, or close to it.
       */
      node_min = (double *) Malloc(count*sizeof(double));
      node_max = (double *) Malloc(count*sizeof(double));
      node_ave = (double *) Malloc(count*sizeof(double));
      DEBUG("About to reduce the sum, max, and min of the call values.\n");
      /* Get the NODEs' min, max, and ave at each step */
      for(call = 0; call < count; call++)
	{
	  mpi_reduce(&(array[call]), &(node_min[call]), 1, MPI_DOUBLE, MPI_MIN, node_base, node_comm);
	  mpi_reduce(&(array[call]), &(node_max[call]), 1, MPI_DOUBLE, MPI_MAX, node_base, node_comm);
	  mpi_reduce(&(array[call]), &(node_ave[call]), 1, MPI_DOUBLE, MPI_SUM, node_base, node_comm);
	}
      if(task == node_base)
	{
	  gap = 0;
	  for(call = count - 1; call >= 0; call--)
	    {
	      /* 
	       *   We want two things here.  1)   The largest gap on the NODE 
	       * where one task has completed n syscalls and a sibling has 
	       * completed no more then n - gap.
	       *   2)  If some tasks never got to call n then don't record any
	       * average value for that step on that NODE (i.e., set it to 0).
	       */
	      while( (node_min[call] > 0) &&((call - gap) > 1) && 
		     (node_max[call - gap - 1] > node_min[call]) ) gap++;
	      if(node_min[call] == 0)
		node_ave[call] = 0;
	      else
		node_ave[call] /= tasks_per_node;
	    }
	}
      free(node_min);
      free(node_max);
      mpi_comm_free(&node_comm);

      DEBUG("Have the NODE averages, now about the table...\n");

      /*
       *    Now we've got the average per call, and gap values for each
       * NODE on its base node.  We'll want to make a communicator of base
       * nodes and gather in all that data.
       */
      
      DEBUG("Done with first subcommunicator.  About to create the second.\n");
      mpi_comm_split(mib->comm, task, node, &node_comm);
      mpi_comm_size(node_comm, &node_size );
      mpi_comm_rank(node_comm, &node_rank );
      table = (double *)Malloc(mib->nodes*sizeof(double));
      if( task == node_group )
	{
	  DEBUG("Table\n");
	  for(call = 0; call < count; call++)
	    {
	      mpi_gather(&(node_ave[call]), 1, MPI_DOUBLE, table, 1, MPI_DOUBLE, node_base, node_comm);
	      if( node_rank == node_base )
		{
		  ip = buffer;
		  for(i = 0; i < mib->nodes; i++)
		    {
		      /* N.B.  the following can only have one double,
			 since varargs doesn't work.  */
		      ret = Snprintf(ip, BUF_LIMIT - (ip - buffer), "%f\t", table[i]);
		      ip += ret;
		      *ip = '\0';
		    }
		  Fprintf(lfd, "%s\n", buffer);
		}
	    }
	  DEBUG("\n\nAbout to reduce the gap values.\n");
	  mpi_reduce(&gap, &max_gap, 1, MPI_INT, MPI_MAX, node_base, node_comm);
	  mpi_reduce(&gap, &min_gap, 1, MPI_INT, MPI_MIN, node_base, node_comm);
	  mpi_reduce(&gap, &ave_gap, 1, MPI_INT, MPI_SUM, node_base, node_comm);
	  if(node == node_base) ave_gap /= mib->nodes;
	}
      free(node_ave);
      free(table);
      free(buffer);
      mpi_comm_free(&node_comm);
      base_report(SHOW_INTERMEDIATE_VALUES, "%s%s\t\t%d\t%d\t%d\n", 
			    "\tNODEs' syscalls gaps (i.e dispersion or unevenness)\n",
			    "\t\taverage\tmin\tmax\n",
			    ave_gap, min_gap, max_gap);
    }
  if(mib->rank == mib->base)
    fclose(lfd);
}

void
report(double write, double read)
{
  time_t t;
  long long range = 1024;
  char range_ch = 'k';
  int xfer;
  char time_str[MAX_BUF];
  char *p;

  t = time(NULL);
  strncpy(time_str, ctime(&t), MAX_BUF);
  p = time_str;
  while( (*p != '\0') && (*p != '\n') && (p - time_str < MAX_BUF) )p++;
  if( *p == '\n' ) *p = '\0';
  if(opts->call_size > 1024*1024) 
    {
      range = 1024*1024;
      range_ch = 'M';
    }
  xfer = opts->call_size/range;
  if ( USE_MPI )
    {
      
      base_report(SHOW_HEADERS, 
		  "%14s           %6s %5s %5s %5s %10s %10s\n", "date", "tasks", "xfer", "call", "time", "write", "read");
      base_report(SHOW_HEADERS, 
		  "%24s %6s %5s %5s %5s %10s %10s\n", " ", " ", " ", "limit", "limit", "MB/s", "MB/s");
      base_report(SHOW_HEADERS, 
		  "%24s %6s %5s %5s %5s %10s %10s\n", "------------------------", "------", "-----", "-----", "-----", "----------", "----------");
      base_report(SHOW_ALL, 
		  "%s %6d %4d%c %5d %5d %10.2f %10.2f\n", time_str, mib->tasks, xfer, range_ch, opts->call_limit, opts->time_limit, write, read);
    }
  else
    {
      base_report(SHOW_HEADERS, 
		  "%6s %14s           %6s %5s %5s %5s %10s %10s\n", "task", "date", "tasks", "xfer", "call", "time", "write", "read");
      base_report(SHOW_HEADERS, 
		  "%6s %24s %6s %5s %5s %5s %10s %10s\n", " ", " ", " ", " ", "limit", "limit", "MB/s", "MB/s");
      base_report(SHOW_HEADERS, 
		  "%6s %24s %6s %5s %5s %5s %10s %10s\n", "------", "------------------------", "------", "-----", "-----", "-----", "----------", "----------");
      printf("%6d %s %6d %4d%c %5d %5d %10.2f %10.2f\n", mib->rank, time_str, mib->tasks, xfer, range_ch, opts->call_limit, opts->time_limit, write, read);
      fflush(stdout);
    }
}

void
base_report(int verb, char *fmt, ...)
{
  va_list args;
  
  /*
   * If this is called before mib and slurm are initialized then just
   * print.  If slurmis initialized but mid is not, then print from the
   * PROCID = 0 task.  If mib is initialized then print from its base task.
   * Don't print unless the verbosity level says to.
   */
  if( (((mib != NULL) && (mib->rank == mib->base)) ||
       ((mib == NULL) && (slurm != NULL) && (slurm->PROCID == 0)) ||
       ((mib == NULL) && (slurm == NULL)))
      && verbosity(verb) )
    {
      va_start(args, fmt);
      vprintf(fmt, args);
      fflush(stdout);
     va_end(args);
    }
}
