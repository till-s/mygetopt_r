#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>

#include "bitMenu.h"

static int menudbd(char*);

int
main(int argc, char ** argv)
{
int	i;
EcMenu	*mp;
char	*name="ecdr814Menus.dbd";

	if (argc>1)
		name=argv[1];
	for ( mp=drvrEcdr814Menus+1; *mp; mp++ ) {
		printf("#define EC_MENU_%s	%i\n",
				(*mp)->menuName, mp-drvrEcdr814Menus);
		for (i=0; i<(*mp)->nels; i++) {
		char buf[100], *spt,*dpt;
		for (spt=(*mp)->items[i].name, dpt=buf; *spt; spt++)
			if (isalnum(*spt) || '_'==*spt)
				*(dpt++)=*spt;
		*dpt=0;
		
		printf("#define EC_MENU_%s_%s	%lu\n",
				(*mp)->menuName,
				buf,
				(*mp)->items[i].bitval);
		}
	}
	return menudbd(name);
}

static int
menudbd(char *name)
{
int	i;
EcMenu	*mp;
FILE	*f=fopen(name,"w");

	if (!f) {
		perror("opening dbd file for writing:");
		return -1;
	}
	for ( mp=drvrEcdr814Menus+1; *mp; mp++ ) {
		fprintf(f,"menu(menu%s) {\n",		(*mp)->menuName);
		for (i=0; i<(*mp)->nels; i++ ) {
		fprintf(f,"    choice(menu%s%i, %c%s%c)\n",
				(*mp)->menuName, i,
				'\"', (*mp)->items[i].name, '\"');
		}
		fprintf(f,"}\n");
	}
	fclose(f);

return 0;
}
