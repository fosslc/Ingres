/*
**    Copyright (c) 1987, 2001 Ingres Corporation
*/
# include	 <compat.h>
# include	 <gl.h>
# include	 <er.h>
# include	 <si.h>
# include	 <st.h>
# include	 <ds.h>
# include	 <lo.h>

/*
** The data structure definition facility
**
**	Write initialized data structures to a file for later use in ABF
**	type applications.  The structures can currently be written in 
**	C or Macro.
**	
**	History
**		10/19/83 (dd) -- VMS CL
**		8/23/84 (peterk) -- redelared DSwrite argument as
**					i4	*val;
**					for lint.
**		06/05/88 (dkh) - Changed DSD_CADDR to DSD_IADDR and
**				 CAST_PCHAR to CAST_PINT for DG.
**		3/91 (Mike S) - If a float is passed to DSwrite, it's by
**				reference. Bug 36311
**		9/91 (bobm) - fix DSD_CHAR to write '%c' instead of %c
**		06-aug-92 (davel)
**			re-write DSmstri() to support full character set
**			in strings. Note DScstri() has not been modified
**			in this manner, as it's unlikely we'll ever want
**			DS to generate C code on VMS.
**		09-nov-92 (davel)
**			Fixed bug in handling of DSD_SPTR vs DSD_STR.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	27-jun-95 (dougb)
**	    Fix AXP/VMS-specific problem with floating-point global
**	    constants in generated images.  Generate bit patterns for
**	    IEEE floats (which are not supported by the MACRO-32 compiler).
**	16-Jun-98 (kinte01)
**	    changed return type of DSaligni,DSlabi,DScstri to VOID instead
**	    of int as the routine does not return anything. Fixes
**	    DEC C 5.7 compiler warning
**      11-Dec-98 (kinte01)
**          change buf definition in DSglabi to Static to satisfy DEC C 6.0
**          compiler warnings
**	19-jul-2000 (kinte01)
**	   Add missing external function references
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	26-oct-2001 (kinte01)
**	    clean up compiler warnings
*/

FUNC_EXTERN STATUS NMt_open();

int		DSmstri();
static int	DScstri();
int		DSmacro_string();
static char	*DSsstr();

/*
** The size table.
** Indexed with a DSD_
*/
static i4 sizetab[] =
{
	4,	/* LONG */
	2,	/* short */
	1,	/* byte */
	4,	/* float */
	8,	/* double */
	1,	/* char */
	0,	/* string */
	4,	/* strp */
	4	/* addr */
};
/*
** The template table contains the templates used to pri4
** out the data type.
*/
typedef struct {
	i4	dsw_lang;
	i4	dsw_type;
	i4	(*dsw_func)();
	char	*dsw_temp;
} DSTWRITE;

