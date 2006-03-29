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
#define _GNU_SOURCE
#include <getopt.h>
#include "config.h"
#include "mpi_wrap.h"
#include "mib.h"
#include "miberr.h"
#include "sys_wrap.h"
#include "options.h"
#include "slurm.h"

Options *Make_Opts();
void check_fs();
BOOL set_string(char *v, char **strp);
BOOL set_flags(char *v, int *flagp, int mask);
BOOL set_int(char *v, int *nump);
BOOL set_longlong(char *v, long long *llnump);

Options *opts;
char *opt_str = "ab::EFIhHl:L:Mnp:PrRs:St:VW";

extern Mib     *mib;
extern SLURM   *slurm;
extern int     use_mpi;
extern char   *version;

void
command_line(int *argcp, char **argvp[])
{
  int argc = *argcp;
  char **argv = *argvp; 
  int opt;
  int idx;
  const struct option long_options[] = 
    {
      {"use_node_aves", no_argument, NULL, 'a'},
      {"random_reads", optional_argument, NULL, 'b'},
      {"show_environment", no_argument, NULL, 'E'},
      {"force", no_argument, NULL, 'F'},
      {"show_intermediate_values", no_argument, NULL, 'I'},
      {"show_headers", no_argument, NULL, 'H'},
      {"call_limit", required_argument, NULL, 'l'},
      {"time_limit", required_argument, NULL, 'L'},
      {"no_mpi", 0 , NULL, 'M'},
      {"new", 0 , NULL, 'n'},
      {"profiles", required_argument, NULL, 'p'},
      {"show_progress", no_argument, NULL, 'P'},
      {"remove", no_argument, NULL, 'r'},
      {"read_only", no_argument, NULL, 'R'},
      {"call_size", required_argument, NULL, 's'},
      {"show_signon", no_argument, NULL, 'S'},
      {"test_dir", required_argument, NULL, 't'},
      {"write_only", no_argument, NULL, 'W'},
      {"version", no_argument, NULL, 'V'},
      {"help", no_argument, NULL, 'h'},
      {0, 0, 0, 0},
    };

  opts = Make_Opts();
  while( (opt = getopt_long(argc, argv, opt_str, long_options, &idx)) != -1)
    {
      switch(opt)
	{
	case 'a' : /* use_node_aves */
	  set_flags("true", &(opts->flags), USE_NODE_AVES);
	  break;
	case 'b' : /* random_reads */
	  set_flags("true", &(opts->flags), RANDOM_READS);
	  if (optarg) set_longlong(optarg, &(opts->granularity));
	  break;
	  break;
	case 'E' : /* show_environment */
	  set_flags("true", &(opts->verbosity), SHOW_ENVIRONMENT);
	  break;
	case 'F' : /* force */
	  set_flags("true", &(opts->flags), FORCE);
	  break;
	case 'I' : /* show_intermediate_values */
	  set_flags("true", &(opts->verbosity), SHOW_INTERMEDIATE_VALUES);
	  break;
	case 'H' : /* show_headers */
	  set_flags("true", &(opts->verbosity), SHOW_HEADERS);
	  break;
	case 'l' : /* call_limit */
	  set_int(optarg, &(opts->call_limit));
	  break;
	case 'L' : /* time_limit */
	  set_int(optarg, &(opts->time_limit));
	  break;
	case 'M' : /* no MPI */
	  use_mpi = FORCE_NO_MPI;
	  break;
	case 'n' : /* new */
	  set_flags("true", &(opts->flags), NEW);
	  break;
	case 'p' : /* profiles */
	  if( (optarg == NULL) && ((slurm->use_SLURM == 0) || (slurm->PROCID == 0)) ) usage();
	  set_string(optarg, &(opts->profiles));
	  break;
	case 'P' : /* show_progress */
	  set_flags("true", &(opts->verbosity), SHOW_PROGRESS);
	  break;
	case 'r' : /* remove */
	  set_flags("true", &(opts->flags), REMOVE);
	  break;
	case 'R' : /* read_only */
	  set_flags("true", &(opts->flags), READ_ONLY);
	  break;
	case 's' : /* call_size */
	  set_longlong(optarg, &(opts->call_size));
	  break;
	case 'S' : /* show_signon */
	  set_flags("true", &(opts->verbosity), SHOW_SIGNON);
	  break;
	case 't' : /* test_dir */
	  if( (optarg == NULL) && ((slurm->use_SLURM == 0) || (slurm->PROCID == 0)) ) usage();
	  set_string(optarg, &(opts->testdir));
	  break;
	case 'V' : /* version */
	  printf("mib-%s\n", version);
	  exit(0);
	case 'W' : /* write_only */
	  set_flags("true", &(opts->flags), WRITE_ONLY);
	  break;
	case 'h' :  /* help */
	default : 
	  if ( (slurm->use_SLURM == 0) || (slurm->PROCID == 0) ) usage(); break;
	}
    }
  /* Anything we've consumed doesn't need to be passed to the MPI initializer */
  *argcp -= optind;
  *argvp += optind;
  if( opts->testdir == NULL)
    {
      base_report(SHOW_ALL, "Mib requires a \"-t <test_dir>\" argument.\n");
      exit(1);
    }
  check_fs();
  if( (check_flags(WRITE_ONLY))  && (check_flags(READ_ONLY)) )
    {
      opts->flags &= ~WRITE_ONLY;
      opts->flags &= ~READ_ONLY;
    }
}

