# include 	<compat.h>
# include	<er.h>
# include	<me.h>
# include	<si.h>
# include	<st.h>
# include	<cm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<equel.h>
# include	<equtils.h>
# include	<eqrun.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<eqscan.h>
# include	<ereq.h>
# include	<ere4.h>

# include	<eqcob.h>

/*
+* Filename:	COBTYPES.C 
**
** Purpose: 	Define all the routines to manage the COB_SYM structure,
**	    	that replaces the SYM value field.  Also, includes routines
**		for processing the ANSI sequence number.
**
** Defines:	cob_to_eqtype(cs, typ, len) - COB_SYM to Equel type and length.
**		cob_use_type(pic, ctyp, cs) - Picture and type to COB_SYM.
**		cob_type( typstr )	    - Usage type str to integer value.
**		cob_picture()		    - Strip off picture string.
**		cob_prtsym( symval )	    - Print COB_SYM for symbol table.
**		cob_clrsym( symval )	    - Clear COB_SYM for symbol table.
**		cob_svinit()		    - Initialize temporary save values.
**		cob_svusage(utyp)	    - Save usage value.
**		cob_svoccurs(found)         - Save occurs clause indicator.
**		cob_svpic(pic)              - Save ptr to picture string.
**		cob_rtusage()		    - Retrieve usage value.
**		cob_rtoccurs()		    - Retrieve occurs info.
**		cob_rtpic()		    - Retrieve picture string ptr.
**		Cscan_seqno(cp)		    - Scan for sequence number.
**		Cput_seqno()		    - Emit sequence number.
**		Cstr_label(label)	    - Stores label number in sequence
**					      number buffer.
** Locals:
**		cob_chr_type( cp, typ )	    - Find stop char.
-*		cob_getnum( pic, p )	    - Get picture number.
**
** Note that this file processes the COB_SYM structure that is pointed at by
** the SYM value field.  The information we store is the Cobol type, length
** and scale. The level number is also stored, but this may not be essential.
**
** Also note that double byte characters have no special meaning in picture
** strings.  The picture string translators (in cob_use_type) assume they
** can look for special characters, such as '(' and 'X'.
**
** History:	13-mar-1985	- Written for Equel/Cobol for IBM. (ncg)
**		15-may-1985	- Modified to support Numeric Edited. (ncg)
**		25-apr-1989	- Added DG support of COMP. (sylviap)
**		03-jul-1989	- Added support for flexible ordering of
**				  data declaration clauses by adding new 
**				  cob_sv and cob_rt functions. (teresa)
**		31-aug-1989	- Added sequence number routines. (teresa)
**		16-nov-1989	- Added MF COBOL support for COMP. (ncg)
**				  Fixed a couple of sequence # spacing bugs.
**		17-jul-1990	- Added decimal support. (teresal)
**      08-30-93 (huffman)
**              add <me.h>
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# define	COB_PICMAX	30	/* Max picture string length */
# define	COB_S_ERLEN	10	/* Default String error length */

# define	picNONE 0		/* Char stop types for cob_chr_type */
# define	picEDIT 1
# define	picDISP 2

/* Locals to manage a free list of COB_SYM's */
# define	COB_SYMMAX	50
static		COB_SYM		*cob_freelist = (COB_SYM *)0;

static	char	*cob_badpic = ERx("_II_");	/* Bad pictures point here */

/* Local storage to save usage, picture, and occurs info 
** temporarily while parsing the optional data declaration clauses.  
** The data declaration clauses can be in any order as long as name (or FILLER)
** follows the level number and REDEFINES follows the optional name clause.
*/
static	char	*sv_pic 	= (char*)0;	/* Ptr to picture string */	
static	i4	sv_utype 	= COB_NOTYPE;	/* Usage type */
static	bool	sv_occurs 	= FALSE;	/* Occurs clause indicator */

i4	cob_getnum();		/* Local routine to process picture number */
char	*cob_chr_type();

/* Buffer to store COBOL sequence no. (and label + period + spaces if needed) */
# define        SC_INDCOL       7		
# define 	SEQ_EMPTY	ERx("      ")	/* Empty seq number */
GLOBALDEF       char    cbl_numbuf[ SC_INDCOL + SC_WORDMAX + 3 ] ZERO_FILL;

