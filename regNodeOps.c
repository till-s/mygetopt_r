/* $Id$ */

#define ECDR814_PRIVATE_IF
#include "regNodeOps.h"
#include "bitMenu.h"
#include "ecdrRegdefs.h"

#ifdef __PPC
#define EIEIO __asm__ __volatile__("eieio");
#define RDBE(addr) (*((volatile Val_t *)addr))
#define WRBE(val, addr) (*((volatile Val_t *)addr)=(val))
#else
#define EIEIO
#define RDBE(addr) (*((volatile Val_t *)addr))
#define WRBE(val, addr) (*((volatile Val_t *)addr)=(val))
#endif


static EcErrStat
get(EcNode n, IOPtr b, Val_t *pv)
{
EcErrStat rval=ecGetRawValue(n,b,pv);
	*pv = (*pv & ECREGMASK(n))>>ECREGPOS(n);
	if (!rval && EcFlgMenuMask & n->u.r.flags) {
		EcMenu		m=ecMenu(n->u.r.flags);
		EcMenuItem	mi;
		int		i;
		for (i=m->nels-1, mi=m->items+i; i>=0; i--,mi--) {
			if (mi->bitval == *pv)
				break;
		}
		if (i<0) return EcErrOutOfRange;
		*pv = i;
	}
	return rval;
}

static EcErrStat
put(EcNode n, IOPtr b, Val_t val)
{

	if (n->u.r.flags & EcFlgReadOnly)
		return EcErrReadOnly;
	if (n->u.r.flags & EcFlgMenuMask) {
		EcMenu		m=ecMenu(n->u.r.flags);
		if (val < 0 || val>m->nels)
			return EcErrOutOfRange;
		val = m->items[val].bitval;
	}
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
	*rp = (RDBE(vp) & 0xffff) | ((RDBE(vp+1) & 0xffff)<<16);
	return EcErrOK;
}

#include "ecdrRegdefs.h"

static EcErrStat
adPutRaw(EcNode n, IOPtr b, Val_t val)
{
volatile Val_t	*vp = (Val_t *)b;

	/* allow write attempt to "static" 
	 * registers only if the receiver is in RESET
	 * state. At this low level, we leave the abstraction
	 * behind and dive directly into the guts...
	 */
	if ( (n->u.r.flags & EcFlgAD6620RWStatic) ) {

		/* must inspect current state of RESET bit */

		register long	mcr = RDBE((AD6620BASE(b) + OFFS_AD6620_MCR));

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
				if (
					(mcr & BITS_AD6620_MCR_RESET) /* reset bit changed */
				     && (mcr & ~BITS_AD6620_MCR_RESET)/* another bit changed */
				   )
					return EcErrAD6620NotReset;
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

	EIEIO; /* make sure read of old value completes */
	WRBE(val & 0xffff, vp);
	EIEIO; /* most probably not necessary in guarded memory */
	WRBE((val>>16)&0xffff, vp+1);
	EIEIO; /* just in case they read it back again: enforce
	        * completion of write before load
	        */
	return EcErrOK;
}


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

void
initRegNodeOps(void)
{
	addNodeOps(&ecdr814RegNodeOps, EcReg);
	addNodeOps(&ecdr814AD6620RegNodeOps, EcAD6620Reg);
}
