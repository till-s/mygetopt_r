#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "drvrEcdr814.h"

static void
printFastkeyInfo(EcCNodeList l, IOPtr p, void* arg)
{
int		depth;
EcCNodeList		pl;

if (l->n->offset) return; /* have visited this node already */
/* oh well, we must calculate the depth... */
for (depth=-1, pl=l; pl->p; depth++, pl=pl->p) ;
if (depth >= 0) {
	char buf[200];
	long fkey;
	assert(depth < 8*sizeof(EcFKey)/FKEY_LEN);
	/* search offset in parent's directory */
	fkey = l->n- l->p->n->u.d.n->nodes;
	/* verify that it fits */
	assert(fkey >=0 && fkey < l->p->n->u.d.n->nels);
	fkey++; /* 0 is used to mark empty key */
	assert(fkey < (1<<FKEY_LEN));
	/* seems ok, transform name replacing '.' by '_' */
	strcpy(buf,l->n->name);
	{ char *chpt;
	  for (chpt=buf; *chpt; chpt++) {
		if (!isalnum(*chpt)) *chpt='_';
	  }
	}
	fprintf((FILE*)arg,"#define FK_%s_%s\t\t%i\n",l->p->n->u.d.n->name,buf,fkey);
	/* mark node visited */
	l->n->offset=1;
}
}

static void
countNodes(EcCNodeList l, IOPtr p, void *arg)
{
(*(unsigned long*)arg)++;
}

static void
clearVisited(EcCNodeList l, IOPtr p, void *arg)
{
	/* abuse from the offset field to mark a node visited */
	l->n->offset=0;
}

int
main(int argc, char ** argv)
{
int mode = 0;
int ch;
while ((ch=getopt(argc,argv,"kc")) >=0) {
	if (mode) {
		fprintf(stderr,"%s: ONLY 1 OPTION ALLOWED\n",argv[0]);
		exit(1);
	}
	mode = ch;
}
switch (mode) {
	case 'k':
		ecCNodeWalk(&ecdr814Board, clearVisited, 0, 0, stdout);
		ecCNodeWalk(&ecdr814Board, printFastkeyInfo, 0, 0, stdout);
	break;

	case 'c':
		{ unsigned long total=0;
		ecCNodeWalk(&ecdr814Board, countNodes, 0, 0, &total);
		printf("Total node count is %i\n",total);
		}
	break;
	
	case 0:
		fprintf(stderr,"%s: NEED AN OPTION\n",argv[0]);
		exit(1);

	default:
		fprintf(stderr,"%s: UNKNOWN OPTION -%c\n",argv[0],mode);
		exit(1);
}
return 0;
}
