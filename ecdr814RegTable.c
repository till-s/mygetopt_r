/* $Id$ */

#include "drvrEcdr814.h"

static EcNodeRec ad6620RawRegDefs[] = {
	/* name,	node type,	offset,		bitmask	 RdOnly flags	initialization	*/
	/* 						start,end		value		*/
{	"mcr",		EcAD6620Reg,	0x1800,	REGUNION( 0, 32,  0,	0,	1)		},	/* leave in reset */
{	"ncoCtl",	EcAD6620Reg,	0x1808,	REGUNION( 0, 32,  0,	0,	0)		},
{	"ncoSyncMsk",	EcAD6620Reg,	0x1810,	REGUNION( 0, 32,  0,	0,	0)		},
{	"ncoFreq",	EcAD6620Reg,	0x1818,	REGUNION( 0, 32,  0,	0,	0)		},
{	"ncoPhase",	EcAD6620Reg,	0x1820,	REGUNION( 0, 32,  0,	0,	0)		},
{	"cic2Scale",	EcAD6620Reg,	0x1828,	REGUNION( 0, 32,  0,	0,	0)		},
{	"cic2Decm",	EcAD6620Reg,	0x1830,	REGUNION( 0, 32,  0,	0,	0)		},
{	"cic5Scale",	EcAD6620Reg,	0x1838,	REGUNION( 0, 32,  0,	0,	0)		},
{	"cic5Decm",	EcAD6620Reg,	0x1840,	REGUNION( 0, 32,  0,	0,	0)		},
{	"rcfCtl",	EcAD6620Reg,	0x1848,	REGUNION( 0, 32,  0,	0,	0)		},
{	"rcfDecm",	EcAD6620Reg,	0x1850,	REGUNION( 0, 32,  0,	0,	0)		},
{	"rcf1stTap",	EcAD6620Reg,	0x1858,	REGUNION( 0, 32,  0,	0,	0)		},
{	"rcfNTaps",	EcAD6620Reg,	0x1860,	REGUNION( 0, 32,  0,	0,	0)		},
};

static EcNodeRec ad6620RegDefs[] = {
	/* name,	node type,	offset,		bitmask	 RdOnly flags	initialization	*/
	/* 						start,end		value		*/
{	"reset",	EcAD6620Reg,	0x1800,	REGUNION( 0, 1,	  0,	0,	1)		},	/* leave in reset */
{	"realcmplx",	EcAD6620Reg,	0x1800,	REGUNION( 1, 3,	  0,	0,	2)		},	/* TODO: special combination - single chan, cmplx */
{	"syncMaster",	EcAD6620Reg,	0x1800,	REGUNION( 3, 8,	  0,	0,	0)		},	/* single chan, cmplx, should not set, 4-7 must be written low */
{	"ncoBypass",	EcAD6620Reg,	0x1808,	REGUNION( 0, 1,	  0,	0,	0)		},	/* off */
{	"phaseDith",	EcAD6620Reg,	0x1808,	REGUNION( 1, 2,	  0,	0,	0)		},	/* off */
{	"ampDith",	EcAD6620Reg,	0x1808,	REGUNION( 2, 3,	  0,	0,	0)		},	/* off */
{	"ncoSyncMsk",	EcAD6620Reg,	0x1810,	REGUNION( 0, 32,  0,	0,	0)		},
{	"ncoFreq",	EcAD6620Reg,	0x1818,	REGUNION( 0, 32,  0,	0,	0)		},
{	"ncoPhase",	EcAD6620Reg,	0x1820,	REGUNION( 0, 32,  0,	0,	0)		},
{	"cic2Scale",	EcAD6620Reg,	0x1828,	REGUNION( 0, 3,   0,	0,	0)		},
{	"inpExpInv",	EcAD6620Reg,	0x1828,	REGUNION( 4, 5,   0,	0,	0)		},	/* TODO exponent mapping */
{	"inpExpOff",	EcAD6620Reg,	0x1828,	REGUNION( 5, 8,   0,	0,	0)		},	/* TODO exponent mapping */
{	"cic2Decm",	EcAD6620Reg,	0x1830,	REGUNION( 0, 8,   0,	0,	0)		},	/* TODO n-1 */
{	"cic5Scale",	EcAD6620Reg,	0x1838,	REGUNION( 0, 5,   0,	0,	0)		},	/* TODO LR 0..20 */
{	"cic5Decm",	EcAD6620Reg,	0x1840,	REGUNION( 0, 8,   0,	0,	0)		},	/* TODO n-1, should be less than 31 */
{	"rcfScale",	EcAD6620Reg,	0x1848,	REGUNION( 0, 8,   0,	0,	0)		},	/* TODO: formula, bit 3-7 must be written 0 */
{	"rcfDecm",	EcAD6620Reg,	0x1850,	REGUNION( 0, 8,   0,	0,	0)		},	/* TODO: range check, limitation (speed) should be less than 32 */
{	"rcf1stTap",	EcAD6620Reg,	0x1858,	REGUNION( 0, 8,   0,	0,	0)		},	/* TODO: range check: wraparound? */
{	"rcfNTaps",	EcAD6620Reg,	0x1860,	REGUNION( 0, 8,   0,	0,	0)		},	/* TODO: range check: wraparound? */
};

