/* $Id$ */
#ifndef EC_FAST_KEY_TILL_H
#define EC_FAST_KEY_TILL_H

#include <stdarg.h>

/* fast key implementation */
#include "drvrEcdr814.h"

#ifndef DO_GEN_FAST_KEYS
#include "fastKeyDefs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* macros to build fastkey paths (for hardcoding) */
#define FASTKEY_PREPEND( prevkey, key) (((key)<<FKEY_LEN) | prevkey)
#define BUILD_FKEY1(a) 		(a)
#define BUILD_FKEY2(a,b) 	FASTKEY_PREPEND( a, b)
#define BUILD_FKEY3(a,b,c)	FASTKEY_PREPEND( a, FASTKEY_PREPEND(b, c))
#define BUILD_FKEY4(a,b,c,d) 	FASTKEY_PREPEND( a, FASTKEY_PREPEND( b, FASTKEY_PREPEND(c, d)))

/*  slower routine but more comfortable:
 *  This composes a fastkey path out of
 *  a vararg list which must be terminated
 *  by an empty key.
 */
EcFKey
ecBFk(EcFKey first, ...);

#define ECMYFKEY(n) (((n) - (n)->parent->u.entries)+1)

#define EMPTY_FKEY		((EcFKey)0)

/* prepend an fkey to a path of fkeys */
extern __inline__ EcFKey 
ecAddFKeyToPath(EcFKey path, EcFKey fkey)
{
register EcFKey mask;
for (mask = (1<<FKEY_LEN)-1; mask & path; mask <<= FKEY_LEN, fkey<<=FKEY_LEN);
return path | fkey;
}

extern __inline__ EcFKey
ecStripFKeyFromPath(EcFKey path)
{
register  EcFKey mask;
for (mask = (1<<FKEY_LEN)-1; mask & path; mask <<= FKEY_LEN);
return path & (mask>>FKEY_LEN);
}

/* convert a NodeList to a fastkey describing
 * the same path
 */
static __inline__ EcFKey
ecNode2FKey(EcNode n)
{
EcFKey rval = EMPTY_FKEY;
while (n->parent) {
	rval = (rval << FKEY_LEN) | ((n->cnode - n->parent->cnode->u.d.n->nodes) + 1);
	n=n->parent;
}
return rval;
}

#ifdef __cplusplus
};
#endif

#endif
