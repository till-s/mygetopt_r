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

#define OFFS_AD6620_MCR		(0x1800)
#define BITS_AD6620_MCR_RESET	(1<<0)

#endif
