/*
**Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<er.h>
# include	<si.h>
# include	<st.h>
# include	<lo.h>
# include	<nm.h>
# include	<ds.h>

/**
** Name:	ds.c -	Data Structure Module Output Facility.
**
** Description:
**	Contains the DS routines that write initialized data structures to a
**	file for later use.  The structures can only be C on UNIX.  The
**	DSL_MACRO support was too painful to comptemplate with the number of
**	processors and versions being supported, so changes were made to ABF
**	to let structures that needed to be assembler to be written in C.
**
**	Defines:
**
**	DSbegin -- Produce data header for output file.
**	DSend -- Produce data end for output file.
**	DSinit -- Produce data header for structure in output file.
**	DSwrite -- Produce data value for structure in output file.
**	DSfinal -- Produce data trailer for structure in output file.
**
** History:
**	x/xx (xxx) -- First written.
**	8/84 (jhw) -- Conditional compilation, code and data added
**			for Sun, Burroughs, and 3b.
**	11/84 (jhw) -- Conditional compilation, code and data added for
**			Pyramid and CCI.
**	12/84 (jhw) -- 3.0 Integration.
**	01/85 (jhw) -- Conditional compilation and code added for VAX
**		System V.
**	85/05/08  18:48:06  wong
**		Added conditional compilation and code for Vax System V and
**		generalized conditional code (from 2.0).  Changed 'DSinit()'
**		to output ".data" on UNIX.  Removed usage of ".string" psuedo-op
**		for Pyramid since it aligns on half-word boundaries
**		(ie, define NO_STRING for Pyramid).
**	85/05/09  11:05:56  wong
**		Removed usage of ".space" for Pyramid since it also does
**		half-word alignment.
**	85/08/28  17:49:28  daveb
**		UTS chokes on static function forward refs.
**	85/09/18  12:39:02  joe
**		Added pyramid fixes and new casts.
**	85/10/30  16:36:23  shelby
**		fixed typos.
**	85/12/03  16:35:44  seiwald
**		Conditional code for elxsi.
**	85/12/10  12:04:07  shelby
**		Changed SUN ifdef for skip instruction so
**		that it now generates a multiple of 4.
**	86/01/07  11:45:36  shelby
**		Updated fix so that skip instruction is aligned and contains
**		the correct value.
**	86/01/08  17:27:50  boba
**		Add necessary stuff for Gould.
**	86/01/09  08:19:56  seiwald
**		Revisions for Elixir's (didn't work before).
**	86/02/13  00:01:21  daveb
**		Remove all DSL_MACRO support for UNIX!!!  Add stuff necessary
**		to have DSL_C wirte correct types for abextract.c written by ABF
**	86/02/20  15:18:46  daveb
**		Add another extern type ("int") to make findform work.
**	86/04/18  17:37:58  adele
**		Set DSD_ADDR back to NULL.  It broke abf.
**	86/04/24  11:21:00  daveb
**		DSL_MACRO is no longer supported on UNIX.
**		See the comments below.
**	86/04/29  09:48:21  daveb
**		Remove some unused vars, add some ARGUSED for lint.
**	87/04/14  13:48:28  arthur
**		Changes made to DS module in PC project.
**	(bruceb)	Fix for bug 12150
**		When printing strings in C, double the backslashes
**		so that later compilation will not totally remove
**		them all.  (e.g.  user input of "a\?" will cause
**		DScstri to produce "a\\?", which the compiler will
**		output as "a\?" again.)
**	(mgw)	Bug # 13338
**		Changed DSD_F8 DSwrite handling to work correctly.
**	6/87 (bobm) add DSD_FORP & DSD_PTR for abextract support
**	10/87 (bobm) integrate 5.0:
**	20-may-88 (bruceb)
**		Change from use of DSD_CADDR to use of DSD_IADDR.
**		The original is no longer needed (effective venus.)
**	89/08/11  wong  Changed so DSD_FNADR will not print out function
**		declaration for NULL or 0 functions (i.e., those that do not
**		have function addresses, which is true for DB procedures.)  This
**		corrects JupBug #7077, the previous change for which caused
**		#7535.
**	4-june-90 (blaise)
**	    Integrated changes from termcl code line:
**		Add the new DS datatype DSD_NATARY, which will be used to 
**		generate (i4 *) for array initialisations.
**	10/8/90 (Mike S)
**		Don't generate an "extern PTR xxx" declaration for something 
**		whose address is cast to a PTR; we have no notion of its type.
**	1-mar-1991 (mgw) Bug # 35068
**		dsnwrites is a toggle, not an accumulator. Using it as an
**		accumulator breaks it when it totals any multiple of 256
**		in the DSfinal() test. Changed it to make it a toggle and
**		renamed it to dswritten.
**	27-mar-91 (rudyw)
**		Comment out text following #endif.
**	9/91 (bobm) - fix DSD_CHAR to write '%c' instead of %c
**	31-jan-92 (bonobo)
**		Deleted double-question marks; ANSI compiler thinks they
**		are trigraph sequences.
**	05-aug-92 (davel)
**		re-wrote DScstri() to support entire character set.
**	09-nov-92 (davel)
**		Fixed DScstri() to distinguish between DSD_STR and DSD_SPTR
**		types.
**      08-feb-93 (smc)
**              Forward declared a number of functions.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	27-may-97 (mcgem01) 
**	    Clean up compiler warnings.
**	18-apr-1998 (canor01)
**	    Static buffer returned by DSglabi() was not static.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

/*
** sizetab -- The size table.
**
**	Indexed with a DSD_
*/

