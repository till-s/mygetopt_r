/* $Id$ */

#include <stdlib.h>
#include <string.h>
#if defined(DIRSHELL) && !defined(__vxworks)
#include <unistd.h>
#endif

#include "dirOps.h"
#include "bitMenu.h"
#include "ecFastKeys.h"


#if (defined(__vxworks) || defined(DEBUG)) && defined(DIRSHELL)

#ifdef __vxworks
#define optind myoptind
#define getopt mygetopt
#endif

/* provide getopt for vxworks */
int myoptind=0;

static int
mygetopt(int argc, char **argv, char *optstr)
{
int rval;
static char		*chpt=0;

	/* leftover options from previous call ? */
	while (!chpt) {
		int nrot,endrot;

		if (myoptind>=argc) {
			return -1; /* no more options */
		}
		for (	endrot=myoptind+1;
			endrot<argc;
			endrot++) {
			if ('-'==argv[endrot][0]) {
				endrot++;
				break;
			}
		}
		nrot=endrot-myoptind;

		while ('-'!=*argv[myoptind] && nrot--) {
			char *tmp=argv[myoptind];
			int i;
			for (i=myoptind; i<endrot-1; i++) {
				argv[i]=argv[i+1];
			}
			argv[endrot-1]=tmp;
		}
		if (nrot<=0) {
			/* no more options found */
			return -1;
		}
		if (!*(chpt=argv[myoptind]+1)) chpt=0; /* empty string */
		myoptind++;
	}
	rval = strchr(optstr,*chpt) ? *chpt : '?';
	if (!*(++chpt)) chpt=0;
	return rval;
}
#endif

extern EcNodeRec ecRootNode;

/* routines for interactive operation */
static EcNodeListRec top = {
	n:	&ecRootNode,
	p:	0
};

static EcNodeList cwd=&top;

static void rprint(EcNodeList l, FILE *f, int *nchars)
{
	if (l->p) {
		rprint(l->p,f,nchars);
		fputc(EC_DIRSEP_CHAR,f);
	}
	{ int n = fprintf(f,l->n->name);
		if (nchars) *nchars+=n;
	}
}


typedef struct LsOptsRec_ {
	FILE		*f;
	unsigned long	flags;
} LsOptsRec, *LsOpts;

static void
printNodeInfo(EcNodeList l, IOPtr p, void *arg)
{
LsOpts	o=(LsOpts)arg;
FILE	*f=o->f ? o->f : stderr;
EcNode 	n=l->n;
EcMenu	m=0;
EcFKey	fk;
int	pad=0;

	if (l->p) {
		rprint(l->p,f,&pad);
		fputc(EC_DIRSEP_CHAR,f);
	}
	pad+=fprintf(f,n->name);

	fk=ecNodeList2FKey(l);

	if (EcNodeIsDir(n)) {
		pad+=fputc(EC_DIRSEP_CHAR,f);
	} else {
		m=ecMenu(n->u.r.flags);
		if ( m ) {
			pad+=fprintf(f," -v");
		}
		if (EcNodeIsArray(n))
			pad+=fprintf(f,"[]");
		while (pad++<35) fputc(' ',f);
		if ( DIROPS_LS_VERBOSE & o->flags ) {
			Val_t v,rv,*memp=0,*pv=&v;
			int nels=1,i;
			EcErrStat e;
			if ( EcNodeIsArray(n) ) {
				nels = n->u.r.pos2;
				pv = memp = (Val_t*) malloc(sizeof(Val_t)*nels);
			}
			e=(ecGetValue(n, p,pv) || (nels>1 && ecGetRawValue(n,p,&rv)));
			if (e) {
				fprintf(f,"ERROR: %s",ecStrError(e));
			} else {
				if (m)
					fprintf(f," @0x%08x: %s (ini: %s, raw: 0x%08x)",
						p, m->items[v].name, m->items[n->u.r.inival].name, rv );
				else {
					if (nels > 1) {
						fprintf(f,"   array [%i] @0x%08x\n",nels,p);
						while (nels--)
							fprintf(f,"    %8i\n", *(pv++));
					} else {
						fprintf(f," @0x%08x: %i (ini: %i, raw: 0x%08x)",
							p, v, n->u.r.inival, rv );
					}
				}
			}
			if (memp) free(memp);
		}
	}
	while (pad++<35) fputc(' ',f);
	if ( DIROPS_LS_FKEYINFO & o->flags ) {
		fprintf(f, "  [FKEY: 0x%08x]", fk);
	}
	fprintf(f,"\n");
	if ( DIROPS_LS_SHOWMENU & o->flags && m ) {
		int i;
		for (i=0; i<m->nels; i++)
			fprintf(f, "   - %2i: %s\n",i,m->items[i].name);
	}
}

/* print current path */
void
ecPwd(FILE *f)
{
int nchars=0;
	if (!f) f=stderr;
	rprint(cwd, f, &nchars);
	fputc('\n',f);
}

