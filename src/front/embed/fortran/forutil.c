/*
** Define EQ_X_LANG where X is EUC or PL1 for the symbol table
*/
# define	EQ_EUC_LANG

# include	<compat.h>
# include	<er.h>
# include	<cm.h>
# include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqscan.h>
# include	<equtils.h>
# include	<eqf.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	forutil.c
** Purpose:	Define some FORTRAN-specific utilities.
**
** Defines:
**	F_declare( name, dims, size, btype, type, rec, block ) - Declare a name
-*	F_issubstring()
** Notes:
**
** History:
**	20-nov-85	- Written (mrw)
**	09-jan-1990	(barbara)
**	    Fixed bug 34495.  Null indicator reference is mistaken by
**	    preprocessor for a substring reference, e.g.
**	    ## getform formname (var:ind = fieldname) 
**	13-aug-1991	(teresal)
**	    Fix bug 39149. Allow a variable in a substring reference,
**	    e.g., var(1:len), only if the variable has not been declared 
**	    to the preprocessor. This will keep the integrity of 
**	    previous 34495 bug fix to avoid null indicator ambiguity 
**	    but will allow the user more flexibility.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
+* Procedure:	F_declare
** Purpose:	Declare a name to the symbol table.
** Parameters:
**	name	- char *	- The name to declare
**	dims	- i4		- 1 if it has explicit dimensions, else 0
**	size	- i4		- Size in bytes of object (if CHAR, 1==>string)
**	btype	- i4		- Base type of the object
**	type	- SYM *		- Type pointer (NULL for standard type)
**	rec	- i4		- Record level at which object is declared
**	block	- i4		- Block in which object is declared
** Return Values:
-*	None.
** Notes:
**
** |-----------------------+------------+------------+---------+-------+-------|
** | Declaration	   | subscript? | substring? | assign? | dims= | size= |
** |=======================+============+============+=========+=======+=======|
** | CHARACTER    nam  === |            |            |         |       |       |
** | CHARACTER*1  nam	   |     no     |     yes    |   yes   |   0   |   1   |
** |-----------------------+------------+------------+---------+-------+-------|
** | CHARACTER*10 nam	   |     no     |     yes    |   yes   |   0   |   1   |
** |-----------------------+------------+------------+---------+-------+-------|
** | CHARACTER    nam*10   |     no     |     yes    |   yes   |   0   |   1   |
** |-----------------------+------------+------------+---------+-------+-------|
** | CHARACTER    nam(10)  |     yes    |     no     |   no    |   1   |   0   |
** |-----------------------+------------+------------+---------+-------+-------|
** | CHARACTER    nam(10)*2|     yes    |  yes, w/() |yes, w/()|   1   |   1   |
** |-----------------------+------------+------------+---------+-------+-------|
** | CHARACTER*10 nam(10)  |     yes    |  yes, w/() |yes, w/()|   1   |   1   |
** |-----------------------+------------+------------+---------+-------+-------|
** | INTEGER      nam(10)  |     yes    |     no     |   no    |   1   |  2/4  |
** |-----------------------+------------+------------+---------+-------+-------|
** | INTEGER*4    nam(10)  |     yes    |     no     |   no    |   1   |   4   |
** |-----------------------+------------+------------+---------+-------+-------|
** | INTEGER      nam*2(10)|     yes    |     no     |   no    |   1   |   2   |
** |-----------------------+------------+------------+---------+-------+-------|
**
** Imports modified:
**	Only the symbol table.
*/

F_declare( name, dims, size, btype, type, rec, block )
char	*name;
i4	dims;
i4	size;
i4	btype;
SYM	*type;
i4	rec;
i4	block;
{
    register SYM	*sy;

    sy = symDcEuc( name, rec, block, syFisVAR, F_CLOSURE, SY_NORMAL );
    if (sy)
    {
	if (btype == T_CHAR)
	{
	    if (size==0 && dims==0)	/* CHARACTER foo ==> CHARACTER*1 foo */
		size = 1;
	}
	sym_s_btype( sy, btype );
	sym_s_dims( sy, dims );
	sym_s_dsize( sy, size );
      /* assign type entry for structure variables */
	if (btype == T_STRUCT && type)
	    sym_type_euc( sy, type);
    }
}

/*
+* Procedure:	F_issubstring
** Purpose:	Scan input and return TRUE iff this is a substring reference.
** Parameters:
**	None.
** Return Values:
-*	TRUE iff this is a substring reference.
** Notes:
**	The parser and scanner MUST be synched up here.
**	Scan up to a newline, ':', '=', EOF,  or ')', without nesting. 
**	That is, we disallow
**		string_name ( func(1):upper )	! looks like quel to me
**
** History:
**	21-dec-1989 (barbara)
**	    Added Unix porting changes from terry -- declare function
**	    as a i4  instead of a bool to avoid casting result.
**	09-jan-1990 (barbara)
**	    Fixed bug 34495.  Null indicator reference is mistaken by
**	    preprocessor for a substring reference, e.g.
**	    ## getform formname (var:ind = fieldname) 
**	    Verify that char beyond ':' is a digit.
**	13-aug-1991 (teresal)
**	    Fix bug 39149 - allow  a variable after ':' only if
**	    the variable is unknown to the preprocessor
**	    (i.e., not in the symbol table) - this way we can assume 
**	    it is not an indicator variable.
*/

i4
F_issubstring()
{
    register char	*cp;

    cp = SC_PTR;
    while(   *cp != SC_EOFCH
	  && *cp != '\n'
	  && *cp !=')'
	  && *cp != ':'
	  && *cp != '='
	  && *cp != ','
	 )
    {
	CMnext(cp);
    }

    if (*cp == ':')
    {
	char    	tmpbuf[SC_WORDMAX + 1];
	register char	*tp;

	CMnext(cp);
	/* Get rid of any leading white spaces */
	while (CMwhite(cp))
	    CMnext(cp);
	if (CMdigit(cp))
	    return TRUE;
	/*
	** Look ahead to see if the name is in the symbol table,
	** if it is not, then we are parsing a substring
	** reference not an indicator variable. Bug 39149
	*/
	tp = tmpbuf;

	while(	 *cp != SC_EOFCH
	      && !CMwhite(cp)
	      && *cp != ')'
	      && *cp != ':'
	      && *cp != '='
	      && *cp != ','
	      && (tp - tmpbuf < SC_WORDMAX)
	      )
	{
	    CMcpyinc(cp, tp);	
	}
	*tp = '\0';
	/* do symbol table look up here */
	if (tp != tmpbuf)
	{
	    if (sym_find(tmpbuf) != NULL)
	    	return FALSE;
	}
	return TRUE;
    }
    return FALSE;
}
