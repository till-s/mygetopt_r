#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "drvrEcdr814.h"

static void
printFastkeyInfo(EcNodeList l, IOPtr p, void* arg)
{
int		depth;
EcNodeList	pl;
/* oh well, we must calculate the depth... */
for (depth=-1, pl=l; pl->p; depth++, pl=pl->p) ;
if (depth > 0) {
	long fkey;
	assert(depth < 8*sizeof(EcFKey)/FKEY_LEN);
	/* search offset in parent's directory */
	fkey = l->n - l->p->n->u.d.n->nodes;
	/* verify that it fits */
	assert(fkey >=0 && fkey < l->p->n->u.d.n->nels);
	fkey++; /* 0 is used to mark empty key */
	assert(fkey < (1<<FKEY_LEN));
	/* seems ok */
	fprintf((FILE*)arg,"#define FK_%s\t\t%i\n",l->n->name,fkey);
}
}

int
main(int argc, char ** argv)
{
walkEcNode(&ecdr814Board, printFastkeyInfo, 0, 0, stdout);
}
