/* $Id$ */

#include "drvrEcdr814.h"
#include "ecdrRegdefs.h"
#include "ecErrCodes.h"
#include "regNodeOps.h"

#include "ecFastKeys.h"

/* This file contains really lowlevel stuff */

/*
 * How to map an AD6620 register address to its
 * channel, channel pair and board?
 *
 * The address looks like
 *
 * 0ppp . cccx xxxx xxxx xxxx
 *
 * ppp gives the channel pair:
 *   1 -> pair 01
 *   2 -> pair 23
 *   3 -> pair 45
 *   4 -> pair 67
 * ccc gives the channel and chip number
 *   1 -> ch 0, chip A
 *   2 -> ch 0, chip B
 *   3 -> ch 1, chip A
 *   4 -> ch 1, chip B
 */ 

#if 0
#define CHIPNO(base)  ( ( ((int)((base)&0xe000) - 0x2000) >> 13) | ( ((int)((base)&0x70000) - 0x10000)>>14) )
#define CHNLNO(base)  (CHIPNO(base)>>1)
#define PAIRNO(base)	( ((int)((base)&0x70000) - 0x10000) >> 16 )

static EcNodeList
addr2NodeList(IOPtr addr, IOPtr *pbase)
{
EcNodeList	l = addEcNode(&ecdr814Board,0);
EcNode		n;
IOPtr		b;
EcFKey		

b = BRDREG_BASE(addr);

/* find the channel pair */
switch (PAIRNO(base)) {
	0:	fk = FK_channelPair_01; break;
	1:	fk = FK_channelPair_23; break;
	2:	fk = FK_channelPair_45; break;
	3:	fk = FK_channelPair_67; break;
	default:
		*base = b;
		return l;	/* number < 0, must be a board level register */
	
}
if (!(n=lookupEcNodeFast(l->n, fk, 0, &l)))
	goto cleanup;

if (


cleanup:
	while (l) l=freeNodeListRec(l);
	return 0;
}

#endif

/* this routine does a couple of consistency checks
 * on AD6620 parameters. It should be called before
 * taking the chip out of reset.
 * Also, this routine carries over the total
 * decimation factor to the respective ECDR 
 * register.
 */


EcErrStat
ad6620ConsistencyCheck(EcNode n, IOPtr addr)
{
EcNodeListRec	top={p: 0, n: &ecdr814Board};
EcNodeList	l=0;
EcNode		rx;
EcErrStat	rval;
IOPtr		base;
EcFKey		fk;
int		tmp;

Val_t		mCiC2, mCiC5, mRCF, nTaps, frstTap;

/* get the relevant AD6620 parameters */
/* find a node for the AD6620. Note that the calculated
 * sum of offsets is of no use because we don't know
 * the exact path.
 */
fk = BUILD_FKEY2( FK_board_01 , FK_channelPair_0A);
if (!(n = lookupEcNodeFast(&ecdr814Board, fk, 0, 0)))
	return EcErrNodeNotFound;

/* now get the AD6620 parameters using the correct
 * register address
 */
base = (IOPtr)AD6620BASE(addr);
if ( (rval=ecLkupNGet(n, FK_ad6620_cic2Decm, base, &mCiC2))  ||
     (rval=ecLkupNGet(n, FK_ad6620_cic5Decm, base, &mCiC5))  ||
     (rval=ecLkupNGet(n, FK_ad6620_rcfDecm,  base, &mRCF))   ||
     (rval=ecLkupNGet(n, FK_ad6620_rcfNTaps,    base, &nTaps))  ||
     (rval=ecLkupNGet(n, FK_ad6620_rcf1stTap,   base, &frstTap)) )
	return rval;

/* do a consistency check */
if (frstTap + nTaps > 256)
	return EcErrOutOfRange;

/* check maximum number of taps allowed (assuming NCH==1,
 * i.e. the chip is always in single channel real mode 
 */
tmp =  mCiC2 * mCiC5 * mRCF;
if (nTaps > (tmp > 256 ? 256 : tmp))
	return EcErrTooManyTaps;

/* TODO:
 * write the total decimation to the respective ECDR register
 * check consistency of total decimation among ad6620 chip pairs
 * on the same channel.
if (tmp & 1)
	ecWarning( ecErrOK, "total decimation is not even");
tmp = (tmp>>1) - 1;
 */

/*
 * TODO: allow mCiC2 == 1 if rx clock higher than acquisition clk
 */
if (mCiC2 < 2 )
	return EcErrTooManyTaps;

return EcErrOK;
}