static EcNodeDirRec ad6620Dir = {EcdrNumberOf(ad6620RegDefs), ad6620RegDefs};
static EcNodeDirRec ad6620RawDir = {EcdrNumberOf(ad6620RawRegDefs), ad6620RawRegDefs};

static EcNodeRec ecdrChannelRegDefs[] = {
{	"brstCntMSB",	EcReg,		0x0,	REGUNION( 0, 1,   0,	0,	0)		},	/* TODO: special: MSB */
{	"syncEna",	EcReg,		0x0,	REGUNION( 1, 2,   0,	0,	0)		},	/* TODO: use with gateEna all syncs work */
{	"gateEna",	EcReg,		0x0,	REGUNION( 2, 3,   0,	0,	0)		},	/* TODO: disable freerun & swsync */
{	"freeRun",	EcReg,		0x0,	REGUNION( 3, 4,   0,	0,	0)		},
{	"counterEna",	EcReg,		0x0,	REGUNION( 4, 5,   0,	0,	0)		},
{	"dualRxPara",	EcReg,		0x0,	REGUNION( 5, 6,   0,	0,	0)		},
{	"trigClear",	EcReg,		0x0,	REGUNION( 15, 16, 0,	0,	0)		},
{	"modeSelect",	EcReg,		0x4,	REGUNION( 0, 3,   0,	0,	0)		},	/* raw AD */
{	"channelID",	EcReg,		0x4,	REGUNION( 3, 6,   0,	0,	0)		},
{	"nAccum",	EcReg,		0x4,	REGUNION( 6, 14,  0,	0,	0)		},
{	"brstCntLSB",	EcReg,		0x8,	REGUNION( 0, 16,  0,	0,	0)		},	/* TODO special values depending on accu mode */
{	"totalDecm",	EcReg,		0xc,	REGUNION( 0, 14,  0,	0,	0)		},	/* TODO: special, depends on AD settings */
};

static EcNodeRec ecdrChannelRawRegDefs[] = {
{	"rctla",	EcReg,		0x0,	REGUNION( 0, 32,  0,	0,	0)		},
{	"rctlb",	EcReg,		0x4,	REGUNION( 0, 32,  0,	0,	0)		},
{	"dws",		EcReg,		0x8,	REGUNION( 0, 32,  0,	0,	0)		},
{	"rdec",		EcReg,		0xc,	REGUNION( 0, 32,  0,	0,	0)		},
};

static EcNodeDirRec ecdrChannelDir = {EcdrNumberOf(ecdrChannelRegDefs), ecdrChannelRegDefs};
static EcNodeDirRec ecdrChannelRawDir = {EcdrNumberOf(ecdrChannelRawRegDefs), ecdrChannelRawRegDefs};

