#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "drvrEcdr814.h"
#include "bitMenu.h"

static void
clearVisited(EcNodeList l, IOPtr p, void *arg)
{
	/* abuse from the offset field to mark a node visited */
	l->n->offset=0;
}


static void rprint(EcNodeList l, char **p)
{
	if (l->p) {
		rprint(l->p,p);
		if (l->p->p) *((*p)++) = EC_DIRSEP_CHAR;
	} else return;
	*p+=sprintf(*p,l->n->name);
}

static void
dbdFieldEntry(EcNodeList l, IOPtr p, void *arg)
{
FILE	*f = (FILE*)arg;
int	hasmenu;
char	pathName[200],*chpt;
char	fieldName[200];

	if (EcNodeIsDir(l->n) || l->n->offset) return;

	l->n->offset = -1; /* mark visited */

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
	if ( ! (chpt=strchr(l->n->name,'.')) )
		chpt = l->n->name;
	else
		*(dpt++)=*(chpt++);
	strcpy(fieldName, chpt);
	strcpy(dpt, chpt);
	}
#endif

	if ( ! (hasmenu = (l->n->u.r.flags & EcFlgMenuMask)) )
		if (l->n->u.r.pos2 - l->n->u.r.pos1 == 1)
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

int
main(int argc, char ** argv)
{
int	i;
EcMenu	*mp;
FILE	*f=stdout;

	walkEcNode(&ecdr814Board, clearVisited, 0, 0, 0);
	walkEcNode(&ecdr814Board, dbdFieldEntry, 0, 0, f);


return 0;
}
