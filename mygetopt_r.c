#include <string.h>

#include "mygetopt_r.h"

/* provide reentrant getopt */

int
mygetopt_r(int argc, char **argv, char *optstr, MyGetOptCtxt ctx)
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

