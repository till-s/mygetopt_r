#ifndef MY_REENTRANT_GETOPT
#define MY_REENTRANT_GETOPT

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
