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

	if (!EcNodeIsDir(n))
		return 0;

	if ( i<0
#ifdef FKEYDEBUG
		|| i > n->cnode->u.d.n->nels
#endif
	)
		return EMPTY_FKEY;
	return &n->u.entries[i];
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

	if (EC_UPDIR_NAME_LEN==l && 0==strncmp(EC_UPDIR_NAME,k,l))
		return n->parent;

	if (!EcNodeIsDir(n)) return 0;

	for (i=n->cnode->u.d.n->nels-1, cnp=&n->cnode->u.d.n->nodes[i];
		 i>=0;
		 i--, cnp--) {
#ifdef KEYDEBUG
	fprintf(stderr,"comparing key to %s\n",cnp->name);
#endif
		if (0==strncmp(cnp->name, k, l) && strlen(cnp->name)==l)
			return &n->u.entries[i];
	}
return 0;
}

EcNode
ecNodeLookup(EcNode n, EcKey key, EcBoardDesc *pbd)
{
EcKey k=key;
EcKey bne;
EcBoardDesc bd= pbd ? *pbd : 0;

	if (EC_ROOT_LOOKUP==n) n=bd ? bd->root : ecdr814Board; /* set to top */

	if ((bne=strchr(k,EC_BRDSEP_CHAR))) {
		k=bne+1;
		/* search for the board descriptor */
		if (!(bd=ecBoardDescLookup(key, bne-key)))
			return 0;
	}
	if (EC_DIRSEP_CHAR == *k || bne) {
		/* start from the root */
		n=bd ? bd->root : ecdr814Board;
		if (EC_DIRSEP_CHAR==*k) k++;
	}
	while ( !EcKeyIsEmpty(k) && n ) {
		if (!(n=ecKeyLookup(n, &k)))
			return 0;
#if defined(KEYDEBUG)
		fprintf(stderr,".%s",n->name);
#endif
	}
	if (pbd) {
		*pbd = bd;
		if (!bd) return 0;
	}
	return n;
}

EcNode
ecNodeLookupFast(EcNode n, EcFKey key)
{
EcFKey k=key;
	if (EC_ROOT_LOOKUP == n) n=ecdr814Board;
	while ( !EcFKeyIsEmpty(k) && n ) {
		if (!(n=ecFKeyLookup(n,&k)))
			return 0;
#if defined(KEYDEBUG)
		fprintf(stderr,".%s",n->name);
#endif
	}
	return n;
}

#ifdef ECDR_OBSOLETE
/* THIS ROUTINE IS OBSOLETE */
/* lookup a key adding all the offsets
 * of the traversed nodes to *p
 * Returns: 0 if the node is not found.
 * Note: the node may not be a leaf
 */
EcCNode
ecCNodeLookup(EcCNode n, EcKey key, IOPtr *p, EcCNodeList *l)
{
int i;
EcKey k=key;
EcCNodeList t=l?*l:0;
EcCNodeList h=t;
EcBoardDesc bd=0;

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

EcCNode
ecCNodeLookupFast(EcCNode n, EcFKey key, IOPtr *p, EcCNodeList *l)
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

#endif


