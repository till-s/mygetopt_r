/*$Id$*/

#include "bitMenu.h"


#include <string.h>
#include <assert.h>

#define MNumberOf	EcdrNumberOf

static EcMenuItemRec	onOffItems[] = {
	{ "off",	0x0 }, 
	{ "on",		0x1 }, 
};

static EcMenuRec			onOffMenu = {
	"onOff",	MNumberOf(onOffItems), (0x1),	onOffItems
};

static EcMenuItemRec	resetRunningItems[] = {
	{ "running",	0x0 }, 
	{ "reset",	0x1 }, 
};

static EcMenuRec			resetMenu = {
	"resetRunning",	MNumberOf(resetRunningItems), (0x1),	resetRunningItems
};

static EcMenuItemRec	realCmplxItems[] = {
	{ "singleChannelReal",		0x0 },
	{ "dualChannelReal",		0x1 },
	{ "singleChannelComplex",	0x2 },
};

static EcMenuRec		realCmplxMenu = {
	"realCmplx",	MNumberOf(realCmplxItems), (0x3),	realCmplxItems
};

static EcMenuItemRec	trigModeItems[] = {
	{ "channelDisabled",	(0x0) },
	{ "triggered",		(0x1) },
	{ "gated",		(0x2) },
	{ "freeRunning",	(0x5) },
};

static EcMenuRec		trigModeMenu = {
	"trigMode",	MNumberOf(trigModeItems), (0x7),	trigModeItems
};

static EcMenuItemRec	modeSelectItems[] = {
	{ "Raw_A/D_No_Acc",	0x0 },
	{ "Raw_RXA_No_Acc",	0x1 },
	{ "Raw_RXB_No_Acc",	0x2 },
	{ "Raw_A&B_No_Acc",	0x3 },
	{ "A/D_Accumulated",	0x4 },
	{ "RXA_Accumulated",	0x5 },
	{ "RXB_Accumulated",	0x6 },
	{ "A&B_Accumulated",	0x7 },
};

static EcMenuRec		modeSelectMenu = {
	"modeSelect",	MNumberOf(modeSelectItems), (0x7),	modeSelectItems
};

static EcMenuItemRec	fifoFlgItems[] = {
	{ "PAE_Off_16",		(0x0)},
	{ "PAE_Off_8",		(0x1)},
	{ "PAE_Off_4",		(0x2)},
	{ "PAE_Off_2",		(0x3)},
	{ "Use_fifoOffset",	(0x4)},
};

static EcMenuRec		fifoFlgMenu = {
	"fifoFlg",	MNumberOf(fifoFlgItems), (0x7),	fifoFlgItems
};

static EcMenuItemRec	rdbackModeItems[] = {
	{ "Channel_0",		(0x0)},
	{ "Channel_1",		(0x1)},
	{ "Channel_2",		(0x2)},
	{ "Channel_3",		(0x3)},
	{ "Channel_4",		(0x4)},
	{ "Channel_5",		(0x5)},
	{ "Channel_6",		(0x6)},
	{ "Channel_7",		(0x7)},
	{ "Ch_0&2",		(0x8)},
	{ "Ch_0&2&4&6",		(0x9)},
	{ "AllChannels",	(0xa)},
};

static EcMenuRec		rdbackModeMenu = {
	"rdbackMode",	MNumberOf(rdbackModeItems), (0xf),	rdbackModeItems
};

static EcMenuItemRec		freqRangeItems[] = {
	{ "0_35MHz",		(0x0)},
	{ "25_60MHz",		(0x2)},
	{ "40_100MHz",		(0x1)},
};

static EcMenuRec		freqRangeMenu = {
	"freqRange",	MNumberOf(freqRangeItems), (0x3),	freqRangeItems
};

static EcMenuItemRec		clkMultItems[] = {
	{ "Rx_=_ADC_Clk",	(0x0)},
	{ "Rx_=_2*ADC_Clk",	(0x1)},
	{ "Rx_=_4*ADC_Clk",	(0x2)},
};

static EcMenuRec		clkMultMenu = {
	"clockMult",	MNumberOf(clkMultItems),  (0x3),	clkMultItems
};

EcMenu	drvrEcdr814Menus[] = {
	0,	/* dummy entry to avoid 1 index */
	&onOffMenu,
	&realCmplxMenu,
	&trigModeMenu,
	&modeSelectMenu,
	&fifoFlgMenu,
	&rdbackModeMenu,
	&freqRangeMenu,
	&clkMultMenu,
	&resetMenu,
	0,	/* tag end of list */
};

#if 0
void
ecRegisterMenu(EcNodeList l, IOPtr p, void *arg)
{
EcNode n=l->n;
if (!EcNodeIsDir(n) && EcFlgMenuMask == (n->u.r.flags & EcFlgMenuMask)) {
	int foundMenuIndex;
	/* uninitialized menu */
	for (foundMenuIndex=MNumberOf(drvrEcdr814Menus)-2; foundMenuIndex>0; foundMenuIndex--) {
		if (!strcmp(drvrEcdr814Menus[foundMenuIndex]->menuName, n->name))
			break; /* found */
	}
	assert( foundMenuIndex > 0 );
	n->u.r.flags &= ~EcFlgMenuMask;
	n->u.r.flags |= foundMenuIndex;
}
}
#endif
