#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "drvrEcdr814.h"
#include "ecdrRegdefs.h"
#include "ecFastKeys.h"

#if defined(DEBUG) && 0
static EcCNodeRec ad[]={
	{ "reg0", EcAD6620Reg, 0x0, {r : {3, 4, 0, 1} } },
	{ "reg1", EcAD6620Reg, 0x10, {r : {14, 18, 0, 5} } },
	{ "reg2", EcAD6620Reg, 0x10, {r : {3, 5, 0, 2} }  },
	{ "reg3", EcAD6620Reg, 0x20, {r : {3, 5, 0, 3} } },
};

static EcCNodeDirRec adnode={
	EcdrNumberOf(ad), ad
};

static EcCNodeRec board[]={
	{ "breg0", EcReg, 0x0, {r: {7, 10, 0, 6} } },
	{ "breg1", EcReg, 0x4, {r: {3, 5, 0,  1} } },
	{ "ad0",   EcDir, 0x100,{ &adnode } },
	{ "ad1",   EcDir, 0x200,{ &adnode } },
};

static EcCNodeDirRec boardnode={
	EcdrNumberOf(board), board
};

static void
rprintNode(EcCNodeList l)
{
	if (l->p) {
		rprintNode(l->p);
		printf(".%s",l->n->name);
	}
}
static void
printNodeName(EcCNodeList l, IOPtr p, void* arg)
{
	Val_t v,rv;
	rprintNode(l);
	ecGetValue(l->n, p,&v);
	ecGetRawValue(l->n,p,&rv);
	printf(" 0x%08x: %i (ini: %i, raw: 0x%08x)\n",
		p, v, l->n->u.r.inival, rv );
}
#endif


int
main(int argc, char ** argv)
{
IOPtr b=0;
EcFKey		key;
EcCNode	n;
EcCNodeList l=0;
char *	buf=malloc(0x100000 +  ECDR_BRDREG_ALIGNMENT);
char *tstdev = (char*)((((unsigned long)buf)+ECDR_BRDREG_ALIGNMENT)&~ECDR_BRDREG_ALIGNMENT);

memset(tstdev,0,0x1000);

drvrEcdr814Init();


printf("node size: %i\n",sizeof(EcCNodeRec));

//root.offset=(unsigned long)tstdev;
#ifdef DEBUG
//ecCNodeWalk( &root, putIniVal, 0, 0, 0);
//ecCNodeWalk( &root, printNodeName, 0, 0, 0);
//ecCNodeWalk( &ecdr814RawCInfo, printNodeName, &b, 0, 0);
ecAddBoard("B0",tstdev,0);
#endif

if (argc<2)
#ifdef DIRSHELL
{ dirshell(); exit(0); }
#else
return 1;
#endif

sscanf(argv[1],"%i", &key);
fprintf(stderr,"looking for 0x%08x: ",key);
l=0;
n=lookupEcCNodeFast(&ecdr814CInfo,key,&b,&l);
printf("\nprinting reverse path:\n  ");
while (l) {
	printf("- %s",l->n->name);
	l=freeNodeListRec(l);
}
printf("\n");

if (n) {
	fprintf(stderr," is type %i @0x%08x",n->t,(unsigned long)b);
} else {
	fprintf(stderr,"...NOT FOUND");
}
fprintf(stderr,"\n");
}