static	DSTWRITE Temptab[] =
{
	DSL_C, DSD_I4,	NULL,
		"\t%d,\n",
	DSL_C, DSD_I2,	NULL,
		"\t%d,\n",
	DSL_C, DSD_I1,	NULL,
		"\t%d,\n",
	DSL_C, DSD_F4,	NULL,
		"\t%f,\n",
	DSL_C, DSD_F8,	NULL,
		"\t%f,\n",
	DSL_C, DSD_CHAR,	NULL,
		"\t'%c',\n",
	DSL_C, DSD_STR,		DScstri,
		NULL,
	DSL_C, DSD_SPTR,	DScstri,
		NULL,
	DSL_C, DSD_ADDR,	NULL,
		"\t&%s,\n",
	DSL_C, DSD_IADDR,	NULL,
		"\tCAST_PINT &%s,\n",
	DSL_C, DSD_ARYADR,	NULL,
		"\t%s,\n",
	DSL_C, DSD_PTADR,	NULL,
		"\tCAST_PVTREE &%s,\n",
	DSL_C, DSD_PLADR,	NULL, 
		"\tCAST_PVALST &%s,\n",
	DSL_MACRO, DSD_I4,	NULL,
		"\t.long\t%d\n",
	DSL_MACRO, DSD_I2,	NULL,
		"\t.word\t%d\n",
	DSL_MACRO, DSD_I1,	NULL,
		"\t.byte\t%d\n",
	DSL_MACRO, DSD_F4,	NULL,
		"\t.long\t^X%8x\t\t; s_floating %f\n",
	DSL_MACRO, DSD_F8,	NULL,
		"\t.long\t^X%8x,^X%8x\t; t_floating %f\n",
	DSL_MACRO, DSD_CHAR,	NULL,
		"\t.byte\t%d\n",
	DSL_MACRO, DSD_STR,	DSmstri,
		NULL,
	DSL_MACRO, DSD_SPTR,	DSmstri,
		NULL,
	DSL_MACRO, DSD_ADDR,	NULL,
		"\t.address\t%s\n",
	DSL_MACRO, DSD_IADDR,	NULL,
		"\t.address\t%s\n",
	DSL_MACRO, DSD_ARYADR,	NULL,
		"\t.address\t%s\n",
	DSL_MACRO, DSD_PTADR,	NULL,
		"\t.address\t%s\n",
	DSL_MACRO, DSD_PLADR,	NULL,
		"\t.address\t%s\n",
	NULL
};

typedef struct {
	i4	dsa_lang;
	i4	dsa_type;
	i4	dsa_align;
	i4	dsa_byte;
	i4	(*dsa_func)();
	char	*dsa_temp;
} DSTALIGN;

static DSTALIGN	aligntab[] =
{
	DSL_C,	DSD_I4,	DSA_C,	4,	NULL,
	NULL,
	DSL_C,	DSD_I2,	DSA_C,	2,	NULL,
	NULL,
	DSL_C,	DSD_I1,	DSA_C,	1,	NULL,
	NULL,
	DSL_C,	DSD_F4,	DSA_C,	4,	NULL,
	NULL,
	DSL_C,	DSD_F8,	DSA_C,	4,	NULL,
	NULL,
	DSL_C,	DSD_CHAR,	DSA_C,	1,	NULL,
	NULL,
	DSL_C,	DSD_STR,	DSA_C,	1,	NULL,
	NULL,
	DSL_C,	DSD_SPTR,	DSA_C,	4,	NULL,
	NULL,
	DSL_C,	DSD_ADDR,	DSA_C,	4,	NULL,
	NULL,
	DSL_C,	DSD_IADDR,	DSA_C,	4,	NULL,
	NULL,
	DSL_C,	DSD_ARYADR,	DSA_C,	4,	NULL,
	NULL,
	DSL_C,	DSD_PTADR,	DSA_C,	4,	NULL,
	NULL,
	DSL_C,	DSD_PLADR,	DSA_C,	4,	NULL,
	NULL,
	DSL_MACRO,	DSD_I4,	DSA_C,	4,	NULL,
	"\t.align\tlong\n",
	DSL_MACRO,	DSD_I2,	DSA_C,	2,	NULL,
	"\t.align\tword\n",
	DSL_MACRO,	DSD_I1,	DSA_C,	1,	NULL,
	"\t.align\tbyte\n",
	DSL_MACRO,	DSD_F4,	DSA_C,	4,	NULL,
	"\t.align\tlong\n",
	DSL_MACRO,	DSD_F8,	DSA_C,	4,	NULL,
	"\t.align\tlong\n",
	DSL_MACRO,	DSD_CHAR,	DSA_C,	1,	NULL,
	NULL,
	DSL_MACRO,	DSD_STR,	DSA_C,	1,	NULL,
	NULL,
	DSL_MACRO,	DSD_SPTR,	DSA_C,	4,	NULL,
	"\t.align\tlong\n",
	DSL_MACRO,	DSD_ADDR,	DSA_C,	4,	NULL,
	"\t.align\tlong\n",
	DSL_MACRO,	DSD_IADDR,	DSA_C,	4,	NULL,
	"\t.align\tlong\n",
	DSL_MACRO,	DSD_ARYADR,	DSA_C,	4,	NULL,
	"\t.align\tlong\n",
	DSL_MACRO,	DSD_PTADR,	DSA_C,	4,	NULL,
	"\t.align\tlong\n",
	DSL_MACRO,	DSD_PLADR,	DSA_C,	4,	NULL,
	"\t.align\tlong\n",
	NULL
};

