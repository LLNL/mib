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
#include "mib.h"
#include "mpi_wrap.h"
#include "options.h"
#include "mib_timer.h"
#include "sys_wrap.h"
#include "mib_timer.h"

void init_filenames();
double write_test();
char *fill_buff();
void init_status(char *str);
void status(int calls, double time);
double read_test();
Results *reduce_results(Results *res);
void profiles(double *array, int count);
void report(double write, double read);

extern Options *opts;

char *version="mib-1.7";

int
main( int argc, char *argv[] )
{
  /*
   *   The command line is sorted out in the "options.c" module.
   * From there you get the options file, which is required for the 
   * proper operation of the program.  The test may iterate, and 
   * the options file is reread on each iteration.
   *   Opts is the structure that hold all the global information
   * concerning the conduct of the test, including where to record
   * its results.  Summary and debug information is sent to stdout.
   * 
   */
  int size;
  int rank;
  char opt_path[MAX_BUF];
  int iteration = 0;
  BOOL stop;
  double read = 0;
  double write = 0;

  mpi_init( &argc, &argv );
  mpi_comm_size(MPI_COMM_WORLD, &size );
  mpi_comm_rank(MPI_COMM_WORLD, &rank );
  init_timer(rank);
  command_line(argc, argv, opt_path, rank);
  do
    {
      opts = read_options(opt_path, rank, size);
      if( opts->comm == MPI_COMM_NULL )
	{
	  mpi_barrier(MPI_COMM_WORLD);
	}
      else
	{
	  init_filenames();
	  DEBUG("Initializations complete.\n");
	  if(!opts->read_only)
	    {
	      write = write_test();
	      if ( ( opts->rank == opts->base) && (opts->verbosity >= VERBOSE) )
		{
		  printf("Aggregate write rate = %10.2f\n", write);
		  fflush(stdout);
		}
	    }
	  if(!opts->write_only)
	    {
	      read = read_test();
	      if ( ( opts->rank == opts->base) && (opts->verbosity >= VERBOSE) )
		{
		  printf("Aggregate read rate = %10.2f\n", read);
		  fflush(stdout);
		}
	    }
	  report(write, read);
	  iteration++;
	  stop = (BOOL)(iteration >= opts->iterations);
	  Free_Opts();
	  close_log();
	  if( opts->comm != MPI_COMM_WORLD)
	    mpi_comm_free(&(opts->comm));
	  mpi_barrier(MPI_COMM_WORLD);
	}
    }
  while(! stop);
  mpi_finalize();
}