/*
+* Procedure: 	cob_to_eqtype 
** Purpose:   	Convert a Cobol type (COB_SYM) to an Equel type and length.
** Parameters:	c_sy	- COB_SYM * - Pointer to a COB_SYM.
**		eqtyp	- i4  *	    - Address of Equel type.
**		eqlen	- i4  *	    - Address of Equel length.
** Returns:	None
**
** Evaluation is based on what we know we will have to generate to the 
** run-time routines.  Some types are basic types but some must be mapped
** to other types at runtime.
**
**	Cobol type	Length	Scale	Equel type
**
**	COMP (not DG)	1-4	0		i2
**			5-9	0		i4
**			10	0		i4 (using i4 temp at run-time)
**			11-18	0		f8
**			any	1		f8
**	COMP (DG)	1-2	0		 1
**			3-4	0		i2
**			5-6	0		 3
**			7-9	0		i4
**			10	0		i4 (using i4 temp at run-time)
**			11-18	0		f8
**			any	1		f8
**	COMP (MF)	1-10	0		i4 (using i4 temp at run-time)
**						   (but store eq len         )
**			11-18	0		f8 (using packed temp)
**			any	1		f8 (using packed temp)
**	COMP-1					f4
**	COMP-2					f8
**	COMP-3		any     1		packed decimal ("(p+2)/2" bytes)
**                      1-10    0		i4 (non-I/O statements) 
**                      1-10    0		packed decimal (I/O statments)
**	Numeric Display	1-10	0		i4 (using i4 temp at run-time)
**			otherwise		f8
**	Numeric Edited	1-10	0		i4 (using i4 temp at run-time)
**			otherwise		f8
-*	INDEX					i4
*/

void
cob_to_eqtype( cs, eqtyp, eqlen )
register	COB_SYM	*cs;
register	i4	*eqtyp, *eqlen;
{
    switch ( COB_MASKTYPE(cs->cs_type) )
    {
      case COB_COMP:
	if (cs->cs_nlen > COB_MAXCOMP)	/* Error already reported */
	{
	    *eqtyp = T_FLOAT;
	    *eqlen = sizeof(f8);
	}
	else if (cs->cs_nscale)
	{
	    *eqtyp = T_FLOAT;
	    *eqlen = sizeof(f8);
	}
	else if (cs->cs_nlen <= 4)
	{
	    *eqtyp = T_INT;
# ifdef DGC_AOS
	    if (cs->cs_nlen <= 2)	   /* S9(1) and S9(2) are 1 byte */
	       *eqlen = 1;
	    else
# endif /* DGC_AOS */
	       *eqlen = sizeof(i2);
	}
	else if (cs->cs_nlen <= COB_MAXCOMP)
	{
	    *eqtyp = T_INT;
# ifdef DGC_AOS
	    if (cs->cs_nlen <= 6)	   /* S9(5) and S9(6) are 3 bytes */
	       *eqlen = 3;
	    else
# endif /* DGC_AOS */
	    *eqlen = sizeof(i4);
	}
	return;

      case COB_1:
	*eqtyp = T_FLOAT;
	*eqlen = sizeof(f4);
	return;

      case COB_2:
	*eqtyp = T_FLOAT;
	*eqlen = sizeof(f8);
	return;

      case COB_3:
	/*
	** To provide for backwards compatibility, if a decimal has zero
	** scale with 1-10 digits, it will be treated as an integer
	** (so SLEEP type statements will still accept it).  At code
	** generation, however, I/O statements will generate this host
	** variable as an decimal while non I/O statements will move
	** it to an integer and pass a temp i4. Using a decimal with
	** zero scale rather than a temp i4 for I/O statements eliminates
	** the previous conversion problem of fitting a 10 digit
	** decimal number whose value exceeds the maximum integer value
	** into an i4.
	**
	** Note: If we ever support a cobol compiler where packed decimal
	** precision can exceed 31 (the maximum percision INGRES supports),
	** then code needs to be added to treat this host variable as a
	** float.  VMS and DG COBOL only supports packed decimal up to
	** 18 digits.
	*/

	if (cs->cs_nscale == 0 && cs->cs_nlen <= COB_MAXCOMP)
	{
	    *eqtyp = T_INT;
	}
	else
	{
	    *eqtyp = T_PACKED;
	}
	/*
	** Since a zero scale decimal will be generated as decimal not
	** integer in I/O statements - encode the length for decimal.
	*/
	*eqlen = (i4) DB_PS_ENCODE_MACRO(cs->cs_nlen,cs->cs_nscale);
	return;

      case COB_EDIT:
      case COB_NUMDISP:
	if (cs->cs_nscale || cs->cs_nlen > COB_MAXCOMP )
	{
	    *eqtyp = T_FLOAT;
	    *eqlen = sizeof(f8);
	}
	else
	{
	    *eqtyp = T_INT;
	    *eqlen = sizeof(i4);
	}
	return;

      case COB_DISPLAY:
	*eqtyp = T_CHAR;
	*eqlen = cs->cs_slen;
	return;

      case COB_INDEX:		/* Real i4 */
      case COB_BADTYPE:
	*eqtyp = T_INT;
	*eqlen = sizeof(i4);
	return;

      case COB_NOTYPE:
      case COB_RECORD:
      default:
	*eqtyp = T_NONE;
	*eqlen = 0;
	return;
    }
}

