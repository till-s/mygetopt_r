/* $Id$ */

#define ECDR814_PRIVATE_IF
#include "drvrEcdr814.h"
#include "ecdrRegdefs.h"
#include "ecErrCodes.h"
#include "regNodeOps.h"

#include "ecFastKeys.h"

#include "ecOsDep.h"

/* This file contains really lowlevel stuff */


/* consistency check for decimation factors and number of taps */

/*
{
nch = 2 : diversity channel real input, 1 otherwise.

ECHOTEK: I believe it's always in single channel real mode

cic5:  input rate < fclk / 2 / nch
;

i.e. CIC2 decimation of 1 requires RX clock >= 2 * acquisition clock;
ECHOTEK: I don't know how the receiver_clock_same (channel pair
setting) relates to the rx_clock_multiplier0..3 / 4..7 (base
board setting).

NTAPS requirement:

ntaps < min( 256, fclk * Mrcf / fclk5) / nch
 = min(256, fclk * Mrcf * Mcic5 * Mcic2 / fclck) / nch

ECHOTEK:
	total decimation register must be set to
        Mcic2*Mcic5*Mrcf/2 - 1
	(what to do if it's not even?)

furthermore:
	1stTap+ntaps <= 256

fclk5 = fclk2 / Mcic5

ECHOTEK: if in dual receiver mode, the number of taps must be even.
         However, I believe this restriction is only necessary
         when "interleaving" the dual RX output samples.

         When tuning both AD6620 to different frequencies, they
         recommend the output rates being integer multiples.

}
*/


/* this routine does a couple of consistency checks
 * on AD6620 parameters. It should be called before
 * taking the chip out of reset.
 * Also, this routine carries over the total
 * decimation factor to the respective ECDR 
 * register.
 */


EcErrStat
ad6620ConsistencyCheck(EcBoardDesc bd, EcNode n)
{
EcErrStat	rval;
int		tmp;
EcFKey		fk;

Val_t		mCiC2, mCiC5, mRCF, nTaps, frstTap, v;

/* get the relevant AD6620 parameters */
/* find a node for the AD6620. Note that the calculated
 * sum of offsets is of no use because we don't know
 * the exact path.
 */
if (!(n = ecNodeLookupFast(n, FK_PARENT)))
	return EcErrNodeNotFound;

/* now get the AD6620 parameters */
if ( (rval=ecLkupNGet(bd, n, FK_ad6620_cic2Decm, &mCiC2))  ||
     (rval=ecLkupNGet(bd, n, FK_ad6620_cic5Decm, &mCiC5))  ||
     (rval=ecLkupNGet(bd, n, FK_ad6620_rcfDecm, &mRCF))   ||
     (rval=ecLkupNGet(bd, n, FK_ad6620_rcfNTaps, &nTaps))  ||
     (rval=ecLkupNGet(bd, n, FK_ad6620_rcf1stTap, &frstTap)) )
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

/* 
 * write the total decimation to the respective ECDR register
 * check consistency of total decimation among ad6620 chip pairs
 * on the same channel.
 */
if (tmp & 1)
	ecWarning( EcErrOK, "total decimation is not even");
tmp = (tmp>>1);

switch (ECMYFKEY(n)) {
	case FK_channelPair_0A:
	case FK_channelPair_0B:
		fk = BUILD_FKEY3(FK_PARENT, FK_channelPair_C0, FK_channel_totDecm_2);
		break;
	case FK_channelPair_1A:
	case FK_channelPair_1B:
		fk = BUILD_FKEY3(FK_PARENT, FK_channelPair_C1, FK_channel_totDecm_2);
		break;
	default:
		return EcErrNodeNotFound;
}

if ((rval = ecLkupNPut(bd, n, fk, tmp)))
	return rval;

/*
 * allow mCiC2 == 1 if rx clock higher than acquisition clk
 */
if ((rval = ecLkupNGet(bd,n,BUILD_FKEY2(FK_PARENT, FK_channelPair_clockSame), &v)))
	return rval;

if ( v && mCiC2 < 2 )
	return EcErrTooManyTaps;

return EcErrOK;
}