static	VOID		DSaligni();
static	int		DSdstr();
static	DSTALIGN	*DSfalign();
static	DSTWRITE	*DSfwrite();
static	char		*DSglabi();
static	VOID		DSlabi();
static	int		DSsstri();
static	int		DStlabi();


VOID
DSbegin(file, lang, name)
	FILE	*file;
	i4	lang;
	char	*name;
{
	if (lang == DSL_MACRO)
	{
		SIfprintf(file, "\t.title\t%s\n", name);
		/*
		** BUG 1410
		** Added ing_ to psect so it doesn't interfere
		** with user psects.
		*/
		SIfprintf(file, "\t.psect\ting_%s,long\n\n", name);
	}
}

VOID
DSend(file, lang)
	FILE	*file;
	i4	lang;
{
	if (lang == DSL_MACRO)
	{
		SIfprintf(file, "\t.end\n");
	}
}

/*
** the state is carried in a bunch of static globals 
*/
static i4	Lang = 0;
static FILE	*File = NULL;
static i4	Align = 0;
static i4	Byte = 0;
static i4	Indst = 0;
static i4	loclab = 0;

STATUS
DSinit(file, lang, align, lab, vis, type)
	FILE	*file;
	i4	lang;
	i4	align;
	char	*lab;
	i4	vis;
	char	*type;
{
	if (Indst == 1)
		return DSN_RECUR;
	File = file;
	Indst = 1;
	if (lang < DSL_LOW || lang > DSL_HI)
		return DSN_LANG;
	Lang = lang;
	if (align < DSA_LOW || lang > DSA_HI)
		return DSN_ALIGN;
	Align = align;
	if (vis < DSV_LOW || vis > DSV_HI)
		return DSN_VIS;
	Byte = 0;
	DStlabi(lab, vis, type);
	return(OK);
}

static	DSTWRITE	*DSfwrite();

DSwrite(type, val, len)
	i4	type;
	char	*val;	/* (i4 *) makes lint happier than (i4) here */
	i4	len;
{
	DSTWRITE	*tp;
	DSTWRITE	*DSfwrite();

	DSaligni(type, len);
	tp = DSfwrite(type);
	if (tp == NULL)
		return DSU_NKNOWN;
	if (tp->dsw_func == NULL)
	{
		/*
		**  Fix for BUG 8601.  Need the decimal delimitor
		**  for floating point numbers due to internationalization.
		**  (dkh)
		*/
		if (type == DSD_F4 || type == DSD_F8) {
		    /*
		    ** Fix AXP/VMS-specific problem with floating-point
		    ** global constants in generated images.  Generate
		    ** values as longwords (bit patterns) since MACRO-32
		    ** doesn't handle IEEE floats.
		    ** (dougb)
		    */
		    if ( DSL_MACRO == Lang ) {
			if ( DSD_F4 == type ) {
			    f4 tmp = *((f8 *)val);

			    SIfprintf( File, tp->dsw_temp,
				      *((u_i4 *)&tmp), *((f8 *)val), '.');
			} else if ( DSD_F8 == type )
			    SIfprintf( File, tp->dsw_temp,
				      ((u_i4 *)val) [0], ((u_i4 *)val) [1],
				      *((f8 *)val), '.');
		    } else {
			SIfprintf(File, tp->dsw_temp, *((f8 *)val), '.');
		    }
		} else {
		    SIfprintf(File, tp->dsw_temp, val, '.');
		}
	}
	else
	{
		(*(tp->dsw_func))(tp, type, val, len);
	}
}

