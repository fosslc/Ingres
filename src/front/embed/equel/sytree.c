/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>				/* 6-x_PC_80x86 */
# include	<er.h>
# include	<si.h>
# include	<equel.h>			/* just for eq_dumpfile */
# define	EQ_EUC_LANG
# include	<eqsym.h>
# include	<eqrun.h>
# include	<ereq.h>

/*
+* Filename:	sytree.c
** Purpose:	Local routines for symtab tree-printing debugging.
**
** Defines:
**	trClass		- Print a string decoding the class of an entry.
**	trPrintNode	- Print a symtab node.
**	trPrval		- Print a client-formatted field.
**	trDodebug	- Dump the symbol table.
** (Data):
**	sym_prtval	- Procedure hook for printing st_value.
**	sym_prbtyp	- Procedure hook for printing st_btype.
** (Local):
**	trClass		- Return the class name of an entry.
**	trBaseType	- Return the base name of an entry.
-*	trPrnode	- Print one field of a symtab node.
** Notes:
**	Not really tree printing anymore.
**
** History:
**	23-Oct-85	- Initial version. (mrw)
**	15-sep-93 (sandyd)
**		Added recognition of VARBYTE type in trBaseType().
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	22-apr-96 (thoda04)
**		Upgraded trDodebug() to ANSI style func proto for Windows 3.1.
**	08-oct-96 (mcgem01)
**		Moved global data to eqdata.c
**	18-Dec-97 (gordy)
**	    Added support for SQLCA host variable declarations.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-mar-2001 (abbjo03)
**	    Add T_WCHAR and T_NVCH for Unicode support.
**      11-nov-2009 (joea)
**          Add T_BOO for Boolean support.
*/

GLOBALREF i4	(*sym_prtval)();
GLOBALREF i4	(*sym_prbtyp)();

/*
+* Procedure:	trClass
** Purpose:	Print a string decoding the class of an entry.
** Parameters:
**	class	- char	- The flag bits.
** Return Values:
-*	Pointer to static buffer of formatted flags.
** Notes:
**	None.
**
** Imports modified:
**	None.
*/

char *
trClass( class )
char	class;
{
    static char	buf[12];
    static struct x {
	int	mask;
	char	on, off;
    } names[] = {
	{ syFisVAR,	'V', ' ' },
	{ syFisTYPE,	'T', ' ' },
	{ syFisCONST,	'C', ' ' },
	{ syFisBASE,	'B', ' ' },
	{ syFisTAG,	'A', ' ' },
	{ syFisSYS,	'S', ' ' },
	{ syFisFORWARD,	'F', ' ' },
	{ syFisORPHAN,	'O', ' ' },
	{ 0,		' ', ' ' }
    };
    register struct x *x;

    for (x=names; x->mask; x++)
    {
	if (class & x->mask)
	    buf[x-names] = x->on;
	else
	    buf[x-names] = x->off;
    }
    buf[x-names] = '\0';
    return buf;
}

/*
+* Procedure:	trBaseType
** Purpose:	Return name of the given base type.
** Parameters:
**	btype	- i4	- The base type to translate
** Return Values:
-*	A pointer to a nul-terminated, length 7, right-justified, string.
** Notes:
**	Any non-standard (eqrun.h) type returns its numerical value.
**	If sym_prbtyp is non-null then it is called to format non-standard
**	types.  Arguments are "char *buf; i4  btype;".  Format is
**	right-justified, length 7, nul-terminated string.
** History:
**	18-Dec-97 (gordy)
**	    Added support for SQLCA host variable declarations.
**	07-mar-2001 (abbjo03)
**	    Add T_WCHAR and T_NVCH.
*/
char *
trBaseType(
i4	btype)
{
    static char	buf[8];

    switch (btype)
    {
      case T_NONE:
	return ERx("   NONE");
      case T_STRUCT:
	return ERx(" STRUCT");
      case T_BYTE:
	return ERx("   BYTE");
      case T_PACKED:
	return ERx(" PACKED");
      case T_BOO:
        return ERx("BOOLEAN");
      case T_INT:
	return ERx("    INT");
      case T_FLOAT:
	return ERx("  FLOAT");
      case T_CHAR:
	return ERx("   CHAR");
      case T_WCHAR:
	return ERx("  WCHAR");
      case T_FORWARD:
	return ERx("FORWARD");
      case T_DBV:
	return ERx("DB_VALU");
      case T_DATE:
	return ERx("   DATE");
      case T_VCH:
	return ERx("VARCHAR");
      case T_VBYTE:
	return ERx("VARBYTE");
      case T_NVCH:
	return ERx("NVARCHA");
      case T_SQLCA:
	return ERx("  SQLCA");
      case T_UNDEF:
	return ERx("  UNDEF");
      default:
	if (sym_prbtyp)
	    (*sym_prbtyp)( buf, btype );
	else
	    STprintf( buf, ERx("%7d"), btype );
	return buf;
    }
}