static i1 sizetab[] =
{
	sizeof(i4),	/* LONG */
	sizeof(i4),	/* i4  */
	sizeof(i2),	/* short */
	sizeof(i1),	/* byte */
	sizeof(f4),	/* float */
	sizeof(f8),	/* double */
	sizeof(char),	/* char */
	0,		/* string */
	sizeof(char *),	/* strp */
	sizeof(char *)	/* addr */
};

/*
** Temptab -- The template table.
**
**	The template table contains the templates used to print
**	out the data type.
*/

typedef struct
{
	i4	dsw_lang;
	i4	dsw_type;
	i4	(*dsw_func)();
	char	*dsw_temp;
} DSTWRITE;

/*
** These functions, and the member declaration above, should be "void",
** but the structure initialization doesn't like it for some reason.
*/

static int	DScstri();
static char *	DSlabmap();

/*
** On UNIX, only DSL_C is now supported (1/86 daveb).
*/
DSTWRITE	Temptab[] =
{
# ifdef	MSDOS
	DSL_C, DSD_I4,	NULL, "\t%ld",
# else
	DSL_C, DSD_I4,	NULL, "\t%d",
# endif	/* MSDOS */
	DSL_C, DSD_NAT,	NULL, "\t%d",
	DSL_C, DSD_I2,	NULL, "\t%d",
	DSL_C, DSD_I1,	NULL, "\t%d",
	DSL_C, DSD_F4,	NULL, "\t%-32.7n",
	DSL_C, DSD_F8,	NULL, "\t%-32.15n",
	DSL_C, DSD_CHAR,NULL, "\t'%c'",
	DSL_C, DSD_STR,	DScstri, NULL,
	DSL_C, DSD_SPTR,DScstri, NULL,
	DSL_C, DSD_ADDR,NULL, "\t&%s",
	DSL_C, DSD_IADDR,NULL,"\tCAST_PINT &%s",
	DSL_C, DSD_ARYADR,NULL,"\t%s",
	DSL_C, DSD_PTADR,NULL, "\tCAST_PVTREE &%s",
	DSL_C, DSD_PLADR,NULL, "\tCAST_PVALST &%s",

	/* added for abextract.c support */
	DSL_C, DSD_VRADR,NULL, "\tCAST_RTSVAR &%s",
	DSL_C, DSD_FRADR,NULL, "\tCAST_FRAMEP &%s",
	DSL_C, DSD_MENU,NULL, "\tCAST_MENU %s",
	DSL_C, DSD_FNADR,NULL, "\tCAST_FNPTR %s",
	DSL_C, DSD_FORP,NULL, "\tCAST_RTSFORM &%s",
	DSL_C, DSD_PTR,NULL, "\tCAST_PTR &%s",
	DSL_C, DSD_NULL, NULL, "\tNULL",
	DSL_C, DSD_NATARY, NULL, "\t(i4 *)%s",
	0,	0,	NULL, NULL
};

