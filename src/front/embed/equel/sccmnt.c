/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<eqscan.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqlang.h>
# include	<equtils.h>
# include	<eqgr.h>
# include	<eqstmt.h>
# include	<eqgen.h>
# include	<ereq.h>


/*
+* Filename:	SCCOMMENT.C 
** Purpose:	Scan and process comments inside an Equel statement. Comment
**		processing may include various internal debugging, using
**		recognized $_ prefixes.
**
** Defines:	
**		sc_comment		- Read a comment.
** Locals
**		sc_endcmnt		- Matches end comment delims.
**		sc_symdbg		- Hook into symbol debugging.
**		dbg_who			- Copyright (c) 2004 Ingres Corporation
**		dbg_echo		- Echo comment.
**		sc_help			- Help on $_ what.
-*		dbg_osl			- Second pass by OSL.
**
** The beginning of a comment has been read.  Get the end comment token string,
** and chomp away till a match with that string is found.  Unfortunately this
** routine also reads over non-Equel lines, and maybe should be updated to
** skip lines that are non-Equel, only looking for lines beginning with a '##'.
**
** Side Effects:
**	- Strips comments from Equel code. They may eventually be stored away
**	  and dumped to the output file.
**	- The special comment $_xxx triggers debugging dumping.
**
** Notes:
**  1. Requires that the end comment delimiter is <= 2 bytes.
**
** History: 	15-nov-1984 -	Modified from original to use 1 and 2-delimiter
**				comments. (ncg)
**		22-may-1987 - 	Modified for new scanner. (ncg)
**		29-jun-87 (bab)	Code cleanup.
**		12-oct-1988 - 	Increased DBG_LEN (ncg)
**		03-dec-93 (swm)	Bug #58641
**				In sc_symdbg() db_val was overloaded with
**				either a symbol address or a hex debug level
**				value. This breaks on platforms where size of
**				pointers is greater than size of ints. Used
**				new sym_addr PTR variable with CVaxptr() to
**				hold the hex address.
**		14-apr-1994 (teresal)
**				Support C++ style comments, i.e., '//' till
**				newline is a comment in C++.
**		22-apr-1996 (thoda04)
**				Cast trPrintNode() arg to proper type.
**		14-sep-98 (mcgem01)
**				Update the copyright notice.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      11-may-99 (hanch04)
**          Update the copyright notice.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Kill strangeness re "DBG_COMMENT".  Delete duplicate defns.
*/

/* Matches end of comment */
static	i4	sc_endcmnt();

/* Lengths of debugging prefixes */
# define	DBG_LEN	10

/* dbg_vis bits */
# define	DU_Q	DML_EQUEL
# define	DU_S	DML_ESQL
# define	DU_B	(DML_ESQL|DML_EQUEL)

i4	dbg_echo();
i4	dbg_who();
i4	dbg_osl();
i4	ecs_dump();
i4	eq_dump();
i4	erec_dump();
i4	esqlcaDump();
i4	EQFWdump();
i4	frs_dump();
i4	inc_dump();
i4	lbl_dump();
i4	sc_dump();
i4	sc_help();
i4	sc_symdbg();
i4	yy_dodebug();

