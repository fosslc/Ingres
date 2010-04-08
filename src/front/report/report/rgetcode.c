/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include	<st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<cm.h>
# include	<er.h>


static RTEXT	BCommands[] =
{
	ERx("bgn "),		C_BEGIN,	RX_NOTPRIM,
	ERx("bgnblk "),	C_BLOCK,	RX_NOTPRIM,
	ERx("bgnblock "),	C_BLOCK,	RX_NOTPRIM,
	ERx("bgnwi "),	C_WITHIN,	RX_NOTPRIM,
	ERx("bgnwithin "),	C_WITHIN,	RX_NOTPRIM,
	ERx("begin "),	C_BEGIN,	RX_BEGIN,
	ERx("beginblk "),	C_BLOCK,	RX_NOTPRIM,
	ERx("beginblock "),	C_BLOCK,	RX_NOTPRIM,
	ERx("beginwi "),	C_WITHIN,	RX_NOTPRIM,
	ERx("beginwithin "),	C_WITHIN,	RX_NOTPRIM,
	ERx("blk "),		C_BLOCK,	RX_NOTPRIM,
	ERx("block "),	C_BLOCK,	RX_RTAB,
	ERx("bot "),		C_BOTTOM,	RX_NOTPRIM,
	ERx("bottom "),	C_BOTTOM,	RX_REG,
	0,		0,		0
};

static RTEXT	CCommands[] =
{
	ERx("ce "),		C_CENTER,	RX_NOTPRIM,
	ERx("cen "),		C_CENTER,	RX_NOTPRIM,
	ERx("center "),	C_CENTER,	RX_REG,
	ERx("cr "),		C_LINESTART,	RX_NOTPRIM,
	0,		0,		0
};

static RTEXT	ECommands[] =
{
	ERx("else "),	C_ELSE,		RX_LRTAB,
	ERx("elseif "),	C_ELSEIF,	RX_ELSEIF,
	ERx("end "),		C_END,		RX_END,
	ERx("endblk "),	C_EBLOCK,	RX_NOTPRIM,
	ERx("endblock "),	C_EBLOCK,	RX_LTAB,
	ERx("endif "),	C_ENDIF,	RX_LTAB,
	ERx("endlet "),	C_ELET,		RX_ELET,
	ERx("endprint "),	C_ETEXT,	RX_EPRINT,
	ERx("endwi "),	C_EWITHIN,	RX_NOTPRIM,
	ERx("endwithin "),	C_EWITHIN,	RX_LTAB,
	0,		0,		0
};

static RTEXT	FCommands[] =
{
	ERx("ff "),		C_FF,		RX_NOTPRIM,
	ERx("ffs "),		C_FF,		RX_NOTPRIM,
	ERx("fmt "),		C_FORMAT,	RX_NOTPRIM,
	ERx("format "),	C_FORMAT,	RX_REG,
	ERx("formfeed "),	C_FF,		RX_NOTPRIM,
	ERx("formfeeds "),	C_FF,		RX_REG,
	0,		0,		0
};

static RTEXT	ICommands[] =
{
	ERx("if "),		C_IF,		RX_IF,
	0,		0,		0
};

static RTEXT	LCommands[] =
{
	ERx("left "),	C_LEFT,		RX_REG,
	ERx("leftmargin "),	C_LM,		RX_REG,
	ERx("let "),		C_LET,		RX_LET,
	ERx("lf "),		C_NEWLINE,	RX_NOTPRIM,
	ERx("linebegin "),	C_LINESTART,	RX_NOTPRIM,
	ERx("lineend "),	C_LINEEND,	RX_REG,
	ERx("linefeed "),	C_NEWLINE,	RX_NOTPRIM,
	ERx("linestart "),	C_LINESTART,	RX_REG,
	ERx("lft "),		C_LEFT,		RX_NOTPRIM,
	ERx("lm "),		C_LM,		RX_NOTPRIM,
	ERx("lnbegin "),	C_TAB,		RX_NOTPRIM,
	ERx("lnend "),	C_LINEEND,	RX_NOTPRIM,
	ERx("lnstart "),	C_TAB,		RX_NOTPRIM,
	ERx("lt "),		C_LEFT,		RX_NOTPRIM,
	0,		0,		0
};

