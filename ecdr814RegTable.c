/* $Id$ */

#include "drvrEcdr814.h"

#define RO 	EcFlgReadOnly
#define MNU	EcFlgMenuMask
#define RST	EcFlgAD6620RStatic
#define RWST	EcFlgAD6620RWStatic

static EcNodeRec ad6620RawRegDefs[] = {
	/* name,	node type,	offset,		bitmask	 	flags	initialization	min value, max value, value offset */
	/* 						start,end		value						   */
{	"mcr",		EcAD6620MCR,	0x1800,	REGUNION( 0, 32, 	0,	1)		},	/* leave in reset */
{	"ncoCtl",	EcAD6620Reg,	0x1808,	REGUNION( 0, 32, 	RWST,	0)		},
{	"ncoSyncMsk",	EcAD6620Reg,	0x1810,	REGUNION( 0, 32, 	0,	0)		},
{	"ncoFreq",	EcAD6620Reg,	0x1818,	REGUNION( 0, 32, 	0,	0)		},
{	"ncoPhase",	EcAD6620Reg,	0x1820,	REGUNION( 0, 32, 	0,	0)		},
{	"cic2Scale",	EcAD6620Reg,	0x1828,	REGUNION( 0, 32, 	0,	0)		},
{	"cic2Decm",	EcAD6620Reg,	0x1830,	REGUNION( 0, 32, 	RWST,	0)		},
{	"cic5Scale",	EcAD6620Reg,	0x1838,	REGUNION( 0, 32, 	0,	0)		},
{	"cic5Decm",	EcAD6620Reg,	0x1840,	REGUNION( 0, 32, 	RWST,	0)		},
{	"rcfCtl",	EcAD6620Reg,	0x1848,	REGUNION( 0, 32, 	0,	0)		},
{	"rcfDecm",	EcAD6620Reg,	0x1850,	REGUNION( 0, 32, 	RWST,	0)		},
{	"rcf1stTap",	EcAD6620Reg,	0x1858,	REGUNION( 0, 32, 	0,	0)		},
{	"rcfNTaps",	EcAD6620Reg,	0x1860,	REGUNION( 0, 32, 	RWST,	0)		},
};

/* we allow writes to these registers only when "reset" is asserted */
static EcNodeRec ad6620RegDefs[] = {
	/* name,	node type,	offset,		bitmask	 	flags	initialization	*/
	/* 						start,end		value		*/
{	"reset",	EcAD6620MCR,	0x1800,	REGUNBIT( 0, 1,		0,	1)		},	/* init: leave in reset */
{	"realCmplx",	EcAD6620Reg,	0x1800,	REGUNION( 1, 3,		RWST|MNU,2)		},	/* treat bits other than reset as ordinary RWST regs */
{	"syncMaster",	EcAD6620Reg,	0x1800,	REGUNLMT( 3, 8,		RWST,	0,		0,	1,	0)},
{	"ncoBypass",	EcAD6620Reg,	0x1808,	REGUNBIT( 0, 1,		RWST,	0)		},	/* off */
{	"phaseDith",	EcAD6620Reg,	0x1808,	REGUNBIT( 1, 2,		RWST,	0)		},	/* off */
{	"ampDith",	EcAD6620Reg,	0x1808,	REGUNBIT( 2, 3,		RWST,	0)		},	/* off */
{	"ncoSyncMsk",	EcAD6620Reg,	0x1810,	REGUNION( 0, 32,	0,	0)		},
{	"ncoFreq",	EcAD6620Reg,	0x1818,	REGUNION( 0, 32,	0,	0)		},
{	"ncoPhase",	EcAD6620Reg,	0x1820,	REGUNION( 0, 32,	0,	0)		},
{	"cic2Scale",	EcAD6620Reg,	0x1828,	REGUNLMT( 0, 3,		0,	0,		0,	8,	0)},
{	"inpExpInv",	EcAD6620Reg,	0x1828,	REGUNBIT( 4, 5,		0,	0)		},
{	"inpExpOff",	EcAD6620Reg,	0x1828,	REGUNLMT( 5, 8,		0,	0,		0,	8,	0)},
{	"cic2Decm",	EcAD6620Reg,	0x1830,	REGUNLMT( 0, 8,		RWST,	1,		1,	16,	-1)},
{	"cic5Scale",	EcAD6620Reg,	0x1838,	REGUNLMT( 0, 5,		0,	0,		0,	20,	0)},
{	"cic5Decm",	EcAD6620Reg,	0x1840,	REGUNLMT( 0, 8,		RWST,	1,		1,	32,	-1)},
{	"rcfScale",	EcAD6620Reg,	0x1848,	REGUNLMT( 0, 8,		0,	1,		1,	8,	-1)},
{	"rcfDecm",	EcAD6620Reg,	0x1850,	REGUNLMT( 0, 8,		RWST,	1,		1,	32,	-1)},
{	"rcf1stTap",	EcAD6620Reg,	0x1858,	REGUNLMT( 0, 8,		0,	0,		0,	255,	0)},
{	"rcfNTaps",	EcAD6620Reg,	0x1860,	REGUNLMT( 0, 8,		RWST,	1,		1,	256,	-1)},	/* TODO:  wraparound? */
};

