#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "drvrEcdr814.h"
#include "bitMenu.h"

static void
clearVisited(EcBoardDesc bd, EcNode l, void *arg)
{
	/* abuse from the offset field to mark a node visited */
	l->cnode->offset=0;
}


static void rprint(EcNode l, char **p)
{
	if (l->parent) {
		rprint(l->parent,p);
		if (l->parent->parent) *((*p)++) = EC_DIRSEP_CHAR;
	} else return;
	*p+=sprintf(*p,l->cnode->name);
}

static void
dbdFieldEntry(EcBoardDesc bd, EcNode l, void *arg)
{
FILE	*f = (FILE*)arg;
int	hasmenu;
char	pathName[200],*chpt;
char	fieldName[200];

	if (EcCNodeIsDir(l->cnode) || l->cnode->offset) return;

	l->cnode->offset = -1; /* mark visited */

	chpt=pathName;
	rprint(l, &chpt);
#if 0

	/* transform pathName to fieldName */
	for (chpt=pathName; *chpt; chpt++)
		if (!isalnum(*chpt) && !'_'==*chpt)
			fprintf(stderr,"WARNING: field name contains illegal characters: %s\n",pathName);

	chpt=fieldName;
	if (isdigit(pathName[0])) {
		/* prepend with '_' */
		*(chpt++) = '_';
	}
	strcpy(chpt, pathName);
	while ((chpt=strchr(chpt,EC_DIRSEP_CHAR)))
		*chpt = '_';
#else
	{
	char *dpt;

	fprintf(f,"# %s\n",pathName);

	dpt=pathName;
	if ( ! (chpt=strchr(l->cnode->name,'.')) )
		chpt = l->cnode->name;
	else
		*(dpt++)=*(chpt++);
	strcpy(fieldName, chpt);
	strcpy(dpt, chpt);
	}
#endif

	if ( ! (hasmenu = (l->cnode->u.r.flags & EcFlgMenuMask)) )
		if (l->cnode->u.r.pos2 - l->cnode->u.r.pos1 == 1)
			hasmenu = -1;

	fprintf(f,"field(%s,DBF_%s) {\n",          fieldName, hasmenu ? "MENU" : "ULONG");
	fprintf(f,"    prompt(%c%s%c)\n",          '\"',pathName,'\"'); /* strip leading '/' */
	fprintf(f,"    promptgroup(GUI_SELECT)\n"  );
	fprintf(f,"    pp(TRUE)\n"                 );
	fprintf(f,"    interest(1)\n"	           );
	if (hasmenu) {
	fprintf(f,"    menu(menu%s)\n",            hasmenu < 1 ? "YesNo" : drvrEcdr814Menus[hasmenu]->menuName);
	}
	fprintf(f,"}\n");
	
}

extern EcNode ecCreateDirectory(EcCNode);

int
main(int argc, char ** argv)
{
int	i;
EcMenu	*mp;
FILE	*f=stdout;
EcNode  root = ecCreateDirectory(&ecdr814CInfo);

	ecNodeWalk(0, root, clearVisited, 0);
	ecNodeWalk(0, root, dbdFieldEntry, f);


return 0;
}
