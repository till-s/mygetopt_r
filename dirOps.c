/* $Id$ */

#include <stdlib.h>

#include "dirOps.h"

extern EcNodeRec root;
/* routines for interactive operation */
static EcNodeListRec top = {
	n:	&root,
	p:	0
};

static EcNodeList cwd=&top;

/* print current path */
static void rprint(EcNodeList l, FILE *f)
{
	if (l->p) {
		rprint(l->p,f);
		fprintf(f,"%s.",l->n->name);
	}
}

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
ecLs(EcKey k, FILE *f)
{
EcNodeList l=0;
EcNode	   n;
	if (!f) f=stderr;
	if (EcKeyIsEmpty(k)) {
		n=cwd->n;
	} else {
	if (!(n=lookupEcNode(cwd->n, k, 0, &l))) {
		fprintf(stderr,"node not found\n");
		return;
	}
	}
	if (EcNodeIsDir(n)) {
		int i;
		EcNode nn;
		for (i=0,nn=n->u.d.n->nodes; i<n->u.d.n->nels; i++,nn++) {
			fprintf(f,"%s%s\n",
				nn->name,
				EcNodeIsDir(nn) ? ".": "");
		}
	} else {
		if (l->p) rprint(l->p,f);
		fprintf(f,n->name);
	}
	while (l) {
		l=freeNodeListRec(l);
	}
}

#ifdef DIRSHELL

void
dirshell(void)
{
int ac=0;
int ch,ai;

char args[10][200];
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
		if (ai<200-1) args[ac][ai++]=ch;
		ch=getchar();
	}
	
	args[ac][ai]=0;
	ai=0;

	if (++ac>=10) {
		while ('\n'!=ch) ch=getchar();
		break;
	}
}

if (!ac) continue;

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
	ecLs(ac>=2 ? args[1] : 0, stderr);
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
