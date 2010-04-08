/*
**      TE Data
**
**  Description
**      File added to hold all GLOBALDEFs in TE facility.
**      This is for use in DLLs only.
**
LIBRARY = IMPCOMPATLIBDATA
**
**  History:
**      12-Sep-95 (fanra01)
**          Created.
**      23-aug-1996 (wilan06)
**          removed redundant include of wn.h
**	05-mar-1997 (canor01)
**	    Moved globals from tetest.c
**	23-sep-97 (mcgem01)
**	    Move TEcmdEnabled from terestor
**      03-May-2001 (fanra01)
**          Sir 104700
**          Add TEstreamEnabled for direct I/O.
**
*/
# include <compat.h>
# include <te.h>
# include <lo.h>
# include <tc.h>
# include "teselect.h"

/*
**  Data from tewrite.c
*/
GLOBALDEF  i2   TEwriteall = 0;


/*
**  Data from temouse
*/
GLOBALDEF i2  TElstMouse = 0; /* 1 -> last "key" was left mouse click. */

/*
**  Data from tetest
*/
GLOBALDEF FILE   *IIte_fpin  = NULL;
GLOBALDEF int    IIte_use_tc = 0;
GLOBALDEF TCFILE *IItc_in    = NULL;
GLOBALDEF FILE   *IIte_fpout = NULL;

/*
** Data from terestor.c
*/
GLOBALDEF  bool TEcmdEnabled = FALSE;
GLOBALDEF  bool TEstreamEnabled = FALSE;