static EcNodeDirRec ad6620Dir = {EcdrNumberOf(ad6620RegDefs), ad6620RegDefs, "ad6620"};
static EcNodeDirRec ad6620RawDir = {EcdrNumberOf(ad6620RawRegDefs), ad6620RawRegDefs, "raw_ad6620"};

static EcNodeRec ecdrChannelRegDefs[] = {
{	"trigMode",	EcReg,		0x0,	REGUNION( 1, 4,		MNU,	0)		},
{	"counterEna",	EcReg,		0x0,	REGUNBIT( 4, 5,		0,	0)		},
{	"dualRxPara",	EcReg,		0x0,	REGUNBIT( 5, 6,		0,	0)		},
{	"trigClear",	EcReg,		0x0,	REGUNBIT( 15, 16,	0,	0)		},
{	"modeSelect",	EcReg,		0x4,	REGUNION( 0, 3,		MNU,	0)		},
{	"channelID",	EcReg,		0x4,	REGUNLMT( 3, 6,		0,	0,		0,	7,	0)},
{	"nAccum",	EcReg,		0x4,	REGUNLMT( 6, 14,	0,	1,		1,	256,	-1)},
{	"burstCnt",	EcBrstCntReg,	0x0,	REGUNLMT( 0, 17,	0,	0,		1,	0x20000,-1)},	/* TODO special values depending on accu mode */
{	"totalDecm",	EcReg,		0xc,	REGUNION( 0, 14,	RO,	0)		},	/* TODO: special, depends on AD settings: they calculate from cics and rcf, check <=16383 */
};

static EcNodeRec ecdrChannelRawRegDefs[] = {
{	"rctla",	EcReg,		0x0,	REGUNION( 0, 32,	0,	0)		},
{	"rctlb",	EcReg,		0x4,	REGUNION( 0, 32,	0,	0)		},
{	"dws",		EcReg,		0x8,	REGUNION( 0, 32,	0,	0)		},
{	"rdec",		EcReg,		0xc,	REGUNION( 0, 32,	0,	0)		},
};

static EcNodeDirRec ecdrChannelDir = {EcdrNumberOf(ecdrChannelRegDefs), ecdrChannelRegDefs, "channel"};
static EcNodeDirRec ecdrChannelRawDir = {EcdrNumberOf(ecdrChannelRawRegDefs), ecdrChannelRawRegDefs, "raw_channel"};

