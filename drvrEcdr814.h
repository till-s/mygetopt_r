/* $Id$ */

#ifndef DRV_ECDR814_TILL_H
#define DRV_ECDR814_TILL_H

#include "ecErrCodes.h"

/* number of array elements */
#define EcdrNumberOf(arr) (sizeof(arr)/sizeof(arr[0]))

#define EC_DIRSEP_CHAR		'/'
#define EC_BRDSEP_CHAR		':'
#define EC_GLOB_CHAR		'*'
#define EC_UPDIR_NAME		".."
#define EC_UPDIR_NAME_LEN	2

/* Node types */
typedef enum {
	EcDir = 0,
	EcReg,
	EcFifoReg,
	EcBrstCntReg,
	EcRdBckReg,
	EcAD6620Reg,
	EcAD6620MCR,
	EcAD6620RCF
} EcCNodeType;

typedef enum {
	EcFlgMenuMask	= (1<<5)-1,	/* uses a menu */
	EcFlgReadOnly	= (1<<8),	/* readonly register */
	EcFlgNoInit	= (1<<9),	/* do not initialize */
	EcFlgAD6620RStatic = (1<<10),	/* may only read if in reset state */
	EcFlgAD6620RWStatic = (1<<11),	/* may only write if in reset state */
	EcFlgArray	= (1<<12)	/* is an array pos1..pos2 */
} EcRegFlags;

/* pointer to physical device registers */
typedef volatile unsigned char *IOPtr;
typedef unsigned long Val_t;

#define FKEY_LEN	5	/* bits */
typedef unsigned long	EcFKey; /* fastkeys */
typedef char		*EcKey; /* string keys */
#define EMPTY_FKEY	((EcFKey)0)
#define UPDIR_FKEY	((EcFKey)((1<<FKEY_LEN)-1))
#define FK_PARENT	UPDIR_FKEY
#define EcFKeyIsEmpty(k)	(EMPTY_FKEY==(k))

#define EcKeyIsUpDir(key)	(0==strcmp(key,".."))
#define EcKeyIsEmpty(k)		(0==(k) || 0==*(k))
#define EcString2Key(k)		(k)

#define	ECDR814_NUM_RCF_COEFFS	4 /* testing */

/* struct describing a node
 * This can be a description of 
 *  - individual ECDR register bits (leaf node)
 *  - ECDR register array (leaf node)
 *  - "directory" of nodes (non-leaf, "dir" - node)
 */
typedef struct EcCNodeRec_ {
	char			*name;
#if 0
	EcCNodeType	t : 4;
	unsigned long	offset: 28;
#else
	EcCNodeType	t;
	unsigned long	offset;
#endif
	union {
		struct {
			struct EcCNodeDirRec_	*n;
		} d;					/* if an EcDir */
		struct {
			unsigned char		pos1;
			unsigned char		pos2;
			EcRegFlags 		flags : 16;
			unsigned long		inival;
			unsigned long   	min,max,adj;
		} r;					/* if a Reg or AD6620Reg */
	} u;
} EcCNodeRec, *EcCNode;

/* a directory of CNodes */
typedef struct EcCNodeDirRec_ {
	long		nels;
	EcCNode		nodes;
	char		*name;
} EcCNodeDirRec, *EcCNodeDir;

/* for building linked lists of CNodes */
typedef struct EcCNodeListRec_ {
	struct EcCNodeListRec_	*p;		/* 'parent' node */
	EcCNode			n;		/* 'this' node */
} EcCNodeListRec, *EcCNodeList;

#define EcCNodeIsDir(n)   (EcDir==(n)->t)
#define EcCNodeIsArray(n) (!EcCNodeIsDir(n) && (EcFlgArray & (n)->u.r.flags))

/* ("class") EcCNodes hold more data and there exists only _one_ instance
 * per register type, describing its properties.
 * EcNodes are smaller and there exists one instance for
 * each register of a particular type that exists on the board.
 * E.g: there are 16 'CIC2 decimation factor' registers on the
 * board. Hence there is one EcCNode describing the register
 * and there are 16 EcNodes for accessing the various instances.
 */