EcErrStat
ecSetupDMADesc( EcDMADesc d, void *buf, int size, unsigned char vec, EcDMAFlags flags)
{
void		*vmeaddr;
unsigned long	mcsr = ((unsigned long)vec) & 0xff;

	if (mcsr & EC_CY961_MCSR_IENA)
		return EcErrOutOfRange;

	/* the fifo spans only ECDR_FIFO_ADDR_RANGE bytes in the
	 * address map of the ECDR
	 */
	if (size > ECDR_FIFO_ADDR_RANGE)
		return EcErrOutOfRange;

	if ( EcDMA_D64 & flags ) {
		if (size % 128) return EcErrOutOfRange;
		size /= 128;
		WRBE( EC_CY961_XFRT_DTSZ_D64 | EC_CY961_XFRT_TYPE_A32_SB64,
			&d->xfrt );
	} else {
		if (size % 64) return EcErrOutOfRange;
		size /= 64;
		WRBE( EC_CY961_XFRT_DTSZ_D32 | EC_CY961_XFRT_TYPE_A32_SB,
			&d->xfrt );
	}

	if ( osdep_local2vme(buf, &vmeaddr) )
		return EcError;

	if ( (unsigned long)vmeaddr % ((flags & EcDMA_D64) ? 256 : 4) )
		return EcErrMisaligned;

	WRBE( (size & 0xff), &d->tlm0);
	WRBE( ((size >> 8) & 0xff), &d->tlm1);
	WRBE( (unsigned long)vmeaddr, &d->vadr );

	if (flags & EcDMA_IEN)
		mcsr|=EC_CY961_MCSR_IENA;

	WRBE( mcsr, &d->mcsr );

	return EcErrOK;
}

EcErrStat
ecStartDMA(EcBoardDesc brd, EcDMADesc d)
{
	EcDMARegs r = (EcDMARegs) (brd->base + CY961_OFFSET);

	/* get the semaphore; remember that the flags are "active low" */
	if ( ! (EC_CY961_SEMA_BUSY & RDBE(&r->sema)) )
		return EcErrDMABusy;

	EIEIO;

	/* initialize the dma registers;
	 * the descriptor already holds big endian values...
	 */
	r->tlm0 = d->tlm0;
	r->tlm1 = d->tlm1;
	r->xfrt = d->xfrt;
	r->vadr = d->vadr;
	/* I believe r->uadr is unused by the ecdr */
	r->mcsr = d->mcsr;

	EIEIO;

	/* verify the settings */
	if (  EC_CY961_SEMA_BUSY != ((~RDBE(&r->sema)) & EC_CY961_SEMA_MASK)) {
		/* release semaphore and return error */
		WRBE(EC_CY961_SEMA_BUSY, &r->sema); /* flags are "active low" */
		return EcErrDMASetup;
	}
	/* seems ok, issue GO writing the board's local address */
	WRBE( (unsigned long)brd->vmeBase | FIFO_OFFSET, &r->ladr );

	return EcErrOK;
}

/* use a ushort return value, so the
 * user may use a ulong for manipulating
 * interrupt status. Having this return
 * short they can be sure we're only
 * manipulating the lower 16 bits
 * without having to mess with a mask :-)
 */
unsigned short
ecGetIntStat(EcBoardDesc bd)
{
return RDBE(bd->base + ECDR_INT_STAT_OFFSET) & ECDR_INT_STAT_MSK;
}

unsigned long
ecGetCYSemaStat( EcBoardDesc bd)
{
	EcDMARegs r = (EcDMARegs) (bd->base + CY961_OFFSET);
	return (~RDBE(&r->sema)) & 0xff;
}

/* clear the semaphore if the present status indicates
 * that we own it (the 'sema' parameter is the value
 * returned by a previous call to ecGetCYSemaStat()
 */
void
ecClearCYSema(EcBoardDesc bd, unsigned long sema)
{
	EcDMARegs r;
	/* sema is an 'active high' value, returned
         * by a previous ecGetCYSemaStat()
	 */
	if (sema & EC_CY961_SEMA_BUSY) return;

	r = (EcDMARegs) (bd->base + CY961_OFFSET);
	WRBE( ((~sema) | EC_CY961_SEMA_BUSY), &r->sema); /* release the SEMA */
}

unsigned long
ecGetCYVector(EcBoardDesc bd)
{
	EcDMARegs r = (EcDMARegs) (bd->base + CY961_OFFSET);
	return RDBE(&r->mcsr) & 0xff;
}

#ifdef ECDR_OBSOLETE

/* NOTE: THIS CAN NOW ALL BE DONE THROUGH DATABASE ACCESSES */
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

#define CHIPNO(base)  ( ( ((int)((base)&0xe000) - 0x2000) >> 13) | ( ((int)((base)&0x70000) - 0x10000)>>14) )
#define CHNLNO(base)  (CHIPNO(base)>>1)
#define PAIRNO(base)	( ((int)((base)&0x70000) - 0x10000) >> 16 )

static EcCNodeList
addr2NodeList(IOPtr addr, IOPtr *pbase)
{
EcCNodeList	l = addEcCNode(&ecdr814CInfo,0);
EcCNode		n;
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
if (!(n=ecCNodeLookupFast(l->n, fk, 0, &l)))
	goto cleanup;

if (


cleanup:
	while (l) l=freeNodeListRec(l);
	return 0;
}

#endif