/*
+* Procedure: 	cob_use_type
** Purpose:	Process picture string and Cobol type into a COB_SYM.
** Parameters:	picture	- char * - Picture string.	( picture s9(4)v9 )
**		cobtype	- i4	 - Cobol usage type.	( usage comp-3    )
** 		cs	- Pointer to global grammar COB_SYM.
** Returns:	None
**
** Given a usage and a pic string this routine determines the
** underlying Cobol type of the variable.
**
** Possible choices are:
**	comp 	- With or without decimal scaling.
# ifdef COB_FLOATS
**	comp-1	- 4-byte floating point.
**	comp-2	- 8-byte floating point.
# endif
**	comp-3	- Packed decimal with or without decimal scaling.
**	display	- Character buffer, numeric display or numeric edited.
-*	record  - Structure reference.
*/

void
cob_use_type( picture, cobtype, cs )
char		*picture;
i4		cobtype;
register	COB_SYM	*cs;
{
    char	*cp;
    i4		err;			/* Error in numeric */
    i4		sign;			/* Signed numeric */
    i4		inscale;		/* Before/after the 'v' point */

    if (cobtype == COB_BADTYPE)	/* Error already sent */
    {
	cs->cs_type = (i1)COB_DISPLAY;
	cs->cs_slen = (i2)COB_S_ERLEN;
	return;
    }

    cs->cs_type = (i1)COB_NOTYPE;
    cs->cs_nlen = (i1)0;
    cs->cs_nscale = (i1)0;

# ifdef COB_FLOATS
    /* 
    ** Cobol comp-1 or comp-2 types are equivalent to host floats or doubles;
    ** No need to process picture string.
    */
    if (cobtype == COB_1)
    {
	cs->cs_type = (i1)COB_1;
	cs->cs_nlen = (i1)sizeof(f4);
	return;
    }
    if (cobtype == COB_2)
    {
	cs->cs_type = (i1)COB_2;
	cs->cs_nlen = (i1)sizeof(f8);
	return;
    }
# else
    if (cobtype == COB_1 || cobtype == COB_2)
    {
	er_write(E_E40011_hcbUSAGE, EQ_ERROR, 1,
		 cobtype == COB_1 ? ERx("COMP-1") : ERx("COMP-2"));
	cs->cs_type = (i1)COB_DISPLAY;
	cs->cs_slen = (i2)COB_S_ERLEN;
	return;
    }
# endif /* COB_FLOATS */

    if (cobtype == COB_INDEX)
    {
	cs->cs_type = (i1)COB_INDEX;
	cs->cs_nlen = (i1)sizeof(i4);
	return;
    }

    /*
    ** If no picture string then this must be a record type. If we went thru
    ** the above section of code, then we won't arrive here, even if there
    ** was no picture clause.
    */
    if (picture == (char *)0)
    {
	if (cobtype == COB_NOTYPE)
	    cs->cs_type = (i1)COB_RECORD;
	else
	{
	    er_write( E_E40007_hcbNEEDPIC, EQ_ERROR, 0 );
	    cs->cs_type = (i1)COB_DISPLAY;
	    cs->cs_slen = (i2)COB_S_ERLEN;
	}
	return;
    }

    /* Error detected while stripping off Picture string in cob_picture */
    if (picture == cob_badpic)
    {
	cs->cs_type = (i1)COB_DISPLAY;
	cs->cs_slen = (i2)COB_S_ERLEN;
	return;
    }

    /* First check to see if we have to use COB_DISPLAY */
    cp = cob_chr_type(picture, picDISP);
    if (*cp)	/* COB_DISPLAY will use type T_CHAR - now let's get length */
    {
	cs->cs_type = (i1)COB_DISPLAY;
	for (cp = picture; *cp; )
	{
	    if (*cp == '(')			/* pic X(100) = 100 not 101 */
		cs->cs_slen += cob_getnum( picture, &cp) -1;
	    else				/* pic XXXXXX */
	    {
		CMbyteinc(cs->cs_slen, cp);
		CMnext(cp);
	    }
	}
	return;
    }

    /* We will be using a numeric type - let's see if it is numeric edited */
    cp = cob_chr_type(picture, picEDIT);
    if (*cp)		/* We will be using numeric edited, decide on scale */
    {
	cs->cs_type = (i1)COB_EDIT;
	for (cp = picture; *cp; )
	{
	    if (*cp == '.' || *cp == 'V' || *cp == 'v')
	    {
		cs->cs_nscale = (i1)1;
		return;
	    }
	    if (*cp == '(')			/* pic Z(4) = 4 not 5 */
		cs->cs_nlen += cob_getnum( picture, &cp) -1;
	    else				/* pic ZZZZ */
	    {
		/* Not really length but close */
		CMbyteinc(cs->cs_nlen, cp);
		CMnext(cp);
	    }
	}
	return;
    }

    /* 
    ** Must be numeric picture string.
    **
    ** There is a picture string, and this must be numeric either Usage
    ** Comp, Comp-3 or Display. Other systems may use other Comp types here 
    ** (RMCobol uses Comp-1 to represent an i2.)
    */
    err = 0;
    sign = 0;		
    inscale = 0;

    for (cp = picture; *cp && !err;)
    {
	if (*cp == 's' || *cp == 'S')
	{
	    if (sign)
	    {
		err++;
		er_write( E_E40012_hcbS2PIC, EQ_ERROR, 1, picture );
	    }
	    sign++;
	    cp++;
	} else if (*cp == '9')
	{
	    cp++;
	    if (inscale)			/* Passed the 'v' or '.' */
	    {
		if (*cp == '(')				/* pic s9v9(3) */
		    cs->cs_nscale += cob_getnum( picture, &cp);
		else					/* pic s9v999  */
		    cs->cs_nscale++;
	    } else
	    {
		if (*cp == '(')				/* pic s9(4) */
		    cs->cs_nlen += cob_getnum( picture, &cp);
		else					/* pic s9999 */
		    cs->cs_nlen++;
	    }
	} else if (*cp == 'v' || *cp == 'V')
	{
	  /* Set scale flag to put rest in cs_nscale */
	    if (inscale)
	    {
		err++;
		er_write( E_E40013_hcbV2PIC, EQ_ERROR, 1, picture );
	    }
	    inscale++;
	    cp++;
	} else
	{
	    /* Unsupported type - probably some editing symbols included */
	    er_write( E_E40002_hcbBAD, EQ_ERROR, 1, picture );
	    err++;
	}
    }

    if (err)
    {
	cs->cs_type = (i1)COB_COMP;
	cs->cs_nlen = (i1)sizeof(i4);
	return;
    }
    /*
    ** At this point cs_nlen is the number of positions before the
    ** decimal point and cs_nscale the number after the decimal.
    ** The total size of the data item is the sum of cs_nlen + cs_nscale.
    ** sign is set if a sign character was specified.
    */
    cs->cs_nlen += cs->cs_nscale;

    /* Allow s9(10) but do not allow s9(9)v9(1) */
    if (   (cobtype == COB_COMP)
	&& (   (cs->cs_nlen > COB_MAXCOMP)
	    || (cs->cs_nlen == COB_MAXCOMP && cs->cs_nscale)
	   )
       )
	er_write( E_E40003_hcbCOMPLEN, EQ_ERROR, 1, picture );
    if (cobtype == COB_NOTYPE || cobtype == COB_DISPLAY)
	cobtype = COB_NUMDISP;
    if (sign)
	cs->cs_type = cobtype;
    else
	cs->cs_type = cobtype | COB_NOSIGN;
}

