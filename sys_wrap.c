/*****************************************************************************\
 *  $Id: sys_wrap.c,v 1.8 2006/01/09 16:26:39 auselton Exp $
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
 *   The system calls can all fail for a variety of reasosn.  In some
 * cases it might be sensible to retry the system call.  In other cases
 * one could probably give up but instead of failing outright one
 * could wait for the next MPI_Barrier and communicate the problem back 
 * to the base node to be reported.  I don't see any of these requiring 
 * MPI_Abort as a starting point for the error handling.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <mpi.h>
#include "mib.h"
#include "miberr.h"
#include "sys_wrap.h"
#include "options.h"

extern Options *opts;
extern Mib     *mib;

int
Open(char *name, int flags)
{
  /*
   *   This is called once per task with O_WRONLY, and once with
   * O_RDONLY (for a different file).
   *   If we can't open the file then FAIL seems like the only 
   * option, but perhaps the user could be given more information
   * about the failure.  Here are the relevant errno codes:
   *  EISDIR
   *  EACCESS
   *  ENAMETOOLONG
   *  ENOENT
   *  EROFS
   *  ENOMEM
   *  EMFILE
   *  ENFILE
   *  Since nothing actually fatal has happened this might ought to 
   * MPI_reduce the cause to the base task for error reporting.
   */
  int fd;
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH ; /* 664 */

  errno = 0;
  if( (fd = open(name, flags, mode)) < 0 )
    {
      if(errno == ENOENT) 
	{
	  flags |= O_CREAT;
	  if( (fd = open(name, flags, mode)) < 0 )
	    {
	      printf("failed to open %s: %d\n", name, errno);
	      fflush(stdout);
	      FAIL();
	    }
	}
    }
  return (fd);
}

void
Unlink(char *name)
{
  /*
   *   This is called once per task if the "new" flag is set.
   *   If we can't unlink the file then ignore the ENOENT error.  Any
   * other error should probably be a FAIL.
   *   Here are the relevant errno codes:
   *  EACCESS
   *  EPERM
   *  EISDIR
   *  EBUSY
   *  EFAULT
   *  ENAMETOOLONG
   *  ENOENT
   *  ENOTDIR
   *  ENOMEM
   *  EROFS
   *  ELOOP
   *  EIO
   *  Since nothing actually fatal has happened this might ought to 
   * MPI_reduce the cause to the base task for error reporting.  Only
   * ENOENT can rightly be ignored.
   */
  int ret;

  errno = 0;
  if( ((ret = unlink(name)) < 0) && (errno != ENOENT) )
    {
        printf("failed to unlink %s: %d\n", name, errno);
	fflush(stdout);
	FAIL();
    }
}

ssize_t
Write(int fd, const void *buf, size_t count)
{
  /*
   *   The relevant errno values are:
   *  EBADF
   *  EINVAL
   *  EFAULT
   *  EFBIG
   *  EINTR
   *  ENOSPC
   *  EIO
   */
  ssize_t wrote;
  ssize_t written = 0;
  int retries = MAX_RETRIES;
  char *bp = (char *)buf;

  errno = 0;
  while ( retries-- && ((wrote = write(fd, (void *)(&(bp[written])), count - written)) < (count - written)) )
    {
      if(wrote < 0)
	{
	  FAIL();
	}
      if(wrote > 0)
	{
	  written += wrote;
	  retries = MAX_RETRIES;
	}
    }
  written += wrote;
  return(written);
}

ssize_t
Read(int fd, void *buf, size_t count)
{
  /*
   * If we get to EOF with reads still to do we should
   * seek back to the start of the file.
   */
  /*
   *   The relevant errno values are:
   *  EINTR
   *  EIO
   *  EISDIR
   *  EBADF
   *  EINVAL
   *  EFAULT
   */
  ssize_t got;
  ssize_t gotten = 0;
  int retries = MAX_RETRIES;
  char *bp = (char *)buf;

  errno = 0;
  while ( retries-- && ((got = read(fd, (void *)(&(bp[gotten])), count - gotten)) < (count - gotten)) )
    {
      if(got < 0)
	{
	  FAIL();
	}
      if(got > 0)
	{
	  gotten += got;
	  retries = MAX_RETRIES;
	}
    }
  gotten += got;
  return(gotten);
}

int
Fsync(int fd)
{
  /*
   *   The relevant errno values are:
   *  EBADF
   *  EROFS
   *  EINVAL
   *  EIO
   */
  int rc;

  errno = 0;
  if ( (rc = fsync(fd)) < 0 )
    /*    FAIL()*/;
  return(rc);
}

int
Close(int fd)
{
  /*
   *   The relevant errno values are:
   *  EBADF
   *  EINTR
   *  EIO
   */
  int rc;

  errno = 0;
  if ( (rc = close(fd)) < 0 )
    FAIL();
  return(rc);
}

int
Exists(const char *path)
{
  /*
   *   The relevant errno values are:
   *  EACCESS
   *  ELOOP
   *  ENAMETOOLONG
   *  ENOENT
   *  ENOTDIR
   *  EROFS
   *  EFAULT
   *  EINVAL
   *  EIO
   *  ENOMEM
   *  ETXTBSY
   */
  int rc;
  struct stat stat_st;

  errno = 0;
  if ( (rc = stat(path, &stat_st)) < 0 )
    if ( errno == ENOENT )
      rc = 0;
    else
      FAIL();
  else
    rc = 1;
  return(rc);
}

/*
 * These library calls should all be conducted from the base task
 * only, except for the Malloc function.  I'm not sure if
 * I do enough to enforce that.
 *
 */

FILE *
Fopen(const char *path, const char *mode)
{
  /*
   *   Can't check for rank == base yet, because opts may not
   * be initialized.
   */
  FILE *fp;

  if ( (fp = fopen(path, mode)) == NULL )
    {
      FAIL();
    }
  return(fp);
}

char *
Fgets(char *buf, int n, FILE *stream)
{
  char *s;
  /*  Verry interesting!  The following array will also cause the
      code to crash.  I think this may be a stack problem!
  double array[100];
  */
  /*
   *   I don't understand why yet, but putting an ASSERT in here
   * causes the code to immediately crash.  If all you do is put a
   * print statement in it works fine.  If that print statment referrs
   * to opt->rank, it fails with signal 15.  I'm beggining to wonder
   * if there's some sort of stack corruption due to a compiler bug,
   * or even just a stack limit or something.
  ASSERT(mib->rank == mib->base);
   */
  if ( (s = fgets(buf, n, stream)) == NULL)
    if(!feof(stream))
      {
	FAIL();
      }
  return(s);
}

void
Fprintf(FILE *stream, char *fmt, char *str)
{
  /* 
   *   This calls for varargs, but the cross compiler doesn't 
   * varargs correctly.  Worth filing a bug report.  
   */
  int ret;

  ASSERT(mib->rank == mib->base);
  if ( (ret = fprintf(stream, fmt, str)) < 0)
    FAIL();
  fflush(stream);
}


int
Snprintf(char *buf, size_t size, char *fmt, double val)
{
  /* 
   *   This calls for varargs, but the cross compiler doesn't 
   * varargs correctly.  Worth filing a bug report.  
   */
  int ret;

  ASSERT(mib->rank == mib->base);
  if ( ((ret = snprintf(buf, size, fmt, val)) < 0) || (ret > size) )
    FAIL();
  return(ret);
}


void *
Malloc(size_t size)
{
  void *b;

  if( (b = malloc(size)) == NULL)
    FAIL();
  return(b);
}
