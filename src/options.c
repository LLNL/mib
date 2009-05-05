/*****************************************************************************\
 *  $Id: options.c,v 1.13 2006/01/09 16:26:39 auselton Exp $
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>     /* statfs */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* strncpy */
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>
#include "mpi_wrap.h"
#include "mib.h"
#include "miberr.h"
#include "sys_wrap.h"
#include "options.h"

Options *Make_Opts();
void check_fs(char *path, int showenv, int force);
BOOL set_string(char *v, char **strp);
BOOL set_flags(char *v, int *flagp, int mask);
BOOL set_int(char *v, int *nump);
BOOL set_longlong(char *v, long long *llnump);

Options *opts;
char *opt_str = "b::EFIhHl:L:np:PrRs:St:VWd";

extern Mib    *mib;
extern char   *version;
extern char   *arch;

Options *
command_line(int *argcp, char **argvp[])
{
  Options *o;
  int argc = *argcp;
  char **argv = *argvp; 
  int opt;
  int idx;
  int i;
  const struct option long_options[] = 
    {
      {"random_reads", optional_argument, NULL, 'b'},
      {"show_environment", no_argument, NULL, 'E'},
      {"force", no_argument, NULL, 'F'},
      {"show_intermediate_values", no_argument, NULL, 'I'},
      {"show_headers", no_argument, NULL, 'H'},
      {"call_limit", required_argument, NULL, 'l'},
      {"time_limit", required_argument, NULL, 'L'},
      {"new", 0 , NULL, 'n'},
      {"profiles", required_argument, NULL, 'p'},
      {"show_progress", no_argument, NULL, 'P'},
      {"remove", no_argument, NULL, 'r'},
      {"read_only", no_argument, NULL, 'R'},
      {"call_size", required_argument, NULL, 's'},
      {"show_signon", no_argument, NULL, 'S'},
      {"test_dir", required_argument, NULL, 't'},
      {"version", no_argument, NULL, 'V'},
      {"write_only", no_argument, NULL, 'W'},
      {"help", no_argument, NULL, 'h'},
      {"directio", no_argument, NULL, 'd'},
      {0, 0, 0, 0},
    };

  o = Make_Opts();
  while( (opt = getopt_long(argc, argv, opt_str, long_options, &idx)) != -1)
    {
      switch(opt)
	{
	case 'b' : /* random_reads */
	  set_flags("true", &(o->flags), RANDOM_READS);
	  if (optarg) set_longlong(optarg, &(o->granularity));
	  break;
	case 'E' : /* show_environment */
	  set_flags("true", &(o->verbosity), SHOW_ENVIRONMENT);
	  break;
	case 'F' : /* force */
	  set_flags("true", &(o->flags), FORCE);
	  break;
	case 'I' : /* show_intermediate_values */
	  set_flags("true", &(o->verbosity), SHOW_INTERMEDIATE_VALUES);
	  break;
	case 'H' : /* show_headers */
	  set_flags("true", &(o->verbosity), SHOW_HEADERS);
	  break;
	case 'l' : /* call_limit */
	  set_int(optarg, &(o->call_limit));
	  break;
	case 'L' : /* time_limit */
	  set_int(optarg, &(o->time_limit));
	  break;
	case 'n' : /* new */
	  set_flags("true", &(o->flags), NEW);
	  break;
	case 'p' : /* profiles */
	  if(optarg == NULL) usage();
	  set_string(optarg, &(o->profiles));
	  break;
	case 'P' : /* show_progress */
	  set_flags("true", &(o->verbosity), SHOW_PROGRESS);
	  break;
	case 'r' : /* remove */
	  set_flags("true", &(o->flags), REMOVE);
	  break;
	case 'R' : /* read_only */
	  set_flags("true", &(o->flags), READ_ONLY);
	  break;
	case 's' : /* call_size */
	  set_longlong(optarg, &(o->call_size));
	  break;
	case 'S' : /* show_signon */
	  set_flags("true", &(o->verbosity), SHOW_SIGNON);
	  break;
	case 't' : /* test_dir */
	  if(optarg == NULL) usage();
	  set_string(optarg, &(o->testdir));
	  break;
	case 'V' : /* version */
	  printf("mib-%s-%s\n", version, arch);
	  exit(0);
	case 'W' : /* write_only */
	  set_flags("true", &(o->flags), WRITE_ONLY);
	  break;
	case 'd' : /* directio */ 
          set_flags("true", &(o->flags), DIRECTIO);
	  break;
	case 'h' :  /* help */
	default : 
	  usage(); 
          break;
	}
    }
  /* 
   * Anything we've consumed doesn't need to be passed to the MPI initializer
   */
  for(i = 1; i < *argcp - (optind - 1); i++)
      (*argvp)[i] = (*argvp)[i + (optind - 1)];
  *argcp -= (optind - 1);
  if( o->testdir == NULL)
    {
      base_report("Mib requires a \"-t <test_dir>\" argument.\n");
      exit(1);
    }
  check_fs(o->testdir, flag_set(o->verbosity, SHOW_ENVIRONMENT), flag_set(o->flags, FORCE));
  if( (o->flags & WRITE_ONLY)  && (o->flags & READ_ONLY) )
    {
      o->flags &= ~WRITE_ONLY;
      o->flags &= ~READ_ONLY;
    }
  return(o);
}

