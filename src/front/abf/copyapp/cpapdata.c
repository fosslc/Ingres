/*
** Copyright (c) 2004 Ingres Corporation
*/
 
# include       <compat.h>
# include       <pc.h>          /* 6-x_PC_80x86 */
# include       <ci.h>
# include       <er.h>
# include       <ex.h>
# include       <me.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include       <fe.h>
# include       "erca.h"

/*
** Name:        data.c
**
** Description: Global data for  facility.
**
** History:
**
**      24-sep-96 (hanch04)
**          Created.
*/

GLOBALDEF bool IIabVision = FALSE;      /* resolve references in iaom */

/*
**	copyappl.c
*/

GLOBALDEF       bool    caPromptflag = FALSE;
                /*
                ** -p flag.  Default is to prompt if an object is found in
                ** the database with the same name as the one being read in.
                */ 
GLOBALDEF       bool    caReplaceflag = FALSE;  /* -r flag */
GLOBALDEF       bool    caQuitflag = FALSE;     /* -q flag */
GLOBALDEF       char    *caUname = ERx("");     /* name of user for -u flag */
GLOBALDEF       char    *caXpipe = ERx("");     /* for -X flag */