/* 
** Internal debugging - no need to translate to another Language (ncg).
*/
GLOBALDEF SC_DBGCOMMENT dbg_tab[] = {
 DU_B, ERx("act"),	(i4 (*)()) act_dump,    1,	ERx("\t\tDump all activate queues"),
 DU_B, ERx("arg"),	(i4 (*)()) arg_dump,    0,	ERx("\t\tDump all info about current args"),
 DU_B, ERx("csr"),   ecs_dump,	     0,	ERx("\t\tDump the cursor structures"),
 DU_B, ERx("echo"),	dbg_echo,    0,	ERx(" \"string\"\tEcho following string"),
 DU_B, ERx("eq"),	eq_dump,     0,	ERx("\t\tDump the eq structure"),
 DU_B, ERx("frs"),	frs_dump,    0,	ERx("\t\tDump frs statement info"),
 DU_B, ERx("gr"),	(i4 (*)()) gr_mechanism,GR_DUMP, ERx("\t\tDump the gr structure"),
 DU_B, ERx("help"),	sc_help,     0,	ERx("\t\tGive a list of comment commands"),
 DU_B, ERx("id"),	(i4 (*)()) id_dump,     0,	ERx("\t\tDump the id manager"),
 DU_B, ERx("inc"),	inc_dump,    0,	ERx("\t\tDump info on current include file"),
 DU_B, ERx("label"),	lbl_dump,    0,	ERx("\t\tDump the label manager"),
    0, ERx("osl"),	dbg_osl,     0,	ERx("\t\tMap for OSL pass 2"),
 DU_S, ERx("rec"),	erec_dump,   0,	ERx("\t\tDump info on current record"),
 DU_B, ERx("rep"),	(i4 (*)()) rep_dump,    0,	ERx("\t\tDump info on current repeat query"),
 DU_B, ERx("ret"),	(i4 (*)()) ret_dump,    0,	ERx("\t\tDump contents of retrieve list"),
 DU_B, ERx("scan"),	sc_dump,     0,	ERx("\t\tDump contents of scanner buffer"),
 DU_S, ERx("sqlca"),	esqlcaDump,  0, 	ERx("\t\tDump the SQLCA structure"),
 DU_B, ERx("str"),	(i4 (*)()) str_dump,    0,	ERx("\t\tDump the string table"),
 DU_B, ERx("sym"), 	sc_symdbg,   0,
    ERx("[+|=(hx)|=hx]\tTurn on symbol table debug or dump symbol table"),
 DU_B, ERx("with"),	EQFWdump,  0,	ERx("\t\tDump forms with clause info"),
    0, ERx("who"),	dbg_who,     0,	ERx("\t\tPrint compiler copyright notice"),
 DU_B, ERx("yy"),	yy_dodebug,  0, ERx("\t\tToggle yydebug"),
    0, (char *)0,	0,	     0,	ERx("")
};

GLOBALDEF SC_DBGCOMMENT	*sc_dbglang ZERO_FILL;	/* Default is unused */

/*
+* Procedure:	sc_comment
** Purpose:	Read off an Equel comment.
**
** Parameters:	boc   - char * - Begin_Comment token from the operator table.
-* Returns:	i4   - Scanner value to continue (Comment stripped).
** Notes:
** 1. Scanner has already positioned itself AFTER the operator that starts the
**    comment.  When done SC_PTR will be positioned after the closing operator
**    of the comment, unless that operator is the new line.  In that case, the
**    SC_PTR will point at the newline.
*/

i4
sc_comment( boc )
char	*boc;
{
    register i4	len;		/* Length of comment delimiter */
    char		*eoc;		/* End comment pointer */
    void		sc_dbgcomment();/* Parse debugging commands */

    len = sc_endcmnt( boc, &eoc );

    /*
    ** Loop until end of comment. When done position SC_PTR after the eoc
    ** unless the eoc is a newline. Check for EOF inside comment.
    */
    for(;;)
    {
	/* If a newline and eoc != newline then keep eating */
	if (SC_PTR[0] == '\n' && eoc[0] != '\n')
	{
	    sc_readline();
	    if (SC_PTR[0] == SC_EOFCH)
	    {
		/* Premature scanner EOF */
		er_write( E_EQ0204_scEOFCMT, EQ_ERROR, 0 );
		return SC_CONTINUE;		/* SC_PTR -> EOF */
	    }
	}

	/* If end of comment matches then return */
	if (   (len == 1 && SC_PTR[0] == eoc[0])
	    || (len == 2 && SC_PTR[0] == eoc[0] && SC_PTR[1] == eoc[1]))
	{
	    /* Skip the eoc unless it is the newline */
	    if (eoc[0] != '\n')
		SC_PTR += len;
	    return SC_CONTINUE;			/* SC_PTR -> '\n' or eoc+1 */
	}
	else if (SC_PTR[0] == '$' && SC_PTR[1] == '_')	/* $_xxx debugging */
	{
	    SC_PTR += 2;				/* Skip $_ */
	    sc_dbgcomment();				/* SC_PTR after $_nam */
	}
	else
	    CMnext(SC_PTR);
    }
}

/*
+* Procedure:	sc_dbgcomment
** Purpose:	Call the specified debugging function
** Parameters:
**	None.
** Return Values:
-*	None
** Notes:
**	Eat the function name and figure out which one it is.  Do nothing
**	for unrecognized names.  If a recognized name is followed by a '?'
**	then print its help message (and eat the '?'), else call it.
**	Position SC_PTR at next spot after name.
*/