static	DSTWRITE *
DSfwrite(type)
	i4	type;
{
	register DSTWRITE	*tp;

	for (tp = Temptab; tp->dsw_lang != NULL; tp++)
		if (tp->dsw_type == type && tp->dsw_lang == Lang)
			return tp;
	return NULL;
}

/*
** force the right alignment of a structure.
** At this point we know the present byte count and what we want
** to output.
*/

static	VOID
DSaligni(type, len)
	i4	type;
	i4	len;
{
	DSTALIGN	*ap;
	i4		size;
	DSTALIGN	*DSfalign();

	ap = DSfalign(type);
	size = sizetab[type-1];
	if (ap != NULL && Byte != 0 && (Byte % ap->dsa_byte) != 0)
	{
		if (ap->dsa_temp != NULL)
			SIfprintf(File, ap->dsa_temp);
		Byte += ap->dsa_byte - (Byte % ap->dsa_byte);
	}
	Byte += size;
	if (size == 0)
		Byte += len;
}

static	DSTALIGN *
DSfalign(type)
	i4	type;
{
	register DSTALIGN	*tp;

	for (tp = aligntab; tp->dsa_lang != NULL; tp++)
		if (tp->dsa_type == type && tp->dsa_lang == Lang
		    && tp->dsa_align == Align)
			return tp;
	return NULL;
}

VOID
DSfinal()
{
	if (Lang == DSL_MACRO)
	{
		DSdstr();
		SIfprintf(File, "\t.disable\tlsb\n");
	}
	else
	{
		SIfprintf(File, "};\n");
	}
	Indst = 0;
}

/*
** generate a label for the beginning of a structure.
*/
static
DStlabi(lab, vis, type)
	char	*lab;
	i4	vis;
	char	*type;
{
	if (Lang == DSL_MACRO)
	{
		SIfprintf(File, "\t.enable\tlsb\n");
		SIfprintf(File, "\t.align\tlong\n");
	}
	DSlabi(lab, vis, type);
	if (Lang == DSL_C)
		SIfprintf(File, " = {\n");
	else
	{
		if (vis == DSV_GLOB)
			SIfprintf(File, ":");
		SIfprintf(File, ":\n");
	}
}

/*
** output a label
*/
static VOID
DSlabi(lab, vis, type)
	char	*lab;
	i4	vis;
	char	*type;
{
	char		*DSglabi();

	if (vis == DSV_LOCAL && lab == NULL)
		lab = DSglabi();
	if (lab == NULL)
		return;
	if (Lang == DSL_C && vis == DSV_LOCAL)
		SIfprintf(File, "static ");
	if (Lang == DSL_C && type != NULL)
		SIfprintf(File, "%s ", type);
	SIfprintf(File, lab);
}

static	char	*
DSglabi()
{
# 	define	labtemp		(Lang == DSL_C ? "DST%d " : "%d$ ")
	static char	buf[BUFSIZ];

	STprintf(buf, labtemp, loclab++);
	return(buf);
}

static int
DScstri(tp, type, val, len)
	DSTWRITE		*tp;
	i4			type;
	register char		*val;
	i4			len;
{
	i4	i;
	i4	slash;

	SIfprintf(File, "\t\"");
	for (i = 1; val != NULL && i != len && *val;  val++)
	{
		switch (*val)
		{
		  case '"':
			SIputc('\\', File);
			SIputc('"', File);
			i++;
			break;

		  case '\\':
			SIputc('\\', File);
			if (slash)
			{
				i++;
				slash = 0;
			}
			else
				slash = 1;
			break;

		  default:
			SIputc(*val, File);
			i++;
		}
	}
	SIfprintf(File, "\",\n");
	return;
}

int
DSmstri(tp, type, val, len)
	DSTWRITE		*tp;
	i4			type;
	register char		*val;
	i2			len;
{
	return DSmacro_string(File, tp, type, val, len);
}