typedef struct EcNodeRec_ {
	EcCNode			  cnode;	/* description of this node */
	union		{
		struct EcNodeRec_ *entries;	/* a list of nodes if it's a directory */
		unsigned long	  offset;  	/* offset into the ECDR register space for leaf nodes */
	} u;
	struct EcNodeRec_	  *parent; 	/* our parent directory */
} EcNodeRec, *EcNode;

#define EcNodeIsDir(n)	(EcDir==(n)->cnode->t)
#define EcNodeIsArray(n) (!EcNodeIsDir(n) && (EcFlgArray & (n)->cnode->u.r.flags))

typedef struct EcBoardDescRec_ *EcBoardDesc;

/* access of leaf nodes */

EcErrStat
ecGetValue(EcBoardDesc bd, EcNode n, Val_t *prval);

EcErrStat
ecPutValue(EcBoardDesc bd, EcNode n, Val_t v);

EcErrStat
ecGetRawValue(EcBoardDesc bd, EcNode n, Val_t *prval);

EcErrStat
ecPutRawValue(EcBoardDesc bd, EcNode n, Val_t v);

EcErrStat
ecLkupNGet(EcBoardDesc bd, EcNode from, EcFKey fkey, Val_t *prval);

EcErrStat
ecLkupNPut(EcBoardDesc bd, EcNode from, EcFKey fkey, Val_t val);

#ifdef ECDR814_PRIVATE_IF
/* operations on a leaf node
 * NOT INTENDED FOR DIRECT USE. Only
 * implementations for new leaf nodes should
 * use this.
 */

#define ECREGMASK(n) (((1<<((n)->u.r.pos1))-1) ^ \
		       (((n)->u.r.pos2 & 31 ? 1<<(n)->u.r.pos2 : 0)-1))
#define ECREGPOS(n) ((n)->u.r.pos1)

typedef struct EcCNodeOpsRec_ {
	struct EcCNodeOpsRec_ *super;
	int			initialized; 	/* must be initialized to 0 */
	EcErrStat	(*get)(EcBoardDesc bd, EcNode n, Val_t *pv);
	EcErrStat	(*getRaw)(EcBoardDesc bd, EcNode n, Val_t *pv);
	EcErrStat	(*put)(EcBoardDesc bd, EcNode n, Val_t v);
	EcErrStat	(*putRaw)(EcBoardDesc bd, EcNode n, Val_t v);
} EcCNodeOpsRec, *EcCNodeOps;

void
addNodeOps(EcCNodeOps ops, EcCNodeType type);

extern EcCNodeOpsRec ecDefaultNodeOps;
#endif

/* node list record allocation primitives */
EcCNodeList
allocNodeListRec(void);

/* free a list record returning its "next" member */
EcCNodeList
freeNodeListRec(EcCNodeList ptr);

/* allocate a nodelist record, set entry to n
 * and prepend to l
 * RETURNS: new NodeListRec
 */
EcCNodeList
addEcCNode(EcCNode n, EcCNodeList l);


/* lookup a key adding all the offsets
 * of the traversed nodes to *p
 * Returns: 0 if the node is not found.
 * Note: the node may not be a leaf
 *
 * If a pointer to a EcCNodeListRec is passed (arg l),
 * ecCNodeLookup will record the traversed path updating
 * *l.
 * It is the responsibility of the caller to free 
 * list nodes allocated by ecCNodeLookup.
 *
 * Also, on request, a fast key is returned which
 * gives a short representation of the traversed
 * path
 */
EcCNode
ecCNodeLookup(EcCNode n, EcKey key, IOPtr *p, EcCNodeList *l);

/* same as ecCNodeLookup but using fast keys */
EcCNode
ecCNodeLookupFast(EcCNode n, EcFKey key, IOPtr *p, EcCNodeList *l);

/* this may be passed to ecNodeLookup to specify
 * the default root node
 */
#define EC_ROOT_LOOKUP	((EcNode)0)

/* lookup a node and a board descriptor (pbd may be NULL) */
EcNode
ecNodeLookup(EcNode n, EcKey key, EcBoardDesc *pbd);

