#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "drvrEcdr814.h"
#include "bitMenu.h"
#include "ecdrRegdefs.h"
#include "ecFastKeys.h"


static void
putIniVal(EcNodeList l, IOPtr p, void* arg)
{
EcNode n=l->n;
if ( ! (n->u.r.flags & (EcFlgReadOnly|EcFlgNoInit)))
	ecPutValue(n, ecNodeList2FKey(l), p, n->u.r.inival);
}

static EcNodeDirRec boardDirectory={0};

EcNodeRec root={
	"/", EcDir, 0, {&boardDirectory}
	
};


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

void
ecWarning(EcErrStat e, char *message, ...)
{
va_list ap;
va_start(ap, message);
fprintf(stderr,"drvrEcdr814 Warning:  %s ",ecStrError(e));
vfprintf(stderr,message,ap);
va_end(ap);
}