/*{
+*  Name: cob_chr_type - Scan a string for a char of a given type.
**
**  Description:
**	This routine looks for numeric edited and simply display picture
**	characters.  Some characters are assigned types which have meaning
**	within picture strings.
**
**  Inputs:
**	cp		- The string to search.
**	typ		- The "terminator" char type
**
**  Outputs:
**	Returns:
**	    Pointer to the char at which we stopped, or to the nul terminator.
**	Errors:
**	    None.
-*
** Notes:
**	The string may contain double-byte characters, which are not
**	candidates for being of the stop type.
**  Side Effects:
**	None.
**  History:
**	11-aug-1987 - Extracted from inline code. (mrw)
*/

char *
cob_chr_type( cp, typ )
char	*cp;
i4	typ;
{
    i4		chtyp;		/* Display type of current character */

    for (; *cp; CMnext(cp))
    {
	if (CMdbl1st(cp))	/* No double byte special chars yet */
	    continue;
	/*
	** Do not check 0, 9, v, s, (, ) as special characters even
	** though some can be considered EDIT or DISP.  They are reserved
	** for real numeric picture strings.
	*/
	switch (*cp)
	{
	  /*
	  ** EDIT characters as defined in PICTURE clause. 'P' is a special
	  ** case and is treated as EDIT.
	  */
	  case '$': case 'B': case 'b': case '/': case 'Z': case 'z':
	  case '*': case ',': case '.': case '-': case '+':
	  case 'C': case 'c': case 'D': case 'd': case 'P': case 'p':
	    chtyp = picEDIT;
	    break;

	  /* DISPLAY characters as defined in PICTURE clause */
	  case 'A': case 'a': case 'X': case 'x':
	    chtyp = picDISP;
	    break;

	  default:	/* Ignore all other characters */
	    chtyp = picNONE;
	    break;
	} /* switch cp */
	if (typ == chtyp)			/* Found a type match */
	    break;
    } /* for searching type match */
    return cp;
}

