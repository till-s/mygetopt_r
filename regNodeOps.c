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
Val_t v = ecGetRawValue(n,b);
	return (v & ~m) | ((val<<ECREGPOS(n))&m);
}

static Val_t
get(EcNode n, IOPtr b)
{
Val_t rval = ecGetRawValue(n,b);
	return (rval & ECREGMASK(n))>>ECREGPOS(n);
}

static void
put(EcNode n, IOPtr b, Val_t val)
{
	ecPutRawValue(n,b,val);
}

static Val_t
getRaw(EcNode n, IOPtr b)
{
volatile Val_t *vp = (Val_t *)b;
	return RDBE(vp);
}


static void
putRaw(EcNode n, IOPtr b, Val_t val)
{
volatile Val_t *vp = (Val_t *)b;
	WRBE(merge(n,b,val),vp);
}

static Val_t
adGetRaw(EcNode n, IOPtr b)
{
volatile Val_t *vp = (Val_t *)b;
	return (RDBE(vp) & 0xffff) | ((RDBE(vp+1) & 0xffff)<<16);
}


static void
adPutRaw(EcNode n, IOPtr b, Val_t val)
{
volatile Val_t *vp = (Val_t *)b;
	val = merge(n,b,val);
	EIEIO; /* make sure read of old value completes */
	WRBE(val & 0xffff, vp);
	EIEIO; /* most probably not necessary in guarded memory */
	WRBE((val>>16)&0xffff, vp+1);
	EIEIO; /* just in case they read it back again: enforce
            * completion of write before load
			*/
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
