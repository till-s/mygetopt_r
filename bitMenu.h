/*$Id$*/
#ifndef EC_BIT_MENU_TILL_H
#define EC_BIT_MENU_TILL_H

#include "drvrEcdr814.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EcMenuRec_ *EcMenu;

typedef struct EcMenuItemRec_ {
	char		*name;
	unsigned long	bitval;
} EcMenuItemRec, *EcMenuItem;

typedef struct EcMenuRec_ {
	char		*menuName;
	int		nels;
	unsigned long	bitmask;
	EcMenuItem	items;
} EcMenuRec;

extern EcMenu drvrEcdr814Menus[];

/* find the index of a menu with 'name' in drvrEcdr814Menus */
int
ecFindMenuIndex(char *name);

static __inline__
EcMenu ecMenu(int index)
{
return	drvrEcdr814Menus[index & EcFlgMenuMask];
}

#ifdef __cplusplus
};
#endif

#endif
