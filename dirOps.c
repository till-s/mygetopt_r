/* $Id$ */

#include <stdlib.h>
#include <string.h>
#if !defined(__vxworks)
#include <unistd.h>
#endif
#include <stdarg.h>

#include "dirOps.h"
#include "bitMenu.h"
#include "ecFastKeys.h"


#if (defined(__vxworks) || defined(DEBUG))

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

/* routines for interactive operation */

static struct {
	EcNode		n;
	EcBoardDesc 	bd;
} cwd = {0};


typedef struct LsOptsRec_ {
	FILE		*f;
	unsigned long	flags;
	EcNode		pathRelative;
} LsOptsRec, *LsOpts;

typedef struct RprintInfoRec_ {
	EcNode		clip;
	int		pos;
	EcBoardDesc	bd;
} RPrintInfoRec, *RPrintInfo;

static void
rprint(EcNode l, FILE *f, RPrintInfo info)
{
int n;
	if ( l == info->clip ) return;

	if (l->parent) {
		rprint(l->parent,f,info);
		fputc(EC_DIRSEP_CHAR,f);
		n = fprintf(f,l->cnode->name);
	} else {
		/* print board name */
		n = info->bd ? fprintf(f,"%s%c",info->bd->name,EC_BRDSEP_CHAR) : 0;
	}
	info->pos+=n;
}