/*
** Table to produce correct types for externs.  The entries
** must correspond to the eintries in Temptab[], above.
*/
static char *exttab[] =
{
	NULL,		/* DSD_I4 */
	NULL,		/* DSD_NAT */
	NULL,		/* DSD_I2 */
	NULL,		/* DSD_I1 */
	NULL,		/* DSD_F4 */
	NULL,		/* DSD_F8 */
	NULL,		/* DSD_CHAR */
	NULL,		/* DSD_STR */

	/* 
	** These are all by reference, and should have types.
	** Those with NULLs haven't yet been used.
	*/

	NULL,		/* DSD_SPTR */
	NULL,		/* DSD_ADDR -- default adress type */
	NULL,		/* DSD_IADDR */
	NULL,		/* DSD_ARYADR */
	NULL,		/* DSD_PTADR */
	NULL,		/* DSD_PLADR */
	NULL,		/* DSD_VRADR -- no extern needed */
	"FRAME",	/* DSD_FRADR */
	"MENU",		/* DSD_MENU */
	"FUNCPTR",	/* FNADR */
	NULL,		/* DSD_FORP -- no extern needed */
	NULL,		/* DSD_PTR */
	NULL,		/* DSD_NULL */
	NULL            /* DSD_NATARY */
};

/*
** aligntab -- the alignment table.
**
**	The alignment table contains the templates used to align
**	the data type.
*/

typedef struct
{
	i4	dsa_lang;
	i4	dsa_type;
	i4	dsa_align;
	i4	dsa_byte;
	i4	(*dsa_func)();
	char	*dsa_temp;
} DSTALIGN;

static DSTALIGN	aligntab[] =
{
	DSL_C,	DSD_I4,		DSA_C,	sizeof(i4),	NULL,	NULL,
	DSL_C,	DSD_NAT,	DSA_C,	sizeof(i4),	NULL,	NULL,
	DSL_C,	DSD_I2,		DSA_C,	sizeof(i2),	NULL,	NULL,
	DSL_C,	DSD_I1,		DSA_C,	sizeof(i1),	NULL,	NULL,
	DSL_C,	DSD_F4,		DSA_C,	sizeof(f4),	NULL,	NULL,
	DSL_C,	DSD_F8,		DSA_C,	sizeof(f8),	NULL,	NULL,
	DSL_C,	DSD_CHAR,	DSA_C,	sizeof(char),	NULL,	NULL,
	DSL_C,	DSD_STR,	DSA_C,	1,		NULL,	NULL,
	DSL_C,	DSD_SPTR,	DSA_C,	sizeof(char *),	NULL,	NULL,
	DSL_C,	DSD_ADDR,	DSA_C,	sizeof(char *),	NULL,	NULL,
	DSL_C,	DSD_IADDR,	DSA_C,	sizeof(int *),  NULL,	NULL,
	DSL_C,	DSD_ARYADR,	DSA_C,	sizeof(char *), NULL,	NULL,
	DSL_C,	DSD_PTADR,	DSA_C,	sizeof(char *), NULL,	NULL,
	DSL_C,	DSD_PLADR,	DSA_C,	sizeof(char *), NULL,	NULL,
	DSL_C,	DSD_VRADR,	DSA_C,	sizeof(char *), NULL,	NULL,
	DSL_C,	DSD_FRADR,	DSA_C,	sizeof(char *), NULL,	NULL,
	DSL_C,	DSD_MENU,	DSA_C,	sizeof(char *), NULL,	NULL,
	DSL_C,	DSD_FNADR,	DSA_C,	sizeof(char *), NULL,	NULL,
	DSL_C,	DSD_FORP,	DSA_C,	sizeof(char *), NULL,	NULL,
	DSL_C,	DSD_PTR,	DSA_C,	sizeof(char *), NULL,	NULL,
	DSL_C,	DSD_NULL,	DSA_C,	sizeof(char *), NULL,	NULL,
	DSL_C,  DSD_NATARY,     DSA_C,  sizeof(char *), NULL,   NULL,

	0,	0,		0,	0,		NULL, NULL
};

