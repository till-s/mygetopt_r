/* $Id$ */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if !defined(__vxworks)
#include <unistd.h>
#endif
#include <stdarg.h>

#include "dirOps.h"
#include "bitMenu.h"
#include "ecFastKeys.h"


#define EC_IDXO_CHAR	'['
#define EC_IDXC_CHAR	']'

#define NOT_EOS(s) (*(s) && EC_DIRSEP_CHAR != *(s) && EC_BRDSEP_CHAR != *(s) && '[' != *(s))


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

/* tiny 'environment' */

typedef struct EcdrShEnvRec_ {
	EcNode		n;
	EcBoardDesc 	bd;
} EcdrShEnvRec, *EcdrShEnv;

static EcdrShEnvRec ecdrenvs[5] = {{0}};

static EcdrShEnv cwd=0;



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
int n=0;
	if ( l == info->clip ) return;

	if (l->parent) {
		rprint(l->parent,f,info);
		fputc(EC_DIRSEP_CHAR,f); n++;
		n += fprintf(f,"%s",l->cnode->name);
	} else {
		/* print board name */
		n += info->bd ? fprintf(f,"%s%c",info->bd->name,EC_BRDSEP_CHAR) : 0;
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
		pad++;
	
		pad += pi.pos;
	}
	pad+=fprintf(f,"%s",n->name);

	fk=ecNode2FKey(l);

	if (EcCNodeIsDir(n)) {
		fputc(EC_DIRSEP_CHAR,f);
		pad++;
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
			int nels=1;
			EcErrStat e;
			if ( EcCNodeIsArray(n) ) {
				nels = n->u.r.u.a.len;
				pv = memp = (Val_t*) malloc(sizeof(Val_t)*nels);
			}
			e=(ecGetValue(bd, l, pv) || (1==nels && ecGetRawValue(bd, l, &rv)));
			if (e) {
				fprintf(stderr,"ERROR: %s\n",ecStrError(e));
			} else {
				IOPtr p = bd->base + l->u.offset;
				if (m)
					fprintf(f," @0x%08lx: %s (ini: %s, raw: 0x%08lx)",
						(unsigned long)p,
						m->items[v].name,
						m->items[n->u.r.inival].name,
						(unsigned long)rv);
				else {
					if (nels > 1) {
						fprintf(f,"   array %c%i%c @0x%08lx\n",
							EC_IDXO_CHAR, nels, EC_IDXC_CHAR,
							(unsigned long)p);
						while (nels--)
							fprintf(f,"    %8li\n", *(pv++));
					} else {
						fprintf(f," @0x%08lx: %li (ini: %li, raw: 0x%08lx)",
							(unsigned long)p,
							v,
							n->u.r.inival,
							rv );
					}
				}
			}
			if (memp) free(memp);
		}
	}
	while (pad++<35) fputc(' ',f);
	if ( DIROPS_LS_FKEYINFO & o->flags ) {
		fprintf(f, "  [FKEY: 0x%08lx]", (unsigned long)fk);
	}
	fprintf(f,"\n");
	if ( (DIROPS_LS_SHOWMENU & o->flags) && m ) {
		int i;
		for (i=0; i<m->nels; i++)
			fprintf(f, "   - %2i: %s\n",i,m->items[i].name);
	}
}

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
unsigned char	i, *ptr;
int		l;
EcErrStat	rval;
	if (!n || !EcNodeIsDir(n)) return EcErrNodeNotFound;
	/* check for .. */
	if (!strncmp(globPat, EC_UPDIR_NAME, EC_UPDIR_NAME_LEN)) {
		ptr = globPat+EC_UPDIR_NAME_LEN;
		if (EC_DIRSEP_CHAR == *ptr) {
			/* ../ detected */
			ptr++;
			l=sprintf(append,"%s%c",EC_UPDIR_NAME,EC_DIRSEP_CHAR);
			return rglob(n->parent, append+l, ptr, p);
		} else if ( ! *ptr) {
			/* end of pattern reached; invoke fn and return */
			sprintf(append,EC_UPDIR_NAME);
			rval = EcErrOK;
			if ((EcNodeIsDir(n->parent) && (GLB_OPT_DIRS & p->options)) ||
			    (!EcNodeIsDir(n->parent) && (GLB_OPT_LEAVES & p->options)) ) {
				p->count++;
				rval = (*p->fn)(p->expansion, p->fnargs);
			}
			return rval;

		} /* else, not a valid .. pattern, continue normally */
	}
	for (i=0; i<n->cnode->u.d.n->nels; i++) {
		/* try to match a path element */
		if ((ptr=globMatch(n->cnode->u.d.n->nodes[i].name, globPat))) {
			/* match */
			if (*ptr && EC_IDXO_CHAR != *ptr) ptr++; /* skip dirsep etc. char */
			if (*ptr && EC_IDXO_CHAR != *ptr) {
				/* end of pattern not reached yet */
				if (EcNodeIsDir(&n->u.entries[i])) {
					/* append the name */
					l=sprintf(append,"%s%c",
						n->cnode->u.d.n->nodes[i].name,
						EC_DIRSEP_CHAR);
					if ((rval=rglob(&n->u.entries[i], append+l, ptr, p)))
						return rval;
				} /* else just skip it */
			} else {
				sprintf(append,"%s%s",
					n->cnode->u.d.n->nodes[i].name,
					(*ptr) ? ptr : (unsigned char*)"" /* append array index */
					);
				/* end of pattern reached, invoke fn */
				rval = EcError;
				if (EcNodeIsDir(&n->u.entries[i])) {
				       if (GLB_OPT_DIRS & p->options)
						rval = EcErrOK;
				} else if (GLB_OPT_LEAVES & p->options)
					rval = EcErrOK;

				if ( EcErrOK==rval ) {
					p->count++;
				       	if ((rval = (*p->fn)(p->expansion, p->fnargs)))
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
	a = parms.expansion;

	/* let's see if the globPat is absolute or relative */
	if (strchr(globPat,EC_BRDSEP_CHAR)) {
		int i;
		EcBoardDesc bd;
		/* board specified, loop over all boards */
		for (i=0; (bd=ecGetBoardDesc(i)); i++) {
			if ((ptr=globMatch(bd->name, globPat)) && EC_BRDSEP_CHAR==*ptr) {
				ptr++;
				a = parms.expansion;
				a+=sprintf(a,"%s%c%c",bd->name,EC_BRDSEP_CHAR,EC_DIRSEP_CHAR);
				if (EC_DIRSEP_CHAR == *ptr)
					ptr++;
				if ((rval=rglob(bd->root, a, ptr, &parms)))
					goto cleanup;
			}
		}
	} else {
		if (EC_DIRSEP_CHAR==*globPat) {
			/* absolute */
			if (!cwd || !cwd->bd) {
				fprintf(stderr,"<no path, use 'ls/cd'>\n");
				goto cleanup;
			}
			*(a++)='/'; *a=0;
			rval=rglob(cwd->bd->root, a, globPat+1, &parms);
		} else {
			/* relative */
			rval=rglob(cwd->n, a, globPat, &parms);
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
	if (!f) f=stderr;

	if (!cwd->bd) {
		fprintf(stderr,"<no path, use 'ls/cd'>\n");
	} else {
		RPrintInfoRec pi;
		pi.clip=0;
		pi.bd  =cwd->bd;
		pi.pos =0;
		rprint(cwd->n, f, &pi);
		fputc('\n',f);
	}
}

static int
getArrayIndex(char **s)
{
int rval;
char *chpt=*s, *endp;
	if (!(chpt=strchr(*s,EC_IDXO_CHAR))) return -1;
	rval=strtoul(chpt+1,&endp,0);
	if (EC_IDXC_CHAR!=*endp) return -1;
	*s = chpt;
	return rval;
}

EcErrStat
ecGet(EcKey k, Val_t *valp, EcMenu *mp)
{
EcErrStat	e=EcErrOK;
EcCNode		n;
EcNode		node;
int		idx;
char		buf[100],*chpt;
Val_t		*arr=0;
EcBoardDesc	bd=cwd->bd;

	if (mp) *mp=0;

	strcpy(buf,k);
	chpt=buf;
	if ((idx=getArrayIndex(&chpt))>=0) {
		*chpt=0; /* strip [idx] */
	}

	if (!(node=ecNodeLookup(cwd->n, buf, &bd))) {
		return EcErrNodeNotFound;
	}
	n=node->cnode;

	if (EcCNodeIsDir(n)) {
		e = EcErrNotLeafNode;
		goto cleanup;
	}
	if ( ((idx>=0) ^ (0 != EcCNodeIsArray(n))) || idx >= n->u.r.u.a.len  ) {
		e = EcErrInvalidIndex;
		goto cleanup;
	}
	if (EcCNodeIsArray(n)) {
		arr = (Val_t*)malloc(sizeof(Val_t)*n->u.r.u.a.len);
		if ((e=ecGetValue(bd, node, arr)))
			goto cleanup;
		*valp = arr[idx];
	} else {
		if ((e=ecGetValue(bd, node, valp)))
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
		fprintf(stderr,"Unable to get '%s' <%s>\n",
				k,
				ecStrError(rval));
		return rval;
	}
	fprintf(f,"%s: ", k);
	if (m) {
		fprintf(f,"%s\n", m->items[v].name);
	} else {
		fprintf(f,"%li (0x%lx)\n",v,v);
	}
	return EcErrOK;
}

EcErrStat
ecPut(EcKey k, va_list ap)
{
Val_t		val=va_arg(ap,Val_t), rdback, oval;
EcNode		node;
EcCNode		n;
EcErrStat	e=EcErrOK;
int		idx;
char		buf[100],*chpt;
Val_t		*arr=0;
EcBoardDesc	bd=cwd->bd;

	oval = val;
	strcpy(buf,k);
	chpt=buf;
	if ((idx=getArrayIndex(&chpt))>=0) {
		*chpt=0; /* strip [idx] */
	}

	if (!(node=ecNodeLookup(cwd->n, buf, &bd))) {
		return EcErrNodeNotFound;
	}
	n=node->cnode;
	if (EcCNodeIsDir(n)) {
		e = EcErrNotLeafNode;
		goto cleanup;
	}
	if ( ((idx>=0) ^ (0 != EcCNodeIsArray(n))) || idx >= n->u.r.u.a.len  ) {
		e = EcErrInvalidIndex;
		goto cleanup;
	}
	if (EcCNodeIsArray(n)) {
		arr = (Val_t*)malloc(sizeof(Val_t)*n->u.r.u.a.len);
		if ((e=ecGetValue(bd,node, arr)))
			goto cleanup;
		arr[idx]=val;
		oval = (Val_t)arr;
	}
	e = ecPutValue(bd, node, oval);

	if (!e && ! (e=ecGet(k, &rdback, 0))) {
		if (rdback != val) {
			fprintf(stderr,"readback failed for %s (got %li == 0x%lx)\n",
					k, rdback, rdback);
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
EcBoardDesc	bd=cwd->bd;

	if (EcKeyIsEmpty(k))
		return;
	if ( ! (n=ecNodeLookup(cwd->n, k, &bd)) ||
			(! EcNodeIsDir(n))) {
		fprintf(stderr,"dirnode not found\n");
	} else {
		cwd->n=n;
		cwd->bd=bd;
		ecPwd(f);
	}
}

/* list node contents */
EcErrStat
ecLs(EcKey k, va_list ap)
{
FILE		*f=va_arg(ap,FILE *);
int		flags=va_arg(ap,int);
EcNode	   	node=cwd->n;
EcCNode		n;
EcBoardDesc	bd=cwd->bd;
LsOptsRec	o = { f: f, flags: flags, pathRelative: cwd->n };
int		i;

	if (!f) f=stderr;

	if (!EcKeyIsEmpty(k)) {
		if (!(node=ecNodeLookup(cwd->n, k, &bd))) {
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
			fprintf(stderr,"no ECDR board found; use ecAddBoard()\n");
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

#define B EC_BRDSEP_CHAR
#define D EC_DIRSEP_CHAR
#define W EC_GLOB_CHAR
#define O EC_IDXO_CHAR
#define C EC_IDXC_CHAR
EcErrStat
ecHelp(FILE *f)
{
fprintf(f,"ECDR 814 Driver version $Name$\n\n");
fprintf(f,"Available Commands:\n\n");

fprintf(f,"    <  <filename>                -- read commands from file\n\n");

fprintf(f,"    cd <path>                    -- change working directory to <path>\n");
fprintf(f,"                                    e.g. chdir to the first receiver of channel 0 on board 0\n\n");

fprintf(f,"				      cd B0%c%c01%c0A\n\n",B,D,D);

fprintf(f,"    get <wpath>[<idx>]           -- read and print value from <wpath>; if <wpath addresses\n");
fprintf(f,"                                    an array, element <idx> must be specified.\n");
fprintf(f,"                                    e.g. read the third RCF coefficient from all receivers\n");
fprintf(f,"                                    on board 0\n\n");

fprintf(f,"				      get B0%c%c%c%c%c%crcfCoeffs%c3%c\n\n",B,D,W,D,W,D,O,C);

fprintf(f,"    help                         -- print this info\n\n");

fprintf(f,"    ls [<opts>] [<wpath>]        -- list directory info, options are\n");
fprintf(f,"                                    -v :print details (hw address, current value,\n");
fprintf(f,"                                        init value, raw register value)\n");
fprintf(f,"                                    -a :recurse into directories\n");
fprintf(f,"                                    -m :list menu items\n");
fprintf(f,"				    -k :print fastKey path\n\n");

fprintf(f,"    prfkey <path>                -- print fastKey equivalent of <path>\n\n");

fprintf(f,"    put <wpath>[<idx>] <value>   -- write <value> to <wpath>; if <wpath> addresses\n");
fprintf(f,"                                    an array, element <idx> must be specified.\n");
fprintf(f,"				    e.g. enable gain on all channels on all boards:\n\n");

fprintf(f,"				      put %c%c%c%c%c%cgainEna 1\n\n",W,B,D,W,D,W);

fprintf(f,"    pwd                          -- print current working directory\n\n");

fprintf(f,"    quit                         -- leave the command interpreter\n\n");

fprintf(f,"    <path>                       -- relative or absolute path specifier:\n");
fprintf(f,"                                    <path> = [ [<board name>, '%c',] '%c' ] { <name>, '%c' }\n",B,D,D);
fprintf(f,"                                    e.g.\n\n");

fprintf(f,"                                      01%cclockSame,  B0%c%crdbackMode, %c01%cC0.gainEna\n\n",D,B,D,D,D);

fprintf(f,"    <wpath>                      -- path with 'wildcard' names (wildcard: '%c')\n",W);
fprintf(f,"                                    e.g.\n\n");

fprintf(f,"                                      %c%c%c%cgainEna 1\n\n",D,W,D,W);

fprintf(f,"    <idx>                        -- array index specifier: '%c', <element nr>, '%c'\n",O,C);
fprintf(f,"                                    e.g:  rcfCoeffs%c3%c\n",O,C);
	return EcErrOK;
}

#undef B
#undef D
#undef W
#undef O
#undef C


#define MAXARGS		10
#define MAXARGCHARS	200

void
ecdrsh(FILE *fin, FILE *fout, FILE *ferr)
{
EcErrStat e;
int ac=0;
int ch,ai;
char *argv[MAXARGS+1];

char args[MAXARGS][MAXARGCHARS];

	/* initialize environment */
	if (!cwd) {
		cwd = ecdrenvs;
		memset(cwd, 0, sizeof(*cwd));
	}
	if (!fin)  fin  = stdin;
	if (!fout) fout = stdout;
	if (!ferr) ferr = stderr;

	for (ac=0; ac< MAXARGS; ac++)
		argv[ac]=&args[ac][0];

	ch = 0;
	while (EOF != ch) {
		fprintf(ferr,"#");
		fflush(ferr);
		ch=getc(fin);
		ac=ai=0;
		while (1) {
			while ('\t'==ch || ' '==ch)
				(ch=getc(fin));
			if ('\n'==ch || EOF==ch) break;
			while ('\t'!=ch && ' '!=ch && '\n'!=ch && EOF!=ch) {
				if (ai<MAXARGCHARS-1) args[ac][ai++]=ch;
				ch=getc(fin);
			}

			args[ac][ai]=0;
			argv[ac]=&args[ac][0];
			ai=0;

			if (++ac>=10) {
				while ('\n'!=ch && EOF!=ch) ch=getc(fin);
				break;
			}
		}

		if (!ac) continue;

		argv[ac]=0;

		if (!strcmp("pwd",args[0]))
		{
			ecPwd(fout);
		}
		else if (!strcmp("cd",args[0]))
		{
			if (ac<2) {
				fprintf(ferr,"dir arg needed\n");
				continue;
			}
			ecCd(args[1],fout);
		}
		else if (!strcmp("ls",args[0]))
		{
			unsigned long flags=0;
			int mch;
			myoptind=1;
			while ((mch=mygetopt(ac,argv,"avmk"))>=0) {
				switch(mch) {
					case 'v': flags|=DIROPS_LS_VERBOSE; break;
					case 'a': flags|=DIROPS_LS_RECURSE; break;
					case 'm': flags|=DIROPS_LS_SHOWMENU; break;
					case 'k': flags|=DIROPS_LS_FKEYINFO; break;
					default:  fprintf(ferr,"unknown option\n"); break;
				}
			}
			if ((e=globbedDo(EcString2Key((myoptind < ac) ? args[myoptind] : 0), 
				  GLB_OPT_ALL,
				  ecLs,
				  fout, flags)))
				continue;
		}
#ifdef DEBUG
		else if (!strcmp("tstopt",args[0]))
		{
			int mch;
			myoptind=0;
			while ((mch=mygetopt(ac,argv,"avk"))) {
				int i;
				fprintf(ferr,"found '%c', optind is %i, argv now:\n",
						mch, myoptind);
				for (i=0; i<ac; i++) {
					fprintf(ferr,"%s | ",argv[i]);
				}
				fprintf(ferr,"\n");
				if (-1==mch) break;
			}
		}
#endif
		else if (!strcmp("get",args[0])) {
			if (ac<2) {
				fprintf(ferr,"key arg needed\n");
				continue;
			}
			if ((e=globbedDo(args[1],
					GLB_OPT_LEAVES,
					ecGetPrint,fout))) {
				continue;
			}
		}
		else if (!strcmp("put",args[0]))
		{
			Val_t		v;
			char		*end;
			if (ac<3 || (v=strtoul(args[2],&end,0), !*args[2] || *end)) {
				fprintf(ferr,"key and value args needed\n");
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
			RPrintInfoRec	pi;
			if (ac<2 || 1!=sscanf(argv[1],"%li",&fk)) {
				fprintf(ferr,"fastkey id arg required\n");
				continue;
			}
			if (!(l=ecNodeLookupFast(cwd->n, fk))) {
				fprintf(ferr,"Node not found!\n");
				continue;
			}
			pi.clip=0; pi.bd=0;
			rprint(l, fout, &pi);
			fprintf(fout,"\n");
		}
		else if (!strcmp("<",args[0]))
		{
			FILE *script;
			if (ac<2) {
				fprintf(ferr,"filename missing\n");
				continue;
			}
			if (!(script=fopen(args[1],"r"))) {
				fprintf(ferr,"unable to open script '%s': %s\n",
						args[1], strerror(errno));
				continue;
			}
			/* push new environment */
			if (++cwd - ecdrenvs >= EcdrNumberOf(ecdrenvs)) {
				fprintf(ferr,"ecdrsh ERROR: too many nested calls...\n");
				cwd--;
				continue;
			}
			*cwd = *(cwd-1); /* copy environment of caller */

			ecdrsh(script,fout,ferr);
			fprintf(ferr,"\n");

			/* pop environment layer */
			if (--cwd < ecdrenvs) cwd=0;
		}
		else if (!strcmp("help",args[0]))
		{
			ecHelp(stderr);
		}
		else if (!strcmp("quit",args[0]))
		{
			break;
		}
		else
		{
			fprintf(ferr,"unknown command '%s'\n",args[0]);
		}
	}
}
