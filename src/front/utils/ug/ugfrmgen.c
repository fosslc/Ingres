/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include     <gl.h>
# include     <sl.h>
# include     <iicommon.h>
# include	<cm.h>
# include	<st.h>
# include	<fe.h>

/**
** Name:    ugfrmgen.c - shared code for 4gl generators in Vision & Windows4gl
**
**	** CODE IN THIS FILE IS SHARED BY Vision and Windows4gl 4gl generators**
**
** Description: routines that are used by both Vision (abf!fg) and
**	Windows4gl 4gl generators.
**
**	This file defines:
**
**	IIUGcvnCheckVarname	check if $varname is valid
**
** History:
**	30-apr-1992 (pete)
**	    Initial version.
**	06-dec-1995 (tsale01)
**		Change explicit checks for $variables to use CMnmstart and CMnmchar
**		so it will work for all char sets.
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
/* static's */

static	char	Dollar[] = "$",
		Underscore[] = "_";

/*{
** Name:        IIUGcvnCheckVarname - check for valid $var name.
**
** Description:
**      Check for legal $variable name.
**      - Name must start with '$' and contain only alpha-numeric and
**	  underscore characters after that. (i.e. "$", "@", "#" are not
**	  legal characters in a $variable name). This allows us to
**	  make valid substitutions of two $vars right next to each
**	  other in the source; e.g., $var1$var2$var3.
**      - Name length must be <= FE_MAXNAME.
**
** Inputs:
**      varname         - (char *) name of $variable to check.
**
** Outputs:
**
**      Returns:
**              OK if name is valid, else FAIL.
**
** History:
**      19-mar-1992 (pete)
**          Initial version.
**	30-apr-1992 (pete)
**	    Moved from abf!fg!fgdeclar.c to utils!ug so can be shared by Vision
**	    and Windows4gl.
**	19-jun-1992 (pete)
**	    Changed definition of valid $variable from "Name must start with
**	    '$' and contain legal Ingres name characters after that." To:
**	    "Name must start with '$' and contain alpha-numeric and underscore
**	    characters after that". With this change, we will no longer try
**	    to interpret $var1$var2 as a single $variable.
**	12-aug-1992 (pete)
**	    Disallow $variables with names that begin with $ii. These are
**	    reserved for built-in $variables.
**	9-feb-1993 (pete)
**	    Don't flag "$_" or "$ii" as an error.
**	17-mar-1993 (pete)
**	    Require character or underscore in first position of name
**	    (e.g. $123 is illegal).
**	6-aug-93 (blaise)
**	    Changed file name from fgutils.c to ugfrmgen.c, to avoid
**	    duplicating file names.
**	06-dec-1995 (tsale01)
**		Change explicit checks for $variables to use CMnmstart and CMnmchar
**		so it will work for all char sets.
*/
STATUS
IIUGcvnCheckVarname(varname)
char    *varname;
{
    char *name_start = varname;
    STATUS stat = OK;

    if (CMcmpcase(varname, Dollar) == 0)
    {
        /* starts with $ */

        CMnext(varname);

	if (!CMnmstart(varname))
	{
	    /* character after $ cannot start a name */
            stat = FAIL;
            goto end;
	}

        CMnext(varname);

	while (CMnmchar(varname) && *varname != EOS)
	{
	    CMnext(varname);
	}
    }
    else
    {
        /* does not start with '$' */
        stat = FAIL;
        goto end;
    }

    if   ((*varname != EOS)                      /* found bad character above */
         || (STlength(name_start) > FE_MAXNAME)) /* too long */
    {
        stat = FAIL;
        goto end;
    }

end:
    return stat;
}