/*
+* Procedure:	cob_getnum 
** Purpose:	Used by picture string processor to skip over '(num)'
** Parameters:	pic - char *  - Picture string for errors.
**		p   - char ** - Pointer to current spot '(' in pic string.
-* Returns:	i4  - Length of number.
*/

i4
cob_getnum( pic, p )
char		*pic;
char		**p;
{
    char		numbuf[ SC_NUMMAX +3 ];	/* 1: nul; 2: ')'; 3: Kanji */
    register char	*s;
    register char	*cp;
    i4			cnt;
    i4			flag;

    cp = *p;		/* Pointing at '(' */
    cp++;		/* Should be first digit */
    s = numbuf;
    flag = FALSE;
    do {
	if (*cp == ')')
	{
	    flag = TRUE;
	    break;
	}
	CMcpyinc( cp, s );
    } while (!flag && *cp && s < &numbuf[SC_NUMMAX]);
    *s = '\0';
    if (!flag)		/* No closing right paren */
    {
	er_write( E_E4000F_hcbPICNUM, EQ_ERROR, 2, pic, numbuf );
	cnt = 1;
    } else
    {
	CMnext( cp );		/* Eat the closing right paren */
	if (CVan(numbuf, &cnt) != OK)
	{
	    er_write( E_E4000F_hcbPICNUM, EQ_ERROR, 2, pic, numbuf );
	    cnt = 1;
	}
    }
    *p = cp;
    return cnt;
}

/*
+* Procedure:	cob_type 
** Purpose:	Map a Cobol usage type string to an internal value.
** Parameters:	typstr	-  char * - Usage type string.	( usage comp-2 )
-* Returns:	i4     -  Internal value for type.	( COB_2        )
** Notes:
**	MF allows COMP-4 and COMP-5 (treat as COMP).
**
**	PACKED-DECIMAL is allowed and treated as COMP-3.
*/

i4
cob_type( typstr )
char	*typstr;
{
    char	*minus;

    if ((minus = STindex(typstr, ERx("-"), 0)) == (char *)0)
	return COB_COMP;
    switch (*++minus)
    {
      case '1':
	return COB_1;
      case '2':
	return COB_2;
      case '3':
	return COB_3;
# ifdef CMS
      case '4':
	return COB_COMP;
# endif /* CMS */ 
# ifdef MF_COBOL
      case '4':
      case '5':
	return COB_COMP;
# endif /* MF_COBOL */
      case 'd':
      case 'D':
	return COB_3;			/* PACKED-DECIMAL */
      default:
	return COB_NOTYPE;
    }
}

/*
+* Procedure:	cob_picture 
** Purpose:	Strip off Cobol picture string.
**
** Parameters:	None, but is synched up with the scanner.
** Returns:	char * - Pointer to picture string (after being added to 
**			 global string table).
**
** Note that this routine must be synchronized with the parser that must have
** just seen the "picture" token with the picture string still to be read.
** The Picture string may be preceeded by whitespace and is terminated
-* at the first whitespace character or period encountered.
**
** If a period '.' is the last character in the picture it is not considered 
** a part of the picture clause and is not added.
**
** Syntax:	PICTURE [IS]  Pic_String[' '|'.']
**		       ^
**		       Parser
*/