void
sc_dbgcomment()
{
    char		d_buf[DBG_LEN +2];
    register char	*d_cp;
    SC_DBGCOMMENT	*d;
    i4			checking;		/* Done in local loop */

    /* All debugging functions are alpha-numeric (no need for funny chars) */
    for (d_cp = d_buf; (d_cp - d_buf < DBG_LEN) && CMalpha(SC_PTR); )
	CMcpyinc(SC_PTR, d_cp);
    *d_cp = '\0';

    checking = sc_dbglang ? 2 : 1;		/* 2 tables or 1 */
    for (d = dbg_tab; checking > 0; d = sc_dbglang, checking--)
    {
	for (; d->dbg_name; d++)
	{
	    if (STbcompare(d->dbg_name, 0, d_buf, 0, TRUE) == 0)
	    {
		if (SC_PTR[0] == '?')		/* Help? */
		{
		    SIprintf( ERx("    $_%s%s.\n"), d->dbg_name, d->dbg_help );
		    SC_PTR++;			/* Eat the '?' */
		}
		else
		    (*d->dbg_func)(d->dbg_arg);
		checking = 0;			/* Force drop out */
		break;
	    }
	}
    }
    SIflush( stdout );
}

/*
+* Procedure:	sc_endcmnt 
** Purpose:	Based on the Begin comment delimiter, find the End comment
**		delimiter.
** 
** Parameters:	begc - char *  - Begin comment delimiter,
**		endc - char ** - Pointer for End comment.
-* Returns:	i4    	       - Length of comment delimiter.
*/

static i4
sc_endcmnt( begc, endc )
register char	*begc;
register char	**endc;
{
    if (begc[0] == '/' && begc[1] == '*')
    {
	*endc = ERx("*/");
	return 2;
    }
    else if (dml_islang(DML_ESQL) && begc[0] == '-' && begc[1] == '-')
    {
	*endc = ERx("\n");
	return 1;
    }
    if (eq_islang(EQ_CPLUSPLUS))
    {
	if (begc[0] == '/' && begc[1] == '/')
	{
	    *endc = ERx("\n");
	    return 1;
	}
    }

    if (eq_islang(EQ_PASCAL))
    {
	if (begc[0] == '{')
	{
	    *endc = ERx("}");
	    return 1;
	} else if (begc[0] == '(' && begc[1] == '*')
	{
	    *endc = ERx("*)");
	    return 2;
	}
    } else if (eq_islang(EQ_FORTRAN) || eq_islang(EQ_BASIC))
    {
	if (begc[0] == '!')
	{
	    *endc = ERx("\n");
	    return 1;
	}
    } else if (eq_islang(EQ_ADA))
    {
        if (begc[0] == '-' && begc[1] == '-')
	{
	    *endc = ERx("\n");
	    return 1;
	}
    }
    er_write( E_EQ0222_scSCCMNT, EQ_ERROR, 1, begc );
    *endc = ERx("\n");
    return 1;
}

/*
+* Procedure:	sc_symdbg
** Purpose:	Symbol table debugging after seeing $_sym.
**
** Parameters:	None
-* Returns:	i4 	- Zero (unused).
** Notes:
**	- we saw "$_sym"
**	- if it is followed by "=hexvalue" then set debugging level
**	  to "hex value"
**	- else if it is followed by "=(symname)" then dump the entry for that
**	  name
**	- else call trDodebug() to dump the symbol table.  If we saw "$_sym+"
**	  then dump all symbols, otherwise just the non-system ones.
**
** History:	03-dec-93 (swm)	Bug #58641
**				In sc_symdbg() db_val was overloaded with
**				either a symbol address or a hex debug level
**				value. This breaks on platforms where size of
**				pointers is greater than size of ints. Used
**				new sym_addr PTR variable with CVaxptr() to
**				hold the hex address.
*/

