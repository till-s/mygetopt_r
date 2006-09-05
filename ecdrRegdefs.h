/* $Id$ */
#ifndef ECDR_REG_BITS_DEFS_H
#define ECDR_REG_BITS_DEFS_H

#include "drvrEcdr814.h"
#include "ecErrCodes.h"

#include <basicIoOps.h>

/*
 * Register Bit definitions.
 *
 * We try to avoid these at all and use the
 * directory lookup engine whenever possible.
 * However, for some lowlevel routines it's sometimes
 * necessary to directly access registers
 *
 * Essentially, the directory is designed
 * to work your way into register space.
 * However, there is no way of accessing
 * ".." which is needed by some lowlevel
 * routines.
 */

/* calculate some base addresses...
 * note that we must not cast this to a
 * pointer at this point (especially, a non char*)
 * because further calculations may be done
 * by the user of the macro
 *
 * also note that the XXXBASE macros needs an
 * address argument that points somewhere INTO
 * the register space already!
 */

#define ECDR_BRDREG_ALIGNMENT		(0x7ffff)
#define BRDREGBASE(addr)	(((unsigned long)(addr)) & ~ECDR_BRDREG_ALIGNMENT)

#define ECDR_RXPAIR_ALIGNMENT		(0xffff)
#define RXPAIRBASE(addr)	(((unsigned long)(addr)) & ~ECDR_RXPAIR_ALIGNMENT)

#define ECDR_AD6620_ALIGNMENT		(0x1fff)
#define AD6620BASE(addr)	(((unsigned long)(addr)) & ~ECDR_AD6620_ALIGNMENT)


#define ECDR_AD6620_MCR_ALIGNMENT	(0x7ff)
#define AD6620_MCRBASE(addr)	(((unsigned long)(addr)) & ~ECDR_AD6620_MCR_ALIGNMENT)

#define BITS_AD6620_MCR_RESET	(1<<0)
#define AD6620_MCR_OFFSET	0x1800

/* there's no point putting these into the database
 * as they are only used by lowlevel routines
 */
#define CY961_OFFSET		(0x8000)
#define FIFO_OFFSET		(0x100000)

/* the interrupt status register */
#define ECDR_INT_STAT_OFFSET	(0x10)
#define ECDR_INT_STAT_MSK	(0x7ff)

/* the interrupt mask register   */
#define ECDR_INT_MSK_OFFSET		(0x0c)

/* address range spanned by the fifo in ECDRs map */
#define ECDR_FIFO_ADDR_RANGE	(0x300000)

#if 0
#define BITS_AD6620_MCR_MASK	((1<<8)-1)	/* bits that are actually used in this register */
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern EcErrStat
ad6620ConsistencyCheck(EcBoardDesc bd, EcNode n);

/* low level operations */
#if defined(__PPC) || defined(_ARCH_PPC)
#define EIEIO __asm__ __volatile__("eieio");
#else
#define EIEIO
#endif

/* read big endian word; note that the 'volatile' keyword
 * is essential. Otherwise, the compiler might generate
 * a half word aligned access in a statement like
 *  blah = RDBE(ptr) & 0xffff;
 * which the ECDR doesnt like at all;
 */
#include <ctype.h>

#if 0
#if BYTE_ORDER == BIG_ENDIAN
#define RDBE(addr) (*((volatile Val_t *)(addr)))
#define WRBE(val, addr) (*((volatile Val_t *)(addr))=(val))
#elif BYTE_ORDER == LITTLE_ENDIAN
#include <netinet/in.h>
#define RDBE(addr) ntohl(*((volatile Val_t *)(addr)))
#define WRBE(val, addr) (*((volatile Val_t *)(addr))=htonl(val))
#else
#error "unable to determine endianness of your machine from system headers"
#endif
#endif

#define RDBE(addr)		in_be32((volatile uint32_t *)(addr))
#define WRBE(val,addr)	out_be32((volatile uint32_t *)(addr),(uint32_t)(val))

#ifdef __cplusplus
};
#endif


#endif
