#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ECDR814_PRIVATE_IF

#undef  INTKEYS
#include "drvrEcdr814.h"
#include "regNodeOps.h"
#include "bitMenu.h"
#include "ecdrRegdefs.h"


EcNodeOpsRec	ecDefaultNodeOps = {
	0, /* super */
	0,
	0,
	0,
	0,
};


static EcNodeOps		nodeOps[8]={&ecDefaultNodeOps,};

EcErrStat
ecGetValue(EcNode n, IOPtr b, Val_t *vp)
{
EcNodeOps ops;
	assert(n->t < EcdrNumberOf(nodeOps) && (ops=nodeOps[n->t]));
	assert(ops->get);
	return ops->get(n,b,vp);
}

EcErrStat
ecGetRawValue(EcNode n, IOPtr b, Val_t *vp)
{
EcNodeOps ops;
	assert(n->t < EcdrNumberOf(nodeOps) && (ops=nodeOps[n->t]));
	assert(ops->getRaw);
	return ops->getRaw(n,b,vp);
}

EcErrStat
ecPutValue(EcNode n, IOPtr b, Val_t val)
{
EcNodeOps ops;
	assert(n->t < EcdrNumberOf(nodeOps) && (ops=nodeOps[n->t]));
	assert(ops->put);
	return ops->put(n,b,val);
}

EcErrStat
ecPutRawValue(EcNode n, IOPtr b, Val_t val)
{
EcNodeOps ops;
	assert(n->t < EcdrNumberOf(nodeOps) && (ops=nodeOps[n->t]));
	assert(ops->putRaw);
	return ops->putRaw(n,b,val);
}

static void
recursive_ini(EcNodeOps ops)
{
EcNodeOps super=ops->super;
	if (super) {
		if (!super->initialized)
			recursive_ini(super);
		if (!ops->get) ops->get = super->get;
		if (!ops->getRaw) ops->getRaw = super->getRaw;
		if (!ops->put) ops->put = super->put;
		if (!ops->putRaw) ops->putRaw = super->putRaw;
	}
	ops->initialized=1;
}

void
addNodeOps(EcNodeOps ops, EcNodeType t)
{
	assert( t << EcdrNumberOf(nodeOps) && !nodeOps[t]);
	recursive_ini(ops);
	nodeOps[t] = ops;
}


#ifdef INTKEYS

#define KEY_LEN			8
static inline EcNode
EcKeyLookup(EcNode n, EcKey *k)
{
int i= (((*k)&((1<<KEY_LEN)-1))-1);
*k>>=KEY_LEN;
if (i>n->u.d.n->nels)
	return 0;
return &n->u.d.n->nodes[i];
}

#else



static inline EcNode
EcKeyLookup(EcNode n, EcKey *key)
{
int i,l;
EcNode rval;
EcKey k=*key;

	*key = strchr(k,'.');
	l = *key ? (*key)++ - k : strlen(k);

	for (i=n->u.d.n->nels-1, rval=&n->u.d.n->nodes[i];
		 i>=0;
		 i--, rval--) {
		if (0==strncmp(rval->name, k, l) && strlen(rval->name)==l)
			return rval;
	}
return 0;
}

#endif

static EcNodeList	freeList=0;

#define CHUNKSZ 50
EcNodeList
allocNodeListRec(void)
{
EcNodeList	l;

if (!freeList) {
int		i;
	assert((l=(EcNodeList)malloc(CHUNKSZ*sizeof(EcNodeListRec))));
	/* initialize the free list of node headers */
	for (i=0; i<CHUNKSZ; i++,freeList=l++) {
		l->n=0;
		l->p=freeList;
	}
}
l = freeList;
freeList = l->p;
l -> p=0;
l -> n=0;
return l;
}

EcNodeList
freeNodeListRec(EcNodeList l)
{
EcNodeList rval;
	if (!l) return 0;
	rval=l->p;
	l->n=0;
	l->p=freeList;
	freeList=l;
	return rval;
}


EcNodeList
addEcNode(EcNode n, EcNodeList l)
{
EcNodeList rval;
	if ((rval=allocNodeListRec())) {
		rval->p=l;
		rval->n=n;
	}
	return rval;
}


/* lookup a key adding all the offsets
 * of the traversed nodes to *p
 * Returns: 0 if the node is not found.
 * Note: the node may not be a leaf
 */
