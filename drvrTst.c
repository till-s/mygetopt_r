#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "drvrEcdr814.h"
#include "ecdrRegdefs.h" /* for BRDREG_ALIGNMENT */
#include "ecFastKeys.h"
#include "dirOps.h"


int
main(int argc, char ** argv)
{
char *	buf=malloc(0x100000 +  ECDR_BRDREG_ALIGNMENT);
char *tstdev = (char*)((((unsigned long)buf)+ECDR_BRDREG_ALIGNMENT)&~ECDR_BRDREG_ALIGNMENT);

memset(tstdev,0,0x1000);

drvrEcdr814Init();

printf("node size: %i\n",sizeof(EcCNodeRec));

#ifdef DEBUG
ecAddBoard("B0",tstdev,0);
#endif

#ifdef DIRSHELL
{ ecdrsh(0,0,0); exit(0); }
#endif

exit(1);
}
