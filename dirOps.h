/* $Id$ */
#ifndef ECDR_DIR_OPS_H
#define ECDR_DIR_OPS_H

#include <stdio.h>

#include "drvrEcdr814.h"

/* routines for interactive operation */

/* print current path */
void
ecPwd(FILE *f);

/* change working directory */
void
ecCd(EcKey k);

/* list node contents */
void
ecLs(EcKey k, FILE *f);

#endif