static EcNodeRec ecdrChannelPairRegDefs[] = {
{	"clockSame",	EcReg,		0x0,	REGUNBIT( 0, 1,		0,	1)		},	/* TODO related to clk multiplier ? */
{	"ncoSyncEna",	EcReg,		0x0,	REGUNBIT( 1, 2,		0,	1)		},
{	"fifoOffset",	EcFifoReg,	0x0,	REGUNLMT( 0, 16,	0,	16,		0,	0xffff,	0)},
{	"nco0ABSync",	EcReg,		0x0,	REGUNBIT( 6, 7,		0,	0)		},
{	"nco1ABSync",	EcReg,		0x0,	REGUNBIT( 7, 8,		0,	0)		},
{	"rx0AReset",	EcReg,		0x0,	REGUNBIT( 8, 9,		0,	1)		},	/* reset at startup */
{	"rx0BReset",	EcReg,		0x0,	REGUNBIT( 9, 10,	0,	1)		},
{	"rx1AReset",	EcReg,		0x0,	REGUNBIT( 10, 11,	0,	1)		},
{	"rx1BReset",	EcReg,		0x0,	REGUNBIT( 11, 12,	0,	1)		},
{	"fifo0Rst",	EcReg,		0x0,	REGUNBIT( 12, 13,	0,	1)		},
{	"fifo1Rst",	EcReg,		0x0,	REGUNBIT( 13, 14,	0,	1)		},
{	"C0",		EcDir,		0x10,	{ d: &ecdrChannelDir }		},
{	"C1",		EcDir,		0x20,	{ d: &ecdrChannelDir }		},
{	"gain0Ena",	EcReg,		0x30,	REGUNBIT( 0, 1,		0,	0)		},
{	"gain1Ena",	EcReg,		0x30,	REGUNBIT( 1, 2,		0,	0)		},
{	"gain0",	EcReg,		0x30,	REGUNLMT( 2, 7,		0,	0,		-7,	24,	7)},
{	"gain1",	EcReg,		0x30,	REGUNLMT( 7, 12,	0,	0,		-7,	24,	7)},
{	"0A",		EcDir,		0x2000,	{ d: &ad6620Dir }		},
{	"0B",		EcDir,		0x4000,	{ d: &ad6620Dir }		},
{	"1A",		EcDir,		0x6000,	{ d: &ad6620Dir }		},
{	"1B",		EcDir,		0x8000,	{ d: &ad6620Dir }		},
};

static EcNodeRec ecdrChannelPairRawRegDefs[] = {
{	"csr",		EcReg,		0x0,	REGUNION( 0, 32,	0,	0)		},
{	"C0",		EcDir,		0x10,	{ d: &ecdrChannelRawDir }	},
{	"C1",		EcDir,		0x20,	{ d: &ecdrChannelRawDir }	},
{	"gain",		EcReg,		0x30,	REGUNION( 0, 32,	0,	0)		},
{	"fifoOffset",	EcReg,		0x40,	REGUNION( 0, 32,	0,	0)		},
{	"0A",		EcDir,		0x2000,	{ d: &ad6620RawDir }		},
{	"0B",		EcDir,		0x4000,	{ d: &ad6620RawDir }		},
{	"1A",		EcDir,		0x6000,	{ d: &ad6620RawDir }		},
{	"1B",		EcDir,		0x8000,	{ d: &ad6620RawDir }		},
};

static EcNodeDirRec ecdrChannelPairDir = {EcdrNumberOf(ecdrChannelPairRegDefs), ecdrChannelPairRegDefs, "channelPair"};
static EcNodeDirRec ecdrChannelPairRawDir = {EcdrNumberOf(ecdrChannelPairRawRegDefs), ecdrChannelPairRawRegDefs, "raw_channelPair"};

static EcNodeRec fifoStatRegDefs[] = {
{	"ch0",		EcReg,		0x0,	REGUNION( 0, 4,		RO,	0)		},
{	"ch1",		EcReg,		0x0,	REGUNION( 4, 8,		RO,	0)		},
{	"ch2",		EcReg,		0x0,	REGUNION( 8, 12,	RO,	0)		},
{	"ch3",		EcReg,		0x0,	REGUNION( 12, 16,	RO,	0)		},
{	"ch4",		EcReg,		0x0,	REGUNION( 16, 20,	RO,	0)		},
{	"ch5",		EcReg,		0x0,	REGUNION( 20, 24,	RO,	0)		},
{	"ch6",		EcReg,		0x0,	REGUNION( 24, 28,	RO,	0)		},
{	"ch7",		EcReg,		0x0,	REGUNION( 28, 32,	RO,	0)		},
};

static EcNodeRec bit0to7RORegDefs[] = {
{	"ch0",		EcReg,		0x0,	REGUNION( 0, 1,		RO,	0)		},
{	"ch1",		EcReg,		0x0,	REGUNION( 1, 2,		RO,	0)		},
{	"ch2",		EcReg,		0x0,	REGUNION( 2, 3,		RO,	0)		},
{	"ch3",		EcReg,		0x0,	REGUNION( 3, 4,		RO,	0)		},
{	"ch4",		EcReg,		0x0,	REGUNION( 4, 5,		RO,	0)		},
{	"ch5",		EcReg,		0x0,	REGUNION( 5, 6,		RO,	0)		},
{	"ch6",		EcReg,		0x0,	REGUNION( 6, 7,		RO,	0)		},
{	"ch7",		EcReg,		0x0,	REGUNION( 7, 8,		RO,	0)		},
};

