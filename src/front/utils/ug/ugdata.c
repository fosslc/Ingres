/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPEMBEDLIBDATA
**
LIBOBJECT = ugdata.c 
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<iicommon.h>
# include       <ugexpr.h>

/*
** Name:        ugdata.c
**
** Description: Global data for ug facility.
**
** History:
**
**      26-sep-96 (mcgem01)
**          Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-apr-2002 (loera01)
**          Added template for GetOpenRoadStyle().
**      08-apr-2002 (loera01)
**          The above change breaks Unix; amended for VMS to match the Unix
**          implementation.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile. Added LIBOBJECT hint which creates a rule
**	    that copies specified object file to II_SYSTEM/ingres/lib

*/

FUNC_EXTERN VOID  IIUGprompt();
GLOBALDEF   VOID  (*IIugPfa_prompt_func)() = IIUGprompt;
/* ugexpr.c */

GLOBALDEF       UG_EXPR_CALLBACKS       IIUGecbExprCallBack;

/* ugfrs.c */

GLOBALDEF i4            (*IIugmfa_msg_function)() = NULL;
GLOBALDEF DB_STATUS     (*IIugefa_err_function)() = NULL;

/*
**      25-feb-1997 (hanch04)
**              Added dummy function for unix
*/

/* Wrapper routine that retrieves the static isOpenROADStyle */
#if defined(UNIX) || defined (VMS)
bool
GetOpenRoadStyle()
{
    return FALSE;
}
#endif /* UNIX */
