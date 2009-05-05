/*****************************************************************************\
 *  $Id: mib.c,v 1.19 2006/01/09 16:26:39 auselton Exp $
 *****************************************************************************
 *  Copyright (C) 2006 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
 *  UCRL-CODE-222725
 *  
 *  This file is part of Mib, an MPI-based parallel I/O benchamrk.
 *  For details, see <http://www.llnl.gov/linux/mib/>.
 *  
 *  Mib is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License (as published by the Free
 *  Software Foundation) version 2, dated June 1991.
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
 *   Mib is an MPI I/O test that has each MPI task perform I/O of a
 * fixed size per system call.  The tasks first write to and then read
 * from files named in a conventional way.  Mib assumes that the files
 * are on a parallel file system. The file a task reads from is
 * distinct from the file written to, if possible.  This defeats
 * caching at the clients.  Optionally, detailed timing data for
 * the system calls is sent to a file (one for writes and one for
 * reads) after the I/O is complete.  For details consult the README
 * or http://www.llnl.gov/linux/mib.
 */

#include <stdio.h>      /* FILE, fopen, etc. */
#include <sys/types.h>  /* open, etc */
#include <sys/stat.h>   /* open, etc */
#define __USE_GNU       /* for O_DIRECT */
#include <fcntl.h>      /* open, etc */
#include <unistd.h>     /* unlink, ssize_t */
#include <stdlib.h>     /* free */
#include <string.h>     /* strncpy */
#include <errno.h>
#include <time.h>
#include <stdarg.h>    /* varargs */
#include <sys/utsname.h>
#include <assert.h>
#include "mpi_wrap.h"
#include "mib.h"
#include "miberr.h"
#include "options.h"
#include "mib_timer.h"
#include "sys_wrap.h"

void get_arch(void);
void sign_on(void);
Mib *Init_Mib(int rank, int size);
double write_test();
char *fill_buff();
void init_status(char *str);
void status(int calls, double time);
double read_test();
Results *reduce_results(Results *res);
void profiles(double *array, int count, char *profile_log_name);
void report(double write, double read);
void _base_report(char *fmt, va_list args);

/* See mib.h.  the size of the communicator and the task's rank are in the Mib struct */
Mib  *mib = NULL;

char *version=MIB_VERSION;
char *arch = "unknown";

/* See options.h.  Elements of the command line are here */
extern Options *opts;
/* Be sure you don't use these macros until after opts is initialized */
#define check_flags(mask) flag_set(opts->flags, mask)
#define verbosity(mask) flag_set(opts->verbosity, mask)

int
main( int argc, char *argv[] )
{
  /*
   *  The overall flow of the program is: 1) initialize everything, 2)
   *  run write test, 3) run read test, 4) print results.
   */
  int size = 0;
  int rank;
  double read = 0;
  double write = 0;

  /* ask kernel what arch we are */
  get_arch();

  /* initialize the opts struct */
  opts = command_line(&argc, &argv);

  mpi_init( &argc, &argv );
  mpi_comm_size(MPI_COMM_WORLD, &size );
  mpi_comm_rank(MPI_COMM_WORLD, &rank );

  /* initialize the timer, check for skew */
  init_timer();
  sign_on();

  mib = Init_Mib(rank, size);

  if(verbosity(SHOW_ENVIRONMENT)) show_details();

  DEBUG("Initializations complete.\n");
  if(! check_flags(READ_ONLY) )
    {
      write = write_test();
      conditional_report(SHOW_INTERMEDIATE_VALUES, "Aggregate write rate = %10.2f\n", write);
    }
  if(! check_flags(WRITE_ONLY) )
    {
      read = read_test();
      conditional_report(SHOW_INTERMEDIATE_VALUES, "Aggregate read rate = %10.2f\n", read);
    }
  report(write, read);
  Free_Opts();
  mpi_finalize();
  exit(0);
}

void
sign_on(void)
{
  /* print time, version, and architecture */
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
  conditional_report(SHOW_SIGNON, signon);
}

void
get_arch(void)
{
  struct utsname uts;

  if (uname(&uts) < 0) {
    perror("uname");
    exit(1);
  }
  arch = strdup(uts.machine);
  if (!arch) {
    fprintf(stderr, "Out of memory\n");
    exit(1);
  }
}

Mib *
Init_Mib(int rank, int size)
{
  Mib *m;

  m = (Mib *)Malloc(sizeof(Mib));
  m->rank =  rank;
  m->size =  size;
  m->base =  0;
  m->comm = MPI_COMM_WORLD;
  return(m);
}

