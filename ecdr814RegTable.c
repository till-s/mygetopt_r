/* $Id$ */

#include "drvrEcdr814.h"

#include "menuDefs.h"

#define RO 	EcFlgReadOnly
#define NI	EcFlgNoInit
#define AR	EcFlgArray
#define Mrc	EC_MENU_realCmplx
#define Mtm	EC_MENU_trigMode
#define Mms	EC_MENU_modeSelect
#define Mrm	EC_MENU_rdbackMode
#define Mfr	EC_MENU_freqRange
#define Mcm	EC_MENU_clockMult
#define Mro	EC_MENU_resetRunning
#define Mpr	EC_MENU_dualRxMode
#define RST	EcFlgAD6620RStatic
#define RWST	EcFlgAD6620RWStatic

/* initial value for burst size */
#define BURSTINI	128

#define NUM_C	ECDR814_NUM_RCF_COEFFS

/* NOTE: the epics record support layer gives the '.' in some field
 * names a special meaning: it is a way to express that fields at a
 * 'outer level' logically belong to a different place.
 *
 * E.g. if a particular receiver is addressed  .../01/0A/xxxfield
 * and it has an associated field in its 'parent' channel pair
 * register, that field may be addressed by converting '/' to '.':
 *
 *  .../01/0A.syncEna
 */

#define REGUNLMT( pos1, pos2, flags, inival, min, max, adj) { r: { u: { s: { (pos1), (pos2) } } , (flags), (inival), (min), (max), (adj)} }
#define REGUNION( pos1, pos2, flags, inival) REGUNLMT( (pos1), (pos2), (flags), (inival), 0, 0, 0 )
#define REGUNBIT( pos1, pos2, flags, inival) REGUNLMT( (pos1), (pos2), (flags), (inival), 0, 1, 0 )
#define REGARRAY( len, flags, inival, min, max, adj ) { r: { u: { a: { (len) } } , ((flags) | EcFlgArr), (inival), (min), (max), (adj)} }
#define REGARRNL( len, flags, inival ) { r: { u: { a: { (len) } } , ((flags) | EcFlgArray), (inival), 0, 0, 0 } }

static EcCNodeRec ad6620RawRegDefs[] = {
	/* name,	node type,	offset,		bitmask	 	flags	initialization	min value, max value, value offset */
	/* 						start,end		value						   */
{	"rcfCoeffs",	EcDblReg,	0x0000, REGARRNL( NUM_C,        AR|RWST|NI,0)           },
{	"rcfData",	EcDblReg,	0x0800, REGARRNL( NUM_C,        AR|RST|RWST|NI,0)           },
{	"mcr",		EcAD6620MCR,	0x1800,	REGUNION( 0, 32, 	0,	1)		},	/* leave in reset */
{	"ncoCtl",	EcDblReg,	0x1808,	REGUNION( 0, 32, 	RWST,	0)		},
{	"ncoSyncMsk",EcDblReg,	0x1810,	REGUNION( 0, 32, 	0,	0)		},
{	"ncoFreq",	EcDblReg,	0x1818,	REGUNION( 0, 32, 	0,	0)		},
{	"ncoPhase",	EcDblReg,	0x1820,	REGUNION( 0, 32, 	0,	0)		},
{	"cic2Scale",EcDblReg,	0x1828,	REGUNION( 0, 32, 	0,	0)		},
{	"cic2Decm",	EcDblReg,	0x1830,	REGUNION( 0, 32, 	RWST,	0)		},
{	"cic5Scale",EcDblReg,	0x1838,	REGUNION( 0, 32, 	0,	0)		},
{	"cic5Decm",	EcDblReg,	0x1840,	REGUNION( 0, 32, 	RWST,	0)		},
{	"rcfCtl",	EcDblReg,	0x1848,	REGUNION( 0, 32, 	0,	0)		},
{	"rcfDecm",	EcDblReg,	0x1850,	REGUNION( 0, 32, 	RWST,	0)		},
{	"rcf1stTap",EcDblReg,	0x1858,	REGUNION( 0, 32, 	0,	0)		},
{	"rcfNTaps",	EcDblReg,	0x1860,	REGUNION( 0, 32, 	RWST,	0)		},
{	"reserved",	EcDblReg,	0x1868,	REGUNION( 0, 32, 	RWST,	0)		}, /* MUST be initialized to 0 */
};