/* lookup a node using fastkeys */
EcNode
ecNodeLookupFast(EcNode n, EcFKey fkey);

/* execute a function for every node in preorder.
 * The function is passed a EcCNodeList pointing to the 
 * node and all its parent directories.
 * For convenience, all offsets have been summed up and are
 * passed to "fn" as well. Note that the offset could
 * be calculated as 
 *	while (l) {
 *           offset+= l->n->offset;
 *           l = l->p;
 *      }
 */

typedef void (*EcCNodeWalkFn)(EcCNodeList l, IOPtr p, void *fnarg);
typedef void (*EcNodeWalkFn) (EcBoardDesc bd, EcNode l, void *fnarg);

void
ecCNodeWalk(EcCNode n, EcCNodeWalkFn fn, IOPtr p, EcCNodeList l, void *fnarg);

void
ecWalkNode(EcNode n, EcNodeWalkFn fn, IOPtr boardBase, void *fnarg);


/* node of the board info directory */
extern EcCNodeRec	ecdr814CInfo;
extern EcCNodeRec	ecdr814RawCInfo;

/* node of the board directory */
extern EcNode		ecdr814Board;

/* initialize the driver */
void drvrEcdr814Init(void);

typedef struct EcBoardDescRec_ {
	char		*name;		/* name of this board */
	IOPtr		base;		/* base address seen by CPU */
/* do not use fields below here; they are 'private' to the driver */
	IOPtr		vmeBase;	/* base address as seen on VME */
	EcNode		root;		/* root node of database */
} EcBoardDescRec;


/* register a board with the driver
 *
 * baseAddress is the board's address
 * on the VME bus (as set by the DIP switches)
 *
 * this routine returns a handle to a board
 * descriptor record. If unneeded, 0 may be passed.
 */
EcErrStat ecAddBoard(char *name, IOPtr baseAddress, EcBoardDesc *pdesc);

/* get the board descriptor of the nth board */
EcBoardDesc ecGetBoardDesc(int instance);
/* keyLen denotes the number of characters that
 * are compared
 * if keyLen is < 0 it is internally set to strlen(k)
 */
EcBoardDesc ecBoardDescLookup(EcKey k, int keyLen);

/* DMA stuff */

typedef unsigned long BEUlong; /* emphasize that it's big endian */

#define EC_CY961_SEMA_MASK	(0xff)
#define EC_CY961_SEMA_ERR_MASK	(0xfc)
#define EC_CY961_SEMA_BUSY	(1<<0)	/* read of sema sets this to 1 */
#define EC_CY961_SEMA_MIRQ	(1<<1)	/* master block irq pending    */
#define EC_CY961_SEMA_TLM0	(1<<2)	/* xfer multiplier is 0        */
#define EC_CY961_SEMA_TTUN	(1<<3)	/* undefined xfer type         */
#define EC_CY961_SEMA_DTSZ	(1<<4)	/* data size unaligned to addr */
#define EC_CY961_SEMA_ALGN	(1<<5)	/* address alignment violation */
#define EC_CY961_SEMA_VMEA	(1<<6)	/* no new VME starting address */
#define EC_CY961_SEMA_BERR	(1<<7)	/* BERR* or LERR*              */

#define EC_CY961_MCSR_IGOOD	(1<<0)	/* enable master block irq status (read) */

#ifdef ECDR814_PRIVATE_IF