static int
getArrayIndex(char **s)
{
int rval;
char *chpt=*s, *endp;
	if (!(chpt=strchr(*s,'['))) return -1;
	rval=strtoul(chpt+1,&endp,0);
	if (']'!=*endp) return -1;
	*s = chpt;
	return rval;
}

EcErrStat
ecGet(EcKey k, Val_t *valp, EcMenu *mp)
{
EcErrStat	e=EcErrOK;
IOPtr		p=0;
EcNode		n;
EcNodeList	l;
int		idx;
char		buf[100],*chpt;
Val_t		*arr=0;

	if (mp) *mp=0;
	for (l=cwd; l; l=l->p)
		p+=l->n->offset;

	strcpy(buf,k);
	chpt=buf;
	if ((idx=getArrayIndex(&chpt))>=0) {
		*chpt=0; /* strip [idx] */
	}

	if (!(n=lookupEcNode(cwd->n, buf, &p, 0))) {
		return EcErrNodeNotFound;
	}
	if (EcNodeIsDir(n)) {
		e = EcErrNotLeafNode;
		goto cleanup;
	}
	if ( ((idx>=0) ^ (0 != EcNodeIsArray(n))) || idx >= n->u.r.pos2  ) {
		e = EcErrInvalidIndex;
		goto cleanup;
	}
	if (EcNodeIsArray(n)) {
		arr = (Val_t*)malloc(sizeof(Val_t)*n->u.r.pos2);
		if (e=ecGetValue(n, p, arr))
			goto cleanup;
		*valp = arr[idx];
	} else {
		if (e=ecGetValue(n, p, valp))
			goto cleanup;
		if (mp) *mp = ecMenu(n->u.r.flags);
	}

cleanup:
	if (arr) free(arr);
	return e;
}

EcErrStat
ecPut(EcKey k, Val_t val)
{
IOPtr		p=0;
EcNode		n;
EcErrStat	e=EcErrOK;
EcNodeList	l;
int		idx;
char		buf[100],*chpt;
Val_t		*arr=0;

	for (l=cwd; l; l=l->p)
		p+=l->n->offset;
	strcpy(buf,k);
	chpt=buf;
	if ((idx=getArrayIndex(&chpt))>=0) {
		*chpt=0; /* strip [idx] */
	}


	if (!(n=lookupEcNode(cwd->n, buf, &p, 0))) {
		return EcErrNodeNotFound;
	}
	if (EcNodeIsDir(n)) {
		e = EcErrNotLeafNode;
		goto cleanup;
	}
	if ( ((idx>=0) ^ (0 != EcNodeIsArray(n))) || idx >= n->u.r.pos2  ) {
		e = EcErrInvalidIndex;
		goto cleanup;
	}
	if (EcNodeIsArray(n)) {
		arr = (Val_t*)malloc(sizeof(Val_t)*n->u.r.pos2);
		if (e=ecGetValue(n, p, arr))
			goto cleanup;
		arr[idx]=val;
		val = (Val_t)arr;
	}
	e = ecPutValue(n, p, val);
cleanup:
	if (arr) free(arr);
	return e;
}

/* change working directory */
void
ecCd(EcKey k)
{
if (EcKeyIsEmpty(k))
	return;
if (EcKeyIsUpDir(k)) {
	if (cwd->p) cwd = freeNodeListRec(cwd);
} else if ( ! lookupEcNode(cwd->n, k, 0, &cwd) ||
	    (! EcNodeIsDir(cwd->n) && (cwd = freeNodeListRec(cwd)) )) {
	 /* Note: cwd=freeNodeListRec() should always be true
	  *       we just want it to be executed if !EcNodeIsDir()
	  */
	fprintf(stderr,"dirnode not found\n");
}
}

/* list node contents */
void
ecLs(EcKey k, FILE *f, int flags)
{
EcNodeList l=0;
EcNode	   n;
IOPtr	   b=0;
LsOptsRec  o = { f: f, flags: flags };
	for (l=cwd; l; l=l->p)
		b+=l->n->offset;
	if (!f) f=stderr;

	if (EcKeyIsEmpty(k)) {
		n=cwd->n;
	} else {
	if (!(n=lookupEcNode(cwd->n, k, &b, &l))) {
		fprintf(stderr,"node not found\n");
		return;
	}
	}
	if (DIROPS_LS_RECURSE & flags) {
		/* don't count cwd twice */
		walkEcNode(n, printNodeInfo, b-(n->offset), 0, (void*)&o);
	} else {
		if (EcNodeIsDir(n)) {
			int i;
			EcNode nn;
			EcNodeListRec thisdir = { p: l, 0 };
			for (i=0,nn=n->u.d.n->nodes; i<n->u.d.n->nels; i++,nn++) {
				thisdir.n=nn;
				printNodeInfo(&thisdir, b + nn->offset, (void*)&o);
			}
		} else {
			printNodeInfo(l, b, (void*)&o);
		}
	}
	while (l) l=freeNodeListRec(l);
}