/* we allow writes to these registers only when "reset" is asserted */
static EcCNodeRec ad6620RegDefs[] = {
	/* name,	node type,	offset,		bitmask	 	flags	initialization	*/
	/* 						start,end		value		*/
{	"rcfCoeffs",EcDblReg,	0x0000, REGARRNL( NUM_C,        AR|RWST|NI,0)           },
{	"rcfData",	EcDblReg,	0x0800, REGARRNL( NUM_C,        AR|RST|RWST|NI,0)           },
{	"state",	EcAD6620MCR,	0x1800,	REGUNBIT( 0, 1,		Mro,	1)		},	/* init: leave in reset */
{	"realCmplx",EcDblReg,	0x1800,	REGUNION( 1, 3,		RWST|Mrc,0)		},	/* treat bits other than reset as ordinary RWST regs */
{	"syncMaster",EcDblReg,	0x1800,	REGUNLMT( 3, 8,		RWST,	0,		0,	1,	0)},
{	"ncoBypass",EcDblReg,	0x1808,	REGUNBIT( 0, 1,		RWST,	0)		},	/* off */
{	"phaseDith",EcDblReg,	0x1808,	REGUNBIT( 1, 2,		RWST,	0)		},	/* off */
{	"ampDith",	EcDblReg,	0x1808,	REGUNBIT( 2, 3,		RWST,	0)		},	/* off */
{	"ncoSyncMsk",EcDblReg,	0x1810,	REGUNION( 0, 32,	0,	0xffffffff)		},
{	"ncoFreq",	EcDblReg,	0x1818,	REGUNION( 0, 32,	0,	0)		},
{	"ncoPhase",	EcDblReg,	0x1820,	REGUNION( 0, 32,	0,	0)		},
{	"cic2Scale",EcDblReg,	0x1828,	REGUNLMT( 0, 3,		0,	0,		0,	8,	0)},
{	"inpExpInv",EcDblReg,	0x1828,	REGUNBIT( 4, 5,		0,	0)		},
{	"inpExpOff",EcDblReg,	0x1828,	REGUNLMT( 5, 8,		0,	0,		0,	8,	0)},
{	"cic2Decm",	EcDblReg,	0x1830,	REGUNLMT( 0, 8,		RWST,	2,		1,	16,	-1)},
{	"cic5Scale",EcDblReg,	0x1838,	REGUNLMT( 0, 5,		0,	0,		0,	20,	0)},
{	"cic5Decm",	EcDblReg,	0x1840,	REGUNLMT( 0, 8,		RWST,	1,		1,	32,	-1)},
{	"rcfScale",	EcDblReg,	0x1848,	REGUNLMT( 0, 8,		0,	4,		0,	7,	0)},
{	"rcfDecm",	EcDblReg,	0x1850,	REGUNLMT( 0, 8,		RWST,	1,		1,	256,	-1)},
{	"rcf1stTap",EcDblReg,	0x1858,	REGUNLMT( 0, 8,		0,	0,		0,	255,	0)},
{	"rcfNTaps",	EcDblReg,	0x1860,	REGUNLMT( 0, 8,		RWST,	1,		1,	256,	-1)},	/* TODO:  wraparound? */
};

static EcCNodeDirRec ad6620Dir = {EcdrNumberOf(ad6620RegDefs), ad6620RegDefs, "ad6620"};
static EcCNodeDirRec ad6620RawDir = {EcdrNumberOf(ad6620RawRegDefs), ad6620RawRegDefs, "raw_ad6620"};

static EcCNodeRec ecdrChannelRegDefs[] = {
{	"trigMode",	EcReg,		0x0,	REGUNION( 1, 4,		Mtm|NI,	0)		},			/* set by the readback mode */
{	"counterEna",	EcReg,		0x0,	REGUNBIT( 4, 5,		0,	0)		},
{	"rxParallel",	EcReg,		0x0,	REGUNBIT( 5, 6,		Mpr,	1)		},
{	"trigClear",	EcReg,		0x0,	REGUNBIT( 15, 16,	0,	0)		},
{	"modeSelect",	EcReg,		0x4,	REGUNION( 0, 3,		Mms,	0)		},
{	"channelID",	EcReg,		0x4,	REGUNLMT( 3, 6,		NI,	0,		0,	7,	0)},	/* filled in at driver ini */
{	"nAccum",	EcReg,		0x4,	REGUNLMT( 6, 14,	0,	1,		1,	256,	-1)},
{	"burstCnt",	EcBrstCntReg,	0x0,	REGUNLMT( 0, 17,	0,	BURSTINI,		1,	0x20000,-1)},	/* TODO special values depending on accu mode */
{	"totDecm_2",	EcReg,		0xc,	REGUNLMT( 0, 14,	0,	1,		1,	16384,  -1)},	/* TODO: special, depends on AD settings: they calculate from cics and rcf, check <=16383 */
};