static RTEXT	NCommands[] =
{
	ERx("ne "),		C_NEED,		RX_NOTPRIM,
	ERx("need "),	C_NEED,		RX_REG,
	ERx("newline "),	C_NEWLINE,	RX_REG,
	ERx("newpage "),	C_NPAGE,	RX_REG,
	ERx("nl "),		C_NEWLINE,	RX_NOTPRIM,
	ERx("no "),		C_END,		RX_NOTPRIM,
	ERx("noff "),	C_NOFF,		RX_NOTPRIM,
	ERx("noffs "),	C_NOFF,		RX_NOTPRIM,
	ERx("nofirstff "),	C_NO1STFF,	RX_REG,
	ERx("noformfeed "),	C_NOFF,		RX_NOTPRIM,
	ERx("noformfeeds "),	C_NOFF,		RX_REG,
	ERx("nou "),		C_NOUL,		RX_NOTPRIM,
	ERx("noul "),	C_NOUL,		RX_NOTPRIM,
	ERx("nounderline "),	C_NOUL,		RX_LTAB,
	ERx("np "),		C_NPAGE,	RX_NOTPRIM,
	ERx("nullstr "),	C_NULLSTR,	RX_NOTPRIM,
	ERx("nullstring "),	C_NULLSTR,	RX_REG,
	0,		0,		0
};

static RTEXT	PCommands[] =
{
	ERx("p "),		C_TEXT,		RX_NOTPRIM,
	ERx("pagelength "),	C_PL,		RX_REG,
	ERx("pl "),		C_PL,		RX_NOTPRIM,
	ERx("pline "),	C_PRINTLN,	RX_NOTPRIM,
	ERx("pln "),		C_PRINTLN,	RX_NOTPRIM,
	ERx("pos "),		C_POSITION,	RX_NOTPRIM,
	ERx("position "),	C_POSITION,	RX_REG,
	ERx("pr "),		C_TEXT,		RX_NOTPRIM,
	ERx("prl "),		C_PRINTLN,	RX_NOTPRIM,
	ERx("prline "),	C_PRINTLN,	RX_NOTPRIM,
	ERx("prln "),	C_PRINTLN,	RX_NOTPRIM,
	ERx("print "),	C_TEXT,		RX_PRINT,
	ERx("printl "),	C_PRINTLN,	RX_NOTPRIM,
	ERx("println "),	C_PRINTLN,	RX_NOTPRIM,
	ERx("printline "),	C_PRINTLN,	RX_PRINT,
	0,		0,		0
};

static RTEXT	RCommands[] =
{
	ERx("rbfaggs "),	C_RBFAGGS,	RX_RBF,
	ERx("rbffield "),	C_RBFFIELD,	RX_RBF,
	ERx("rbfhead "),	C_RBFHEAD,	RX_RBF,
	ERx("rbfpbot "),	C_RBFPBOT,	RX_RBF,
	ERx("rbfptop "),	C_RBFPTOP,	RX_RBF,
	ERx("rbfsetup "),	C_RBFSETUP,	RX_RBF,
	ERx("rbftitle "),	C_RBFTITLE,	RX_RBF,
	ERx("rbftrim "),	C_RBFTRIM,	RX_RBF,
	ERx("right "),	C_RIGHT,	RX_REG,
	ERx("rightmargin "),	C_RM,		RX_REG,
	ERx("rm "),		C_RM,		RX_NOTPRIM,
	ERx("rt "),		C_RIGHT,	RX_NOTPRIM,
	0,		0,		0
};

static RTEXT	TCommands[] =
{
	ERx("t "),		C_TAB,		RX_NOTPRIM,
	ERx("tab "),		C_TAB,		RX_REG,
	ERx("tb "),		C_TAB,		RX_NOTPRIM,
	ERx("tfmt "),	C_TFORMAT,	RX_NOTPRIM,
	ERx("tformat "),	C_TFORMAT,	RX_REG,
	ERx("then "),	C_THEN,		RX_THEN,
	ERx("top "),		C_TOP,		RX_REG,
	ERx("tp "),		C_TOP,		RX_NOTPRIM,
	0,		0,		0
};

static RTEXT	UCommands[] =
{
	ERx("u "),		C_UL,		RX_NOTPRIM,
	ERx("ul "),		C_UL,		RX_NOTPRIM,
	ERx("ulc "),		C_ULC,		RX_NOTPRIM,
	ERx("ulchar "),	C_ULC,		RX_NOTPRIM,
	ERx("ulcharacter "),	C_ULC,		RX_REG,
	ERx("underline "),	C_UL,		RX_RTAB,
	0,		0,		0
};

static RTEXT	WCommands[] =
{
	ERx("wi "),		C_WITHIN,	RX_NOTPRIM,
	ERx("wid "),		C_WIDTH,	RX_NOTPRIM,
	ERx("width "),	C_WIDTH,	RX_REG,
	ERx("within "),	C_WITHIN,	RX_RTAB,
	0,		0,		0
};


