/* $Id$ */

#ifndef REG_NODE_OPS_H
#define  REG_NODE_OPS_H

#include "drvrEcdr814.h"

#ifdef ECDR814_PRIVATE_IF
extern EcCNodeOpsRec ecdr814RegNodeOps;
extern EcCNodeOpsRec ecdr814AD6620RegNodeOps;
#endif

extern void
initRegNodeOps(void);

#endif