static EcCNodeRec ecdrChannelRawRegDefs[] = {
{	"rctla",	EcReg,		0x0,	REGUNION( 0, 32,	0,	0)		},
{	"rctlb",	EcReg,		0x4,	REGUNION( 0, 32,	0,	0)		},
{	"dws",		EcReg,		0x8,	REGUNION( 0, 32,	0,	0)		},
{	"rdec",		EcReg,		0xc,	REGUNION( 0, 32,	0,	0)		},
};

static EcCNodeDirRec ecdrChannelDir = {EcdrNumberOf(ecdrChannelRegDefs), ecdrChannelRegDefs, "channel"};
static EcCNodeDirRec ecdrChannelRawDir = {EcdrNumberOf(ecdrChannelRawRegDefs), ecdrChannelRawRegDefs, "raw_channel"};

static EcCNodeRec ecdrChannelPairRegDefs[] = {
{	"clockSame",	EcReg,		0x0,	REGUNBIT( 0, 1,		NI,	0)		},	/* TODO related to clk multiplier ? */
{	"ncoSyncEna",	EcReg,		0x0,	REGUNBIT( 1, 2,		0,	1)		},
{	"fifoOffset",	EcFifoReg,	0x0,	REGUNLMT( 0, 16,	NI,	16,		0,	0xffff,	0)}, /* set by the readback mode */
{	"C0.ncoSync",	EcReg,		0x0,	REGUNBIT( 6, 7,		0,	0)		},
{	"C1.ncoSync",	EcReg,		0x0,	REGUNBIT( 7, 8,		0,	0)		},
{	"0A.rxReset",	EcReg,		0x0,	REGUNBIT( 8, 9,		0,	1)		},	/* reset at startup */
{	"0B.rxReset",	EcReg,		0x0,	REGUNBIT( 9, 10,	0,	1)		},
{	"1A.rxReset",	EcReg,		0x0,	REGUNBIT( 10, 11,	0,	1)		},
{	"1B.rxReset",	EcReg,		0x0,	REGUNBIT( 11, 12,	0,	1)		},
{	"C0.fifoRst",	EcReg,		0x0,	REGUNBIT( 12, 13,	0,	1)		},
{	"C1.fifoRst",	EcReg,		0x0,	REGUNBIT( 13, 14,	0,	1)		},
{	"C0",		EcDir,		0x10,	{ d: {&ecdrChannelDir} }		},
{	"C1",		EcDir,		0x20,	{ d: {&ecdrChannelDir} }		},
{	"C0.attnEna",	EcReg,		0x30,	REGUNBIT( 0, 1,		0,	0)		},
{	"C1.attnEna",	EcReg,		0x30,	REGUNBIT( 1, 2,		0,	0)		},
{	"C0.attn",	EcReg,		0x30,	REGUNLMT( 2, 7,		0,	0,		-7,	24,	7)},
{	"C1.attn",	EcReg,		0x30,	REGUNLMT( 7, 12,	0,	0,		-7,	24,	7)},
{	"C0.fifoDly",	EcDblReg,	0x38,	REGUNLMT( 0, 24,	EcFlgP2Enbl,	0,		0,	(1<<24)-1,	0)},
{	"C1.fifoDly",	EcDblReg,	0x40,	REGUNLMT( 0, 24,	EcFlgP2Enbl,	0,		0,	(1<<24)-1,	0)},
{	"C0.fwVersion",	EcReg,	0x48,	REGUNLMT( 0, 16,	RO|NI,	0,		0,	0xffff,	0)},	/* R/O reg */
{	"C1.fwVersion",	EcReg,	0x48,	REGUNLMT( 0, 16,	RO|NI,	0,		0,	0xffff,	0)},	/* R/O reg */
/* Treating fifoWCnt as a EcFlgP2Enbl register has the side-effect of enabling this register as an IRQ
 * source if it is non-zero
 */
{	"C0.fifoWCnt",	EcDblReg,	0x4c,	REGUNLMT( 0, 17,	EcFlgP2Enbl,	BURSTINI,		0,	(1<<17)-1,	0)},
{	"C1.fifoWCnt",	EcDblReg,	0x54,	REGUNLMT( 0, 17,	EcFlgP2Enbl,	BURSTINI,		0,	(1<<17)-1,	0)},
{	"0A",		EcDir,		0x2000,	{ d: {&ad6620Dir} }		},
{	"0B",		EcDir,		0x4000,	{ d: {&ad6620Dir} }		},
{	"1A",		EcDir,		0x6000,	{ d: {&ad6620Dir} }		},
{	"1B",		EcDir,		0x8000,	{ d: {&ad6620Dir} }		},
};