/* forward declarations */
STATUS   DSwrite();
static VOID     DSaligni();
static VOID     DSaligni();
static VOID     DSlabi();
static char     *DSglabi();
static STATUS   DSdumptmp();
static DSTWRITE *DSfwrite();
static DSTALIGN *DSfalign();
static VOID     DStlabi();



/*
** DSbegin -- Produce data header for output file.
**
**	This routine produces the necessary code to mark the start of the
**	output file as a valid source file in the indicated language
**	with the given name.
**
**	Parameters:
**		file -- (input only) The file pointer of the file to be used
**			for output.
**
**		lang -- (input only) The output language type (see "DS.h").
**
**		name -- (VMS, input only) The module name.
**
**	Returns:
**		STATUS -- OK.
**
**	History:
**		x/xx (xxx) -- First written.
**		1/86 (daveb)  Only DSL_C spoken here.  This is now a
**				UNIX only file.
*/

/*ARGSUSED*/
VOID
DSbegin(file, lang, name)
FILE	*file;
i4	lang;
char	*name;
{
	if (lang == DSL_MACRO)
	{
		SIprintf("Internal error:  DSinit -- DSL_MACRO no longer supported on UNIX\n");
	}
	return ;
}

/*
** DSend -- Produce data end for output file.
**
**	This routine produces the necessary code to mark the end of the
**	output file as a valid source file in the indicated language.
**	On UNIX, this routine does nothing.
**
**	Parameters:
**		file -- (input only) The file pointer of the file to be used
**			for output.
**
**		lang -- (input only) The output language type (see "DS.h").
**
**	Returns:
**		STATUS -- OK.
**
**	History:
**		x/xx (xxx) -- First written.
**		1/86 (daveb) Only DSL_C spoken here.
*/

/*ARGSUSED*/
VOID
DSend(file, lang)
FILE	*file;
i4	lang;
{
	return;
}


/*
** State of structure output
**
**		Align -- The current alignment in the structure.
**
**		Byte -- The current byte in the structure.
**
**		Lang -- The languague the data structure is being output in.
**
**		File -- The file pointer of the output file.
**
**		Indst -- The recursive entry flag for data structure.
*/

static i4	Align = 0;
static i4	Byte = 0;
static FILE	*File = NULL;
static i4	Lang = 0;
static bool	Indst = FALSE;
static i4	loclab = 0;
static bool	AddComma = FALSE;

/*
** These added to buffer the actual structure while we write
** out extern definitions for DSL_C (daveb).
**
**	dswritten	-- TRUE if a DSwrite has been done in this structure.
**	dsfilebuf	-- buffer holding the name of the scratch file.
**	dsfp		-- file pointer of the open file.
**	dsfloc		-- LOCATION of the scratch file.
*/

static bool	dswritten = FALSE;
static char 	dsfilebuf[MAX_LOC] = { 0 };
static FILE	*dsfp = NULL;
static LOCATION	dsfloc = { 0 };

