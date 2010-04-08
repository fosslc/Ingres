/*
**Copyright (c) 2009 Ingres Corporation
*/
#ifdef VMS

#include <string.h>
#include <descrip.h>
#include <lib$routines.h>
#include <libclidef.h>

#endif

#include <stdio.h>
#include "compat.h"
#include "gl.h"
#include "iicommon.h"


/*
**  iidbmaxname - Retrieve the value of DB_MAXNAME into symbol IIDB_MAXNAME
**
**  Description:
**  This program is intended to retrieve the value of DB_MAXNAME
**  so that the value can be used to generate other source files that are
**  not compiled and are not able to access the DB_MAXNAME value directly.
**
**  For VMS the DB_MAXNAME value is defined as a symbol called IIDB_MAXNAME.
**
**  For Unix/Windows the DB_MAXNAME value is output to stdout.
**
**  History:
**      24-Jun-09 (bonro01)
**              Created
**
*/

int main()
{
#ifdef VMS
   char dbmaxname[8];
   int tbltype = LIB$K_CLI_LOCAL_SYM;
   $DESCRIPTOR(sym, "IIDB_MAXNAME");
   struct dsc$descriptor_s val = {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, dbmaxname};

   sprintf(dbmaxname, "%d", DB_MAXNAME);
   val.dsc$w_length = strlen(dbmaxname);
   return lib$set_symbol(&sym, &val, &tbltype);
#else
  printf("%d\n", DB_MAXNAME);
#endif
}