static EcNodeRec bit0to7RegDefs[] = {
{	"ch0",		EcReg,		0x0,	REGUNBIT( 0, 1,		0,	0)		},
{	"ch1",		EcReg,		0x0,	REGUNBIT( 1, 2,		0,	0)		},
{	"ch2",		EcReg,		0x0,	REGUNBIT( 2, 3,		0,	0)		},
{	"ch3",		EcReg,		0x0,	REGUNBIT( 3, 4,		0,	0)		},
{	"ch4",		EcReg,		0x0,	REGUNBIT( 4, 5,		0,	0)		},
{	"ch5",		EcReg,		0x0,	REGUNBIT( 5, 6,		0,	0)		},
{	"ch6",		EcReg,		0x0,	REGUNBIT( 6, 7,		0,	0)		},
{	"ch7",		EcReg,		0x0,	REGUNBIT( 7, 8,		0,	0)		},
};

static EcNodeRec bit16to23RORegDefs[] = {
{	"ch0",		EcReg,		0x0,	REGUNION( 16, 17,	RO,	0)		},
{	"ch1",		EcReg,		0x0,	REGUNION( 17, 18,	RO,	0)		},
{	"ch2",		EcReg,		0x0,	REGUNION( 18, 19,	RO,	0)		},
{	"ch3",		EcReg,		0x0,	REGUNION( 19, 20,	RO,	0)		},
{	"ch4",		EcReg,		0x0,	REGUNION( 20, 21,	RO,	0)		},
{	"ch5",		EcReg,		0x0,	REGUNION( 21, 22,	RO,	0)		},
{	"ch6",		EcReg,		0x0,	REGUNION( 22, 23,	RO,	0)		},
{	"ch7",		EcReg,		0x0,	REGUNION( 23, 24,	RO,	0)		},
};

static EcNodeRec bit24to31RORegDefs[] = {
{	"ch0",		EcReg,		0x0,	REGUNION( 24, 25,	RO,	0)		},
{	"ch1",		EcReg,		0x0,	REGUNION( 25, 26,	RO,	0)		},
{	"ch2",		EcReg,		0x0,	REGUNION( 26, 27,	RO,	0)		},
{	"ch3",		EcReg,		0x0,	REGUNION( 27, 28,	RO,	0)		},
{	"ch4",		EcReg,		0x0,	REGUNION( 28, 29,	RO,	0)		},
{	"ch5",		EcReg,		0x0,	REGUNION( 29, 30,	RO,	0)		},
{	"ch6",		EcReg,		0x0,	REGUNION( 30, 31,	RO,	0)		},
{	"ch7",		EcReg,		0x0,	REGUNION( 31, 32,	RO,	0)		},
};

static EcNodeDirRec fifoStatDir = { EcdrNumberOf(fifoStatRegDefs), fifoStatRegDefs, "fifoStat" };
static EcNodeDirRec bit16to23RODir = { EcdrNumberOf(bit16to23RORegDefs), bit16to23RORegDefs, "statBitsH" };
static EcNodeDirRec bit24to31RODir = { EcdrNumberOf(bit24to31RORegDefs), bit24to31RORegDefs, "statBitsHH" };