void
init_filenames()
{
  /*
   *   The file that is the target for the write system calls is given
   * a conventional name based on the target directory and the task number.
   * The read target is similarly named but based on a task number "180
   * degrees" around the job, so to speek.  This is intended to guarantee,
   * as best one may, that the reader will not exploit locally cached 
   * data from the write. 
   */

  int read_rank = (opts->rank + (opts->tasks/2)) % opts->tasks;

  snprintf(opts->write_target, MAX_BUF, "%s/mibData.%08d", opts->testdir, opts->rank);
  snprintf(opts->read_target, MAX_BUF, "%s/mibData.%08d", opts->testdir, read_rank);
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
  int flag = O_WRONLY;/* the flag value for the open */
  double time_limit;
  double time;
  double rate = 0;
  ssize_t xfer;
  Results *res;
  Results *red;       /* individual results are reduced into this struct */

  DEBUG("Starting write test.\n");
  res = (Results *)Malloc(sizeof(Results));
  res->timings = (double *)Malloc(opts->call_limit*opts->time_limit*sizeof(double));
  buf = fill_buff();
  /*
   *  This motif is repeated several times.  You get a barrier then an
   * action, or an action then a barrier, or both.  Timestamps are
   * taken at each step.  Here the file is being opened.  It better
   * already be there or mib will fail.  Us the "file-layout.sh"
   * utility.
   */
  mpi_barrier(opts->comm);
  res->start_open = get_time();
  if( opts->new) 
    {
      Unlink(opts->write_target);
      flag |= O_CREAT;
    }
  wf = Open(opts->write_target, flag);
  res->end_open = get_time();
  mpi_barrier(opts->comm);

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
  if( (opts->base == opts->rank) && (opts->progress) )
    {
      printf("\n");
      fflush(stdout);
    }

  /*
   *   Some final data points after the loop.
   */
  res->num = call;
  res->end = res->timings[call];
  /*
   *   I don't really consider this fsync optional, but it is important to
   * isolate its effect.  As far as the CNs are concerned the test is
   * over, but the file system is still chewing.
   */
  Fsync(wf);
  res->finish_fsync = get_time();
  mpi_barrier(opts->comm);

  /*
   *   Now to close the files.  This version does not report anything
   * about file open and file close times, though it could if that
   * were of interest.
   */
  res->start_close = get_time();
  Close(wf);
  res->end_close = get_time();
  mpi_barrier(opts->comm);
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
  mpi_allreduce(&(last_call), &(opts->last_write_call), 1, MPI_INT, MPI_MAX, opts->comm);
  if ( (opts->rank == opts->base) && (opts->verbosity >= VERBOSE) )
    {
      printf("Call %d was the last write recorded\n", opts->last_write_call);
    }
  while(call < opts->last_write_call)
    {
      call++;
      res->timings[call] = 0;
    }
  /*
   *   Subsequent reports will be in MB rather than bytes.
   */
  res->transferred /= (1024*1024);
  if ( (opts->base == opts->rank) && (opts->verbosity >= NORMAL) )
    printf("\nAfter %d calls and %f seconds\n", opts->last_write_call, res->end_test - res->start_open);

  /*
   *   Red will end up with the aggregate amout of data transfered and the
   * earliest and latest values for the various timestamps.  From that it
   * is a simple calculation to get the aggregate data rate.
   */
  red = reduce_results(res);
  if(opts->rank == opts->base)
    {
      time = red->finish_fsync - red->start;
      if(time != 0)
	rate = red->transferred/time;
      if ( opts->verbosity >= VERBOSE )
	{
	  printf("%f MB written in %f seconds\n", red->transferred, time);
	  if ( red->short_transfers != 0 )
	    printf("%d short writes\n", red->short_transfers);
	}
    }
  free(red);

  if(opts->profiles)
    {
      /*
       *   The job of reporting out details of the system call timings is
       *   left to this function.  The info goes to the
       *   write.syscalls.aves file in the log directory.  A summary of
       *   the unevenness of the system calls, if any, is reported to
       *   stdout.
       */
      DEBUG("Write table:\n");
      write_log();
      profiles(res->timings, opts->last_write_call + 1);
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
   * timings_{ION, call}
   * Sum_{n} transferred
   * Max_{n} finish_fsync
   * Min_{n} start_close
   * Max_{n} end_close
   * All but the arrays can be brought to rank == base.
   * The arrays will need individual ION subcommunicators, and then
   * a fancy subcommunicator to bring all the ION arrays to the 
   * rank == base node.
   *  I can stuff all the easy ones into another Results struct.  I'll
   * have to do something different for the ION arrays.  I'll fob that 
   * off to the profiles() routine.
   */
  Results *red;

  red = (Results *)Malloc(sizeof(Results));
  red->timings = NULL;

  mpi_reduce(&(res->start_open), &(red->start_open), 1, MPI_DOUBLE, 
	     MPI_MIN, opts->base, opts->comm);
  mpi_reduce(&(res->end_open), &(red->end_open), 1, MPI_DOUBLE, 
	     MPI_MAX, opts->base, opts->comm);
  mpi_reduce(&(res->start), &(red->start), 1, MPI_DOUBLE, 
	     MPI_MIN, opts->base, opts->comm);
  mpi_reduce(&(res->transferred), &(red->transferred), 1, MPI_DOUBLE, 
	     MPI_SUM, opts->base, opts->comm);
  mpi_reduce(&(res->short_transfers), &(red->short_transfers), 1, MPI_INT, 
	     MPI_SUM, opts->base, opts->comm);
  mpi_reduce(&(res->end), &(red->end), 1, MPI_DOUBLE, 
	     MPI_MAX, opts->base, opts->comm);
  mpi_reduce(&(res->finish_fsync), &(red->finish_fsync), 1, MPI_DOUBLE, 
	     MPI_MAX, opts->base, opts->comm);
  mpi_reduce(&(res->start_close), &(red->start_close), 1, MPI_DOUBLE, 
	     MPI_MIN, opts->base, opts->comm);
  mpi_reduce(&(res->end_close), &(red->end_close), 1, MPI_DOUBLE, 
	     MPI_MAX, opts->base, opts->comm);
  mpi_reduce(&(res->end_test), &(red->end_test), 1, MPI_DOUBLE, 
	     MPI_MAX, opts->base, opts->comm);
  return(red);
}


#define LOWER_BITS 44
char *
fill_buff()
{
  /*
   * Since it must be compatible with IOR tests (at least
   * simple ones) this is borrowed (and suitably adapted) directly 
   * from IOR.  It is the only non-original code in mib.
   * N.B.  IOR sensibly does some gymnastics to get a page-alligned 
   * buffer.  On BGL this is probably moot, but might be interesting
   * to look into.
   */
  char *buffer;
  unsigned long long count, hi, lo, lomask;
  unsigned long long *buf;


  buffer = (char *)Malloc(opts->call_size);
  buf  = (unsigned long long *)buffer;
  lomask = -1ULL >> (sizeof(unsigned long long)*8 - LOWER_BITS);
  hi = opts->rank;
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
  if( (opts->base == opts->rank) && (opts->progress) )
    {
      printf("\n%s\nshould last about this long---------------------->\n", str);
      fflush(stdout);
    }
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
      if( (opts->base == opts->rank) && (opts->progress) )
	{
	  printf(pbuf);
	  fflush(stdout);
	}
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

  DEBUG("Starting read.\n");
  res = (Results *)Malloc(sizeof(Results));
  res->timings = (double *)Malloc(opts->call_limit*opts->time_limit*sizeof(double));
  buf = (char *)Malloc(opts->call_size);

  /*
   *   Open the target file.  
   */
  mpi_barrier(opts->comm);
  res->start_open = get_time();
  rf = Open(opts->read_target, O_RDONLY);
  res->end_open = get_time();
  mpi_barrier(opts->comm);

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
      xfer = Read(rf, buf, opts->call_size);
      res->transferred += xfer;
      if( xfer < opts->call_size )
	res->short_transfers++;
      res->timings[call] = get_time();
      status(call, res->timings[call] - res->timings[0]);
    }
  while( (call < opts->call_limit) && (res->timings[call] < time_limit) );
  if( (opts->base == opts->rank) && (opts->progress) )
    {
      printf("\n");
      fflush(stdout);
    }

  /* 
   *   Note the time when done.
   */
  res->num = call;
  res->end = res->timings[call];
  res->finish_fsync = 0;  /* reads don't use this value */

  /*
   *   Close the file.
   */
  mpi_barrier(opts->comm);
  res->start_close = get_time();
  Close(rf);
  if( opts->remove) 
    {
      Unlink(opts->read_target);
    }
  res->end_close = get_time();
  mpi_barrier(opts->comm);
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
  mpi_allreduce(&(last_call), &(opts->last_read_call), 1, MPI_INT, MPI_MAX, opts->comm);
  if ( (opts->rank == opts->base) && (opts->verbosity >= VERBOSE) )
    {
      printf("Call %d was the last read recorded\n", opts->last_read_call);
    }
  while(call < opts->last_read_call)
    {
      call++;
      res->timings[call] = 0;
    }
  /*
   *   Subsequent reports will be in MB rather than bytes.
   */
  res->transferred /= (1024*1024);
  if ( (opts->base == opts->rank) && (opts->verbosity >= NORMAL) )
    printf("\nAfter %d calls and %f seconds\n", opts->last_read_call, res->end_test - res->start_open);

  /*
   *   Red will end up with the aggregate amout of data transfered and the
   * earliest and latest values for the various timestamps.  From that it
   * is a simple calculation to get the aggregate data rate.
   */
  red = reduce_results(res);
  if(opts->rank == opts->base)
    {
      time = red->end - red->start;
      if(time != 0)
	rate = red->transferred/time;
      if ( opts->verbosity >= VERBOSE )
	{
	  printf("%f MB read in %f seconds\n", red->transferred, time);
	  if ( red->short_transfers != 0 )
	    printf("%d short reads\n", red->short_transfers);
	}
    }
  free(red);

  if(opts->profiles)
    {
      /*
       *   The job of reporting out details of the system call timings is
       *   left to this function.  The info goes to the
       *   read.syscalls.aves file in the log directory.  A summary of
       *   the unevenness of the system calls, if any, is reported to
       *   stdout.
       */
      DEBUG("Read table:\n");
      read_log();
      profiles(res->timings,  opts->last_read_call + 1);
      DEBUG("Reductions complete.\n");
    }

  free(res->timings);
  free(res);
  DEBUG("Read complete.\n");
  return(rate);
}