void
usage( void )
{
  printf("usage: mib [%s]\n", opt_str);
  printf("See the man page for \"long_opts\" equivalents.\n");
  printf("    -b [<gran>]     :  Random seeks (optional granularity) before each read.\n");
  printf("    -E              :  Show environment of test in output.\n");
  printf("    -F              :  Mib does not normally allow I/O to any FS\n");
  printf("                    :  but Lustre.  This overrides the \"safety\".\n");
  printf("    -I              :  Show intermediate values in output.\n");
  printf("    -h              :  Print this message and exit.\n");
  printf("    -H              :  Show headers in output.\n");
  printf("    -l <call_limit> :  Issue no more than this many system calls.\n");
  printf("    -L <time_limit> :  Do not issue new system calls after this many\n");
  printf("                    :    seconds (limits are per phase for write and\n");
  printf("                    :    read phases).\n");
  printf("    -n              :  Create new files if files were already present\n");
  printf("                    :    (will always create new files if none were\n");
  printf("                    :    present).\n");
  printf("    -p <profiles>   :  Output system call timing profiles to \n");
  printf("                    :    <profiles>.write and <profiles>.read.\n");
  printf("    -P              :  Show progress bars during testing.\n");
  printf("    -r              :  Remove files when done.\n");
  printf("    -R              :  Only perform the read test.\n");
  printf("    -s <call_size>  :  Use system calls of this size (default 512k).\n");
  printf("                    :    Numbers may use abreviations k, K, m, or M.\n");
  printf("    -S              :  Show signon message, including date, program\n");
  printf("                    :    name, and version.\n");
  printf("    -t <test_dir>   :  Required. I/O transactions to and from this\n");
  printf("                    :    directory.\n");
  printf("    -V              :  Print the version and exit.\n");
  printf("    -W              :  Only perform the write test\n");
  printf("                    :    (if -R and -W are both present both tests \n");
  printf("                    :     will run, but that's the default anyway).\n");
  printf("    -d              :  Open files with O_DIRECT.\n");
  exit(0);
}

Options *
Make_Opts()
{
  Options *o;

  o = (Options *)Malloc(sizeof(Options));
  o->profiles = NULL;
  o->testdir = NULL;
  o->call_limit = 4096;
  o->call_size = 524288;
  o->granularity = 1;
  o->time_limit = 60;
  o->flags = DEFAULTS;
  o->verbosity = QUIET;
  return(o);
}

void
Free_Opts()
{
  if(opts->profiles != NULL) free(opts->profiles);
  if(opts->testdir != NULL) free(opts->testdir);
  if(opts != NULL)
    free(opts);
}

