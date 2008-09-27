#!/usr/bin/python
#*****************************************************************************\
#*  $Id: lwatch.py 1.9 2005/11/30 20:24:57 auselton Exp $
#*****************************************************************************
#*  Copyright (C) 2001-2002 The Regents of the University of California.
#*  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
#*  Written by Andrew Uselton <uselton2@llnl.gov>
#*  UCRL-CODE-222725
#*  
#*  This file is part of Mib, an MPI-based parallel I/O benchamrk
#*  For details, see <http://www.llnl.gov/linux/mib/>.
#*  
#*  Mib is free software; you can redistribute it and/or modify it under
#*  the terms of the GNU General Public License (as published by the Free
#*  Software Foundation version 2, dated June 1991.
#*  
#*  Mib is distributed in the hope that it will be useful, but WITHOUT 
#*  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
#*  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
#*  for more details.
#*  
#*  You should have received a copy of the GNU General Public License along
#*  with Mib; if not, write to the Free Software Foundation, Inc.,
#*  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
#*****************************************************************************/
#
# lwatch.py -f <file_system> [-v]
#   <file_system> is the URL for an lmtd daemon monitoring a given file system
#   -v will send some extra commentary to stderr during start up
# This utility was cribbed by reverse engineering the non-GUI portion
# from Chris Morrone's excellent "xwatch-lustre" utility:
# Which is a part of Lustre Administrative Tools.
#  UCRL-CODE-2006-155801
# For details, see <http://www.llnl.gov/linux/lustre-admin/>.
#
# This command line tool contacts a single "lmtd" daemon as given on
# the command line.  Once initialized it, loops every 5 seconds polling
# the lmtd for information about the file system in question.  It
# prints a timestamp and the aggregate read and write rates to stdout.
# If the communication fails for any reason that timestamped line gets
# zeros, and the utility goes on.  It terminates upon receiving a
# sigkill.

__version__ = "$Revision: 1.0 $"
# $Source: ~/src/lwatch/lwatch.py,v $

import os
import time
import cPickle as pickle
import optparse
import re
import socket
import sys
import thread
from types import *
import xmlrpclib
from lmtsupport import *
import lmtxml

socket.setdefaulttimeout(15)
verbose = False
fs = []
url = ""

class OST:
    def __init__(self, name):
        self.name = name
        self.bytes_written = 0
        self.bytes_read = 0
        self.time_stamp = 0

    def init(self, svc):
        self.bytes_written = svc.write_bytes
        self.bytes_read = svc.read_bytes
        self.time_stamp = svc.timestamp
        
    def rate(self, svc):
        write = 0
        read  = 0
        if svc.timestamp > self.time_stamp:
            write = (svc.write_bytes - self.bytes_written)/(svc.timestamp - self.time_stamp)
            read = (svc.read_bytes - self.bytes_read)/(svc.timestamp - self.time_stamp)
            self.bytes_written = svc.write_bytes
            self.bytes_read = svc.read_bytes
            self.time_stamp = svc.timestamp
        return (write, read)

        
def unpickleStats(stats):
    """Recursive function that unpickles a list of pickled objects.

    If a list entry is a list rather than a string, unpickleStats
    calls itself recursively to handle that list.
    """
    if type(stats) == StringType:
        stats = [stats]
    services = []
    if type(stats) == ListType:
        for s in stats:
            if type(s) == StringType:
                services.extend(pickle.loads(s))
            elif type(s) == ListType:
                services.extend(unpickleStats(s))
    return services

def initStats(osts):
    global url
    
    # Contact the LMT Collector to retrieve the lustre statistics
    server = xmlrpclib.ServerProxy(url)
    rpc = 'getAllInfo'
    try:
        stats = getattr(server, rpc)()
        services = unpickleStats(stats)
        for service in services:
            if osts.has_key(service.uuid):
                osts[service.uuid].init(service)
                
    except Exception, e:
        print >> sys.stderr, 'xmlrpc failed', e

def pollStats(osts):
    global url

    skip = False
    aggw = 0
    aggr = 0
    # Contact the LMT Collector to retrieve the lustre statistics
    server = xmlrpclib.ServerProxy(url)
    rpc = 'getAllInfo'
    try:
        stats = getattr(server, rpc)()
    except Exception, e:
        print >> sys.stderr, 'xmlrpc failed', e
    services = unpickleStats(stats)
    for service in services:
        if osts.has_key(service.uuid):
            (write, read) = osts[service.uuid].rate(service)
            if ((write == 0) or (read == 0)): skip = True
            aggw += write
            aggr += read
    print time.ctime(time.time()), aggr/(1024*1024), aggw/(1024*1024)
    sys.stdout.flush()
    
def main():
    global verbose
    global url
    global fs
    osts = {}
    # Parse the command line parameters
    parser = optparse.OptionParser()
    parser.add_option("-f", "--filesystem", dest="filesystem", default="",
                      help="filesystem URL", metavar="URL")
    parser.add_option("-v", "--verbose",
                      action="store_true", dest="verbose", default=False,
                      help="Print status messages to stderr")

    (options, args) = parser.parse_args()
    verbose = options.verbose
    url = options.filesystem
    if url == "":
        print >>sys.stderr, "URL required"
        exit

    # Collect and parse the Luste XML into useful objects
    if verbose:
        print >> sys.stderr, "Retrieving Lustre XML from", url, "...",
        sys.stdout.flush()
    try:
        xml = xmlrpclib.Server(url).getLustreXML()
    except Exception, e:
        print >> sys.stderr, 'xmlrpclib getLustreXML failed'
    if verbose:
        print >> sys.stderr, "done."
        sys.stdout.flush()
        print >> sys.stderr, "Parsing Lustre XML ...",
        sys.stdout.flush()
    try:
        fs = lmtxml.parseString(xml)[0]
    except Exception, e:
        print >> sys.stderr, 'lmtxml parseString failed'
    if verbose:
        print >> sys.stderr, "done."
        sys.stdout.flush()
    for ost in fs.ost:
        osts[ost.name] = OST(ost.name)
    initStats(osts)
    while 1:
        pollStats(osts)
        time.sleep(5)
    
if __name__ == '__main__':
    main()