void
profiles(double *array,   int count)
{
  /*
   *   Create subcommunicators for every group of <cns_per_ions> 
   * sibling CNs.  Then each can do a reduction to get averages
   * for the ION.  Release those subcommunicators.
   *   Create a subcommunicator out of the <base> from each of
   * the previous subcommunicators.  Send all the data to the 
   * base of this new subcommunicator (also the base of MPI_WORLD),
   * so that it can be sent to the NFS target directory.
   *   Keep in mind that this function requires the "opts->ions"
   * configuration parameter to be set correctly.  If it is not 
   * then the computations are not going to be valid, and there 
   * may even be stability issues.  In particular, default operation
   * should insure that profiling is OFF.
   */
  int ion, cn;
  MPI_Comm ion_comm;
  int ion_base = 0;
  int ion_group = 0;
  int call, gap, max_gap, min_gap, ave_gap;
  double *table;
  char *buffer;
      /*
       *   BUF_LIMIT is a hack.  I want enough space so that each
       * ION can print one double.  I do this rather than having
       * <num_ions> separate print statements.  That would be very 
       * slow.
       */
  int BUF_LIMIT;
  char *ip;
  double *ion_min, *ion_max, *ion_ave;
  int ion_size, ion_rank;
  int ret;
  int i;
  int cns_per_ion = opts->cns/opts->ions;

  /*
   *   The <ion, cn> pair is unique to each task.  The mpi spli
   * creates a subcommunicator corresponding to the sibling CNs 
   * associated with each ION.  The cn = 0 task in each 
   * subcommunicator is the base to which the others send their data.
   */
  if ( ! opts->use_ion_aves )
    {
      ion = opts->rank;
      cn = opts->rank;
      BUF_LIMIT = 20*opts->cns;
      buffer = Malloc(BUF_LIMIT);
      DEBUG("Table\n");
      if ( (opts->base == opts->rank) && (opts->verbosity >= VERBOSE) )
	printf("Table of %d tasks with up to %d system calls\n", opts->cns, count);
      table = (double *)Malloc(opts->cns*sizeof(double));
      for(call = 0; call < count; call++)
	{
	  mpi_gather(&(array[call]), 1, MPI_DOUBLE, table, 1, MPI_DOUBLE, opts->base, opts->comm);
	  if( opts->rank == opts->base )
	    {
	      ip = buffer;
	      for(i = 0; i < opts->cns; i++)
		{
		  /* N.B.  the following can only have one double,
		     since varargs doesn't work.  */
		  ret = Snprintf(ip, BUF_LIMIT - (ip - buffer), "%f\t", table[i]);
		  ip += ret;
		  *ip = '\0';
		}
	      log_it("%s\n", buffer);
	    }
	}
    }
  else
    {
      ion = opts->rank/cns_per_ion;
      cn = opts->rank % cns_per_ion;

      BUF_LIMIT = 20*opts->ions;
      buffer = Malloc(BUF_LIMIT);
      DEBUG("About to execute the first split.\n");
      mpi_comm_split(opts->comm, ion, cn, &ion_comm);
      
      /*
       *   The primary goal of this exercise is to get the avearge
       * behavior accross each ION and send that array of averages 
       * to the MPI_WORLD base task for output to NFS.
       *   Prior to send the averages this code makes a quick check
       * to see that, within the set of sibling CNs, the timings are
       * in lock step, or close to it.
       */
      ion_min = (double *) Malloc(count*sizeof(double));
      ion_max = (double *) Malloc(count*sizeof(double));
      ion_ave = (double *) Malloc(count*sizeof(double));
      DEBUG("About to reduce the sum, max, and min of the call values.\n");
      /* Get the IONs' min, max, and ave at each step */
      for(call = 0; call < count; call++)
	{
	  mpi_reduce(&(array[call]), &(ion_min[call]), 1, MPI_DOUBLE, MPI_MIN, ion_base, ion_comm);
	  mpi_reduce(&(array[call]), &(ion_max[call]), 1, MPI_DOUBLE, MPI_MAX, ion_base, ion_comm);
	  mpi_reduce(&(array[call]), &(ion_ave[call]), 1, MPI_DOUBLE, MPI_SUM, ion_base, ion_comm);
	}
      if(cn == ion_base)
	{
	  gap = 0;
	  for(call = count - 1; call >= 0; call--)
	    {
	      /* 
	       *   We want two things here.  1)   The largest gap on the ION 
	       * where one CN has completed n syscalls and a sibling has 
	       * completed no more then n - gap.
	       *   2)  If some CNs never got to call n then don't record any
	       * average value for that step on that ION (i.e., set it to 0).
	       */
	      while( (ion_min[call] > 0) &&((call - gap) > 1) && 
		     (ion_max[call - gap - 1] > ion_min[call]) ) gap++;
	      if(ion_min[call] == 0)
		ion_ave[call] = 0;
	      else
		ion_ave[call] /= cns_per_ion;
	    }
	}
      free(ion_min);
      free(ion_max);
      mpi_comm_free(&ion_comm);

      DEBUG("Have the ION averages, now about the table...\n");

      /*
       *    Now we've got the average per call, and gap values for each
       * ION on its base node.  We'll want to make a communicator of base
       * nodes and gather in all that data.
       */
      
      DEBUG("Done with first subcommunicator.  About to create the second.\n");
      mpi_comm_split(opts->comm, cn, ion, &ion_comm);
      mpi_comm_size(ion_comm, &ion_size );
      mpi_comm_rank(ion_comm, &ion_rank );
      table = (double *)Malloc(opts->ions*sizeof(double));
      if( cn == ion_group )
	{
	  DEBUG("Table\n");
	  for(call = 0; call < count; call++)
	    {
	      mpi_gather(&(ion_ave[call]), 1, MPI_DOUBLE, table, 1, MPI_DOUBLE, ion_base, ion_comm);
	      if( ion_rank == ion_base )
		{
		  ip = buffer;
		  for(i = 0; i < opts->ions; i++)
		    {
		      /* N.B.  the following can only have one double,
			 since varargs doesn't work.  */
		      ret = Snprintf(ip, BUF_LIMIT - (ip - buffer), "%f\t", table[i]);
		      ip += ret;
		      *ip = '\0';
		    }
		  log_it("%s\n", buffer);
		}
	    }
	  DEBUG("\n\nAbout to reduce the gap values.\n");
	  mpi_reduce(&gap, &max_gap, 1, MPI_INT, MPI_MAX, ion_base, ion_comm);
	  mpi_reduce(&gap, &min_gap, 1, MPI_INT, MPI_MIN, ion_base, ion_comm);
	  mpi_reduce(&gap, &ave_gap, 1, MPI_INT, MPI_SUM, ion_base, ion_comm);
	  if(ion == ion_base) ave_gap /= opts->ions;
	}
      free(ion_ave);
      free(table);
      free(buffer);
      mpi_comm_free(&ion_comm);
      if ( (opts->rank == opts->base) && (opts->verbosity >= VERBOSE) )
	{
	  printf("\tIONs' syscalls gaps (i.e dispersion or unevenness)\n");
	  printf("\t\taverage\tmin\tmax\n");
	  printf("\t\t%d\t%d\t%d\n", ave_gap, min_gap, max_gap);
	}
    }
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
  
  if(opts->rank == opts->base)
  {
    if  (opts->verbosity >= NORMAL) 
      {
	printf("%24s %6s %5s %5s %5s %10s %10s\n", "date", "tasks", "xfer", "call", "time", "write", "read");
	printf("%24s %6s %5s %5s %5s %10s %10s\n", " ", " ", " ", "limit", "limit", "MB/s", "MB/s");
	printf("%24s %6s %5s %5s %5s %10s %10s\n", "------------------------", "------", "-----", "-----", "-----", "----------", "----------");
      }
    printf("%s %6d %4d%c %5d %5d %10.2f %10.2f\n", time_str, opts->tasks, xfer, range_ch, opts->call_limit, opts->time_limit, write, read);
  }
}