static RTEXT	*Cmdlist[] =
{
	BCommands,
	CCommands,
	ECommands,
	FCommands,
	ICommands,
	LCommands,
	NCommands,
	PCommands,
	RCommands,
	TCommands,
	UCommands,
	WCommands,
	0
};






RTEXT	*
rffindcmd(str)
char	*str;
{
	register	RTEXT	*cmdtype;
	register	char	*sp = str;
	register	char	*wp;

	cmdtype = NULL;
	switch(*sp)
	{
		case 'b':
			cmdtype = BCommands;
			break;

		case 'c':
			cmdtype = CCommands;
			break;

		case 'e':
			cmdtype = ECommands;
			break;

		case 'f':
			cmdtype = FCommands;
			break;

		case 'i':
			cmdtype = ICommands;
			break;

		case 'l':
			cmdtype = LCommands;
			break;

		case 'n':
			cmdtype = NCommands;
			break;

		case 'p':
			cmdtype = PCommands;
			break;

		case 'r':
			cmdtype = RCommands;
			break;

		case 't':
			cmdtype = TCommands;
			break;

		case 'u':
			cmdtype = UCommands;
			break;

		case 'w':
			cmdtype = WCommands;
			break;

		default:
			break;
	}

	if (cmdtype == NULL)
	{
		return(NULL);
	}
	else
	{
		for(; cmdtype->rt_name; cmdtype++)
		{
			for (sp = str, wp = cmdtype->rt_name; *sp++ == *wp++ ;)
				;
			
			sp--;
			wp--;
			if ((*sp == '\0') && CMspace(wp))
			{
				return(cmdtype);
			}
		}
		return(NULL);
	}
/*	return(cmdtype ? rfchkcmd(str, cmdtype) : cmdtype);	*/
}




/*
**   R_GET_CODE - return the RTEXT code for the command given.
**
**	Parameters:
**		*ctext - command to check. may contain upper or lower
**			case characters.
**
**	Returns:
**		Code of the command, if it exists.  Returns -1 if
**		the command is not in the list.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		3.0, 3.4.
**
**	History:
**		3/24/81 (ps) - written.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**		24-jan-1993 (rdrane)
**			Remove NOFIRSTFORMFEED spelling (it's too long),
**			indicater that NOFIRSTFF is the RX_REG spelling,
**			and obsolete commented-out code.  b58602.
*/

ACTION	
r_gt_code(ctext)
register	char	*ctext;

{
	/* internal declarations */

	register  	RTEXT	*word;		/* ptr to command table */

	/* start of routine */


	/* mask out any upper case characters */

	CVlower(ctext);

	word = rffindcmd(ctext);
	return(word ? word->rt_code : C_ERROR);
}





/*
**   R_GET_RTEXT - get an RTEXT primary structure for the primary
**	entry for a keyword.
**
**	Parameters:
**		ctext	command name.
**
**	Returns:
**		address of primary entry for this RTEXT structure.
**		NULL if not found.
**	
**	Called by:
**		r_dmp_rep.
**
**	History:
**		9/12/82 (ps)	written.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
*/

RTEXT	*	
r_gt_rtext(ctext)
register	char	*ctext;
{
	/* internal declarations */

	register  	RTEXT	*word,*pref;	/* ptr to command table */
	register	RTEXT	**list;

	/* start of routine */

	/* mask out any upper case characters */

	CVlower(ctext);

	word = rffindcmd(ctext);
/*	for (word = Commands; word->rt_name; word++)
	{	check next command 
		if (STequal(word->rt_name,ctext) )
		{
			if (word->rt_type != RX_NOTPRIM)
			{	 this is the primary ref 
				return (word);
			}
			break;
		}
	}
*/

	if (word == NULL)
		return(NULL);

	if (word->rt_type != RX_NOTPRIM)
	{
		return(word);
	}


	/* Now try to find the primary ref */

	for (list = Cmdlist; *list; list++)
	{
		for (pref = *list; pref->rt_name; pref++)
		{
			if ((word->rt_code == pref->rt_code) && (pref->rt_type != RX_NOTPRIM))
			{
				return(pref);
			}
		}
	}

/*	for (pref = Commands; pref->rt_name; pref++)
	{
		if ((word->rt_code == pref->rt_code) &&
		    (pref->rt_type != RX_NOTPRIM))
		{	 found it 
			return (pref);
		}
	}
*/

	return (NULL);
}