double
write_test()
{
  /*
   *   The results structure (see mib.h) holds the timings of the
   * various stages of the test as well as particulars, like how many
   * system calls were actually made in this task.  The "timings"
   * field is an array of doubles that will hold the return values
   * from checking the time at the end of each system call.  Wouldn't
   * it be cool to get all sorts of other diagnostic info at the end
   * of each system call?
   */

  int call = 0;
  int last_call;
  char *buf;          /* what gets sent in the system call */
  int wf;             /* the file handle for the writes */
  int flag = O_WRONLY | O_CREAT; /* the flag value for the open */
  double time_limit;
  double time;
  double rate = 0;
  ssize_t xfer;
  Results *res;
  Results *red;       /* individual results are reduced into this struct */
  char write_target[MAX_BUF];
  int last_write_call;

  DEBUG("Starting write test.\n");
  if ((opts->flags & DIRECTIO))
    {
      flag |= O_DIRECT;
    }
  res = (Results *)Malloc(sizeof(Results));
  res->timings = (double *)Malloc(opts->call_limit*opts->time_limit*sizeof(double));
  buf = fill_buff();
  snprintf(write_target, MAX_BUF, "%s/mibData.%08d", opts->testdir, mib->rank);
  /*
   *  This motif is repeated several times.  You get a barrier then an
   * action, or an action then a barrier, or both.  Timestamps are
   * taken at each step.  Here the file is being opened.  
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
  /* The status stuff is what produces the otional progress bar. */
  init_status("write phase");
  res->start = res->timings[call] = get_time();
  time_limit = res->timings[call] + opts->time_limit;

  /*
   *   This is the meat of the test.  The loop is terminated by the
   * first occurance of no more time or no more syscalls.  It might be
   * interesting to see what happens if you barrier at each
   * iteration.  Haven't tried that.
   */
  do
    {
      call++;
      xfer = Write(wf, buf, opts->call_size);
      /* this shouldn't ever happen, but it's good to know when it does :) */
      assert(xfer >= 0);
      res->transferred += xfer;
      if (xfer < opts->call_size) res->short_transfers++; 
      res->timings[call] = get_time();
      status(call, res->timings[call] - res->timings[0]);
    }
  while( (call < opts->call_limit) && (res->timings[call] < time_limit) );
  conditional_report(SHOW_PROGRESS, "\n");

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
   *   We want every tasks to have a value for every system call up to
   * the maximum seen in any task.  This fills in  a zero for each entry 
   * after a task's last actual call but before the last global call.
   */
  last_call = call;
  mpi_allreduce(&(last_call), &(last_write_call), 1, MPI_INT, MPI_MAX, mib->comm);
  conditional_report(SHOW_INTERMEDIATE_VALUES, "Call %d was the last write recorded\n", last_write_call);
  while(call < last_write_call)
    {
      call++;
      res->timings[call] = 0;
    }
  /*
   *   Subsequent reports will be in MB rather than bytes.
   */
  res->transferred /= (1024*1024);
  conditional_report(SHOW_INTERMEDIATE_VALUES, 
	      "\nAfter %d calls and %f seconds\n", 
	      last_write_call, res->end_test - res->start_open);

  /* 
   * If there is no MPI, but only one task we should still be able to get
   * profiles, but that will have to be an embellishment for later.
   */
  if(mib->size > 1)
    {
      /*
       *   "Red" will end up with the aggregate amout of data transfered and the
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
      conditional_report(SHOW_INTERMEDIATE_VALUES, "%f MB written in %f seconds\n", red->transferred, time);
      if ( red->short_transfers != 0 )
	conditional_report(SHOW_INTERMEDIATE_VALUES, "%d short writes\n", red->short_transfers);
      free(red);
      
      if( opts->profiles != NULL )
	{
	  /*
	   *   The job of reporting out details of the system call
	   *   timings is left to this function.  The info goes to the
	   *   file specified in the "-p <profile>.write" option.
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
   * The results of interest are:
   * Min_{n} start_open
   * Max_{n} end_open
   * Min_{n} start
   * timings_{node, call}
   * Sum_{n} transferred
   * Max_{n} finish_fsync
   * Min_{n} start_close
   * Max_{n} end_close
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
   *   This just fills the buffer with long long sized checks of <task, count> 
   * pairs.
   */
  char *buff;
  long long count = 0;
  long long *llarray;
  int lsize = sizeof(long long);

  buff = (char *)IOMalloc(opts->call_size);
  llarray  = (long long *)buff;
  while((char *)&(llarray[count]) < (buff + opts->call_size - lsize))
    {
      llarray[count] = (mib->rank << (lsize/2)) + count;
      count++;
    }
  return(buff);
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
   * will provide the progress measure.  No real effort is made to keep it
   * reliable or timely.  It's best effort and works fine for a large number
   * of quick I/O operations.
   */
  int i;

  progress = 0;
  for(i = 0; i < EXPECTATION - 1; i++)
    pbuf[i] = '*';
  pbuf[EXPECTATION - 1] = '\0';
  conditional_report(SHOW_PROGRESS, "\n%s\nshould last about this long---------------------->\n", str);
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

  current = (current_time < current_calls) ? current_calls : current_time;
  if( (current > progress) && (current < EXPECTATION - 1) )
    {
      pbuf[current-progress] = '\0';

      conditional_report(SHOW_PROGRESS, pbuf);
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
  int flag = O_RDONLY;
  Results *red = NULL;
  Results *res = NULL;
  char read_target[MAX_BUF];
  int read_rank = ((mib->rank + (mib->size/2)) % mib->size) + mib->base;
  int last_read_call;
  off_t offset;
  double gran;
  struct stat stats;

  DEBUG("Starting read.\n");
  if ((opts->flags & DIRECTIO))
    {
      flag |= O_DIRECT;
    }
  res = (Results *)Malloc(sizeof(Results));
  res->timings = (double *)Malloc(opts->call_limit*opts->time_limit*sizeof(double));
  buf = (char *)IOMalloc(opts->call_size);
  snprintf(read_target, MAX_BUF, "%s/mibData.%08d", opts->testdir, read_rank);

  /* 
   * N.B. there is no optional unlink at the beginning of the reads 
   */
  res->before_unlink = get_time();
  /*
   *   Open the target file.  
   */
  if( ! Exists(read_target) )
    {
      fprintf(stderr, "No file: %s\n", read_target);
      FAIL();
    }
  mpi_barrier(mib->comm);
  res->start_open = get_time();
  rf = Open(read_target, flag);
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
	  fprintf(stderr, "In task %d, the file size (%ld) is too small for the granularity (%lld)\n", 
		      mib->rank, (long)stats.st_size, opts->granularity);
	  FAIL();
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
  conditional_report(SHOW_PROGRESS, "\n");

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
  conditional_report(SHOW_INTERMEDIATE_VALUES, "Call %d was the last read recorded\n", last_read_call);
  while(call < last_read_call)
    {
      call++;
      res->timings[call] = 0;
    }
  /*
   *   Subsequent reports will be in MB rather than bytes.
   */
  res->transferred /= (1024*1024);
  conditional_report(SHOW_INTERMEDIATE_VALUES, 
	      "\nAfter %d calls and %f seconds\n", 
	      last_read_call, res->end_test - res->start_open);

  if(mib->size > 1)
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
      conditional_report(SHOW_INTERMEDIATE_VALUES, 
		  "%f MB read in %f seconds\n", red->transferred, time);
      if ( red->short_transfers != 0 )
	conditional_report(SHOW_INTERMEDIATE_VALUES, "%d short reads\n", red->short_transfers);
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
   * Print out a table with one row per system call up to the maximum
   * of all calls across all tasks, zero filled for tasks that didn't
   * get that many calls in.  One collumn in the row is for each task.
   * Each entry is a double corresponding to when that task finished
   * that system call.  All values are relative to a global zero
   * coordinated by an initial barrier. 
   *
   * A gather gets each task's values into the base task, that way one
   * task doesn't have to be able to hold more than num_tasks doubles.  
   */
  int call;
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
  int ret;
  int i;
  FILE *lfd;
  char profile_log_name[MAX_BUF];

  if(mib->rank == mib->base)
    {
      if ( (ret = snprintf(profile_log_name, MAX_BUF, "%s.%s", opts->profiles, io_direction)) < 0)
	FAIL();
      lfd = Fopen(profile_log_name, "w");
    }

  /*
   */
  BUF_LIMIT = 20*mib->size;
  buffer = Malloc(BUF_LIMIT);
  DEBUG("Table\n");
  conditional_report(SHOW_INTERMEDIATE_VALUES, 
	      "Table of %d tasks with up to %d system calls\n", mib->size, count);
  table = (double *)Malloc(mib->size*sizeof(double));
  for(call = 0; call < count; call++)
    {
      mpi_gather(&(array[call]), 1, MPI_DOUBLE, table, 1, MPI_DOUBLE, mib->base, mib->comm);
      if( mib->rank == mib->base )
	{
	  ip = buffer;
	  for(i = 0; i < mib->size; i++)
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
  if(mib->rank == mib->base)
    fclose(lfd);
  free(table);
  free(buffer);
}

void
report(double write, double read)
{
  /* 
   * The headers are optional.  Print the results along with a little
   * info about the test. 
   */
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
  conditional_report(SHOW_HEADERS, 
		  "%14s           %6s %5s %5s %5s %10s %10s\n", "date", "tasks", "xfer", "call", "time", "write", "read");
  conditional_report(SHOW_HEADERS, 
		  "%24s %6s %5s %5s %5s %10s %10s\n", " ", " ", " ", "limit", "limit", "MB/s", "MB/s");
  conditional_report(SHOW_HEADERS, 
		  "%24s %6s %5s %5s %5s %10s %10s\n", "------------------------", "------", "-----", "-----", "-----", "----------", "----------");
  conditional_report(SHOW_ALL, 
		  "%s %6d %4d%c %5d %5d %10.2f %10.2f\n", time_str, mib->size, xfer, range_ch, opts->call_limit, opts->time_limit, write, read);
}


void
conditional_report(int verb, char *fmt, ...)
{
  va_list args;
  
  if( !verbosity(verb) ) return;
  va_start(args, fmt);
  _base_report(fmt, args);
  va_end(args);
}

void
base_report(char *fmt, ...)
{
  va_list args;
  
  /*
   * If this is called before mib is initialized then just
   * print.  If mib is initialized then print from its base task.
   * Don't print unless the verbosity level says to.
   */
  va_start(args, fmt);
  _base_report(fmt, args);
  va_end(args);
}

void
_base_report(char *fmt, va_list args)
{
  if(mib != NULL && mib->rank != mib->base) return;
  vprintf(fmt, args);
  fflush(stdout);
}