static EcCNodeRec ecdrChannelPairRawRegDefs[] = {
{	"csr",		EcReg,		0x0,	REGUNION( 0, 32,	0,	0)		},
{	"C0",		EcDir,		0x10,	{ d: {&ecdrChannelRawDir} }	},
{	"C1",		EcDir,		0x20,	{ d: {&ecdrChannelRawDir} }	},
{	"attn",		EcReg,		0x30,	REGUNION( 0, 32,	0,	0)		},
{	"C0.fifoDly",EcDblReg,	0x38,	REGUNION( 0, 32,	0,	0)		},
{	"C1.fifoDly",EcDblReg,	0x40,	REGUNION( 0, 32,	0,	0)		},
{	"fifoOffset",	EcReg,	0x40,	REGUNION( 0, 32,	0,	0)		},
{	"fwVersion",	EcReg,	0x48,	REGUNION( 0, 32,	RO,	0)		},
{	"C0.fifoWCnt",EcDblReg,	0x4c,	REGUNION( 0, 32,	0,	0)		},
{	"C1.fifoWCnt",EcDblReg,	0x54,	REGUNION( 0, 32,	0,	0)		},
{	"0A",		EcDir,		0x2000,	{ d: {&ad6620RawDir} }		},
{	"0B",		EcDir,		0x4000,	{ d: {&ad6620RawDir} }		},
{	"1A",		EcDir,		0x6000,	{ d: {&ad6620RawDir} }		},
{	"1B",		EcDir,		0x8000,	{ d: {&ad6620RawDir} }		},
};

static EcCNodeDirRec ecdrChannelPairDir = {EcdrNumberOf(ecdrChannelPairRegDefs), ecdrChannelPairRegDefs, "channelPair"};
static EcCNodeDirRec ecdrChannelPairRawDir = {EcdrNumberOf(ecdrChannelPairRawRegDefs), ecdrChannelPairRawRegDefs, "raw_channelPair"};

static EcCNodeRec fifoStatRegDefs[] = {
{	"ch0",		EcReg,		0x0,	REGUNION( 0, 4,		RO,	0)		},
{	"ch1",		EcReg,		0x0,	REGUNION( 4, 8,		RO,	0)		},
{	"ch2",		EcReg,		0x0,	REGUNION( 8, 12,	RO,	0)		},
{	"ch3",		EcReg,		0x0,	REGUNION( 12, 16,	RO,	0)		},
{	"ch4",		EcReg,		0x0,	REGUNION( 16, 20,	RO,	0)		},
{	"ch5",		EcReg,		0x0,	REGUNION( 20, 24,	RO,	0)		},
{	"ch6",		EcReg,		0x0,	REGUNION( 24, 28,	RO,	0)		},
{	"ch7",		EcReg,		0x0,	REGUNION( 28, 32,	RO,	0)		},
};

static EcCNodeDirRec fifoStatDir = { EcdrNumberOf(fifoStatRegDefs), fifoStatRegDefs, "fifoStat" };