char	*
cob_picture()
{
    register char	*cp;
    char		picbuf[ SC_WORDMAX +1];
    i4			picerr;

    while (CMspace(SC_PTR) || *SC_PTR == '\t')
	CMnext(SC_PTR);
    cp = picbuf;
    if (*SC_PTR == 'i' || *SC_PTR == 'I')
    {
	CMcpyinc( SC_PTR, cp );
	if (*SC_PTR == 's' || *SC_PTR == 'S')
	{
	    CMcpyinc( SC_PTR, cp );
	    *cp = '\0';
	    gen_host( G_H_KEY, picbuf );
	    while (CMspace(SC_PTR) || *SC_PTR == '\t')
		CMnext(SC_PTR);
	    cp = picbuf;
	}
    }
    if (   *SC_PTR == SC_EOFCH
	|| *SC_PTR == '\n'
	|| (*SC_PTR == '.' && CMwhite(SC_PTR+1))
       )
    {
	er_write( E_E40009_hcbNOPIC, EQ_ERROR, 0 );
	return (char *)0;
    }

    /* Eat until trailing white space or terminating period */
    for (picerr=0; !CMwhite(SC_PTR) && *SC_PTR != SC_EOFCH;)
    {
	/*
	** If we are on the terminating period then stop eating, and leave
	** SC_PTR at terminator
	*/
	if (*SC_PTR == '.' && CMwhite(SC_PTR+1))
	    break;
	CMcpyinc( SC_PTR, cp );
	if (cp >= &picbuf[COB_PICMAX])
	{
	    if (!picerr)
	    {
		*cp = '\0';
		er_write( E_E4000E_hcbPICLONG, EQ_ERROR, 2, picbuf,
							    er_na(COB_PICMAX) );
		picerr++; 
	    } 
	    cp = picbuf; 
	}
    }
    /* This is it, return string */
    *cp = '\0';
    return picerr ? cob_badpic : str_add( STRNULL, picbuf ); 
} /* cob_picture */


/*
+* Procedure:	cob_prtsym  
** Purpose:	Print the COB_SYM structure within the SYM value field.
** Parameters:	symval 	- i4  * - The SYM value field (really a COB_SYM).
-* Returns:	0 	- Dummy return value.
**
** This routine is called indirectly be setting sym_prtval to point at it.
** It calls trPrval on each member in the structure.
*/

i4
cob_prtsym( symval )
i4	*symval;
{
    register COB_SYM	*cs = (COB_SYM *)symval;
    register i4	typ;
    static char		*cb_type_names[] = {
	ERx("COMP"),			/* COB_COMP */
	ERx("COMP-1"),		/* COB_1 */
	ERx("COMP-2"),		/* COB_2 */
	ERx("COMP-3"),		/* COB_3 */
	ERx("COMP-4"),		/* COB_4 */
	ERx("COMP-5"),		/* COB_5 */
	ERx("COMP-6"),		/* COB_6 */
	ERx("NO TYPE"),		/* COB_NOTYPE */
	ERx("DISPLAY"),		/* COB_DISPLAY */
	ERx("RECORD"),		/* COB_RECORD */
	ERx("INDEX"),		/* COB_INDEX */
	ERx("NUMERIC DISPLAY"),	/* COB_NUMDISP */
	ERx("NUMERIC EDITED"),	/* COB_EDIT */
	ERx("BAD TYPE")		/* COB_BADTYPE */
    };

    trPrval( ERx("level:  %2d"), cs->cs_lev );
    typ = cs->cs_type & ~COB_NOSIGN;
    if (typ >= COB_COMP && typ <= COB_BADTYPE)
    {
	if (cs->cs_type & COB_NOSIGN)
	    trPrval( ERx("type:    %s (unsigned)"), cb_type_names[typ] );
	else
	    trPrval( ERx("type:    %s"), cb_type_names[typ] );
    } else
    {
	trPrval( ERx("type:   %2d"), cs->cs_type );
    }
    if (cs->cs_type == COB_DISPLAY)
	trPrval( ERx("slen:   %d"), cs->cs_slen );
    else
    {
	trPrval( ERx("nlen:   %2d"), cs->cs_nlen );
	trPrval( ERx("nscale: %2d"), cs->cs_nscale );
    }
    return 0;
}

