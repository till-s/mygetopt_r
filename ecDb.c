#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "drvrEcdr814.h"

static inline EcCNode
lookupEcFKey(EcCNode n, EcFKey *k)
{
int i= (((*k)&((1<<FKEY_LEN)-1))-1);

	*k>>=FKEY_LEN;
	if ( i<0
#ifdef FKEYDEBUG
		|| i > n->node->u.d.n->nels
#endif
	)
		return EMPTY_FKEY;
	return &n->u.d.n->nodes[i];
}

static inline EcNode
ecFKeyLookup(EcNode n, EcFKey *k)
{
int i= (((*k)&((1<<FKEY_LEN)-1))-1);

	*k>>=FKEY_LEN;

	if (UPDIR_FKEY-1 == i)
		return n->parent;
	if ( i<0
#ifdef FKEYDEBUG
		|| i > n->node->u.d.n->nels
#endif
	)
		return EMPTY_FKEY;
	return &n->u.dir[i];
}

static inline EcCNode
lookupEcKey(EcCNode n, EcKey *key)
{
int i,l;
EcCNode rval;
EcKey k=*key;

#ifdef KEYDEBUG
	fprintf(stderr,"looking up %s in %s\n",k,n->name);
#endif
	*key = strchr(k,EC_DIRSEP_CHAR);
	l = *key ? (*key)++ - k : strlen(k);

	for (i=n->u.d.n->nels-1, rval=&n->u.d.n->nodes[i];
		 i>=0;
		 i--, rval--) {
#ifdef KEYDEBUG
	fprintf(stderr,"comparing key to %s\n",rval->name);
#endif
		if (0==strncmp(rval->name, k, l) && strlen(rval->name)==l)
			return rval;
	}
return 0;
}

static inline EcNode
ecKeyLookup(EcNode n, EcKey *key)
{
int i,l;
EcCNode cnp;
EcKey k=*key;

#ifdef KEYDEBUG
	fprintf(stderr,"looking up %s in %s\n",k,n->name);
#endif
	*key = strchr(k,EC_DIRSEP_CHAR);
	l = *key ? (*key)++ - k : strlen(k);

	if (l==EC_UPDIR_NAME_LEN && 0==strncmp(EC_UPDIR_NAME,k,l))
		return n->parent;

	for (i=n->node->u.d.n->nels-1, cnp=&n->node->u.d.n->nodes[i];
		 i>=0;
		 i--, cnp--) {
#ifdef KEYDEBUG
	fprintf(stderr,"comparing key to %s\n",cnp->name);
#endif
		if (0==strncmp(cnp->name, k, l) && strlen(cnp->name)==l)
			return &n->u.dir[i];
	}
return 0;
}

/* lookup a key adding all the offsets
 * of the traversed nodes to *p
 * Returns: 0 if the node is not found.
 * Note: the node may not be a leaf
 */
EcCNode
lookupEcCNode(EcCNode n, EcKey key, IOPtr *p, EcCNodeList *l)
{
int i;
EcKey k=key;
EcCNodeList t=l?*l:0;
EcCNodeList h=t;

	while ( !EcKeyIsEmpty(k) && n ) {
		if (!EcCNodeIsDir(n)  || !(n=lookupEcKey(n,&k)))
			goto cleanup; /* Key not found */
		if (l) h=addEcCNode(n,h);
#if defined(KEYDEBUG)
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
ecLookupNode(EcNode n, EcKey key)
{
EcKey k=key;
	while ( !EcKeyIsEmpty(k) && n ) {
		if (!EcNodeIsDir(n) || ! (n=ecKeyLookup(n, &k)))
			return 0;
#if defined(KEYDEBUG)
		fprintf(stderr,".%s",n->name);
#endif
	}
	return n;
}

EcCNode
lookupEcCNodeFast(EcCNode n, EcFKey key, IOPtr *p, EcCNodeList *l)
{
int i;
EcFKey k=key;
EcCNodeList t=l?*l:0;
EcCNodeList h=t;

	while ( !EcFKeyIsEmpty(k) && n ) {
		if (!EcCNodeIsDir(n)  || !(n=lookupEcFKey(n,&k)))
			goto cleanup; /* Key not found */
		if (l) h=addEcCNode(n,h);
#if defined(KEYDEBUG)
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
ecLookupNodeFast(EcNode n, EcFKey key)
{
EcFKey k=key;
	while ( !EcFKeyIsEmpty(k) && n ) {
		if (!EcNodeIsDir(n)  || !(n=ecFKeyLookup(n,&k)))
			return 0;
#if defined(KEYDEBUG)
		fprintf(stderr,".%s",n->name);
#endif
	}
	return n;
}


#include "ecdr814RegTable.c"

void
ecCNodeWalk(EcCNode n, void (*fn)(EcCNodeList,IOPtr,void*), IOPtr p, EcCNodeList parent, void *fnarg)
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
ecWalkNode(EcNode n, void (*fn)(EcNode n, IOPtr boardBase, void *arg), IOPtr boardBase, void *arg)
{
	/* call fn */
	fn(n, boardBase, arg);

	if (EcNodeIsDir(n)) {
		int i,nels=n->node->u.d.n->nels;
		EcNode nn;
		for (i=0, nn=n->u.dir; i < nels; i++, nn++)
			ecWalkNode(nn, fn, boardBase, arg);
	}
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



