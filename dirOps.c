/* $Id$ */

#include <stdlib.h>
#include <unistd.h>

#include "dirOps.h"

extern EcNodeRec root;
/* routines for interactive operation */
static EcNodeListRec top = {
	n:	&root,
	p:	0
};

static EcNodeList cwd=&top;

static void rprint(EcNodeList l, FILE *f)
{
	if (l->p) {
		rprint(l->p,f);
		fprintf(f,"%s.",l->n->name);
	}
}


typedef struct LsOptsRec_ {
	FILE		*f;
	unsigned long	flags;
} LsOptsRec, *LsOpts;

static void
printNodeName(EcNodeList l, IOPtr p, void *arg)
{
LsOpts	o=(LsOpts)arg;
FILE	*f=o->f ? o->f : stderr;
EcNode 	n=l->n;

	if (l->p) {
		rprint(l->p,f);
	}
	fprintf(f,n->name);
	if (EcNodeIsDir(n)) {
		fprintf(f,".");
	} else {
		if ( DIROPS_LS_VERBOSE & o->flags) {
			fprintf(f," @0x%08x: %i (ini: %i, raw: 0x%08x)",
			p, ecGetValue(n, p), n->u.r.inival, ecGetRawValue(n,p) );
		}
	}
	fprintf(f,"\n");
}

/* print current path */
void
ecPwd(FILE *f)
{
	rprint(cwd, f ? f : stderr);
}

int
ecGet(EcKey k, Val_t *valp)
{
IOPtr p=0;
EcNode n;
EcNodeList l;
	for (l=cwd; l; l=l->p)
		p+=l->n->offset;
	if (!(n=lookupEcNode(cwd->n, k, &p, 0))) {
		return -1;
	}
	if (EcNodeIsDir(n))
		return -2;
	*valp=ecGetValue(n, p);
	return 0;
}

int
ecPut(EcKey k, Val_t val)
{
IOPtr p=0;
EcNode n;
EcNodeList l;
	for (l=cwd; l; l=l->p)
		p+=l->n->offset;
	if (!(n=lookupEcNode(cwd->n, k, &p, 0))) {
		return -1;
	}
	if (EcNodeIsDir(n))
		return -2;
	ecPutValue(n, p, val);
	return 0;
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
		walkEcNode(n, printNodeName, b-(n->offset), 0, (void*)&o);
	} else {
		if (EcNodeIsDir(n)) {
			int i;
			EcNode nn;
			EcNodeListRec empty = { 0 };
			for (i=0,nn=n->u.d.n->nodes; i<n->u.d.n->nels; i++,nn++) {
				empty.n=nn;
				printNodeName(&empty, b + nn->offset, (void*)&o);
			}
		} else {
			printNodeName(l, b, (void*)&o);
		}
	}
	while (l) {
		l=freeNodeListRec(l);
	}
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
	optind=0;
	while ((ch=getopt(ac,argv,"av"))>=0) {
		switch(ch) {
			case 'v': flags|=DIROPS_LS_VERBOSE; break;
			case 'a': flags|=DIROPS_LS_RECURSE; break;
			default:  fprintf(stderr,"unknown option\n"); break;
		}
	}
	ecLs(EcString2Key((optind < ac) ? args[optind] : 0), stderr, flags);
} else
if (!strcmp("get",args[0])) {
	Val_t v;
	if (ac<2) {
		fprintf(stderr,"key arg needed\n");
		continue;
	}
	switch (ecGet(args[1],&v)) {
		default:
			fprintf(stderr,"unknown error\n");
		case -1:
			fprintf(stderr,"node not found\n");
			continue;
		case -2:
			fprintf(stderr,"node not a leaf\n");
			continue;
		case 0:
			break;
	}
	fprintf(stderr,"VALUE: %i (0x%x)\n",v,v);
} else
if (!strcmp("put",args[0])) {
	Val_t v;
	if (ac<3 || 1!=sscanf(args[2],"%i",&v)) {
		fprintf(stderr,"key and value args needed\n");
		continue;
	}
	switch (ecPut(args[1],v) || ecGet(args[1],&v)) {
		default:
			fprintf(stderr,"unknown error\n");
		case -1:
			fprintf(stderr,"node not found\n");
			continue;
		case -2:
			fprintf(stderr,"node not a leaf\n");
			continue;
		case 0:
			break;
	}
	fprintf(stderr,"read back VALUE: %i (0x%x)\n",v,v);
} else
if (!strcmp("quit",args[0])) {
	return;
} else {
	fprintf(stderr,"unknown command '%s'\n",args[0]);
}
}

}
#endif
