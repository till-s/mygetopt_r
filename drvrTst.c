#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "drvrEcdr814.h"
#include "ecdrRegdefs.h"
#include "ecFastKeys.h"

#ifdef DEBUG
static EcNodeRec ad[]={
	{ "reg0", EcAD6620Reg, 0x0, {r : {3, 4, 0, 1} } },
	{ "reg1", EcAD6620Reg, 0x10, {r : {14, 18, 0, 5} } },
	{ "reg2", EcAD6620Reg, 0x10, {r : {3, 5, 0, 2} }  },
	{ "reg3", EcAD6620Reg, 0x20, {r : {3, 5, 0, 3} } },
};

static EcNodeDirRec adnode={
	EcdrNumberOf(ad), ad
};

static EcNodeRec board[]={
	{ "breg0", EcReg, 0x0, {r: {7, 10, 0, 6} } },
	{ "breg1", EcReg, 0x4, {r: {3, 5, 0,  1} } },
	{ "ad0",   EcDir, 0x100,{ &adnode } },
	{ "ad1",   EcDir, 0x200,{ &adnode } },
};

static EcNodeDirRec boardnode={
	EcdrNumberOf(board), board
};

static void
rprintNode(EcNodeList l)
{
	if (l->p) {
		rprintNode(l->p);
		printf(".%s",l->n->name);
	}
}
static void
printNodeName(EcNodeList l, IOPtr p, void* arg)
{
	Val_t v,rv;
	rprintNode(l);
	ecGetValue(l->n, ecNodeList2FKey(l), p,&v);
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
EcNode	n;
EcNodeList l=0;
char *	buf=malloc(0x100000 +  ECDR_AD6620_ALIGNMENT);
char *tstdev = (char*)((((unsigned long)buf)+ECDR_AD6620_ALIGNMENT)&~ECDR_AD6620_ALIGNMENT);

memset(tstdev,0,0x1000);

drvrEcdr814Init();


printf("node size: %i\n",sizeof(EcNodeRec));

//root.offset=(unsigned long)tstdev;
#ifdef DEBUG
//walkEcNode( &root, putIniVal, 0, 0, 0);
//walkEcNode( &root, printNodeName, 0, 0, 0);
//walkEcNode( &ecdr814RawBoard, printNodeName, &b, 0, 0);
ecAddBoard("B0",tstdev);
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
n=lookupEcNodeFast(&ecdr814Board,key,&b,&l);
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
