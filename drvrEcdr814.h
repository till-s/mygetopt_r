/* $Id$ */

#ifndef DRV_ECDR814_TILL_H
#define DRV_ECDR814_TILL_H

/* number of array elements */
#define EcdrNumberOf(arr) (sizeof(arr)/sizeof(arr[0]))

/* Node types */
typedef enum {
	EcDir = 0,
	EcReg,
	EcAD6620Reg,
	EcCoeffList
} EcNodeType;

/* pointer to physical device registers */
typedef volatile unsigned char *IOPtr;

/* struct describing a node
 * This can be a description of 
 *  - individual ECDR register bits (leaf node)
 *  - ECDR register array (leaf node)
 *  - "directory" of nodes (non-leaf, "dir" - node)
 */
typedef struct EcNodeRec_ {
	char			*name;
#if 0
	EcNodeType		t : 4;
	unsigned long	offset: 28;
#else
	EcNodeType	t;
	unsigned long	offset;
#endif
	union {
		struct {
			struct EcNodeDirRec_	*n;
		} d;					/* if an EcDir */
		struct {
			unsigned char		pos1;
			unsigned char		pos2;
			unsigned char		rdOnly;
			unsigned char		flags;
			unsigned long		inival;
		} r;					/* if a Reg or AD6620Reg */
		struct {
			unsigned char			size;
		} c;					/* if a CoeffList */
	} u;
} EcNodeRec, *EcNode;

#define REGUNION( pos1, pos2, rdonly, flags, inival) { r: (pos1), (pos2), (rdonly), (flags), (inival) }

#define EcNodeIsDir(n) (EcDir==(n)->t)

/* a directory of nodes */
typedef struct EcNodeDirRec_ {
	long		nels;
	EcNode		nodes;
} EcNodeDirRec, *EcNodeDir;

/* for building linked lists of nodes */
typedef struct EcNodeListRec_ {
	struct EcNodeListRec_	*p;
	EcNode			n;
} EcNodeListRec, *EcNodeList;

/* access of leaf nodes */
typedef unsigned long Val_t;

Val_t
ecGetValue(EcNode n, IOPtr b);

void
ecPutValue(EcNode n, IOPtr b, Val_t v);

Val_t
ecGetRawValue(EcNode n, IOPtr b);

void
ecPutRawValue(EcNode n, IOPtr b, Val_t v);

#ifdef ECDR814_PRIVATE_IF
/* operations on a leaf node
 * NOT INTENDED FOR DIRECT USE. Only
 * implementations for new leaf nodes should
 * use this.
 */

#define ECREGMASK(n) (((1<<((n)->u.r.pos1))-1)^((1<<((n)->u.r.pos2))-1))
#define ECREGPOS(n) ((n)->u.r.pos1)

typedef struct EcNodeOpsRec_ {
	struct EcNodeOpsRec_ *super;
	int			initialized; 	/* must be initialized to 0 */
	Val_t		(*get)(EcNode n, IOPtr b);
	Val_t		(*getRaw)(EcNode n, IOPtr b);
	void		(*put)(EcNode n, IOPtr b, Val_t v);
	void		(*putRaw)(EcNode n, IOPtr b, Val_t v);
} EcNodeOpsRec, *EcNodeOps;

void
addNodeOps(EcNodeOps ops, EcNodeType type);

extern EcNodeOpsRec ecDefaultNodeOps;
#endif

/* chose key implementation (integers vs. strings) */
#ifdef INTKEYS

typedef unsigned long	EcKey;
#define EcKeyIsEmpty(k)	((k)==0)

#else

typedef char *EcKey;
#define EcKeyIsUpDir(key)	(0==strcmp(key,".."))
#define EcKeyIsEmpty(k)	((k)==0)
#define EcString2Key(k)	(k)

#endif

/* node list record allocation primitives */
EcNodeList
allocNodeListRec(void);

/* free a list record returning its "next" member */
EcNodeList
freeNodeListRec(EcNodeList ptr);

/* allocate a nodelist record, set entry to n
 * and prepend to l
 * RETURNS: new NodeListRec
 */
EcNodeList
addEcNode(EcNode n, EcNodeList l);


/* lookup a key adding all the offsets
 * of the traversed nodes to *p
 * Returns: 0 if the node is not found.
 * Note: the node may not be a leaf
 *
 * If a pointer to a EcNodeListRec is passed (arg l),
 * lookupEcNode will record the traversed path updating
 * *l.
 * It is the responsibility of the caller to free 
 * list nodes allocated by lookupEcNode.
 */
EcNode
lookupEcNode(EcNode n, EcKey key, IOPtr *p, EcNodeList *l);

/* execute a function for every leaf (i.e. non-directory)
 * node.
 * The function is passed a EcNodeList pointing to the 
 * leaf node and all its parent directories.
 * For convenience, all offsets have been summed up and are
 * passed to "fn" as well. Note that the offset could
 * be calculated as 
 *	while (l) {
 *           offset+= l->n->offset;
 *           l = l->p;
 *      }
 */
void
walkEcNode(EcNode n, void (*fn)(EcNodeList l, IOPtr p, void *fnarg), IOPtr p, EcNodeList l, void *fnarg);

/* root node of the board directory */
extern EcNodeRec	ecdr814Board;

#endif