#ifdef DIRSHELL

#define MAXARGS		10
#define MAXARGCHARS	200

void
dirshell(void)
{
int ac=0;
int ch,ai;
char *argv[MAXARGS+1];

char args[MAXARGS][MAXARGCHARS];

for (ac=0; ac< MAXARGS; ac++)
	argv[ac]=&args[ac][0];

while (1) {
fprintf(stderr,"#");
fflush(stderr);
ch=getchar();
ac=ai=0;
while (1) {
	while ('\t'==ch || ' '==ch)
		(ch=getchar());
	if ('\n'==ch) break;
	while ('\t'!=ch && ' '!=ch && '\n'!=ch) {
		if (ai<MAXARGCHARS-1) args[ac][ai++]=ch;
		ch=getchar();
	}
	
	args[ac][ai]=0;
	argv[ac]=&args[ac][0];
	ai=0;

	if (++ac>=10) {
		while ('\n'!=ch) ch=getchar();
		break;
	}
}

if (!ac) continue;

argv[ac]=0;

if (!strcmp("pwd",args[0])) {
	ecPwd(stderr);
} else
if (!strcmp("cd",args[0])) {
	if (ac<2) {
		fprintf(stderr,"dir arg needed\n");
		continue;
	}
	ecCd(args[1]);
} else
if (!strcmp("ls",args[0])) {
	unsigned long flags=0;
	int ch;
	myoptind=1;
	while ((ch=mygetopt(ac,argv,"avmk"))>=0) {
		switch(ch) {
			case 'v': flags|=DIROPS_LS_VERBOSE; break;
			case 'a': flags|=DIROPS_LS_RECURSE; break;
			case 'm': flags|=DIROPS_LS_SHOWMENU; break;
			case 'k': flags|=DIROPS_LS_FKEYINFO; break;
			default:  fprintf(stderr,"unknown option\n"); break;
		}
	}
	ecLs(EcString2Key((myoptind < ac) ? args[myoptind] : 0), stderr, flags);
} else
#ifdef DEBUG
if (!strcmp("tstopt",args[0])) {
	int ch;
	myoptind=0;
	while ((ch=mygetopt(ac,argv,"avk"))) {
		int i;
		fprintf(stderr,"found '%c', optind is %i, argv now:\n",
				ch, myoptind);
		for (i=0; i<ac; i++) {
			fprintf(stderr,"%s | ",argv[i]);
		}
		fprintf(stderr,"\n");
		if (-1==ch) break;
	}
} else
#endif
if (!strcmp("get",args[0])) {
	Val_t		v;
	EcMenu		m;
	EcErrStat	e;
	if (ac<2) {
		fprintf(stderr,"key arg needed\n");
		continue;
	}
	if (e=ecGet(args[1],&v, &m)) {
		fprintf(stderr,"ERROR: %s\n",ecStrError(e));
		continue;
	}
	fprintf(stderr,"VALUE: ");
	if (m) {
		fprintf(stderr,"%s\n", m->items[v].name);
	} else {
		fprintf(stderr,"%i (0x%x)\n",v,v);
	}
} else
if (!strcmp("put",args[0])) {
	Val_t		v;
	EcMenu		m;
	EcErrStat	e;
	char		*end;
	if (ac<3 || (v=strtoul(args[2],&end,0), !*args[2] || *end)) {
		fprintf(stderr,"key and value args needed\n");
		continue;
	}
	if ((e=ecPut(args[1],v)) || (e=ecGet(args[1],&v,&m))) {
		fprintf(stderr,"ERROR: %s\n",ecStrError(e));
		continue;
	}
	fprintf(stderr,"read back VALUE: ");
	if (m) {
		fprintf(stderr,"%s\n", m->items[v].name);
	} else {
		fprintf(stderr,"%i (0x%x)\n",v,v);
	}
} else
if (!strcmp("prfkey",args[0])) {
	EcNodeList	l=0;
	EcFKey		fk;
	IOPtr		b=0;
	if (ac<2 || 1!=sscanf(argv[1],"%i",&fk)) {
		fprintf(stderr,"fastkey id arg required\n");
		continue;
	}
	if (!lookupEcNodeFast(top.n, fk, &b, &l)) {
		fprintf(stderr,"Node not found!\n");
		continue;
	}
	rprint(l, stderr, 0);
	fprintf(stderr,"\n");
	while (l) l=freeNodeListRec(l);
} else
if (!strcmp("quit",args[0])) {
	return;
} else {
	fprintf(stderr,"unknown command '%s'\n",args[0]);
}
}

}
#endif

void
ptop(void)
{
printf("n: 0x%08x, p: 0x%08x\n",top.n, top.p);
}
