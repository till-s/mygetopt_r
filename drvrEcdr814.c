#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "drvrEcdr814.h"
#include "ecdrRegdefs.h"
#include "ecFastKeys.h"

static int drvrInited=0;

static void
putIniVal(EcNodeList l, IOPtr p, void* arg)
{
EcNode n=l->n;
if ( EcNodeIsDir(n) )return;
if ( ! (n->u.r.flags & (EcFlgReadOnly|EcFlgNoInit)))
	ecPutValue(n, p, n->u.r.inival);
}

static EcNodeDirRec boardDirectory={0, name: "root"};

EcNodeRec ecRootNode={
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


EcErrStat
ecAddBoard(char *name, IOPtr b)
{
EcErrStat	rval=EcError;
EcNodeDir	bd=&boardDirectory;
EcNode		n;
	/* lazy init */
	if ( ! drvrInited )
		drvrEcdr814Init();

	/* ecdrRegdefs.h makes the assumption that
	 * the base address is aligned to 0x1fff
	 */
	assert( ! ((unsigned long)b & ECDR_BRDREG_ALIGNMENT) );
	/* try to detect board */
	/* initialize */
	/* raw initialization sets pretty much everything to zero */
	walkEcNode( &ecdr814RawBoard, putIniVal, b, 0, 0);
	/* now set individual bits as defined in the board table */
	walkEcNode( &ecdr814Board, putIniVal, b, 0, 0);

	/* write channel ids */
	{
	int 	i;
	IOPtr	a;
	EcFKey	fk[8] = {
		BUILD_FKEY3( FK_board_01 , FK_channelPair_C0, FK_channel_channelID ),
		BUILD_FKEY3( FK_board_01 , FK_channelPair_C1, FK_channel_channelID ),
		BUILD_FKEY3( FK_board_23 , FK_channelPair_C0, FK_channel_channelID ),
		BUILD_FKEY3( FK_board_23 , FK_channelPair_C1, FK_channel_channelID ),
		BUILD_FKEY3( FK_board_45 , FK_channelPair_C0, FK_channel_channelID ),
		BUILD_FKEY3( FK_board_45 , FK_channelPair_C1, FK_channel_channelID ),
		BUILD_FKEY3( FK_board_67 , FK_channelPair_C0, FK_channel_channelID ),
		BUILD_FKEY3( FK_board_67 , FK_channelPair_C1, FK_channel_channelID ),
		};
		for (i=0; i < 8; i++) {
			a = b;
			assert ( (n = lookupEcNodeFast(&ecdr814Board, fk[i], &a, 0) )
				 && (EcErrOK == ecPutValue(n, a, i)) );
		}
	}
	/* TODO other initialization (VME, RW...) */

	/* on success, create directory entry */
	bd->nels+=2;
	assert(bd->nodes = (EcNode) realloc(bd->nodes,
						sizeof((*(bd->nodes))) * bd->nels));

	n=bd->nodes+(bd->nels-2);
	n->name=name;
	n->offset=(unsigned long)b;
	n->t = EcDir;
	n->u.d.n = ecdr814Board.u.d.n;
	n++;
	/* create board entry for access to raw registers */
	n->name=malloc(strlen(name)+5);
	n->offset=(unsigned long)b;
	n->t = EcDir;
	n->u.d.n = ecdr814RawBoard.u.d.n;
	sprintf(n->name,"%s_raw",name);
	rval=EcErrOK;

cleanup:
	return rval;

}


void
drvrEcdr814Init(void)
{
	/* initialize tiny virtual fn tables */
	initRegNodeOps();
	drvrInited=1;
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
	"too many taps for total decimation",
	"invalid array index",
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

