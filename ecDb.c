#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "drvrEcdr814.h"

#include "ecdr814RegTable.c"

void
ecCNodeWalk(EcCNode n, EcCNodeWalkFn fn, IOPtr p, EcCNodeList parent, void *fnarg)
{
/* add ourself to the linked list of parent nodes */
EcCNodeListRec	link={p: parent, n: n};	

	/* add offset of this node */
	p += n->offset;

	/* call fn */
	fn(&link, p, fnarg);

	if (EcCNodeIsDir(n)) {
		int i;
		EcCNode nn;
		for (i=0, nn=n->u.d.n->nodes; i < n->u.d.n->nels; i++, nn++)
			ecCNodeWalk(nn, fn, p, &link, fnarg);
	}
}

void
ecNodeWalk(EcBoardDesc bd, EcNode n, EcNodeWalkFn fn, void *arg)
{
	/* call fn */
	fn(bd, n, arg);

	if (EcNodeIsDir(n)) {
		int i,nels=n->cnode->u.d.n->nels;
		EcNode nn;
		for (i=0, nn=n->u.entries; i < nels; i++, nn++)
			ecNodeWalk(bd, nn, fn, arg);
	}
}

/* create the directory of all instances */

static void
countNodes(EcCNodeList l, IOPtr b, void *pcnt)
{
(*(unsigned long*)pcnt)++;
}

static void
initEcNodes(EcNode thisNode, unsigned long offset, EcNode *pfree)
{
EcNode	n;
EcNode	end;
EcCNode	cn;
int	i;

	/* 'thisNode' is a directory; create nodes
	 * for all of its entries and initialize them
	 */
	
	/* get free nodes */
	n = (*pfree);
	(*pfree)+=thisNode->cnode->u.d.n->nels;
	/* store actual value; pfree is modified by
	 * recursive calls to initEcNodes...
	 */
	end = *pfree;

	/* link new list of entries into this directory */
	thisNode->u.entries = n;

	/* get CNode for this directory */
	cn = thisNode->cnode->u.d.n->nodes;
	/* add offset */
	offset += thisNode->cnode->offset;

	/* initialize the entries */
	while (n < end) {
		n->cnode = cn;
		n->parent = thisNode;
		if (EcCNodeIsDir(cn)) {
			n->u.entries = 0;
			/* recursively create directory entries */
			initEcNodes(n, offset, pfree);
		} else {
			/* directory offset + leaf node offset */
			n->u.offset = offset + cn->offset;
		}
		n++;
		cn++;
	}
}

EcNode
ecCreateDirectory(EcCNode classRootNode)
{
unsigned long	nNodes;
EcNode		rval, free;

	/* count the number of nodes */
	nNodes=0;
	ecCNodeWalk(classRootNode, countNodes, 0, 0, &nNodes);
	/* allocate space */
	rval=free=(EcNode)malloc(sizeof(*rval) * nNodes);
	if (!rval)
		return 0;
	/* create the directory structure */
	/* create root node */
	free++;
	rval->cnode = classRootNode;
	rval->u.entries = 0;
	rval->parent = 0;
	/* recursively create all nodes */
	initEcNodes(rval, 0, &free);
	return rval;
}

/* Node list stuff */

static EcCNodeList	freeList=0;

#define ALLOCDBG
#define CHUNKSZ 50
EcCNodeList
allocNodeListRec(void)
{
EcCNodeList	l;
#ifdef ALLOCDBG
static int	once=1;
#endif

if (!freeList) {
int		i;
	assert((l=(EcCNodeList)malloc(CHUNKSZ*sizeof(EcCNodeListRec))));
	/* initialize the free list of node headers */
	for (i=0; i<CHUNKSZ; i++,freeList=l++) {
		l->n=0;
		l->p=freeList;
	}
#ifdef ALLOCDBG
	assert( once-- > 0 );
#endif
}
l = freeList;
freeList = l->p;
l -> p=0;
l -> n=0;
return l;
}

EcCNodeList
freeNodeListRec(EcCNodeList l)
{
EcCNodeList rval;
	if (!l) return 0;
	rval=l->p;
	l->n=0;
	l->p=freeList;
	freeList=l;
	return rval;
}


EcCNodeList
addEcCNode(EcCNode n, EcCNodeList l)
{
EcCNodeList rval;
	if ((rval=allocNodeListRec())) {
		rval->p=l;
		rval->n=n;
	}
	return rval;
}