/*
+* Procedure:	cob_clrsym 
** Purpose:	Clear out the COB_SYM structure within the SYM value field.
** Parameters:	symval 	- i4  * - The SYM value field (really a COB_SYM).
-* Returns:	0 	- Dummy return value.
**
** This routine is called indirectly be setting sym_delval to point at it.
*/

i4
cob_clrsym( symval )
i4	*symval;
{
    register	COB_SYM	*cs = (COB_SYM *)symval;

    if (cs != (COB_SYM *)0)
    {
	MEfill( (u_i2)sizeof(*cs), '\0', (char *)cs );
	cs->cs_next = cob_freelist;
	cob_freelist = cs;
    }
    return 0;
}

/*
+* Procedure:	cob_newsym 
** Purpose:	Get a new COB_SYM element and return it to caller.
** Parameters: 	None
-* Returns: 	New COB_SYM pointer.
**
** Attempt to pick the element off freelist.  If there are no more on the 
** freelist then allocate COB_SYMMAX's more for the freelist and return
** the first one. 
*/

COB_SYM *
cob_newsym()
{
    register  COB_SYM 	*cs;
    register  i4     	i;

    if (cob_freelist == (COB_SYM *)0)
    {
	cob_freelist = (COB_SYM *) MEreqmem(0,COB_SYMMAX*sizeof(*cs),TRUE,NULL);
	if (cob_freelist == NULL)
	    er_write( E_EQ0001_eqNOMEM, EQ_FATAL, 2,
			er_na(COB_SYMMAX*sizeof(*cs)), ERx("cob_newsym()") );
	/* Make array of storage into linked list */
	for (i = 1, cs = cob_freelist; i < COB_SYMMAX; i++, cs++)
	    cs->cs_next = cs + 1;
	cs->cs_next = (COB_SYM *)0;
    }
    cs = cob_freelist;
    cob_freelist = cs->cs_next;
    cs->cs_next = (COB_SYM *)0;			/* Disconnect from list */
    cs->cs_type = (i1)COB_NOTYPE;
    return cs;
}

/*
+* Procedure:	cob_svinit
** Purpose:	Initialize the temporary save data declaration info for usage, 
**		picture, and occurs to their default values.
** Parameters: 	None
-* Returns: 	None
**
*/

void
cob_svinit()
{
    sv_utype = COB_NOTYPE;	/* Init to the default type */
    sv_occurs = FALSE;		/* Init to no OCCURS clause found */
    sv_pic = (char*)0;		/* Init to no PICTURE string */
}

/*
+* Procedure:	cob_svusage
** Purpose:	Store the usage type temporarily.
** Parameters: 	utyp 	- i4	    - Usage type.	
-* Returns: 	None
**
*/

void
cob_svusage(utyp)
i4	utyp;
{
    if (sv_utype != COB_NOTYPE)	/* Have we already found a USAGE clause? */
    {
	er_write(E_E40017_hcbDUPL, EQ_ERROR, 1, ERx("USAGE"));
    }
    else
    {
        sv_utype = utyp;
    }
}

/*
+* Procedure:	cob_svoccurs 
** Purpose:	Store the OCCURS clause indicator.
** Parameters: 	found	- bool	    - Indicates if an OCCURS clause was found.
-* Returns: 	None
**
*/

void
cob_svoccurs(found)
bool	found;
{
    if (sv_occurs != FALSE)	/* Have we already found an OCCURS clause? */
    {
	er_write(E_E40017_hcbDUPL, EQ_ERROR, 1, ERx("OCCURS"));
    }
    else
    {
        sv_occurs = found;
    }
}

/*
+* Procedure:	cob_svpic 
** Purpose:	Store temporarily the ptr to the picture string.
** Parameters: 	pic	- char *    - Ptr to the picture string
-* Returns: 	None
**
*/

void
cob_svpic(pic)
char	*pic;
{
    if (sv_pic != (char*)0)	/* Have we already found an PICTURE clause? */
    {
	er_write(E_E40017_hcbDUPL, EQ_ERROR, 1, ERx("PICTURE"));
    }
    else
    {
        sv_pic = pic;		/* save the picture string ptr */
    }
}

/*
+* Procedure:	cob_rtusage 
** Purpose:	Retrieve the stored usage type and return it to caller.
** Parameters: 	None
-* Returns: 	i4	- Usage type
**
*/

i4
cob_rtusage()
{
    return sv_utype;
}

