/* $Id$ */

#define ECDR814_PRIVATE_IF
#include "regNodeOps.h"
#include "bitMenu.h"
#include "ecdrRegdefs.h"

#include <assert.h>

static EcErrStat
get(EcNode n, EcFKey fk, IOPtr b, Val_t *pv)
{
EcErrStat rval;
register Val_t val;

	if (n->u.r.flags & EcFlgAD6620RStatic && ! ad6620IsReset(b)) {
		/* some AD6620 registers may only be read while it is in reset state */
		return EcErrAD6620NotReset;
	}
	rval=ecGetRawValue(n,b,pv);
	val = (*pv & ECREGMASK(n))>>ECREGPOS(n);
	if (!rval && EcFlgMenuMask & n->u.r.flags) {
		EcMenu		m=ecMenu(n->u.r.flags);
		EcMenuItem	mi;
		int		i;
		for (i=m->nels-1, mi=m->items+i; i>=0; i--,mi--) {
			if (mi->bitval == val)
				break;
		}
		if (i<0) return EcErrOutOfRange;
		val = i;
	}
	/* we must do range checking before adjusting the value
	 * (Val_t is unsigned, hence se must do the comparisons
	 * on the unsigned values)
	 */
	if (n->u.r.min != n->u.r.max &&
		( val < n->u.r.min + n->u.r.adj || val > n->u.r.max + n->u.r.adj ) )
		ecWarning( EcErrOutOfRange, " _read_ value of of range ???", 0);
	val-=n->u.r.adj;
	*pv=val;

	return rval;
}

static EcErrStat
put(EcNode n, EcFKey fk, IOPtr b, Val_t val)
{

	if (n->u.r.flags & EcFlgReadOnly)
		return EcErrReadOnly;

	if ((n->u.r.flags & EcFlgAD6620RWStatic) && ! ad6620IsReset(b)) {
		return EcErrAD6620NotReset;
	}

	if (n->u.r.flags & EcFlgMenuMask) {
		EcMenu		m=ecMenu(n->u.r.flags);
		if (val < 0 || val>m->nels)
			return EcErrOutOfRange;
		val = m->items[val].bitval;
	}

	val += n->u.r.adj;

	/* see comment in 'get' about why we do comparison after adjustment */
	if (n->u.r.min != n->u.r.max &&
		( val < n->u.r.min + n->u.r.adj  || val > n->u.r.max + n->u.r.adj ) )
		return EcErrOutOfRange;


	{ /* merge into raw value */
	register EcErrStat	rval;
	register Val_t		m = ECREGMASK(n);
	Val_t 			rawval; 
		if (EcErrOK != (rval = ecGetRawValue(n,b,&rawval)))
			return rval;

		rawval = (rawval & ~m) | ((val<<ECREGPOS(n))&m);

		return ecPutRawValue(n,b,rawval);
	}
}

static EcErrStat
getRaw(EcNode n, IOPtr b, Val_t *rp)
{
volatile Val_t *vp = (Val_t *)b;
	*rp = RDBE(vp);
	return EcErrOK;
}


static EcErrStat
putRaw(EcNode n, IOPtr b, Val_t val)
{
volatile Val_t *vp = (Val_t *)b;
	WRBE(val,vp);
	return EcErrOK;
}

static EcErrStat
adGetRaw(EcNode n, IOPtr b, Val_t *rp)
{
volatile Val_t *vp = (Val_t *)b;

#if 0 /* doing this in 'get' now */
	/* filter coefficients must not be dynamically read
	 * (AD6620 datasheet rev.1)
	 *
	 * Note that we should prohibit access to the NCO
	 * frequency as well if in SYNC_MASTER mode, as this
	 * would cause a sync pulse. However, we assume
	 * the AD6620s on an ECDR board to always be sync
	 * slaves.
	 */
	if (n->u.r.flags & EcFlgAD6620RStatic) {
		/* calculate address of MCR */
		register volatile Val_t *mcrp = (Val_t*)(AD6620BASE(b) + OFFS_AD6620_MCR);
		register long	isreset = (RDBE(mcrp) & BITS_AD6620_MCR_RESET);
			EIEIO; /* make sure there is no out of order hardware access beyond
				* this point
				*/
			if (!isreset) return EcErrAD6620NotReset;
	}
#endif
	*rp = (RDBE(vp) & 0xffff) | ((RDBE(vp+1) & 0xffff)<<16);
	return EcErrOK;
}


static EcErrStat
adPutMCR(EcNode n, EcFKey fk, IOPtr b, Val_t val)
{

	if (n->u.r.flags & EcFlgReadOnly)
		return EcErrReadOnly;

	val += n->u.r.adj;

	/* see comment in 'get' about why we do comparison after adjustment */
	if (n->u.r.min != n->u.r.max &&
		( val < n->u.r.min + n->u.r.adj  || val > n->u.r.max + n->u.r.adj ) )
		return EcErrOutOfRange;

	{ /* merge into raw value */
	register EcErrStat	rval;
	register Val_t		m = ECREGMASK(n);
	Val_t 			orawval, rawval; 
		if (EcErrOK != (rval = ecGetRawValue(n,b,&orawval)))
			return rval;

		/* merge old raw value and val into raw value */
		rawval = (orawval & ~m) | ((val<<ECREGPOS(n))&m);

		/* now let's see what they intend to do */
		orawval = rawval ^ orawval; /* look at what bits changed */
		if ( (orawval & ~ BITS_AD6620_MCR_RESET) &&		/* bits other than reset have changed */
		     ((orawval | ~rawval) & BITS_AD6620_MCR_RESET) ) {	/* reset has also changed or is low */
				/* reject attempt to change */
				return EcErrAD6620NotReset;
		} else {
			/* only reset has changed (or noting at all) */
			if ( ! (rawval&BITS_AD6620_MCR_RESET) ) {
				/* taking it out of reset, do consistency check */
				rval=ad6620ConsistencyCheck(fk);
				EIEIO;
				if (rval) return rval; /* reject inconsistent configuration */
			}
		}

		return ecPutRawValue(n,b,rawval);
	}
}