#define MAX_NAME 30
struct fs_es
{
  char name[MAX_NAME];
  long f_type;
  int warn;
};

const struct fs_es fs_list[] = 
  {
    {"ADFS_SUPER_MAGIC",      0xadf5, 1},
    {"AFFS_SUPER_MAGIC",      0xADFF, 1},
    {"BEFS_SUPER_MAGIC",      0x42465331, 1},
    {"BFS_MAGIC",             0x1BADFACE, 1},
    {"CIFS_MAGIC_NUMBER",     0xFF534D42, 1},
    {"CODA_SUPER_MAGIC",      0x73757245, 1},
    {"COH_SUPER_MAGIC",       0x012FF7B7, 1},
    {"CRAMFS_MAGIC",          0x28cd3d45, 1},
    {"DEVFS_SUPER_MAGIC",     0x1373, 1},
    {"EFS_SUPER_MAGIC",       0x00414A53, 1},
    {"EXT_SUPER_MAGIC",       0x137D, 1},
    {"EXT2_OLD_SUPER_MAGIC",  0xEF51, 1},
    {"EXT2_SUPER_MAGIC",      0xEF53, 1},
    {"EXT3_SUPER_MAGIC",      0xEF53, 1},
    {"HFS_SUPER_MAGIC",       0x4244, 1},
    {"HPFS_SUPER_MAGIC",      0xF995E849, 1},
    {"HUGETLBFS_MAGIC",       0x958458f6, 1},
    {"ISOFS_SUPER_MAGIC",     0x9660, 1},
    {"JFFS2_SUPER_MAGIC",     0x72b6, 1},
    {"JFS_SUPER_MAGIC",       0x3153464a, 1},
    {"MINIX_SUPER_MAGIC",     0x137F, 1},
    {"MINIX_SUPER_MAGIC2",    0x138F, 1},
    {"MINIX2_SUPER_MAGIC",    0x2468, 1},
    {"MINIX2_SUPER_MAGIC2",   0x2478, 1},
    {"MSDOS_SUPER_MAGIC",     0x4d44, 1},
    {"NCP_SUPER_MAGIC",       0x564c, 1},
    {"NFS_SUPER_MAGIC",       0x6969, 1},
    {"NTFS_SB_MAGIC",         0x5346544e, 1},
    {"OPENPROM_SUPER_MAGIC",  0x9fa1, 1},
    {"PROC_SUPER_MAGIC",      0x9fa0, 1},
    {"QNX4_SUPER_MAGIC",      0x002f, 1},
    {"REISERFS_SUPER_MAGIC",  0x52654973, 1},
    {"ROMFS_MAGIC",           0x7275, 1},
    {"SMB_SUPER_MAGIC",       0x517B, 1},
    {"SYSV2_SUPER_MAGIC",     0x012FF7B6, 1},
    {"SYSV4_SUPER_MAGIC",     0x012FF7B5, 1},
    {"TMPFS_MAGIC",           0x01021994, 1},
    {"UDF_SUPER_MAGIC",       0x15013346, 1},
    {"UFS_MAGIC",             0x00011954, 1},
    {"USBDEVICE_SUPER_MAGIC", 0x9fa2, 1},
    {"VXFS_SUPER_MAGIC",      0xa501FCF5, 1},
    {"XENIX_SUPER_MAGIC",     0x012FF7B4, 1},
    {"XFS_SUPER_MAGIC",       0x58465342, 1},
    {"_XIAFS_SUPER_MAGIC",    0x012FD16D, 1},
    {"S_MAGIC_LUSTRE",        0x0BD00BD0, 0},
    {"", 0, 0}
  };