/*
** DSinit -- Produce data header for structure in output file.
**
**	This routine begins output of a new data structure to a file.  If
**	the language is 'C', the type should be a valid C data type.  All
**	subsequent calls to 'DSwrite()' place the values in the data
**	structure initialized with this call.
**
**	This routine initializes the state variables and calls 'DStlabi()'
**	to write the data header for the structure.
**
**	Parameters:
**		file -- (input only) The file pointer of the file to be used
**			for output.
**
**		lang -- (input only) The output language type (see "DS.h").
**
**		align -- (input only) The alignment of the structure.
**
**		name -- (input only) The structure name.
**
**		vis -- (input only) The scope of the structure.
**
**		type -- (input only) A string containing the type of the
**			structure (C only).
**
**	Returns:
**		STATUS --	OK, if success
**				DSN_RECUR, if recursively called
**				DSN_LANG, if invalid language
**				DSN_ALIGN, if invalid alignment
**				DSN_VIS, if invalid scope.
**
**	History:
**		x/xx (xxx) -- First written.
**		1/86 (daveb) -- Added stuff to write externs for C,
**				collecting the actual initialization
**				in 'DSfile'
**		3/91 (Mike S) -- Return FAIL, not NULL, on file open failure
*/

STATUS
DSinit(file, lang, align, name, vis, type)
FILE	*file;
i4	lang;
i4	align;
char	*name;
i4	vis;
char	*type;
{
	LOCATION	tloc;

	if (Indst)
		return DSN_RECUR;

	Indst = TRUE;

	if ( lang != DSL_C )
		return DSN_LANG;

	Lang = lang;

	if (align < DSA_LOW || lang > DSA_HI)
		return DSN_ALIGN;

	Align = align;

	if (vis < DSV_LOW || vis > DSV_HI)
		return DSN_VIS;

	File = file;
	Byte = 0;
	dswritten = FALSE;
	AddComma = FALSE;

	if( dsfp == NULL )
	{
		/*
		** Send the structure initializers to the temp file dsfp,
		** and write the externs to the real File file.
		*/
		dsfp = file;

		if (NMt_open("w", "DS", "dst", &tloc, &File) != OK)
			return FAIL;
		LOcopy(&tloc, dsfilebuf, &dsfloc);
	}


	DStlabi(name, vis, type);

	return OK;
}

/*
** DSwrite -- Produce data value for structure in output file.
**
**
**	This routine writes an extern for the value, then writes an
**	initializer for the type of the value in the buffer file.
**
**	Parameters:
**		type -- (input only) The value type.
**
**		val -- (input only) The value.  Assume that all types overlay
**				the declared parameter (in particular, that for
**				an "f8" value, "*(f8 *)&val", is the value).
**
**		len -- (input only) The length if 'val' is a character string.
**			If 'len' is zero, use all the string.
**
**	Returns:
**		STATUS --	OK, if success
**				DSU_NKNOWN if unknown type.
**
** History:
**	x/xx (xxx) -- First written.
**
**	8/84 (jhw) -- Conditional compilation, code and data added
**				for Sun, Burroughs, and 3b.
**
**	1/86 (daveb) -- For DSL_C, write externs to dsfp, and
**			initializers to File.  Remove DSL_MACRO stuff.
**	08/89 (jhw) --  Changed so DSD_FNADR will not print out function
**		declaration for NULL or 0 functions (i.e., those that do not
**		have function addresses, which is true for DB procedures.)  This
**		corrects JupBug #7077, the previous change for which caused
**		#7535.
**	1-mar-1991 (mgw) Bug # 35068
**		Changed dsnwrites to dswritten and just set it to 1, instead
**		of incrementing it, since it's only being used as a toggle.
**	3/91 (Mike S) Bug 36334
**		Float4's are also passed by reference
**	16-dec-93 (swm)
**		Bug #58642
**		Changed type of val from i4 to char * to match the CL
**		specification.
*/