/*
+* Procedure:	cob_rtoccurs 
** Purpose:	Retrieve the OCCURS clause indicator and return it to caller.
** Parameters: 	None
-* Returns: 	bool	- OCCURS indicator, i.e., was an array was declared.
**
*/
bool
cob_rtoccurs()
{
    return sv_occurs;
}

/*
+* Procedure:	cob_rtpic 
** Purpose:	Retrieve the ptr to the picture string and return it to caller.
** Parameters: 	None
-* Returns: 	char *	- Ptr to the picture string
**
*/
char *
cob_rtpic()
{
    return sv_pic;
}

/*
+* Procedure:   Cscan_seqno
** Purpose:     Scans for an ANSI COBOL sequence number and stores it to
**              be output later on.
**
** Parameters:  cp - current line pointer
** Return Values: Returns pointer to the position in the line after the 
-*		  sequence number.
** Notes:
**      1. This function is only called for the ANSI command flag.
**	2. Allows commenting out of ESQL lines by using a comment indicator
**	   in column 1.
**	3. Allow the word "EXEC" to be in sequence area.
**	4. If the sequence number ends up as spaces then ignore it as it
**	   only throws off code generation format later.
**	5. There will never be tabs in the sequence number.
*/

char *
Cscan_seqno(cp)
char	*cp;
{
    register i4	i = 1;
    register char	*nb = cbl_numbuf;       /* sequence number buffer */
    register char	*ecp = cp;		/* search for EXEC */
    char		*ocp = cp;		/* Original cp */

    cbl_numbuf[0] = '\0';       /* default no sequence number */

    /* Allow commenting-out of ESQL line using * in col 1 */
    if (*cp == '*')
	return cp;

    /*
    ** Skip area of white space - search for EXEC.  Ignore tabs as these
    ** are handled specially in the next search. If found EXEC return.
    */
    for (ecp = cp; CMspace(ecp) && (ecp - cp < SC_INDCOL); CMnext(ecp))
	;
    if (   STbcompare(ecp, 4, ERx("exec"), 4, TRUE) == 0
	&& (CMspace(ecp+4) || *(ecp+4) == '\t'))
	return cp;

    /* look for COBOL sequence number  - any character except newline */
    while (i < SC_INDCOL && *cp != '\n')
    {
	if (*cp == '\t')
	{
	    while (i < SC_INDCOL)	/* Found a tab - pad to sequence # */
	    {
		i++;
		*nb++ = ' ';
	    }
	    cp++;			/* Skip the tab */
	    break; 			/* Don't look any further */
	}
	else
	{
	    CMbyteinc (i, cp);
	    CMcpyinc (cp, nb);	/* Save sequence number */
	}
    }
    *nb = '\0';
    /*
    ** If the sequence number is empty (just blanks) ignore it, as an empty
    ** sequence number throws off the indentation in the code generator for
    ** initial statements.
    */
    if (STcompare(cbl_numbuf, SEQ_EMPTY) == 0)
    {
	cbl_numbuf[0] = '\0';
	return ocp;
    }
    else
	return cp;                	/* Eat sequence number */
}

/*
+* Procedure:   Cput_seqno
** Purpose:     Emits COBOL line numbers
** Parameters:  None
** Return Values: True          Sequence number exists and was emitted.
-*                False         No sequence number was output.
** Notes:
**      If a sequence number exists, then it is emitted.
*/
 
bool
Cput_seqno()
{
     if (cbl_numbuf[0] != '\0')
     {
	 gen_Cseqnum( cbl_numbuf );
	 cbl_numbuf[0] = '\0';
	 return TRUE;
     }
     return FALSE;
}

/*
+* Procedure:   Cstr_label
** Purpose:     Stores labels with the sequence number.
** Parameters:  label - buffer containing the label.
-* Return Values: None
** Notes:
**      1. If a label was found, it is added to the sequence number buffer
**         to be output later.
**	2. There will never be tabs in the sequence number.
**	3. If the sequence number is empty then a blank sequence number is 
**	   inserted.
*/

void
Cstr_label(label)
char    *label;
{
    char	*nb = cbl_numbuf;       /* sequence number buffer */

    if (*nb == EOS)			/* was empty but need a label */
	STprintf(nb, ERx("%s "), SEQ_EMPTY);
    else
	STcat(nb,ERx(" "));             /* Put in indicator column space */
    STcat(nb,label);
    STcat(nb,ERx("\n"));		/* Put label on a separate line */
}
