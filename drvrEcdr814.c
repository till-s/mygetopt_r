#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "drvrEcdr814.h"
#include "ecdrRegdefs.h"
#include "ecFastKeys.h"

#include "ecOsDep.h"

static void
putIniVal(EcBoardDesc bd, EcNode n, void* arg)
{
EcCNode cn=n->cnode;
if ( EcCNodeIsDir(cn) )return;
if ( ! (cn->u.r.flags & (EcFlgReadOnly|EcFlgNoInit)))
	ecPutValue(bd, n, cn->u.r.inival);
}

static EcCNodeDirRec boardDirectory={0, name: "root"};

EcCNodeRec ecRootNode={
	"/", EcDir, 0, {&boardDirectory}
	
};

static EcBoardDesc	boards[16];
static int		nBoards=0;

EcNode			ecdr814Board=0;
static EcNode		ecdr814RawBoard=0;

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
ecAddBoard(char *name, IOPtr vme_b, EcBoardDesc *pdesc)
{
EcErrStat	rval=EcError;
EcCNodeDir	bd=&boardDirectory;
EcNode		n;
IOPtr		b;
Val_t		v;
	/* lazy init */
	if ( ! ecdr814Board )
		drvrEcdr814Init();

	if (pdesc) *pdesc=0;

	/* ecdrRegdefs.h makes the assumption that
	 * the base address is aligned to 0x1fff
	 */
	assert( ! ((unsigned long)vme_b & ECDR_BRDREG_ALIGNMENT) );
	/* map vme address to local */
	assert(0==osdep_vme2local(vme_b, &b));
	/* TODO try to detect board; they should
	 * really provide a RO register with a
	 * (version etc.) signature...
	 */
	assert(0==osdep_memProbe(b/* TODO + feature register offset */,
				  0/* read */, 
				  4/* bytes */,
				  &v));

#if 0 /* TODO check against known value, firmware version etc */
	/* use RDBE for portability */
	assert( RDBE(&v) == SOME_VALUE );
#endif
	/* allocate and initialize board descriptor entry */
	if (EcdrNumberOf(boards)<=nBoards) {
		rval=EcErrOutOfRange;
		goto cleanup;
	}


	boards[nBoards] = (EcBoardDesc)malloc(sizeof(EcBoardDescRec));
	boards[nBoards]->name = name; /* probably, we should make a copy here */
	boards[nBoards]->base = b;
	boards[nBoards]->vmeBase = vme_b;

	/* initialize */
	/* raw initialization sets pretty much everything to zero */
	ecNodeWalk( & boards[nBoards], ecdr814RawBoard, putIniVal, 0);
	/* now set individual bits as defined in the board table */
	ecNodeWalk( & boards[nBoards], ecdr814Board, putIniVal, 0);

	/* write channel ids */
	{
	int 	i;
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
			assert ( (n = ecNodeLookupFast(ecdr814Board, fk[i]) )
				 && (EcErrOK == ecPutValue(boards[nBoards],n, i)) );
		}
	}
	/* TODO other initialization (VME, RW...) */

#if 0
	/* on success, create directory entry */
	bd->nels+=2;
	assert(bd->nodes = (EcCNode) realloc(bd->nodes,
						sizeof((*(bd->nodes))) * bd->nels));

	n=bd->nodes+(bd->nels-2);
	n->name=name;
	n->offset=(unsigned long)b;
	n->t = EcDir;
	n->u.d.n = ecdr814CInfo.u.d.n;

	n++;

	/* create board entry for access to raw registers */
	n->name=malloc(strlen(name)+5);
	n->offset=(unsigned long)b;
	n->t = EcDir;
	n->u.d.n = ecdr814RawCInfo.u.d.n;
	sprintf(n->name,"%s_raw",name);
#endif
	if (pdesc) {
		*pdesc=boards[nBoards];
	}
	nBoards++;


	rval=EcErrOK;


cleanup:
	return rval;

}

EcBoardDesc ecGetBoardDesc(int instance)
{
	if (instance < 0 || instance >=nBoards)
		return 0;
	return boards[instance];
}

extern EcNode ecCreateDirectory(EcCNode);

void
drvrEcdr814Init(void)
{
	if (ecdr814Board) return; /* already initialized */

	/* initialize tiny virtual fn tables */
	initRegNodeOps();
	/* create the board database */
	ecdr814Board=ecCreateDirectory(&ecdr814CInfo);
	/* create the database for raw register access */
	ecdr814RawBoard=ecCreateDirectory(&ecdr814RawCInfo);
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
	"misaligned DMA buffer",
	"DMA engine busy",
	"DMA setup invalid",
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

EcFKey
ecBFk(EcFKey first, ...)
{
EcFKey	rval=EMPTY_FKEY;
/* there cannot be fastkey paths longer than this */
EcFKey	stack[8*sizeof(EcFKey)/FKEY_LEN + 1];
va_list	ap;
int	sp=0;

	va_start(ap, first);
	stack[sp++]=first;

	while (EMPTY_FKEY != (stack[sp++]=va_arg(ap, EcFKey)))
		/* do nothing */;
	va_end(ap);

	/* now reverse the stack */
	while (--sp >= 0) 
		rval = (rval << FKEY_LEN) | stack[sp];
	return rval;
}
