/* $Id$ */
#ifndef ECDR_REG_BITS_DEFS_H
#define ECDR_REG_BITS_DEFS_H

#include "drvrEcdr814.h"
#include "ecErrCodes.h"

/*
 * Register Bit definitions.
 *
 * We try to avoid these at all and use the
 * directory lookup engine whenever possible.
 * However, for some lowlevel routines it's sometimes
 * necessary to directly access registers
 * (i.e. for consistency checks)
 */

/* calculate the base address of the chip
 * note that we must not cast this to a
 * pointer at this point (especially, a non char*)
 * because further calculations may be done
 * by the user of the macro
 *
 * also note that the AD6620BASE macro needs an
 * address argument that points somewhere INTO
 * the ad6620 register space already!
 */

#define ECDR_AD6620_ALIGNMENT	(0x1fff)
#define AD6620BASE(addr)	(((unsigned long)(addr)) & ~ECDR_AD6620_ALIGNMENT)

#define OFFS_AD6620_MCR		(0x1800)
#define BITS_AD6620_MCR_RESET	(1<<0)
#define BITS_AD6620_MCR_MASK	((1<<8)-1)	/* bits that are actually used in this register */

extern EcErrStat
ad6620ConsistencyCheck(EcFKey fk);

/* low level operations */
#ifdef __PPC
#define EIEIO __asm__ __volatile__("eieio");
#define RDBE(addr) (*((Val_t *)addr))
#define WRBE(val, addr) (*((Val_t *)(addr))=(val))
#else
#define EIEIO
#define RDBE(addr) (*((Val_t *)(addr)))
#define WRBE(val, addr) (*((Val_t *)(addr))=(val))
#endif

/* we should probably use assert( b & ECDR_AD6620_ALIGNMENT ) */
unsigned long blahaddr;
int		blahval;

extern inline int
ad6620IsReset(volatile void *b)
{
int rval = BITS_AD6620_MCR_RESET & RDBE(AD6620BASE(b)+OFFS_AD6620_MCR);
//	blahaddr =AD6620BASE(b)+OFFS_AD6620_MCR;
	//blahval = RDBE(blahaddr);
	//return blahval&BITS_AD6620_MCR_RESET;
	EIEIO;
	return rval;
}

#endif
