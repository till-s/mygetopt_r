#ifndef MY_REENTRANT_GETOPT
#define MY_REENTRANT_GETOPT
/* $Id$ */

/* Author: Till Straumann <strauman@slac.stanford.edu> */

/* A reentrant getopt implementation */


/* Licensing: EPICS Open License */

/*
 * Copyright 2002, Stanford University and
 * 		Till Straumann <strauman@slac.stanford.edu>
 * 
 * Stanford Notice
 * ***************
 * 
 * Acknowledgement of sponsorship
 * * * * * * * * * * * * * * * * *
 * This software was produced by the Stanford Linear Accelerator Center,
 * Stanford University, under Contract DE-AC03-76SFO0515 with the Department
 * of Energy.
 * 
 * Government disclaimer of liability
 * - - - - - - - - - - - - - - - - -
 * Neither the United States nor the United States Department of Energy,
 * nor any of their employees, makes any warranty, express or implied,
 * or assumes any legal liability or responsibility for the accuracy,
 * completeness, or usefulness of any data, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately
 * owned rights.
 * 
 * Stanford disclaimer of liability
 * - - - - - - - - - - - - - - - - -
 * Stanford University makes no representations or warranties, express or
 * implied, nor assumes any liability for the use of this software.
 * 
 * This product is subject to the EPICS open license
 * - - - - - - - - - - - - - - - - - - - - - - - - - 
 * Consult the LICENSE file or http://www.aps.anl.gov/epics/license/open.php
 * for more information.
 * 
 * Maintenance of notice
 * - - - - - - - - - - -
 * In the interest of clarity regarding the origin and status of this
 * software, Stanford University requests that any recipient of it maintain
 * this notice affixed to any distribution by the recipient that contains a
 * copy or derivative of this software.
 */


/* provide reentrant getopt */

/* repeated calls to mygetopt_r need the
 * context info passed in.
 * On the first call, the context must be
 * initialized to all zeroes.
 */
typedef struct MyGetOptCtxtRec_ {
	int		optind;
	char	*optarg;
	char	*chpt;
} MyGetOptCtxtRec, *MyGetOptCtxt;

#ifdef __cplusplus
extern "C" 
#endif
int
mygetopt_r(int argc, char **argv, char *optstr, MyGetOptCtxt ctx);

#endif
