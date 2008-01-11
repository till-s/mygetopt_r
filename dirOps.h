/* $Id$ */
#ifndef ECDR_DIR_OPS_H
#define ECDR_DIR_OPS_H

#include <stdio.h>
#include <stdarg.h>

#include "drvrEcdr814.h"

#ifdef __cplusplus
extern "C" {
#endif

/* routines for interactive operation */

/* print current path */
void
ecPwd(FILE *f);

/* change working directory */
void
ecCd(EcKey k, FILE *f);

/* list node contents */

/* possible flags */
#define DIROPS_LS_RECURSE	(1<<0)
#define DIROPS_LS_VERBOSE	(1<<1)
#define DIROPS_LS_SHOWMENU	(1<<2)
#define DIROPS_LS_FKEYINFO	(1<<3)

/* argument list must be
 * (FILE *f, int flags)
 */
EcErrStat
ecLs(EcKey k, va_list ap);

/* Interactive 'shell' interface */
void
ecdrsh(FILE *fin, FILE *fout, FILE *ferr);

#ifdef __cplusplus
};
#endif

#endif