void
usage( void )
{
  printf("usage: mib [%s]\n", opt_str);
  printf("    -a              :  Use average profile times accross node.\n");
  printf("    -b [<gran>]     :  Random seeks (optional granularity) before each read\n");
  printf("                    :  seek(fd, gran*file_size*rand()/RAND_MAX, SEEK_SET\n");
  printf("    -E              :  Show environment of test in output.\n");
  printf("    -I              :  Show intermediate values in output.\n");
  printf("    -h              :  This message\n");
  printf("    -H              :  Show headers in output.\n");
  printf("    -l <call_limit> :  Issue no more than this many system calls.\n");
  printf("    -L <time_limit> :  Do not issue new system calls after this many\n");
  printf("                    :    seconds (limits are per phase for write and\n");
  printf("                    :    read phases).\n");
  printf("    -M              :  Do not use MPI even if it is available.\n");
  printf("    -n              :  Create new files if files were already present\n");
  printf("                    :    (will always create new files if none were\n");
  printf("                    :    present).\n");
  printf("    -p <profiles>   :  Output system call timing profiles to \n");
  printf("                    :    <profiles>,write and <profiles>.read.\n");
  printf("    -P              :  Show progress bars during testing.\n");
  printf("    -r              :  Remove files when done.\n");
  printf("    -R              :  Only perform the read test.\n");
  printf("    -s <call_size>  :  Use system calls of this size (default 512k).\n");
  printf("                    :    Numbers may use abreviations k, K, m, or M.\n");
  printf("    -S              :  Show signon message, including date, program\n");
  printf("                    :    name, and version number.\n");
  printf("    -t <test_dir>   :  Required. I/O transactions to and from this\n");
  printf("                    :    directory\n");
  printf("    -V              :  Print the version and exit\n");
  printf("    -W              :  Only perform the write test\n");
  printf("                    :    (if -R and -W are both present both tests \n");
  printf("                    :     will run, but that's the default anyway)\n");
  exit(0);
}

Options *
Make_Opts()
{
  Options *opts;

  opts = (Options *)Malloc(sizeof(Options));
  opts->profiles = NULL;
  opts->testdir = NULL;
  opts->call_limit = 4096;
  opts->call_size = 524288;
  opts->granularity = 1;
  opts->time_limit = 60;
  opts->flags = DEFAULTS;
  opts->verbosity = QUIET;
  return(opts);
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
check_fs()
{
  int found = 0;
  int index = 0;
  int ret;
  struct statfs buf;

  errno = 0;
  if ( (ret = statfs(opts->testdir, &buf) < 0) )
  {
    base_report(SHOW_ALL, "Could not statfs \"%s\" (%d): %s\n", 
		opts->testdir, errno, strerror(errno));
    exit(1);
  }
  while ( (!found) && (fs_list[index].f_type != 0) )
    {
      if (fs_list[index].f_type == buf.f_type) found = 1;
      else index++;
    }
  if (found)
    {
      if( (fs_list[index].warn == 1) && (!check_flags(FORCE)) )
      {
	base_report(SHOW_ALL, "Mib will not write to a file system of type \"%s\" (f_type = %#lx) unless you use the \"-F\" (--force) option \n", 
		    fs_list[index].name, fs_list[index].f_type);
	exit(1);
      }
      base_report(SHOW_ENVIRONMENT, "Testing a file system of type \"%s\" (f_type = %#lx)\n",
		  fs_list[index].name, fs_list[index].f_type);
    }
  else
    {
      base_report(SHOW_ENVIRONMENT, "Testing an unregistered file system with f_type = %#lx\n",
		  buf.f_type);
    }
}  

void
show_details()
{
  char cluster[MAX_BUF];
  char *p;

  if ( mib->rank == mib->base )
    {
      if( slurm->use_SLURM)
	{
	  strncpy(cluster, slurm->NODELIST, MAX_BUF);
	  for(p = cluster; ( (*p != '\0') &&
			     (*p != '[') &&
			     ( (*p < '0') || (*p > '9') ) &&
			     ( p - cluster < MAX_BUF ) ); p++);
	  if( (p > cluster) && 
	      (p - cluster < MAX_BUF) &&
	      (*(p-1) == 'i') 
	      ) *(p-1) = '\0';
	  if( (p > cluster) && 
	      (p - cluster < MAX_BUF)  && 
	      (*p == '[') ||
	      ( ((*p >= '0') && (*p <= '9')) )  ) *p = '\0';
	}
      else
	cluster[0] = '\0';
      printf("cluster                  = %s\n", cluster);      
      printf("new                      = %s\n", ((opts->flags & NEW) ? "true" : "false"));
      printf("remove                   = %s\n", ((opts->flags & REMOVE) ? "true" : "false"));
      printf("nodes                    = %d\n", mib->nodes);
      printf("testdir                  = %s\n", opts->testdir);
      printf("call_limit               = %d\n", opts->call_limit);
      printf("call_size                = %lld\n", opts->call_size);
      printf("time_limit               = %d\n", opts->time_limit);
      printf("tasks                    = %d\n", mib->tasks);
      printf("write_only               = %s\n", ((opts->flags & WRITE_ONLY) ? "true" : "false"));
      printf("read_only                = %s\n", ((opts->flags & READ_ONLY) ? "true" : "false"));
      printf("profiles                 = %s\n", ((opts->profiles == NULL) ? "no" : opts->profiles));
      printf("use_node_aves            = %s\n", ((opts->flags & USE_NODE_AVES) ? "true" : "false"));
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

BOOL
set_string(char *v, char **strp)
{
  int ret;

  if( *strp != NULL) 
      free(*strp);
  *strp = Malloc(strnlen(v, MAX_BUF) + 1);
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