static void
printNodeInfo(EcBoardDesc bd, EcNode l, void *arg)
{
LsOpts	o=(LsOpts)arg;
FILE	*f=o->f ? o->f : stderr;
EcCNode n=l->cnode;
EcMenu	m=0;
EcFKey	fk;
int	pad=0;

	if (l->parent && l->parent!=o->pathRelative) {
		RPrintInfoRec pi;

		pi.clip = o->pathRelative;
		pi.bd	 = bd;
		pi.pos  = pad;

		rprint(l->parent,f,&pi);
		fputc(EC_DIRSEP_CHAR,f);
	
		pad = pi.pos;
	}
	pad+=fprintf(f,n->name);

	fk=ecNode2FKey(l);

	if (EcCNodeIsDir(n)) {
		pad+=fputc(EC_DIRSEP_CHAR,f);
	} else {
		m=ecMenu(n->u.r.flags);
		if ( m ) {
			pad+=fprintf(f," -v");
		}
		if (EcCNodeIsArray(n))
			pad+=fprintf(f,"[]");
		while (pad++<35) fputc(' ',f);
		if ( DIROPS_LS_VERBOSE & o->flags ) {
			Val_t v,rv,*memp=0,*pv=&v;
			int nels=1,i;
			EcErrStat e;
			if ( EcCNodeIsArray(n) ) {
				nels = n->u.r.pos2;
				pv = memp = (Val_t*) malloc(sizeof(Val_t)*nels);
			}
			e=(ecGetValue(bd, l, pv) || (1==nels && ecGetRawValue(bd, l, &rv)));
			if (e) {
				fprintf(f,"ERROR: %s",ecStrError(e));
			} else {
				IOPtr p = bd->base + l->u.offset;
				if (m)
					fprintf(f," @0x%08x: %s (ini: %s, raw: 0x%08x)",
						p,
						m->items[v].name, m->items[n->u.r.inival].name,
						rv );
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


#define NOT_EOS(s) (*(s) && EC_DIRSEP_CHAR != *(s) && EC_BRDSEP_CHAR != *(s))

/* search for the last occurrence of needle (n..e) in haystack (h) */
static inline char *
my_strstrn(char *h, char *n, char *e)
{
char *hh,*nn;
char *rval=0;
	do {
		/* skip haystack until a char matches needle */
		if (*h != *n) continue;
		hh=h; nn=n;
		/* try to match */
		while (*nn==*hh) {
			nn++; hh++;
			/* reached end of needle -> record match
			 * and try again
			 */
			if (nn >= e) rval=h;
		}
		/* match failed; increment haystack and try again */
	} while ((hh=h++), NOT_EOS(hh));
	return rval;
}

/* match a key 'k' against a 'glob pattern' 'g' which
 * may contain '*' wildcard characters. Both of the
 * strings may be '0' or EC_DIRSEP_CHAR terminated.
 */
static EcKey
globMatch(EcKey k, EcKey g)
{
EcKey	endp;
int	l;
	/* search end of head string */
	for (endp=g; NOT_EOS(endp) && EC_GLOB_CHAR!=*endp; endp++)
		/* do nothing else */;
	l=endp-g;
	if (strncmp(k,g,l)) return 0; /* head doesn't match */
	/* strip head */
	k+=l; g=endp;
	/* the string consists now of a sequence:  {<wildcard>, <stringpart>} <NULL> */
	/* entering the while loop, *g=='*' (after *g test) */
	while (*g) {
		/* get start of next string element */
		while (EC_GLOB_CHAR==*g) g++;
		if ( ! NOT_EOS(g)) /* wildcard at end of string matches everything */
			return g; /* (success) */
		/* search end of string element */
		for (endp=g; NOT_EOS(endp) && EC_GLOB_CHAR!=*endp; endp++)
			/* do nothing else */;
		/* search last occurrence of next string element */
		if (!(k=my_strstrn(k,g,endp)))
			return 0; /* no match found */
		/* skip string element */
		k+=(endp-g); g=endp;
	}
	/* are both strings consumed ? */
	return NOT_EOS(k) ? 0 : g;
}

#define GLB_OPT_LEAVES	(1<<0)	/* apply function to leaves */
#define GLB_OPT_DIRS	(1<<1)	/* apply function to dirs */
#define GLB_OPT_ALL	(GLB_OPT_LEAVES | GLB_OPT_DIRS)

typedef struct RGlobParmsRec_ {
	char 		expansion[500];
	va_list		fnargs;
	EcErrStat	(*fn)(EcKey expansion, va_list args);
	int		options;
	int		count;
} RGlobParmsRec, *RGlobParms;

static EcErrStat
rglob(EcNode n, EcKey append, EcKey globPat, RGlobParms p)
{
unsigned char i, *ptr;
	if (!n || !EcNodeIsDir(n)) return EcErrNodeNotFound;
	for (i=0; i<n->cnode->u.d.n->nels; i++) {
		/* try to match a path element */
		if ((ptr=globMatch(n->cnode->u.d.n->nodes[i].name, globPat))) {
			EcErrStat	rval;
			/* match */
			if (*ptr) ptr++; /* skip dirsep etc. char */
			if (*ptr) {
				/* end of pattern not reached yet */
				if (EcNodeIsDir(&n->u.entries[i])) {
					int l;
					/* append the name */
					l=sprintf(append,"%s%c",
						n->cnode->u.d.n->nodes[i].name,
						EC_DIRSEP_CHAR);
					if (rval=rglob(&n->u.entries[i], append+l, ptr, p))
						return rval;
				} /* else just skip it */
			} else {
				strcpy(append, n->cnode->u.d.n->nodes[i].name);
				/* end of pattern reached, invoke fn */
				rval = EcError;
				if (EcNodeIsDir(&n->u.entries[i]) && (GLB_OPT_DIRS & p->options))
					rval = EcErrOK;
				else if (GLB_OPT_LEAVES & p->options)
					rval = EcErrOK;

				if ( EcErrOK==rval) {
					p->count++;
				       	if (rval = (*p->fn)(p->expansion, p->fnargs))
					return rval;
				}
			}
		}
	}
	return EcErrOK;
}

EcErrStat
globbedDo(EcKey globPat, int options, EcErrStat (*fn)(EcKey, va_list args), ...)
{
EcErrStat	rval=EcErrOK;
RGlobParmsRec	parms;
EcKey		ptr,a;

	parms.expansion[0]=0;
	va_start(parms.fnargs,fn);
	parms.fn=fn;
	parms.options = options;
	parms.count=0;

	if (EcKeyIsEmpty(globPat)) {
		/* direct invokation */
		fn(globPat,parms.fnargs);
		goto cleanup;
	}
	/* let's see if the globPat is absolute or relative */
	if (strchr(globPat,EC_BRDSEP_CHAR)) {
		int i;
		EcBoardDesc bd;
		/* board specified, loop over all boards */
		for (i=0; bd=ecGetBoardDesc(i); i++) {
			if ((ptr=globMatch(bd->name, globPat)) && EC_GLOB_CHAR==*ptr) {
				a = parms.expansion;
				a+=sprintf(a,"%s%c",bd->name,EC_GLOB_CHAR);
				if (rval=rglob(bd->root, a, ptr, &parms))
					goto cleanup;
			}
		}
	} else {
		a=parms.expansion;
		if (EC_DIRSEP_CHAR==*globPat) {
			/* absolute */
			*(a++)='/'; *a=0;
			rval=rglob(cwd.bd->root, a, globPat+1, &parms);
		} else {
			/* relative */
			rval=rglob(cwd.n, a, globPat, &parms);
		}
	}

	if (rval) {
		fprintf(stderr, "Error at '%s' (%s)\n",
				parms.expansion,
				ecStrError(rval));
	} else {
		if (0==parms.count)
			fprintf(stderr,"Warning: 0 matches found\n");
	}

cleanup:
	va_end(parms.fnargs);
	return rval;
}
 

/* print current path */
void
ecPwd(FILE *f)
{
int nchars=0;
	if (!f) f=stderr;

	if (!cwd.bd) {
		fprintf(f,"<no path, use 'ls/cd'>\n");
	} else {
		RPrintInfoRec pi;
		pi.clip=0;
		pi.bd  =cwd.bd;
		pi.pos =0;
		rprint(cwd.n, f, &pi);
	}
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
EcCNode		n;
EcNode		node;
EcCNodeList	l;
int		idx;
char		buf[100],*chpt;
Val_t		*arr=0;
EcBoardDesc	bd=cwd.bd;

	if (mp) *mp=0;

	strcpy(buf,k);
	chpt=buf;
	if ((idx=getArrayIndex(&chpt))>=0) {
		*chpt=0; /* strip [idx] */
	}

	if (!(node=ecNodeLookup(cwd.n, buf, &bd))) {
		return EcErrNodeNotFound;
	}
	n=node->cnode;

	if (EcCNodeIsDir(n)) {
		e = EcErrNotLeafNode;
		goto cleanup;
	}
	if ( ((idx>=0) ^ (0 != EcCNodeIsArray(n))) || idx >= n->u.r.pos2  ) {
		e = EcErrInvalidIndex;
		goto cleanup;
	}
	if (EcCNodeIsArray(n)) {
		arr = (Val_t*)malloc(sizeof(Val_t)*n->u.r.pos2);
		if (e=ecGetValue(bd, node, arr))
			goto cleanup;
		*valp = arr[idx];
	} else {
		if (e=ecGetValue(bd, node, valp))
			goto cleanup;
		if (mp) *mp = ecMenu(n->u.r.flags);
	}

cleanup:
	if (arr) free(arr);
	return e;
}

EcErrStat
ecGetPrint(EcKey k, va_list ap)
{
FILE		*f=va_arg(ap,FILE*);
Val_t		v;
EcMenu		m;
EcErrStat	rval;

	if ((rval=ecGet(k,&v,&m))) {
		fprintf(f,"Unable to get '%s' <%s>\n",
				k,
				ecStrError(rval));
		return rval;
	}
	fprintf(f,"%s: ", k);
	if (m) {
		fprintf(f,"%s\n", m->items[v].name);
	} else {
		fprintf(f,"%i (0x%x)\n",v,v);
	}
	return EcErrOK;
}

EcErrStat
ecPut(EcKey k, va_list ap)
{
Val_t		val=va_arg(ap,Val_t), rdback;
EcNode		node;
EcCNode		n;
EcErrStat	e=EcErrOK;
int		idx;
char		buf[100],*chpt;
Val_t		*arr=0;
EcBoardDesc	bd=cwd.bd;

	strcpy(buf,k);
	chpt=buf;
	if ((idx=getArrayIndex(&chpt))>=0) {
		*chpt=0; /* strip [idx] */
	}

	if (!(node=ecNodeLookup(cwd.n, buf, &bd))) {
		return EcErrNodeNotFound;
	}
	n=node->cnode;
	if (EcCNodeIsDir(n)) {
		e = EcErrNotLeafNode;
		goto cleanup;
	}
	if ( ((idx>=0) ^ (0 != EcCNodeIsArray(n))) || idx >= n->u.r.pos2  ) {
		e = EcErrInvalidIndex;
		goto cleanup;
	}
	if (EcCNodeIsArray(n)) {
		arr = (Val_t*)malloc(sizeof(Val_t)*n->u.r.pos2);
		if (e=ecGetValue(bd,node, arr))
			goto cleanup;
		arr[idx]=val;
		val = (Val_t)arr;
	}
	e = ecPutValue(bd, node, val);

	if (!e && ! (e=ecGet(k, &rdback, 0))) {
		if (rdback != val) {
			fprintf(stderr,"readback failed for %s (got %i == 0x%x)\n",
					k, rdback);
			e = EcError;
		}
	}
cleanup:
	if (arr) free(arr);
	return e;
}

/* change working directory */
void
ecCd(EcKey k, FILE *f)
{
EcNode 		n;
EcBoardDesc	bd=cwd.bd;

	if (EcKeyIsEmpty(k))
		return;
	if ( ! (n=ecNodeLookup(cwd.n, k, &bd)) ||
			(! EcNodeIsDir(n))) {
		fprintf(f,"dirnode not found\n");
	} else {
		cwd.n=n;
		cwd.bd=bd;
		ecPwd(f);
	}
}

/* list node contents */
EcErrStat
ecLs(EcKey k, va_list ap)
{
FILE		*f=va_arg(ap,FILE *);
int		flags=va_arg(ap,int);
EcNode	   	node=cwd.n;
EcCNode		n;
EcBoardDesc	bd=cwd.bd;
LsOptsRec	o = { f: f, flags: flags, pathRelative: cwd.n };
int		i;

	if (!f) f=stderr;

	if (!EcKeyIsEmpty(k)) {
		if (!(node=ecNodeLookup(cwd.n, k, &bd))) {
			fprintf(stderr,"node not found\n");
			return EcErrNodeNotFound;
		}
		/* check whether it's an absolute path specification */
		if (EC_DIRSEP_CHAR == *k || strchr(k, EC_BRDSEP_CHAR)) {
			/* absolute path */
			o.pathRelative = 0;
		} /* pre-initialized: path relative to cwd */
	}
	if (!node) {
		/* list all boards */
		i=0;
		while ((bd=ecGetBoardDesc(i))) {
			fprintf(f,"%s%c\n",bd->name,EC_BRDSEP_CHAR);
			i++;
		}
		if (0==i) {
			fprintf(f,"no ECDR board found; use ecAddBoard()\n");
		}
		return EcErrNodeNotFound;
	}
	n=node->cnode;
	if (DIROPS_LS_RECURSE & flags) {
		/* don't count cwd twice */
		ecNodeWalk(bd, node, printNodeInfo, (void*)&o);
	} else {
		if (EcCNodeIsDir(n)) {
			int i;
			EcNode nn;
			for (i=0,nn=node->u.entries; i<n->u.d.n->nels; i++,nn++) {
				printNodeInfo(bd, nn, (void*)&o);
			}
		} else {
			printNodeInfo(bd, node, (void*)&o);
		}
	}
	return EcErrOK;
}


#define MAXARGS		10
#define MAXARGCHARS	200

void
ecdrsh(void)
{
EcErrStat e;
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

		if (!strcmp("pwd",args[0]))
		{
			ecPwd(stderr);
		}
		else if (!strcmp("cd",args[0]))
		{
			if (ac<2) {
				fprintf(stderr,"dir arg needed\n");
				continue;
			}
			ecCd(args[1],stderr);
		}
		else if (!strcmp("ls",args[0]))
		{
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
			if (e=globbedDo(EcString2Key((myoptind < ac) ? args[myoptind] : 0), 
				  GLB_OPT_ALL,
				  ecLs,
				  stderr, flags))
				continue;
		}
#ifdef DEBUG
		else if (!strcmp("tstopt",args[0]))
		{
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
		}
#endif
		else if (!strcmp("get",args[0])) {
			Val_t		v;
			EcMenu		m;
			if (ac<2) {
				fprintf(stderr,"key arg needed\n");
				continue;
			}
			if (e=globbedDo(args[1],
					GLB_OPT_LEAVES,
					ecGetPrint,stderr)) {
				continue;
			}
		}
		else if (!strcmp("put",args[0]))
		{
			Val_t		v;
			EcMenu		m;
			char		*end;
			if (ac<3 || (v=strtoul(args[2],&end,0), !*args[2] || *end)) {
				fprintf(stderr,"key and value args needed\n");
				continue;
			}
			if ((e=globbedDo(args[1],
					 GLB_OPT_LEAVES,
					 ecPut,
					 v))) {
				continue;
			}
		}
		else if (!strcmp("prfkey",args[0]))
		{
			EcNode		l=0;
			EcFKey		fk;
			IOPtr		b=0;
			RPrintInfoRec	pi;
			if (ac<2 || 1!=sscanf(argv[1],"%i",&fk)) {
				fprintf(stderr,"fastkey id arg required\n");
				continue;
			}
			if (!(l=ecNodeLookupFast(cwd.n, fk))) {
				fprintf(stderr,"Node not found!\n");
				continue;
			}
			pi.clip=0; pi.bd=0;
			rprint(l, stderr, &pi);
			fprintf(stderr,"\n");
		}
		else if (!strcmp("quit",args[0]))
		{
			return;
		}
		else
		{
			fprintf(stderr,"unknown command '%s'\n",args[0]);
		}
	}

}