static EcNodeRec ecdrBoardRegDefs[] = {
{	"rdbackMode",	EcReg,		0x0,	REGUNION( 0, 4,		MNU,	0)		},	/* LR, affects fifoFlags (if !offset): 1chan: OFFSET_16, 2 channels: OFFSET 8, 4 channels: OFFSET 4, 8channels: OFFSET 2, unused channels get their syncEna and gateEna cleared  pair uses 01.0 and 23.0, 4 channels uses 01.0, 23.0, 45.0, 67.0 */
{	"headerEna",	EcReg,		0x0,	REGUNBIT( 4, 5,		0,	0)		},
{	"vmeOnly",	EcReg,		0x0,	REGUNBIT( 5, 6,		0,	0)		},
{	"d64_2kEna",	EcReg,		0x0,	REGUNBIT( 6, 7,		0,	0)		},
{	"freqRange",	EcReg,		0x0,	REGUNION( 7, 9,		MNU,	0)		},
{	"extSyncEna",	EcReg,		0x0,	REGUNBIT( 9, 10,	0,	0)		},
{	"swSync",	EcReg,		0x0,	REGUNBIT( 10, 11,	0,	0)		},
{	"pktCntRst",	EcReg,		0x0,	REGUNBIT( 11, 12,	0,	0)		},
{	"lclBusRst",	EcReg,		0x0,	REGUNBIT( 12, 13, 	0,	0)		},
{	"clkMult0_3",	EcReg,		0x0,	REGUNION( 17, 19,	MNU,	0)		},	/* TODO wait 15ms */
{	"clkMult4_7",	EcReg,		0x0,	REGUNION( 19, 21,	MNU,	0)		},	/* TODO wait 15ms */
{	"fifoStat",	EcDir,		0x4,	{ d: &fifoStatDir }		},
{	"intVec",	EcReg,		0x8,	REGUNLMT( 0, 8,		0,	0,		0,	255,	0)},
{	"intMsk",	EcReg,		0xc,	REGUNLMT( 0, 11,	0,	0,		0,	0x7ff,	0)},
{	"intStat",	EcReg,		0x10,	REGUNLMT( 0, 11,	RO,	0,		0,	0x7ff,	0)},
{	"pktCnt",	EcReg,		0x14,	REGUNION( 0, 24,	RO,	0)		},
{	"adcOvrRng",	EcDir,		0x14,	{ d: &bit24to31RODir }		},
{	"extHdr",	EcReg,		0x18,	REGUNION( 0, 16,	RO,	0)		},
{	"fifoOvfl",	EcDir,		0x18,	{ d: &bit16to23RODir }		},
{	"fifoUdfl",	EcDir,		0x18,	{ d: &bit24to31RODir }		},
{	"01",		EcDir,		0x10000,{ d: &ecdrChannelPairDir }	},
{	"23",		EcDir,		0x20000,{ d: &ecdrChannelPairDir }	},
{	"45",		EcDir,		0x30000,{ d: &ecdrChannelPairDir }	},
{	"67",		EcDir,		0x40000,{ d: &ecdrChannelPairDir }	},
};

static EcNodeRec ecdrBoardRawRegDefs[] = {
{	"csr",		EcReg,		0x0,	REGUNION( 0, 32,	0,	0)		},
{	"flags",	EcReg,		0x4,	REGUNION( 0, 32,	RO,	0)		},
{	"intVec",	EcReg,		0x8,	REGUNION( 0, 32,	0,	0)		},
{	"intMsk",	EcReg,		0xc,	REGUNION( 0, 32,	0,	0)		},
{	"intStat",	EcReg,		0x10,	REGUNION( 0, 32,	RO,	0)		},
{	"head0",	EcReg,		0x14,	REGUNION( 0, 32,	RO,	0)		},
{	"head1",	EcReg,		0x18,	REGUNION( 0, 32,	RO,	0)		},
{	"01",		EcDir,		0x10000,{ d: &ecdrChannelPairRawDir }	},
{	"23",		EcDir,		0x20000,{ d: &ecdrChannelPairRawDir }	},
{	"45",		EcDir,		0x30000,{ d: &ecdrChannelPairRawDir }	},
{	"67",		EcDir,		0x40000,{ d: &ecdrChannelPairRawDir }	},
};

static EcNodeDirRec ecdrBoardDir = {EcdrNumberOf(ecdrBoardRegDefs), ecdrBoardRegDefs, "board"};
static EcNodeDirRec ecdrBoardRawDir = {EcdrNumberOf(ecdrBoardRawRegDefs), ecdrBoardRawRegDefs, "raw_board"};

EcNodeRec	ecdr814Board = {
	"/",		EcDir,		0x0,	&ecdrBoardDir
};
EcNodeRec	ecdr814RawBoard = {
	"/",		EcDir,		0x0,	&ecdrBoardRawDir
};

#undef RO
#undef MNU