EcNode
lookupEcNode(EcNode n, EcKey key, IOPtr *p, EcNodeList *l)
{
int i;
EcKey k=key;
EcNodeList t=l?*l:0;
EcNodeList h=t;

#if 0
	if (l) h=addEcNode(n,*l);
#endif
	
	while ( !EcKeyIsEmpty(k) && n ) {
		if (!EcNodeIsDir(n)  || !(n=EcKeyLookup(n,&k)))
			goto cleanup; /* Key not found */
		if (l) h=addEcNode(n,h);
#if defined(DEBUG) && 0
		fprintf(stderr,".%s",n->name);
#endif
		if (p) *p+=n->offset;
	}

	if (l) {
		*l=h;
	}
	return n;

cleanup:
	while (h!=t) {
		h=freeNodeListRec(h);
	}
	return 0;
}


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
#endif

static EcNodeDirRec boardDirectory={0};

EcNodeRec root={
	"/", EcDir, 0, {&boardDirectory}
	
};

#include "ecdr814RegTable.c"

void
walkEcNode(EcNode n, void (*fn)(EcNodeList,IOPtr,void*), IOPtr p, EcNodeList parent, void *fnarg)
{
	/* add ourself to the linked list of parent nodes */
	EcNodeListRec	link={p: parent, n: n};	
	/* add offset of this node */
	p += n->offset;
	if (EcNodeIsDir(n)) {
		int i;
		EcNode nn;
		for (i=0, nn=n->u.d.n->nodes; i < n->u.d.n->nels; i++, nn++)
			walkEcNode(nn, fn, p, &link, fnarg);
	} else {
		/* call fn for leaves */
		fn(&link, p, fnarg);
	}
}

static void
putIniVal(EcNodeList l, IOPtr p, void* arg)
{
EcNode n=l->n;
if ( ! (n->u.r.flags & (EcFlgReadOnly|EcFlgNoInit)))
	ecPutValue(n, p, n->u.r.inival);
}

#ifdef DEBUG
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
	ecGetValue(l->n,p,&v);
	ecGetRawValue(l->n,p,&rv);
	printf(" 0x%08x: %i (ini: %i, raw: 0x%08x)\n",
		p, v, l->n->u.r.inival, rv );
}
#endif

/* try to locate an  ECDR814 board at
 * base address 'b'.
 * On success, initialize it and create
 * a directory node in the root directory.
 * RETURNS: 0 on success, -1 on failure
 *
 * NOTE: the name string will _not_ be
 * copied but will be 'owned' by the driver
 * on success.
 */


int
ecAddBoard(char *name, IOPtr b)
{
int		rval=-1;
EcNodeDir	bd=&boardDirectory;
EcNode		n;

	/* ecdrRegdefs.h makes the assumption that
	 * the base address is aligned to 0x1fff
	 */
	assert( ! ((unsigned long)b & ECDR_AD6620_ALIGNMENT) );
	/* try to detect board */
	/* initialize */
	/* raw initialization sets pretty much everything to zero */
	walkEcNode( &ecdr814RawBoard, putIniVal, b, 0, 0);
	/* now set individual bits as defined in the board table */
	walkEcNode( &ecdr814Board, putIniVal, b, 0, 0);
	/* TODO other initialization (VME, RW...) */

	/* on success, create directory entry */
	assert(bd->nodes = (EcNode) realloc(bd->nodes,
						sizeof((*(bd->nodes))) * (++bd->nels)));

	n=bd->nodes+(bd->nels-1);
	n->name=name;
	n->offset=(unsigned long)b;
	n->t = EcDir;
	n->u.d.n = ecdr814Board.u.d.n;
	rval=0;

cleanup:
	return rval;

}

void
drvrEcdr814Init(void)
{
	/* initialize tiny virtual fn tables */
	initRegNodeOps();
	/* register the menus */
	walkEcNode( &ecdr814Board, ecRegisterMenu, 0, 0, 0);
}

static char *ecErrNames[] = {
	"no error",
	"unknown error",
	"read only register",
	"no Memory",
	"device not found",
	"directory node not found",
	"node is not a leaf entry",
	"AD6620 must be in reset state for writing",
	"value out of range",
};

char *
ecStrError(EcErrStat e)
{
	e=-e;
	if (e >= EcdrNumberOf(ecErrNames))
		e=EcError;
	return ecErrNames[e];
}


#if !defined(__vxworks) && !defined(__rtems)

int
main(int argc, char ** argv)
{
IOPtr b=0;
EcKey		key;
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

#ifdef DIRSHELL
dirshell();
#endif

if (argc<2) return 1;
#ifdef INTKEYS
sscanf(argv[1],"%i", &key);
#else
key = argv[1];
#endif
fprintf(stderr,"looking for 0x%08x: ",key);
l=0;
n=lookupEcNode(&ecdr814Board,key,&b,&l);
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

#endif
