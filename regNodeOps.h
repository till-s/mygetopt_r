/* $Id$ */

#ifndef REG_NODE_OPS_H
#define  REG_NODE_OPS_H

#include "drvrEcdr814.h"

/* operations on a leaf node
 * NOT INTENDED FOR DIRECT USE. Only
 * implementations for new leaf nodes should
 * use this.
 */

#define ECREGMASK(n) (((1<<((n)->u.r.u.s.pos1))-1) ^ \
		       (((n)->u.r.u.s.pos2 & 31 ? 1<<(n)->u.r.u.s.pos2 : 0)-1))
#define ECREGPOS(n) ((n)->u.r.u.s.pos1)

typedef struct EcCNodeOpsRec_ {
	struct EcCNodeOpsRec_ *super;
	int			initialized; 	/* must be initialized to 0 */
	EcErrStat	(*get)(EcBoardDesc bd, EcNode n, Val_t *pv);
	EcErrStat	(*getRaw)(IOPtr *addr, EcNode n, Val_t *pv);
	EcErrStat	(*put)(EcBoardDesc bd, EcNode n, Val_t v);
	EcErrStat	(*putRaw)(IOPtr *addr, EcNode n, Val_t v);
} EcCNodeOpsRec, *EcCNodeOps;

extern EcCNodeOpsRec ecdr814RegNodeOps;
extern EcCNodeOpsRec ecdr814AD6620RegNodeOps;

void
addNodeOps(EcCNodeOps ops, EcCNodeType type);

extern EcCNodeOpsRec ecDefaultNodeOps;

extern void
initRegNodeOps(void);

#endif
