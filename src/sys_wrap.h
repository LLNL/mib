/*****************************************************************************\
 *  $Id: sys_wrap.h,v 1.5 2005/11/30 20:24:57 auselton Exp $
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

#define MAX_RETRIES 10

#ifndef BOOL_DEF
typedef enum {FALSE, TRUE} BOOL;
#define BOOL_DEF
#endif

int Open(char *name, int flags);

int  Fstat(int filedes, struct stat *buf);

off_t  Lseek(int filedes, off_t offset, int whence);

void Unlink(char *name);

ssize_t Write(int fd, const void *buf, size_t count);

ssize_t Read(int fd, void *buf, size_t count);

int Fsync(int fd);

int Close(int fd);

int Exists(const char *path);

/* 
 *   These wrappers for library calls got tossed in here for want of
 * better idea.
 */
FILE *Fopen(const char *path, const char *mode);

char *Fgets(char *buf, int n, FILE *stream);

void Fprintf(FILE *stream, char *fmt, char *str);

int Snprintf(char *buf, size_t size, char *fmt, double val);

void *Malloc(size_t size);

void *IOMalloc(size_t size);