static EcNodeRec ecdrChannelPairRegDefs[] = {
{	"clockSame",	EcReg,		0x0,	REGUNION( 0, 1,  0,	0,	1)		},	/* TODO related to clk multiplier ? */
{	"ncoSyncEna",	EcReg,		0x0,	REGUNION( 1, 2,  0,	0,	1)		},
{	"fifoFlgOff",	EcReg,		0x0,	REGUNION( 2, 4,  0,	0,	0)		},	/* TODO related to offset, same for all channels ; depends on rdbackMode */
{	"fifoUseOff",	EcReg,		0x0,	REGUNION( 4, 5,	 0,	0,	0)		},	/* TODO */
{	"fifoFlgWri",	EcReg,		0x0,	REGUNION( 5, 6,	 0,	0,	1)		},	/* TODO: linked to other fifo stuff */
{	"nco0ABSync",	EcReg,		0x0,	REGUNION( 6, 7,	 0,	0,	0)		},
{	"nco1ABSync",	EcReg,		0x0,	REGUNION( 7, 8,	 0,	0,	0)		},
{	"rx0AReset",	EcReg,		0x0,	REGUNION( 8, 9,	 0,	0,	1)		},	/* reset at startup */
{	"rx0BReset",	EcReg,		0x0,	REGUNION( 9, 10, 0,	0,	1)		},
{	"rx1AReset",	EcReg,		0x0,	REGUNION( 10, 11,0,	0,	1)		},
{	"rx1BReset",	EcReg,		0x0,	REGUNION( 11, 12,0,	0,	1)		},
{	"fifo0Rst",	EcReg,		0x0,	REGUNION( 12, 13,0,	0,	1)		},
{	"fifo1Rst",	EcReg,		0x0,	REGUNION( 13, 14,0,	0,	1)		},
{	"C0",		EcDir,		0x10,	&ecdrChannelDir			},
{	"C1",		EcDir,		0x20,	&ecdrChannelDir			},
{	"gain0Ena",	EcReg,		0x30,	REGUNION( 0, 1,  0,	0,	0)		},
{	"gain1Ena",	EcReg,		0x30,	REGUNION( 1, 2,  0,	0,	0)		},
{	"gain0",	EcReg,		0x30,	REGUNION( 2, 7,  0,	0,	0)		},	/* TODO shifted values -12..39 */
{	"gain1",	EcReg,		0x30,	REGUNION( 7, 12, 0,	0,	0)		},	/* TODO shifted values */
{	"fifoOffset",	EcReg,		0x40,	REGUNION( 0, 16, 0,	0,	0)		},	/* TODO related to offset */
{	"0A",		EcDir,		0x2000,	&ad6620Dir,			},
{	"0B",		EcDir,		0x4000,	&ad6620Dir,			},
{	"1A",		EcDir,		0x6000,	&ad6620Dir,			},
{	"1B",		EcDir,		0x8000,	&ad6620Dir,			},
};

static EcNodeRec ecdrChannelPairRawRegDefs[] = {
{	"csr",		EcReg,		0x0,	REGUNION( 0, 32,  0,	0,	0)		},
{	"C0",		EcDir,		0x10,	&ecdrChannelRawDir		},
{	"C1",		EcDir,		0x20,	&ecdrChannelRawDir		},
{	"gain",		EcReg,		0x30,	REGUNION( 0, 32,  0,	0,	0)		},
{	"fifoOffset",	EcReg,		0x40,	REGUNION( 0, 32,  0,	0,	0)		},
{	"0A",		EcDir,		0x2000,	&ad6620RawDir,			},
{	"0B",		EcDir,		0x4000,	&ad6620RawDir,			},
{	"1A",		EcDir,		0x6000,	&ad6620RawDir,			},
{	"1B",		EcDir,		0x8000,	&ad6620RawDir,			},
};

static EcNodeDirRec ecdrChannelPairDir = {EcdrNumberOf(ecdrChannelPairRegDefs), ecdrChannelPairRegDefs};
static EcNodeDirRec ecdrChannelPairRawDir = {EcdrNumberOf(ecdrChannelPairRawRegDefs), ecdrChannelPairRawRegDefs};