void
check_fs(char *path, int showenv, int force)
{
  int found = 0;
  int index = 0;
  int ret;
  struct statfs buf;

  errno = 0;
  if ( (ret = statfs(path, &buf) < 0) )
  {
    base_report("Could not statfs \"%s\" (%d): %s\n", 
		path, errno, strerror(errno));
    exit(1);
  }
  while ( (!found) && (fs_list[index].f_type != 0) )
    {
      if (fs_list[index].f_type == buf.f_type) found = 1;
      else index++;
    }
  if (found)
    {
      if( (fs_list[index].warn == 1) && (!force) )
      {
	base_report("Mib will not write to a file system of type \"%s\" (f_type = %#lx) unless you use the \"-F\" (--force) option \n", 
		    fs_list[index].name, fs_list[index].f_type);
	exit(1);
      }
      if(showenv) base_report("Testing a file system of type \"%s\" (f_type = %#lx)\n",
		  fs_list[index].name, fs_list[index].f_type);
    }
  else
    {
      if(showenv) base_report("Testing an unregistered file system with f_type = %#lx\n",
		  buf.f_type);
    }
}  

void
show_details()
{
  if ( mib->rank == mib->base )
    {
      printf("new                      = %s\n", ((opts->flags & NEW) ? "true" : "false"));
      printf("remove                   = %s\n", ((opts->flags & REMOVE) ? "true" : "false"));
      printf("testdir                  = %s\n", opts->testdir);
      printf("call_limit               = %d\n", opts->call_limit);
      printf("call_size                = %lld\n", opts->call_size);
      printf("time_limit               = %d\n", opts->time_limit);
      printf("tasks                    = %d\n", mib->size);
      printf("write_only               = %s\n", ((opts->flags & WRITE_ONLY) ? "true" : "false"));
      printf("read_only                = %s\n", ((opts->flags & READ_ONLY) ? "true" : "false"));
      printf("profiles                 = %s\n", ((opts->profiles == NULL) ? "no" : opts->profiles));
      if ( opts->flags & RANDOM_READS )
	printf("random_reads             = %lld\n", opts->granularity);
      printf("show_signon              = %s\n", ((opts->verbosity & SHOW_SIGNON) ? "true" : "false"));
      printf("show_headers             = %s\n", ((opts->verbosity & SHOW_HEADERS) ? "true" : "false"));
      printf("show_environment         = %s\n", ((opts->verbosity & SHOW_ENVIRONMENT) ? "true" : "false"));
      printf("show_progress            = %s\n", ((opts->verbosity & SHOW_PROGRESS) ? "true" : "false"));
      printf("show_intermediate_values = %s\n", ((opts->verbosity & SHOW_INTERMEDIATE_VALUES) ? "true" : "false"));
      fflush(stdout);
    }
}

static int 
mystrnlen(char *str, int max)
{
  int n;
  for (n = 0; n < max; n++)
     if (str[n] == '\0')
        break;
  return n;
}

BOOL
set_string(char *v, char **strp)
{
  int ret;

  if( *strp != NULL) 
      free(*strp);
  *strp = Malloc(mystrnlen(v, MAX_BUF) + 1);
  if ( (ret = snprintf(*strp, MAX_BUF, "%s", v)) < 0)
    FAIL();
  return(TRUE);
}

BOOL
set_flags(char *v, int *flagp, int mask)
{
  if( (v[0] == 't') ||
      (v[0] == 'y') ||
      (v[0] == '1') )
    {
      *flagp |= mask;
      return(TRUE);
    }
  if( (v[0] == 'f') ||
      (v[0] == 'n') ||
      (v[0] == '0') )
    {
      *flagp &= ~mask;
      return(TRUE);
    }
  return(FALSE);
}

BOOL
set_int(char *v, int *nump)
{
  *nump = atoi(v);
  return(TRUE);
}

BOOL
set_longlong(char *v, long long *llnump)
{
  int ret;
  char c;

  ret = sscanf(v, "%lld %c", llnump, &c);
  if (ret < 1) FAIL();
  if(ret > 1)
    {
      switch((int)c)
	{
	case 'k':
	case 'K': (*llnump) *= 1024;
	  break;
	case 'm':
	case 'M': (*llnump) *= 1024*1024;
	  break;
	default: break;
	}
    }	  
  return(TRUE);
}