STATUS
DSwrite(type, val, len)
i4	type;
char	*val;
i4	len;
{
	register DSTWRITE	*tp;
	register i4		i;

	DSaligni(type, (i2)len);
	tp = DSfwrite(type);
	i = tp - Temptab;

	if (tp == NULL)
		return DSU_NKNOWN;

	dswritten = TRUE;

	/* Write the extern if needed */
	if ( NULL != exttab[ i ] && ( type != DSD_FNADR
				/* ... but not for NULL function addresses */
				|| ( !STequal((char *)val, "NULL")
					&& !STequal((char *)val, "0") ) ) )
	{
		SIfprintf( dsfp, type == DSD_FNADR ? 
					"%s %s();\n" : "extern %s %s;\n", 
				exttab[ i ], val
		);
	}

	if (AddComma)
		SIfprintf(File, ",\n");
	else
		AddComma = TRUE;

	if ( tp->dsw_func == NULL )
	{
		/*
		** Conditionally load a fn or an in value to SIfprintf().
		** Note this is essentially as follows:
		**
		**	(tp->dsw_type != DSD_Fn ? (i4)val : *(f8 *)val)
		**
		** Do not use this expression, however, since its type is
		** not defined at compile time.  (Some compilers assume the
		** same type for both subexpressions and convert one or the
		** other, which is not what was intended.)
		*/
		if (tp->dsw_type != DSD_F8 && tp->dsw_type != DSD_F4)
			SIfprintf(File, tp->dsw_temp, val);
		else
 			SIfprintf(File, tp->dsw_temp, *((f8 *)val), '.');
	}
	else
		(*(tp->dsw_func))(tp, type, val, (i2)len);

	return OK;
}

/*
** DSfwrite -- return template table entry for type.
*/


static DSTWRITE *
DSfwrite(type)
i4	type;
{
	register DSTWRITE	*tp;

	for (tp = Temptab; tp->dsw_lang != 0; tp++)
		if (tp->dsw_type == type && tp->dsw_lang == Lang)
			return tp;

	return (DSTWRITE *)NULL;
}

/*
** DSaligni -- force the right alignment of a structure.
**
**	At this point we know the present byte count and what we want
**	to output.
*/

static VOID
DSaligni(type, len)
i4	type;
i2	len;
{
	DSTALIGN	*ap;

	ap = DSfalign(type);

	if (ap != NULL && Byte != 0 && (Byte % ap->dsa_byte) != 0)
	{
		if (ap->dsa_temp != NULL)
			SIfprintf(File, ap->dsa_temp);

		Byte += ap->dsa_byte - (Byte % ap->dsa_byte);
	}

	Byte += sizetab[type-1] != 0 ? sizetab[type-1] : len;
}

/*
** DSfalign -- return alignment table entry for type.
*/

static DSTALIGN *
DSfalign(type)
i4	type;
{
	register DSTALIGN	*tp;

	for (tp = aligntab; tp->dsa_lang != 0; tp++)
		if (tp->dsa_type == type && tp->dsa_lang == Lang
		    && tp->dsa_align == Align)
			return tp;

	return (DSTALIGN *)NULL;
}


/*
** DSfinal -- Produce data trailer for structure in output file.
**
**	This routine closes the current data structure and resets
**	so that subsequent data structures can be produced.
**
**	Parameters:
**		None.
**
**	Returns:
**		STATUS -- OK.
**
**	History:
**		x/xx (xxx) -- First written.
**
**		1/86 (daveb) -- For DSL_C, dump the accumulated file
**				containing the real initializations.
*/

VOID
DSfinal()
{
	STATUS	DSdstr();

	if( !dswritten )
		SIfprintf( File, "\t0\n" );
	else
		/* Put a nl where ',' would be */
		SIfprintf( File, "\n" );

	SIfprintf(File, "\n};\n");
	DSdumptmp();

	Indst = FALSE;
	return ;
}