i4
sc_symdbg()
{
    char		d_buf[DBG_LEN +1];
    register char	*d_cp;
    u_i4		db_val;
    PTR			sym_addr;

    if (SC_PTR[0] != '=')
    {
	if (SC_PTR[0] == '+')
	{
	    SC_PTR++;			/* Skip the '+' */
	    trDodebug( TRUE );
	} else
	    trDodebug( FALSE );
	return 0;
    }

    /* '$_sym = (SYM * address)' or '$_sym = debug mode' */
    SC_PTR++;				/* Skip the '=' */
    if (SC_PTR[0] == '(')		/* Magic symdump of a (SYM *) */
    {
	SC_PTR++;			/* Skip the '(' */
	for (d_cp = d_buf; CMhex(SC_PTR) && d_cp < &d_buf[DBG_LEN];)
	    CMcpyinc(SC_PTR, d_cp);
	*d_cp = '\0';
	if (SC_PTR[0] == ')')		
	{
	    SC_PTR++;			/* Skip the ')' */
	    CVaxptr( d_buf, &sym_addr );
	    trPrintNode( (SYM *) sym_addr );
	}
    }
    else
    {
	for (d_cp = d_buf; CMhex(SC_PTR) && d_cp < &d_buf[DBG_LEN];)
	    CMcpyinc(SC_PTR, d_cp);
	*d_cp = '\0';
	CVahxl( d_buf, &db_val );
	dbSETBUG( db_val );
    }
    return 0;
}

/* 
+* Procedure:	dbg_who 
** Purpose:	History message report after seeing $_who.
**
** Parameters:	None
-* Returns:	None
*/

i4
dbg_who()
{
    SIprintf( ERx("INGRES Preprocessor -\n") );
    SIprintf( ERx(" Copyright (c) 2004 Ingres Corporation\n") );
    SIflush( stdout );
    return 0;
}

/* 
+* Procedure:	dbg_echo 
** Purpose:	Echo a string constant after seeing $_echo.
**
** Parameters:	None
-* Returns:	i4 	- Zero (unused).
**
** User syntax:	$_echo "string"
*/

i4
dbg_echo()
{
    register char	*delim;

    while (CMwhite(SC_PTR) && SC_PTR[0] != '\n')
	CMnext(SC_PTR);
    if (eq->eq_flags & EQ_2POSL)
	SIprintf( ERx(" ") );
    else if (dml->dm_lang == DML_EQUEL)
	SIprintf( ERx("## ") );
    else if (dml->dm_lang == DML_ESQL)
	SIprintf( ERx("EXEC ") );
    if (SC_PTR[0] == '\'' || SC_PTR[0] == '"')
    {
	/* Search for closing quote */
	if (delim = STindex(SC_PTR+1, SC_PTR, 0))
	{
	    delim[0] = '\0';
	    SIprintf(ERx("%s\n"), SC_PTR+1);
	    delim[0] = SC_PTR[0];			/* Restore buffer */
	    SC_PTR = delim+1;				/* End of string */
	}
    }
    SIflush( stdout );
    return 0;
}

/*
+* Procedure:	sc_help
** Purpose:	Print a list of the available special comments ($_help).
**
** Parameters:	None
-* Returns:	i4 	- Zero (unused).
*/

i4
sc_help()
{
    register SC_DBGCOMMENT *d;

    SIprintf( ERx("Internal debugging commands are:\n") );
    for (d=dbg_tab; d->dbg_name; d++)
    {
	if (dml->dm_lang & d->dbg_vis)
	    SIprintf( ERx("    $_%s%s.\n"), d->dbg_name, d->dbg_help );
    }
    for (d=sc_dbglang; d && d->dbg_name; d++)
    {
	SIprintf( ERx("    $_%s%s.\n"), d->dbg_name, d->dbg_help );
    }
    SIflush( stdout );
    return 0;
}

/* 
+* Procedure:	dbg_osl 
** Purpose:	Hook for second pass of the OSL processor. (Like the C
**		preprocesorsor #line directive.)
**
** Parameters:	None
-* Returns:	i4 	- Zero (unused).
**
** User syntax:	$_osl=number (on a line by itself)
*/

i4
dbg_osl()
{
    char		d_buf[DBG_LEN +1];
    register char	*d_cp;
    i4			dbg_val;

    if (SC_PTR[0] == '=')
    {
	SC_PTR++;			/* Skip the '=' */
	for (d_cp = d_buf; CMdigit(SC_PTR) && d_cp < &d_buf[DBG_LEN]; )
	    CMcpyinc(SC_PTR, d_cp);
	*d_cp = '\0';
	if (CVan(d_buf, &dbg_val) == OK)
	{
	    if (dbg_val == 0)			/* Turn off */
		eq->eq_flags &= ~EQ_2POSL;
	    else
	    {
		eq->eq_line = dbg_val - 1;	/* Ignore this newline */
		eq->eq_flags |= EQ_2POSL;
	    }
	}
    }
    return 0;
}