/*
** DSmacro_string -- output Macro string.
**
**
**
** Description:
**	Output a VMS Macro language string constant.  This routine has
**	been extended to support writing strings which contain unprintable
**	characters. VAX Macro supports the use of expressions in .ASCIx
**	instructions that will be used for this purpose.  For example,
**	the string "hello\nworld" would be written in Macro as:
**
**		.ASCIZ	"hello"-
**			<10>-
**			"world"
**	
**	This routine keeps a static table of which characters need to be
**	written using this expression mechanism.  In addition, any characters
**	which require a delimitter other than a double quote are also indicated
**	in this table; currently only the quote character requires a different
**	delimitter. For example, the string "a \"quoted string\" is special"
**	would be written as:
**
**		.ASCIZ	"a "-
**			@"@-
**			"quoted string"-
**			@"@-
**			" is special"
**	
**	The len argument indicates the maximum number of characters to write,
**	with the EOS included in the count.  For the DSD_SPTR type, a len of 
**	0 indicates that the entire string should be written (up to the 
**	terminating null character).  For the DSD_SPTR type, characters are 
**	never written past the null terminator even if the specified len 
**	is beyond the end of the string.
**
**	The len argument for the DSD_STR type specified exactly the number of
**	characters to write, and null characters are treated literally, and 
**	included in the output.  A len of 0 for DSD_STR means to output
**	zero characters.
**
**	A few examples:
**
**	DSmacro_string(fp, tp, DSD_SPTR, "hello", 0)	!output = "hello"
**	DSmacro_string(fp, tp, DSD_SPTR, "hello", 3)	!output = "he"
**	DSmacro_string(fp, tp, DSD_SPTR, "hello", 8)	!output = "hello"
**	DSmacro_string(fp, tp, DSD_SPTR, "h\000i", 4)	!output = "h"
**	DSmacro_string(fp, tp, DSD_STR, "hello", 0)	!output = ""
**	DSmacro_string(fp, tp, DSD_STR, "hello", 1)	!output = ""
**	DSmacro_string(fp, tp, DSD_STR, "h\000ello", 4)	!output = "h\000e"
**	DSmacro_string(fp, tp, DSD_STR, "hello", 6)	!output = "hello"
**	DSmacro_string(fp, tp, DSD_STR, "hello", 7)	!output = "hello\000"
**
**	Parameters:
**		fp	-- SI file to write into.
**		tp	-- (not used)
**		type	-- DSD_STR or DSD_SPTR
**		val	-- character string to write.
**		len	-- Length of string, including EOS.
**
**
** History:
**	5-aug-92 (davel)
**		re-written based on strategy in front!abf!codegen!cgpkg.c
*/

/*
** The following table maps an input value of type u_char
** into a code of type char that indicates how the input value
** is to be represented in the output string.
** The table is initialized on the first call to DSmstri().
**
** The meaning of the codes are:
** (1) '\000' (0) indicates that the input value should be represented
**     as a decimal value in angle brackets (see above example).
** (2) ' ' (space) indicates that the input value should be represented
**     by itself (it's not a "special" character).
** (3) any other code value indicates that the input value requires an
**     alternate delimitter, specified by the code in this table. Currently
**     a code of this type only exists for the double-quote character.
**
*/

static	bool 	need_init = TRUE;
static	char	print_class[1 << BITSPERBYTE] = {0};