static EcErrStat
adPutRaw(EcNode n, IOPtr b, Val_t val)
{
volatile Val_t	*vp = (Val_t *)b;

#if 0        /* this is now done by the higher level routines */
	/* allow write attempt to "static" 
	 * registers only if the receiver is in RESET
	 * state. At this low level, we leave the abstraction
	 * behind and dive directly into the guts...
	 */
	if ( (n->u.r.flags & EcFlgAD6620RWStatic) ) {

		/* must inspect current state of RESET bit */

		register unsigned long	mcr = RDBE((AD6620BASE(b) + OFFS_AD6620_MCR));

		/* it's OK to do the following:
		 *  if in reset state
		 *  - change any other register than MCR
		 *  - change either the reset bit or any other bit in MCR
		 *    but not both.
		 *  if we're out of reset
		 *  - the only allowed operation
		 *    is just setting the reset bit
		 */
		if ( (BITS_AD6620_MCR_RESET & mcr) ) {
			if ( OFFS_AD6620_MCR == n->offset ) {
				/* trying to modify mcr; let's see which bits change: */
				mcr = (val^mcr) & BITS_AD6620_MCR_MASK;
				if ( mcr & BITS_AD6620_MCR_RESET) {
					/* reset bit switched off */
					if ( mcr & ~BITS_AD6620_MCR_RESET) {
						/* another bit changed as well */
						return EcErrAD6620NotReset;
					}
					/* TODO do consistency check on all settings */
				}
				/* other MCR bits changed while reset held */
			}
		} else {
			/* only flipping reset is allowed */
			if ( OFFS_AD6620_MCR != n->offset  ||
			    ((val ^ mcr) & BITS_AD6620_MCR_MASK)
			     != BITS_AD6620_MCR_RESET )
				return EcErrAD6620NotReset;
		}
		EIEIO; /* make sure there is no out of order hardware access beyond
			* this point
			*/
	}
#endif

	EIEIO; /* make sure read of old value completes */
	WRBE(val & 0xffff, vp);
	EIEIO; /* most probably not necessary in guarded memory */
	WRBE((val>>16)&0xffff, vp+1);
	EIEIO; /* just in case they read it back again: enforce
	        * completion of write before load
	        */
	return EcErrOK;
}

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

EcNodeOpsRec	ecDefaultNodeOps = {
	0, /* super */
	0,
	0,
	0,
	0,
};


EcNodeOpsRec ecdr814RegNodeOps = {
	&ecDefaultNodeOps,
	0,
	get,
	getRaw,
	put,
	putRaw
};

EcNodeOpsRec ecdr814AD6620RegNodeOps = {
	&ecdr814RegNodeOps,
	0,
	0,
	adGetRaw,
	0,
	adPutRaw
};

EcNodeOpsRec ecdr814AD6620MCRNodeOps = {
	&ecdr814AD6620RegNodeOps,
	0,
	0,
	0,
	adPutMCR,
	0
};

static EcNodeOps	nodeOps[8]={&ecDefaultNodeOps,};

/* public routines to access leaf nodes */

EcErrStat
ecGetValue(EcNode n, EcFKey fk, IOPtr b, Val_t *vp)
{
EcNodeOps ops;
	assert(n->t < EcdrNumberOf(nodeOps) && (ops=nodeOps[n->t]));
	assert(ops->get);
	return ops->get(n,fk,b,vp);
}

EcErrStat
ecGetRawValue(EcNode n, IOPtr b, Val_t *vp)
{
EcNodeOps ops;
	assert(n->t < EcdrNumberOf(nodeOps) && (ops=nodeOps[n->t]));
	assert(ops->getRaw);
	return ops->getRaw(n,b,vp);
}

EcErrStat
ecPutValue(EcNode n, EcFKey fk, IOPtr b, Val_t val)
{
EcNodeOps ops;
	assert(n->t < EcdrNumberOf(nodeOps) && (ops=nodeOps[n->t]));
	assert(ops->put);
	return ops->put(n,fk,b,val);
}

EcErrStat
ecPutRawValue(EcNode n, IOPtr b, Val_t val)
{
EcNodeOps ops;
	assert(n->t < EcdrNumberOf(nodeOps) && (ops=nodeOps[n->t]));
	assert(ops->putRaw);
	return ops->putRaw(n,b,val);
}

static void
recursive_ini(EcNodeOps ops)
{
EcNodeOps super=ops->super;
	if (super) {
		if (!super->initialized)
			recursive_ini(super);
		if (!ops->get) ops->get = super->get;
		if (!ops->getRaw) ops->getRaw = super->getRaw;
		if (!ops->put) ops->put = super->put;
		if (!ops->putRaw) ops->putRaw = super->putRaw;
	}
	ops->initialized=1;
}

void
addNodeOps(EcNodeOps ops, EcNodeType t)
{
	assert( t << EcdrNumberOf(nodeOps) && !nodeOps[t]);
	recursive_ini(ops);
	nodeOps[t] = ops;
}

void
initRegNodeOps(void)
{
	addNodeOps(&ecdr814RegNodeOps, EcReg);
	addNodeOps(&ecdr814AD6620RegNodeOps, EcAD6620Reg);
	addNodeOps(&ecdr814AD6620MCRNodeOps, EcAD6620MCR);
}