static EcNodeRec ecdrBoardRegDefs[] = {
{	"rdbackMode",	EcReg,		0x0,	REGUNION( 0, 4,	  0,	0,	0)		},	/* LR, affects fifoFlags */
{	"headerEna",	EcReg,		0x0,	REGUNION( 4, 5,	  0,	0,	0)		},
{	"vmeOnly",	EcReg,		0x0,	REGUNION( 5, 6,	  0,	0,	0)		},
{	"d64_2kEna",	EcReg,		0x0,	REGUNION( 6, 7,	  0,	0,	0)		},
{	"freqRange",	EcReg,		0x0,	REGUNION( 7, 9,	  0,	0,	0)		},	/* unusual ordering */
{	"extSyncEna",	EcReg,		0x0,	REGUNION( 9, 10,  0,	0,	0)		},
{	"swSync",	EcReg,		0x0,	REGUNION( 10, 11, 0,	0,	0)		},
{	"pktCntRst",	EcReg,		0x0,	REGUNION( 11, 12, 0,	0,	0)		},
{	"lclBusRst",	EcReg,		0x0,	REGUNION( 12, 13, 0, 	0,	0)		},
{	"clkMult0_3",	EcReg,		0x0,	REGUNION( 17, 19, 0,	0,	0)		},	/* TODO unusual ordering, wait 15ms */
{	"clkMult4_7",	EcReg,		0x0,	REGUNION( 19, 21, 0,	0,	0)		},	/* TODO unusual ordering, wait 15ms */
{	"fifo0Stat",	EcReg,		0x4,	REGUNION( 0, 4,	  1,	0,	0)		},	/* TODO LR */
{	"fifo1Stat",	EcReg,		0x4,	REGUNION( 4, 8,	  1,	0,	0)		},	/* TODO LR */
{	"fifo2Stat",	EcReg,		0x4,	REGUNION( 8, 12,  1,	0,	0)		},	/* TODO LR */
{	"fifo3Stat",	EcReg,		0x4,	REGUNION( 12, 16, 1,	0,	0)		},	/* TODO LR */
{	"fifo4Stat",	EcReg,		0x4,	REGUNION( 16, 20, 1,	0,	0)		},	/* TODO LR */
{	"fifo5Stat",	EcReg,		0x4,	REGUNION( 20, 24, 1,	0,	0)		},	/* TODO LR */
{	"fifo6Stat",	EcReg,		0x4,	REGUNION( 24, 28, 1,	0,	0)		},	/* TODO LR */
{	"fifo7Stat",	EcReg,		0x4,	REGUNION( 28, 32, 1,	0,	0)		},	/* TODO LR */
{	"intVec",	EcReg,		0x8,	REGUNION( 0, 8,	  0,	0,	0)		},
{	"intCh0Msk",	EcReg,		0xc,	REGUNION( 0, 1,	  0,	0,	0)		},
{	"intCh1Msk",	EcReg,		0xc,	REGUNION( 1, 2,	  0,	0,	0)		},
{	"intCh2Msk",	EcReg,		0xc,	REGUNION( 2, 3,	  0,	0,	0)		},
{	"intCh3Msk",	EcReg,		0xc,	REGUNION( 3, 4,	  0,	0,	0)		},
{	"intCh4Msk",	EcReg,		0xc,	REGUNION( 4, 5,	  0,	0,	0)		},
{	"intCh5Msk",	EcReg,		0xc,	REGUNION( 5, 6,	  0,	0,	0)		},
{	"intCh6Msk",	EcReg,		0xc,	REGUNION( 6, 7,	  0,	0,	0)		},
{	"intCh7Msk",	EcReg,		0xc,	REGUNION( 7, 8,	  0,	0,	0)		},
{	"intCh7Msk",	EcReg,		0xc,	REGUNION( 7, 8,	  0,	0,	0)		},
{	"rwyTimMsk",	EcReg,		0xc,	REGUNION( 8, 9,	  0,	0,	0)		},
{	"rwyAMsk",	EcReg,		0xc,	REGUNION( 9, 10,  0,	0,	0)		},
{	"rwyBMsk",	EcReg,		0xc,	REGUNION( 10, 11, 0,	0,	0)		},
{	"intCh0Stat",	EcReg,		0x10,	REGUNION( 0, 1,	  1,	0,	0)		},
{	"intCh1Stat",	EcReg,		0x10,	REGUNION( 1, 2,	  1,	0,	0)		},
{	"intCh2Stat",	EcReg,		0x10,	REGUNION( 2, 3,	  1,	0,	0)		},
{	"intCh3Stat",	EcReg,		0x10,	REGUNION( 3, 4,	  1,	0,	0)		},
{	"intCh4Stat",	EcReg,		0x10,	REGUNION( 4, 5,	  1,	0,	0)		},
{	"intCh5Stat",	EcReg,		0x10,	REGUNION( 5, 6,	  1,	0,	0)		},
{	"intCh6Stat",	EcReg,		0x10,	REGUNION( 6, 7,	  1,	0,	0)		},
{	"intCh7Stat",	EcReg,		0x10,	REGUNION( 7, 8,	  1,	0,	0)		},
{	"rwyTimStat",	EcReg,		0x10,	REGUNION( 8, 9,	  1,	0,	0)		},
{	"rwyAStat",	EcReg,		0x10,	REGUNION( 9, 10,  1,	0,	0)		},
{	"rwyBStat",	EcReg,		0x10,	REGUNION( 10, 11, 1,	0,	0)		},
{	"pktCnt",	EcReg,		0x14,	REGUNION( 0, 24,  1,	0,	0)		},
{	"ch0OvrRng",	EcReg,		0x14,	REGUNION( 24, 25, 1,	0,	0)		},
{	"ch1OvrRng",	EcReg,		0x14,	REGUNION( 25, 26, 1,	0,	0)		},
{	"ch2OvrRng",	EcReg,		0x14,	REGUNION( 26, 27, 1,	0,	0)		},
{	"ch3OvrRng",	EcReg,		0x14,	REGUNION( 27, 28, 1,	0,	0)		},
{	"ch4OvrRng",	EcReg,		0x14,	REGUNION( 28, 29, 1,	0,	0)		},
{	"ch5OvrRng",	EcReg,		0x14,	REGUNION( 29, 30, 1,	0,	0)		},
{	"ch6OvrRng",	EcReg,		0x14,	REGUNION( 30, 31, 1,	0,	0)		},
{	"ch7OvrRng",	EcReg,		0x14,	REGUNION( 31, 32, 1,	0,	0)		},
{	"extHdr",	EcReg,		0x18,	REGUNION( 0, 16,  1,	0,	0)		},
{	"ch0FifoOfl",	EcReg,		0x18,	REGUNION( 16, 17, 1,	0,	0)		},
{	"ch1FifoOfl",	EcReg,		0x18,	REGUNION( 17, 18, 1,	0,	0)		},
{	"ch2FifoOfl",	EcReg,		0x18,	REGUNION( 18, 19, 1,	0,	0)		},
{	"ch3FifoOfl",	EcReg,		0x18,	REGUNION( 19, 20, 1,	0,	0)		},
{	"ch4FifoOfl",	EcReg,		0x18,	REGUNION( 20, 21, 1,	0,	0)		},
{	"ch5FifoOfl",	EcReg,		0x18,	REGUNION( 21, 22, 1,	0,	0)		},
{	"ch6FifoOfl",	EcReg,		0x18,	REGUNION( 22, 23, 1,	0,	0)		},
{	"ch7FifoOfl",	EcReg,		0x18,	REGUNION( 23, 24, 1,	0,	0)		},
{	"ch0FifoUfl",	EcReg,		0x18,	REGUNION( 24, 25, 1,	0,	0)		},
{	"ch1FifoUfl",	EcReg,		0x18,	REGUNION( 25, 26, 1,	0,	0)		},
{	"ch2FifoUfl",	EcReg,		0x18,	REGUNION( 26, 27, 1,	0,	0)		},
{	"ch3FifoUfl",	EcReg,		0x18,	REGUNION( 27, 28, 1,	0,	0)		},
{	"ch4FifoUfl",	EcReg,		0x18,	REGUNION( 28, 29, 1,	0,	0)		},
{	"ch5FifoUfl",	EcReg,		0x18,	REGUNION( 29, 30, 1,	0,	0)		},
{	"ch6FifoUfl",	EcReg,		0x18,	REGUNION( 30, 31, 1,	0,	0)		},
{	"ch7FifoUfl",	EcReg,		0x18,	REGUNION( 31, 32, 1,	0,	0)		},
{	"01",		EcDir,		0x10000,	&ecdrChannelPairDir,	},
{	"23",		EcDir,		0x20000,	&ecdrChannelPairDir,	},
{	"45",		EcDir,		0x30000,	&ecdrChannelPairDir,	},
{	"67",		EcDir,		0x40000,	&ecdrChannelPairDir,	},
};

