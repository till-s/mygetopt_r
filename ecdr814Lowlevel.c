/* $Id$ */

#include "ecdrRegdefs.h"
#include "ecErrCodes.h"

#include "ecFastKeys.h"

/* This file contains really lowlevel stuff */

/* this routine does a couple of consistency checks
 * on AD6620 parameters. It should be called before
 * taking the chip out of reset.
 * Also, this routine carries over the total
 * decimation factor to the respective ECDR 
 * register.
 */

extern EcNodeRec root;

static EcErrStat
getADReg(EcNode n, EcFKey fk, IOPtr base, Val_t *pval)
{
IOPtr addr=base;
	if (!(n=lookupEcNodeFast(n, fk, &addr, &l)))
		return EcErrNodeNotFound;
	return ecGetValue(n, 
}

EcErrStat
ad6620ConsistencyCheck(EcFKey fk)
{
EcNodeListRec	top={p: 0, n: &root};
EcNodeList	l=0;
EcNode		n,rx;
EcErrStat	rval;
IOPtr		base=0, addr;

Val_t		mCiC2, mCIC5, mRCF, nTaps, 1stTap;

/* find our entry */
if (!(rx=lookupEcNodeFast(top.n, fk, &base, &l)))
	return EcErrNodeNotFound;

addr=base;
if (!(n=lookupFcNodeFast(rx, FK_cic2Decm, &addr, 0)))
	return EcErrNodeNotFound;




return EcErrOK;
}
