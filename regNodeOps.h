/* $Id$ */

#ifndef REG_NODE_OPS_H
#define  REG_NODE_OPS_H

#include "drvrEcdr814.h"

#ifdef ECDR814_PRIVATE_IF
extern EcNodeOpsRec ecdr814RegNodeOps;
extern EcNodeOpsRec ecdr814AD6620RegNodeOps;
#endif

EcErrStat
ecLkupNGet(EcNode, EcFKey, IOPtr, Val_t *);

EcErrStat
ecLkupNPut(EcNode, EcFKey, IOPtr, Val_t);

extern void
initRegNodeOps(void);

#endif