/*
+* Procedure:	trPrintNode
** Purpose:	Print one symbol table node.
** Parameters:
**	t	- SYM *	- The node to print.
** Return Values:
-*	None.
** Notes:
**	- Prints to the debugging output.
**	- Calls sym_prtval (if it is non-null) to format the st_val field.
**
** Imports modified:
**	None.
*/

void
trPrintNode(t)
SYM	*t;
{
    void	trPrnode();
    i4		len = STlength(sym_str_name(t));

    trPrnode( ERx("***************"));
  /* SIfprintf is broken! */
    if (len >= 7)
	trPrnode( ERx("*name: %s*"), sym_str_name(t) );
    else
	trPrnode( ERx("*name: %7s*"), sym_str_name(t) );
    trPrnode( ERx("*addr: %7x*"), t );
    trPrnode( ERx("*hash: %7x*"), t->st_hash );
    trPrnode( ERx("*type: %7x*"), t->st_type );
    trPrnode( ERx("*prev: %7x*"), t->st_prev );
    trPrnode( ERx("*par:  %7x*"), t->st_parent );
    trPrnode( ERx("*chld: %7x*"), t->st_child );
    trPrnode( ERx("*sibl: %7x*"), t->st_sibling );
    if (sym_prtval) {
	trPrnode( ERx("*valu:        *") );
	_VOID_ (*sym_prtval)( t->st_value );
    } else
	trPrnode( ERx("*valu: %7x*"), t->st_value );
    trPrnode( ERx("*vis:  %7d*"), t->st_vislim );
    trPrnode( ERx("*blok: %7d*"), t->st_block );
    trPrnode( ERx("*levl: %7d*"), t->st_reclev );
    trPrnode( ERx("*btyp: %s*"),  trBaseType(t->st_btype) );
    trPrnode( ERx("*dims: %7d*"), t->st_dims );
    trPrnode( ERx("*spac: %7d*"), t->st_space );
    trPrnode( ERx("*hval: %7d*"), t->st_hval );
    trPrnode( ERx("*ndir: %7d*"), t->st_indir );
    trPrnode( ERx("*flag: %7s*"), trClass(t->st_flags) );
    trPrnode( ERx("*dsiz: %7d*"), t->st_dsize );
    trPrnode( ERx("***************") );
    SIflush( eq->eq_dumpfile );
}

/*
+* Procedure:	trPrnode
** Purpose:	Print one line of a symtab entry to the dumpfile (internal).
** Parameters:
**	format	- char *	- A printf string.
**	arg	- char *	- An argument to "format".
** Return Values:
-*	None.
** Notes:
**	Just SIfprintf's it to the dumpfile and terminates it with a newline.
**	No more than one arg to the printf string is allowed.
**
** Imports modified:
**	<what do we touch that we don't really own>
*/

void
trPrnode( format, arg )
char	*format;
char	*arg;
{
    register FILE
		*dp = eq->eq_dumpfile;

    SIfprintf( dp, format, arg );
    SIfprintf( dp, ERx("\n") );
}

/*
+* Procedure:	trPrval
** Purpose:	Print one line of a client-formatted st_val string.
** Parameters:
**	format	- char *	- A printf string.
**	arg	- char *	- An argument to "format".
** Return Values:
-*	None.
** Notes:
**	- Terminate the line with a star-newline (prefix it with a star
**	  and the right amount of indentation)
**	- Takes a printf string format, and no more than one format arg.
**
** Imports modified:
**	None.
*/

void
trPrval( format, arg )
char	*format;
char	*arg;
{
    register FILE
		*dp = eq->eq_dumpfile;

    SIfprintf( dp, ERx("*       ") );
    SIfprintf( dp, format, arg );
    SIfprintf( dp, ERx("*\n") );
}

/*
+* Procedure:	trDodebug
** Purpose:	Dump the symbol table.
** Parameters:
**	all	- bool	- If TRUE then dump all symbols, else only non-system
**			  ones.
** Return Values:
-*	None.
** Notes:
**	Usually you don't care about pre-defined symbols.
**
** Imports modified:
**	None.
*/

void
trDodebug( bool all )
{
    i4		i;
# if defined(NT_GENERIC) && defined(__USE_LIB__)
    GLOBALDLLREF SYM	*syHashtab[];
# else              /* NT_GENERIC && __USE_LIB__ */
    GLOBALREF SYM	*syHashtab[];
# endif             /* NT_GENERIC && __USE_LIB__ */
    register FILE
		*dp = eq->eq_dumpfile;

    for (i=0; i<syHASH_SIZ; i++)
    {
	register i4	j = 0;
	register SYM	*p;

	for (p=syHashtab[i]; p; p=p->st_hash)
	{
	    if( all || ((p->st_flags & syFisSYS)==0) )
	    {
		SIfprintf( dp, ERx("syHashtab[%03d,%03d]:\n"), i, j++ );
		trPrintNode( p );
	    }
	}
    }
    sym_s_debug();
}