static EcNodeRec ecdrBoardRawRegDefs[] = {
{	"csr",		EcReg,		0x0,	REGUNION( 0, 32,  0,	0,	0)		},
{	"flags",	EcReg,		0x4,	REGUNION( 0, 32,  1,	0,	0)		},
{	"intVec",	EcReg,		0x8,	REGUNION( 0, 32,  0,	0,	0)		},
{	"intMask",	EcReg,		0xc,	REGUNION( 0, 32,  0,	0,	0)		},
{	"intStat",	EcReg,		0x10,	REGUNION( 0, 32,  1,	0,	0)		},
{	"head0",	EcReg,		0x14,	REGUNION( 0, 32,  1,	0,	0)		},
{	"head1",	EcReg,		0x18,	REGUNION( 0, 32,  1,	0,	0)		},
{	"01",		EcDir,		0x10000,	&ecdrChannelPairRawDir,	},
{	"23",		EcDir,		0x20000,	&ecdrChannelPairRawDir,	},
{	"45",		EcDir,		0x30000,	&ecdrChannelPairRawDir,	},
{	"67",		EcDir,		0x40000,	&ecdrChannelPairRawDir,	},
};

static EcNodeDirRec ecdrBoardDir = {EcdrNumberOf(ecdrBoardRegDefs), ecdrBoardRegDefs};
static EcNodeDirRec ecdrBoardRawDir = {EcdrNumberOf(ecdrBoardRawRegDefs), ecdrBoardRawRegDefs};

EcNodeRec	ecdr814Board = {
	"/",		EcDir,		0x0,	&ecdrBoardDir
};
EcNodeRec	ecdr814RawBoard = {
	"/",		EcDir,		0x0,	&ecdrBoardRawDir
};