int
DSmacro_string(fp, tp, type, val, len)
FILE			*fp;
DSTWRITE		*tp;
i4			type;
register char		*val;
i2			len;
{

	register int	i;
	register char	*icp;			/* pointer into input string */
	register char	ic, c;

	bool quote_open = FALSE;

	if (type == DSD_SPTR)
	{
		if ( (icp = DSsstr(val, len) ) != NULL )
		{
			SIfprintf(fp, "\t.address\t%s\n", icp);
		}
		return;
	}
	/* Assert: type specified is DSD_STR, and len is exactly the number of
	** charaacters to write.  Initialize print_class table on first call.
	*/
	if (need_init)
	{
		need_init = FALSE;
		for (icp = ERx("abcdefghijklmnopqrstuvwxyz"); ic = *icp; icp++)
		{
		    	print_class[(u_char)ic] = ' ';
		}
		for (icp = ERx("ABCDEFGHIJKLMNOPQRSTUVWXYZ"); ic = *icp; icp++)
		{
	    		print_class[(u_char)ic] = ' ';
		}
		for (icp = ERx("0123456789"); ic = *icp; icp++)
		{
	    		print_class[(u_char)ic] = ' ';
		}
		for (icp = ERx(" \'\?\\|!#$%^&*()_+-={}[]:~;<>,./"); 
							ic = *icp; icp++)
		{
	    		print_class[(u_char)ic] = ' ';
		}
		print_class[(u_char)'\"'] = '@';
    	}

	SIfprintf(fp, "\t.asciz\t");
	for (i = 1; val != NULL && i < len;  i++, val++)
	{
		c = print_class[(u_char)*val];
		if (c == ' ')		/* printable, non-special character */
		{
			if (!quote_open)
			{
				SIputc('\"', fp);
				quote_open = TRUE;
			}
			SIputc(*val, fp);
		}
		else if (c == 0)	/* expression required */
		{
			if (quote_open)
			{
				/* close the open double-quote, and position
				** on indented next line for the expression
				*/
				SIfprintf(fp, "\"-\n\t\t");
				quote_open = FALSE;
			}
			/* put the value out as a decimal in angle brackets */
			SIfprintf(fp, "<%03d>-\n\t\t", (u_char)*val);
		}
		else
		{
			/* need a special delimitter - currently this is only
			** to handle the double-quote character.
			*/
			if (quote_open)
			{
				/* close the open double-quote, and position
				** on indented next line.
				*/
				SIfprintf(fp, "\"-\n\t\t");
				quote_open = FALSE;
			}
			SIfprintf(fp, "%c%c%c-\n\t\t", c, *val, c);
		}
	}
	/* if the quote is open, then just terminate with a closing quote
	** and a newline. If the quote is not open, then we are positioned
	** on an indented line, with the previous line ending with a
	** continuation (hyphen) - so we have to close with two quotes
	** and a newline.
	*/
	if (quote_open)
		SIfprintf(fp, "\"\n");
	else
		SIfprintf(fp, "\"\"\n");
}

/*
** BUG 5689
** This section changed from storing strings in a character
** buffer to using a file so that as many strings as possible
** could be written.
** The character strings are written to a temp
** file that will later be added to the end of the result
** file.
*/
static char 	filebuf[MAX_LOC] = {0};
static FILE	*fp = NULL;
static LOCATION	floc = {0};

static char *
DSsstr(str, len)
char	*str;
i2	len;
{
	static char	buf[MAX_LOC];
	LOCATION	tloc;
	i4		rval;

	/*
	** First time through set up temp file.
	*/
	if (fp == NULL)
	{
		if ((rval = NMt_open("w", "DS", "dat", &tloc, &fp)) != OK)
			return NULL;
		LOcopy(&tloc, filebuf, &floc);		
	}
	/*
	** Write to temp file
	*/
	if (str == NULL)
		str = ERx("");
	SIfprintf(fp, "%d$:", loclab);
	if (len == 0)
		len = STlength(str) + 1;
	DSmacro_string(fp, NULL, DSD_STR, str, len);
	/*
	** Write label to buf for caller to use.
	*/
	STprintf(buf, "%d$ ", loclab++);
	return buf;
}

static
DSdstr()
{
	char	buf[MAX_LOC];

	if (fp != NULL)
	{
		SIclose(fp);
		if (SIopen(&floc, "r", &fp) != OK)
			return FAIL;
		while (SIgetrec(buf, MAX_LOC, fp) == OK)
			SIputrec(buf, File);
		SIclose(fp);
		LOdelete(&floc);
	}
	fp = NULL;
}