/*
** DStlabi -- generate a label for the beginning of a structure.
*/

static VOID
DStlabi(lab, vis, type)
char	*lab;
i4	vis;
char	*type;
{
	DSlabi(lab, vis, type);
	SIfprintf(File, " = {\n");
}

/*
** DSlabi - output a label
*/

static VOID
DSlabi(lab, vis, type)
char	*lab;
i4	vis;
char	*type;
{
	if (vis == DSV_LOCAL && lab == NULL)
		lab = DSglabi();

	if (lab == NULL)
		return;

	switch ( Lang )
	{
	case DSL_C:

		if ( vis == DSV_LOCAL )
			SIfprintf(File, "static ");
		if ( type != NULL)
			SIfprintf(File, "%s ", type);
#		ifdef MSDOS
		/* the microsoft extension "far" indicates that the 
		   data item is to be placed in it's own segment rather
		   than into the default data segment.. this is desirable
		   to reduce the size of the default data segment */
		if ( vis == DSV_LOCAL )
			SIfprintf(File, "far ");
#		endif
		SIfprintf(File, DSlabmap( lab ));
	}
}

/*
** DSglabi - generate local label.
**
**	Returns a pointer to a static location that is overwritten
**	with each call.
*/

static char *
DSglabi()
{
	static char	buf[BUFSIZ];

	(void)STprintf(buf, "DST%d", loclab++);
	return buf;
}

/*
** DScstri -- output C string.
**
**
**
** Description:
**	Output a C language string constant.  This involves (1) enclosing the 
**	entire constant within quotes; and (2) escaping any special characters
**	within the string.  See the description of the print_class table below
**	for a discussion of which characters are "special" and how they are
**	printed.
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
**	DScstri(tp, DSD_SPTR, "hello", 0)	!output = "hello"
**	DScstri(tp, DSD_SPTR, "hello", 3)	!output = "he"
**	DScstri(tp, DSD_SPTR, "hello", 7)	!output = "hello"
**	DScstri(tp, DSD_SPTR, "h\000i", 4)	!output = "h"
**	DScstri(tp, DSD_STR, "hello", 0)	!output = ""
**	DScstri(tp, DSD_STR, "hello", 1)	!output = ""
**	DScstri(tp, DSD_STR, "h\000ello", 4)	!output = "h\000e"
**	DScstri(tp, DSD_STR, "hello", 6)	!output = "hello"
**	DScstri(tp, DSD_STR, "hello", 7)	!output = "hello\000"
**
**	Parameters:
**		tp	-- (not used)
**		type	-- DSD_STR or DSD_SPTR
**		val	-- character string to write.
**		len	-- Length of string, including EOS.
**
** History:
**	5-aug-92 (davel)
**		re-written based on strategy in front!abf!codegen!cgpkg.c
**	5-nov-92 (davel)
**		Differentiate between types DSD_STR and DSD_SPTR with regard
**		to detecting a null terminator, to match what the CL doc says.
*/

/*
** The following table maps an input value of type u_char
** into a code of type char that indicates how the input value
** is to be represented in a C string or character literal.
** The table is initialized on the first call to DScstri().
**
** The meaning of the codes are:
** (1) '\000' (0) indicates that the input value should be represented
**     by an octal escape sequence.
** (2) ' ' (space) indicates that the input value should be represented
**     by itself (it's not a "special" character).
** (3) any other code value indicates that the input value should be represented
**     by the code value, preceded by a '\' (backslash).
**
** Input values which fall into cases (2) and (3) must work on all
** possible C compilers that customers might use to compile our generated C.
** For (2), I have chosen space, letters, numbers, '_' (underscore - valid
** in an identifier), and all special characters valid as "punctuation"
** (e.g. as an expression operator or in a preprocessor directive).
** For (3), I have chosen the control characters documented in 2.4.3
** in appendix A of Kernighan and Ritchie: '\n', '\t', '\b', '\r', '\f'.
** I also precede each of \ ' " ? by \.  (I precede ? by \ so that trigraphs
** won't cause problems with ANSI-compliant compilers; otherwise, e.g.
** the string "QQ(" (where Q represents a question mark) could get 
** interpreted as "[").  The \ is superflous but harmless before ' in string 
** literals and before " and ? in character literals.
**
** Note that the generated C source file will not necessarily be
** portable across platforms. This is not currently an issue in DS.
** If portability *were* an issue, we'd have problems going between platforms
** with different character sets: characters that fall into case (1)
** would cause problems where the application is interested in the characters
** rather than their binary values, and characters that fall into case (2) & (3)
** would cause problems where the application is interested in the characters'
** binary values.
*/

