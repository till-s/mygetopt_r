#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "drvrEcdr814.h"

static inline EcNode
EcFKeyLookup(EcNode n, EcFKey *k)
{
int i= (((*k)&((1<<FKEY_LEN)-1))-1);
*k>>=FKEY_LEN;
if (i>n->u.d.n->nels || i<0)
	return EMPTY_FKEY;
return &n->u.d.n->nodes[i];
}

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

static EcNodeList	freeList=0;

#define ALLOCDBG
#define CHUNKSZ 50
EcNodeList
allocNodeListRec(void)
{
EcNodeList	l;
#ifdef ALLOCDBG
static int	once=1;
#endif

if (!freeList) {
int		i;
	assert((l=(EcNodeList)malloc(CHUNKSZ*sizeof(EcNodeListRec))));
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

EcNode
lookupEcNodeFast(EcNode n, EcFKey key, IOPtr *p, EcNodeList *l)
{
int i;
EcFKey k=key;
EcNodeList t=l?*l:0;
EcNodeList h=t;

#if 0
	if (l) h=addEcNode(n,*l);
#endif
	
	while ( !EcKeyIsEmpty(k) && n ) {
		if (!EcNodeIsDir(n)  || !(n=EcFKeyLookup(n,&k)))
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