static EcCNodeRec ecdrBoardRegDefs[] = {
{	"rdbackMode",	EcReg,		0x0,	REGUNION( 0, 4,		Mrm,	0)		},	/* LR, affects fifoFlags (if !offset): 1chan: OFFSET_16, 2 channels: OFFSET 8, 4 channels: OFFSET 4, 8channels: OFFSET 2, unused channels get their syncEna and gateEna cleared  pair uses 01.0 and 23.0, 4 channels uses 01.0, 23.0, 45.0, 67.0 */
{	"headerEna",	EcReg,		0x0,	REGUNBIT( 4, 5,		0,	0)		},
{	"vmeOnly",	EcReg,		0x0,	REGUNBIT( 5, 6,		0,	0)		},
{	"d64_2kEna",	EcReg,		0x0,	REGUNBIT( 6, 7,		0,	0)		},
{	"freqRange",	EcReg,		0x0,	REGUNION( 7, 9,		Mfr,	2)		},
{	"extSyncEna",	EcReg,		0x0,	REGUNBIT( 9, 10,	0,	0)		},
{	"swSync",	EcReg,		0x0,	REGUNBIT( 10, 11,	0,	0)		},
{	"pktCntRst",	EcReg,		0x0,	REGUNBIT( 11, 12,	0,	0)		},
{	"lclBusRst",	EcReg,		0x0,	REGUNBIT( 12, 13, 	0,	0)		},
{	"clkMult03",	EcReg,		0x0,	REGUNION( 17, 19,	Mcm,	0)		},	/* TODO wait 15ms */ /* this sets associated clockSame */
{	"clkMult47",	EcReg,		0x0,	REGUNION( 19, 21,	Mcm,	0)		},	/* TODO wait 15ms */ /* this sets associated clockSame */
{	"fifoStat",	EcDir,		0x4,	{ d: {&fifoStatDir} }		},
{	"intVec",	EcReg,		0x8,	REGUNLMT( 0, 8,		0,	0,		0,	255,	0)},
{	"intMsk",	EcReg,		0xc,	REGUNLMT( 0, 11,	0,	0,		0,	0x7ff,	0)},
{	"intStat",	EcReg,		0x10,	REGUNLMT( 0, 11,	RO,	0,		0,	0x7ff,	0)},
{	"pktCnt",	EcReg,		0x14,	REGUNION( 0, 24,	RO,	0)		},
{	"adcOvrRng",	EcReg,		0x14,	REGUNION(24, 32,	RO,	0)		},
{	"extHdr",	EcReg,		0x18,	REGUNION( 0, 16,	RO,	0)		},
{	"fifoOvfl",	EcReg,		0x18,	REGUNION(16, 24,	RO,	0)		},
{	"fifoUdfl",	EcReg,		0x18,	REGUNION(24, 32,	RO,	0)		},
{	"fwVersion",EcReg,		0x1c,	REGUNION(0, 8,	RO,	0)		},
{	"brdVersion",EcReg,		0x1c,	REGUNION(8, 16,	RO,	0)		},
{	"01",		EcDir,		0x10000,{ d: {&ecdrChannelPairDir} }	},
{	"23",		EcDir,		0x20000,{ d: {&ecdrChannelPairDir} }	},
{	"45",		EcDir,		0x30000,{ d: {&ecdrChannelPairDir} }	},
{	"67",		EcDir,		0x40000,{ d: {&ecdrChannelPairDir} }	},
};

static EcCNodeRec ecdrBoardRawRegDefs[] = {
{	"csr",		EcReg,		0x0,	REGUNION( 0, 32,	0,	0)		},
{	"flags",	EcReg,		0x4,	REGUNION( 0, 32,	RO,	0)		},
{	"intVec",	EcReg,		0x8,	REGUNION( 0, 32,	0,	0)		},
{	"intMsk",	EcReg,		0xc,	REGUNION( 0, 32,	0,	0)		},
{	"intStat",	EcReg,		0x10,	REGUNION( 0, 32,	RO,	0)		},
{	"head0",	EcReg,		0x14,	REGUNION( 0, 32,	RO,	0)		},
{	"head1",	EcReg,		0x18,	REGUNION( 0, 32,	RO,	0)		},
{	"version",	EcReg,		0x1c,	REGUNION( 0, 32,	RO,	0)		},
{	"01",		EcDir,		0x10000,{ d: {&ecdrChannelPairRawDir} }	},
{	"23",		EcDir,		0x20000,{ d: {&ecdrChannelPairRawDir} }	},
{	"45",		EcDir,		0x30000,{ d: {&ecdrChannelPairRawDir} }	},
{	"67",		EcDir,		0x40000,{ d: {&ecdrChannelPairRawDir} }	},
};

static EcCNodeDirRec ecdrBoardDir = {EcdrNumberOf(ecdrBoardRegDefs), ecdrBoardRegDefs, "board"};
static EcCNodeDirRec ecdrBoardRawDir = {EcdrNumberOf(ecdrBoardRawRegDefs), ecdrBoardRawRegDefs, "raw_board"};

EcCNodeRec	ecdr814CInfo = {
	"/",		EcDir,		0x0,	{ d: {&ecdrBoardDir}},
};
EcCNodeRec	ecdr814RawCInfo = {
	"/",		EcDir,		0x0,	{ d: {&ecdrBoardRawDir}},
};

#undef RO 
#undef NI
#undef AR
#undef Mrc
#undef Mtm
#undef Msm
#undef Mrm
#undef Mfr
#undef Mcm
#undef Mro
#undef Mpr
#undef RST
#undef RWST
#undef NUM_C
