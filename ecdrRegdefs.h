/* $Id$ */
#ifndef ECDR_REG_BITS_DEFS_H
#define ECDR_REG_BITS_DEFS_H

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
 */

#define ECDR_AD6620_ALIGNMENT	(0x1fff)
#define AD6620BASE(addr)	(((unsigned long)(addr)) & ~ECDR_AD6620_ALIGNMENT)

#define OFFS_AD6620_MCR		(0x1800)
#define BITS_AD6620_MCR_RESET	(1<<0)
#define BITS_AD6620_MCR_MASK	((1<<8)-1)	/* bits that are actually used in this register */

#endif