static	bool 	need_init = TRUE;
static	char	print_class[1 << BITSPERBYTE] = {0};

/*ARGSUSED*/
static int
DScstri(tp, type, val, len)
DSTWRITE		*tp;
i4			type;
register char		*val;
i2			len;
{
	register int	nlen = len;
	register char	*icp;			/* pointer into input string */
	register char	ic, c;

	/*
	** Initialize print_class table on first call.
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
		for (icp = ERx(" |!#$%^&*()_+-={}[]:~;<>,./"); ic = *icp; icp++)
		{
	    		print_class[(u_char)ic] = ' ';
		}
		print_class[(u_char)'\n'] = 'n';
		print_class[(u_char)'\t'] = 't';
		print_class[(u_char)'\b'] = 'b';
		print_class[(u_char)'\r'] = 'r';
		print_class[(u_char)'\f'] = 'f';

		print_class[(u_char)'\\'] = '\\';
		print_class[(u_char)'\''] = '\'';
		print_class[(u_char)'\"'] = '\"';
		print_class[(u_char)'\?'] = '\?';
    	}

	SIfprintf(File, "\t\"");

	while (val != NULL &&
		( ( type == DSD_SPTR && *val && (len == 0 || --nlen > 0) ) ||
		  ( type == DSD_STR && --nlen > 0 )
		)
	      )
	{
		c = print_class[(u_char)*val];
		if (c == ' ')		/* printable, non-special character */
		{
			SIputc(*val, File);
		}
		else if (c == 0)	/* octal escape sequence required */
		{
			SIfprintf(File, "\\%03o", (u_char)*val);
		}
		else			/* 2-character escape sequence */
		{
			SIputc('\\', File);
			SIputc(c, File);
		}
		val++;
	}
	SIputc('"', File);
}

/*
** DSdumptmp -- dump the file being saved in File to the real output
**            in dsfp, making File be the real output stream.
*/

static STATUS
DSdumptmp()
{
	char	buf[MAX_LOC];

	if (dsfp != NULL)
	{
		/*
		** Close the temp file, make it the real output stream,
		** and copy the deferred stuff into it.
		*/
		SIclose( File );
		File = dsfp;

		if (SIopen(&dsfloc, "r", &dsfp) != OK)
			return FAIL;

		while (SIgetrec(buf, MAX_LOC, dsfp) == OK)
			SIputrec(buf, File);

		SIclose(dsfp);
		LOdelete(&dsfloc);
		dsfp = NULL;
	}
	return OK;
}

/*
** DSlabmap() -- map a label to correct visiblity by the OS.
**		 NOP most places, on ELXSI changed '_' to '$'
**		 to account for system brain damage.
**
**	History:
**		1/86 (daveb)  -- Replaced DSelxcon for ABF/DS rewrite.
*/

static char *
DSlabmap( label )
char * label;
{
# ifndef ELXSI
	return (label);
# else
	char *nval;
	static char DSelxbuf[ 32 ];

	for( nval = DSelxbuf; *val != EOS; val++, nval++ )
		*nval = ( *val == '_' ) ? '$' : *val;
	*nval = EOS;

	return ( DSelxbuf );
# endif
}
