#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "drvrEcdr814.h"

static void
printFastkeyInfo(EcBoardDesc bd, EcNode l, void* arg)
{
int	depth;
EcNode	pl;

if (l->cnode->offset) return; /* have visited this node already */
/* oh well, we must calculate the depth... */
for (depth=-1, pl=l; pl->parent; depth++, pl=pl->parent) ;
if (depth >= 0) {
	char buf[200];
	long fkey;
	assert(depth < 8*sizeof(EcFKey)/FKEY_LEN);
	/* search offset in parent's directory */
	fkey = l->cnode- l->parent->cnode->u.d.n->nodes;
	/* verify that it fits */
	assert(fkey >=0 && fkey < l->parent->cnode->u.d.n->nels);
	fkey++; /* 0 is used to mark empty key */
	assert(fkey < (1<<FKEY_LEN));
	/* seems ok, transform name replacing '.' by '_' */
	strcpy(buf,l->cnode->name);
	{ char *chpt;
	  for (chpt=buf; *chpt; chpt++) {
		if (!isalnum(*chpt)) *chpt='_';
	  }
	}
	fprintf((FILE*)arg,"#define FK_%s_%s\t\t%i\n",l->parent->cnode->u.d.n->name,buf,fkey);
	/* mark node visited */
	l->cnode->offset=1;
}
}

static void
countNodes(EcBoardDesc bd, EcNode l, void *arg)
{
(*(unsigned long*)arg)++;
}

static void
clearVisited(EcBoardDesc bd, EcNode l, void *arg)
{
	/* abuse from the offset field to mark a node visited */
	l->cnode->offset=0;
}

extern EcNode ecCreateDirectory(EcCNode);

int
main(int argc, char ** argv)
{
int mode = 0;
int ch;
EcNode root;
while ((ch=getopt(argc,argv,"kc")) >=0) {
	if (mode) {
		fprintf(stderr,"%s: ONLY 1 OPTION ALLOWED\n",argv[0]);
		exit(1);
	}
	mode = ch;
}
root=ecCreateDirectory(&ecdr814CInfo);
switch (mode) {
	case 'k':
		ecNodeWalk(0, root, clearVisited, stdout);
		ecNodeWalk(0, root, printFastkeyInfo, stdout);
	break;

	case 'c':
		{ unsigned long total=0;
		ecNodeWalk(0, root, countNodes, &total);
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
