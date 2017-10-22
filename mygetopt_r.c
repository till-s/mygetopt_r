/* A reentrant getopt implementation */

/* SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 *
 * Authorship
 * ----------
 * This software (CEXP - C-expression interpreter and runtime
 * object loader/linker) was created by
 *
 *    Till Straumann <strauman@slac.stanford.edu>, 2002-2008,
 * 	  Stanford Linear Accelerator Center, Stanford University.
 *
 * Acknowledgement of sponsorship
 * ------------------------------
 * This software was produced by
 *     the Stanford Linear Accelerator Center, Stanford University,
 * 	   under Contract DE-AC03-76SFO0515 with the Department of Energy.
 * 
 * Government disclaimer of liability
 * ----------------------------------
 * Neither the United States nor the United States Department of Energy,
 * nor any of their employees, makes any warranty, express or implied, or
 * assumes any legal liability or responsibility for the accuracy,
 * completeness, or usefulness of any data, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately owned
 * rights.
 * 
 * Stanford disclaimer of liability
 * --------------------------------
 * Stanford University makes no representations or warranties, express or
 * implied, nor assumes any liability for the use of this software.
 * 
 * Stanford disclaimer of copyright
 * --------------------------------
 * Stanford University, owner of the copyright, hereby disclaims its
 * copyright and all other rights in this software.  Hence, anyone may
 * freely use it for any purpose without restriction.  
 * 
 * Maintenance of notices
 * ----------------------
 * In the interest of clarity regarding the origin and status of this
 * SLAC software, this and all the preceding Stanford University notices
 * are to remain affixed to any copy or derivative of this software made
 * or distributed by the recipient and are to be affixed to any copy of
 * software made or distributed by the recipient that contains a copy or
 * derivative of this software.
 * 
 * SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 */ 

#include <string.h>

#include "mygetopt_r.h"


int
mygetopt_r(int argc, char **argv, const char *optstr, MyGetOptCtxt ctx)
{
int				rval;
char			*optfound;

	if (!ctx->optind) {
		/* first call */
		ctx->optind=1;
		ctx->chpt=0;
	}
	ctx->optarg=0;

	/* leftover options from previous call ? */
	while (!ctx->chpt) {
		int nrot,endrot;

		if (ctx->optind>=argc) {
			return -1; /* no more options */
		}
		for (	endrot=ctx->optind+1;
			endrot<argc;
			endrot++) {
			if ('-'==argv[endrot][0]) {
				char *optc;
				/* for every option with an argument, we must shift that argument */
				for (optc=argv[endrot]+1; endrot<argc && *optc; optc++) {
					if ((optfound=strchr(optstr,*optc)) && ':'==*(optfound+1))
						endrot++;
				}
				endrot++;
				break;
			}
		}
		
		nrot=endrot-(ctx->optind);

		while ('-'!=*argv[ctx->optind] && nrot--) {
			char *tmp=argv[ctx->optind];
			int i;
			for (i=ctx->optind; i<endrot-1; i++) {
				argv[i]=argv[i+1];
			}
			argv[endrot-1]=tmp;
		}
		if (nrot<=0) {
			/* no more options found */
			return -1;
		}
		if (!*(ctx->chpt=argv[ctx->optind]+1)) ctx->chpt=0; /* empty string */
		ctx->optind++;
	}
	rval = (optfound=strchr(optstr,*ctx->chpt)) ? *ctx->chpt : '?';
	if (!*(++ctx->chpt)) ctx->chpt=0;
	if (optfound && ':'==*(optfound+1)) {
		/* has an argument, store in optarg and skip */
		ctx->optarg=argv[ctx->optind++];
	}
	return rval;
}