#define VBEUlong volatile BEUlong
typedef struct EcDMARegsRec_ {
	/* register contents as used
	 * by the CY961
	 */
/* NOTE the bits in 'sema' are "active low"
 *      i.e. the bit must be tested for 0 value:
 *         is_busy =  ! (sema & EC_CY961_SEMA_BUSY);
 */
	VBEUlong	sema;	/* semaphore test & set                */
	VBEUlong	tlm0;	/* xfer length multiplier 0            */
	VBEUlong	tlm1;	/* xfer length multiplier 1            */
#define EC_CY961_XFRT_MTRD	(1<<0)
#define EC_CY961_XFRT_DTSZ_MSK	(3<<1)
#define EC_CY961_XFRT_DTSZ_D08	(0<<1)
#define EC_CY961_XFRT_DTSZ_D16	(1<<1)
#define EC_CY961_XFRT_DTSZ_D32	(2<<1)
#define EC_CY961_XFRT_DTSZ_D64	(3<<1)
#define EC_CY961_XFRT_TYPE_MSK	(0x1f<<3)
/* Note: not all possible modes are defined here */
#define EC_CY961_XFRT_TYPE_A32_SB    (0x10)  /* A32 SUP BLT            */
#define EC_CY961_XFRT_TYPE_A32_SD    (0xc8)  /* A32 SUP data           */
#define EC_CY961_XFRT_TYPE_A32_SB64  (0x50)  /* A32 64 bit BLT         */
	VBEUlong	xfrt;	/* transfer type                       */
	VBEUlong	ladr;   /* block (CY) local address and GO     */
	VBEUlong	vadr;	/* block VME address                   */
	VBEUlong	uadr;   /* VME A40/A64 upper address           */
#define EC_CY961_MCSR_IENA	(1<<0)	/* enable master block VME irq (write) */
	VBEUlong	mcsr;	/* master block status & irq           */
} EcDMARegsRec, *EcDMARegs;

#undef VBEUlong
#endif

/* NOTE:
 * 	The EcDMADescRec struct is declared here _only_
 *      for the purpose of memory allocation.
 *      Its fields should _not_ be accessed by the
 *	application in any way.
 */ 

typedef struct EcDMADescRec_ {
	BEUlong		tlm0;	/* xfer length multiplier 0            */
	BEUlong		tlm1;	/* xfer length multiplier 1            */
	BEUlong		xfrt;
	BEUlong		vadr;	/* block VME address                   */
	BEUlong		mcsr;	/* master block status & irq           */
} EcDMADescRec, *EcDMADesc;

/* initialize a DMA descriptor record
 * NOTE that the transfer size (and hence the fifo burst counts)
 * must be a multiple of 16*4 bytes (D32) or 64*4 bytes (D64)!
 */
typedef enum {
	EcDMA_D64=(1<<0),	/* do D64 transfer (as opposed to D32  */
	EcDMA_IEN=(1<<1)	/* enable IRQ when done; APP MUST INSTALL HANDLER */
} EcDMAFlags;

/* NOTE: the interrupt vector is the number returned by the CY961
 *       during a VME IACK cycle. The CY961 uses the LSB for status
 *       information, however ('1' meaning OK, '0' indicating a termination
 *       due to a bus error [local or VME]).
 */
EcErrStat
ecSetupDMADesc( EcDMADesc	desc,   /* descriptor to be initialized */
		void		*buffer,/* buffer address */
		int		size,   /* buffer size (in bytes) */
		unsigned char	vector, /* interrupt vector; LSB _must_ be 0 */
		EcDMAFlags	flags); /* see below */


/* start DMA on a board using buffer described
 * by d. d must have been initialized by ecSetupDMA before.
 */
EcErrStat
ecStartDMA( EcBoardDesc board, EcDMADesc d);

/* a fast way of reading the interrupt status register
 * without going through the database
 */
unsigned long
ecGetIntStat( EcBoardDesc bd);

/* get the sema status register of the CY961
 * NOTE: the returned value has been _inverted_, i.e.
 * the bit values are "active high"...
 * IMPORTANT: using this call might yield the semaphore
 * (return value & EC_CY961_SEMA_BUSY) == 0
 * In this case, it must be cleared by calling ecClearCYSema()
 */
unsigned long
ecGetCYSemaStat(EcBoardDesc bd);

/* clear the semaphore if the present status indicates
 * that we own it (the 'sema' parameter is the value
 * returned by a previous call to ecGetCYSemaStat()
 */
void
ecClearCYSema(EcBoardDesc bd, unsigned long sema);

/* read the Master Status ID (interrupt vector) register.
 * The EC_CY961_MCSR_IGOOD bit indicates if the interrupt
 * occurred due to successful DMA completion (1) or a
 * bus error (0).
 */
unsigned long
ecGetCYVector(EcBoardDesc bd);
#endif
