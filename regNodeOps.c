/* $Id$ */

#define ECDR814_PRIVATE_IF
#include "regNodeOps.h"

#ifdef __PPC
#define EIEIO __asm__ __volatile__("eieio");
#define RDBE(addr) (*((volatile Val_t *)addr))
#define WRBE(val, addr) (*((volatile Val_t *)addr)=(val))
#else
#define EIEIO
#define RDBE(addr) (*((volatile Val_t *)addr))
#define WRBE(val, addr) (*((volatile Val_t *)addr)=(val))
#endif

static inline Val_t
merge(EcNode n, IOPtr b, Val_t val)
{
Val_t m = ECREGMASK(n);
Val_t v; 
	ecGetRawValue(n,b,&v);
	return (v & ~m) | ((val<<ECREGPOS(n))&m);
}

static EcErrStat
get(EcNode n, IOPtr b, Val_t *pv)
{
EcErrStat rval=ecGetRawValue(n,b,pv);
	*pv = (*pv & ECREGMASK(n))>>ECREGPOS(n);
	return rval;
}

static EcErrStat
put(EcNode n, IOPtr b, Val_t val)
{
	if (n->u.r.flags & EcFlgReadOnly)
		return EcErrReadOnly;
	return ecPutRawValue(n,b,val);
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
	WRBE(merge(n,b,val),vp);
	return EcErrOK;
}

static EcErrStat
adGetRaw(EcNode n, IOPtr b, Val_t *rp)
{
volatile Val_t *vp = (Val_t *)b;
	*rp = (RDBE(vp) & 0xffff) | ((RDBE(vp+1) & 0xffff)<<16);
	return EcErrOK;
}

#include "ecdrRegdefs.h"

static EcErrStat
adPutRaw(EcNode n, IOPtr b, Val_t val)
{
volatile Val_t *vp = (Val_t *)b;
	/* allow write attempt only if the receiver is in RESET
	 * state. At this low level, we leave the abstraction
	 * behind and dive directly into the guts...
	 */
	if ( (n->offset != OFFS_AD6620_MCR ||
	     !(val & BITS_AD6620_MCR_RESET ))
	    && ! (*(Val_t*)(b-n->offset+OFFS_AD6620_MCR) & BITS_AD6620_MCR_RESET))
		return EcErrAD6620NotReset;

	val = merge(n,b,val);
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
