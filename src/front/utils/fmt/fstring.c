/* 
**	Copyright (c) 2004 Ingres Corporation  
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	"format.h"
# include	<cm.h>

/**
** Name:	fstring.c  -	This file contains the routine for formatting
**				strings with the various C and T formats.
**
** Description:
**	This file defines:
**
**	f_string		-This procedure actually handles all formats for strings.
**	f_stcheck		-Check a string template for validity, get length of mask.
**	f_stsetup		-Check a string template for validity, setup masks.
**	f_dtcheck		-Check a date template for validity, get length of mask.
**	f_dtsetup		-Check a date template for validity, setup masks.
**	fmt_stvalid		-Validate a string against a string template.
**	fmt_stposition	-Supervise positioning in a string template controled
**				     field.
**  f_inst			-Convert a display value into a DBV for a string
**					 format template.
**	fmt_stmand		-Check a string against the template in regard to
**					 mandatory bits.
**
**	local:
**	f_stparse		-Parse a string template and return 2 masks
**					 and a character class array  
**					 useful to entry control operations.
**	f_dtparse		-Parse a date template and return 2 masks
**					 useful to entry control operations.
**  f_stchval		-Validate a single character against the string template.
**
** History:
**	22-dec-86 (grant)	Taken from rxstring in the report directory.
**	20-apr-89 (cmr) 	Added support for new format (cu) which is
**                              used internally by unloaddb().  It is
**                              similar to cf (break at words when printing
**				strings that span lines) but it breaks at
**                              quoted strings as well as words.
**      28-jul-89 (cmr) 	Added support for 2 new formats 'cfe' and 'cje' .
**      20-sep-89 (sylviap) 	Added support for Q format.
**	03-dec-90 (steveh)	Fixed bug 33558. This bug caused the report 
**				writer to incorrectly break when fields were 
**				formatted using the C0 format.  This was caused
**				by junk left over at the end of the data value
**				after formatting by f_string.  f_string now 
**				pads the formatted data string with blanks.
**	2/7/91 (elein) 35807
**		Fix to bug fix 33558. MEfill into db_t_text field needs to be
**		be the correct length.  Was using db_length, but that does
**		not account for db_t_count area.  Overrunning the db_t_text
**		field was causing an fmt struc to be overwritten on VMS
**		and an error message was being displayed when that fmt was
**		subsequently validated.
**      3/21/91 (elein)         (b35574) Add FALSE parameter to call to
**				f_colfmt. TRUE is used internally for boxed
**				messages only.  They need to have control
**				characters suppressed.
**  7-25-91 (tom) - added support for string templates
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	01-04-96 (tsale01)
**		The calculation of fmt_width in f_dtparse deals with
**		characters only (probably due to confusion between character
**		and byte). Add code to adjust length by number of double byte
**		characters encountered.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */

/* entry control bits set in the entry control masks */
# define EC_MANDATORY 0x01 
# define EC_CONSTCHAR 0x02 

# define MAX_CCIDX 12		/* maximum number of user character classes that
							   can appear in any particular template, 
							   whether they use a [x=blah] or just [blah] */
# define CC_USER_OFFSET 30	/* offset used to differentiate between
								built-in character classes and character
								class which the user has defined on the fly
								(see discussion of Character Class Table */
# define NUM_USER_LETTER 5	/* number of letters that are reserved for the
							   user to user in character class definition */

/* static's */

/* Character Class Table 

This table contains the built-in character class definitions.
The f_stparse routine creates a fixed width array of characters
which serve as a content control mask. Every character in the mask
contains an index into one of two tables 1) the built-in table 
defined here, or 2) a table set up for the field dynamically with
the user defined character classes. We discriminate between the
two by using CC_USER_OFFSET. If it's a user class the index
appearing in the content control mask has CC_USER_OFFSET added to it.


   Syntax for these character class strings: 

	<char>			- indicates a single valid character
	\<char>			- escaped character (necessary for all non-alpha
					  non-digit characters)
	<char>-<char>	- indicates a range of valid characters
	<CMchar>		- CM character class (one of # @ * + & % :)
	/U	or /u		- this sequence indicates the desire to uppercase
	/L	or /l		- this sequence indicates the desire to lowercase
	//x				- this sequence defines 'x' to be the thing to 
					  use when there is no entry in a particular position,
					  a space replacement
	Notes:

		- case of alpha characters is not significant in user
		  entered strings, but here they should be lowercase only.

		- all of these string should be delimited with a ']' character.
		  this is because we support user defined masks which occur
		  in the format string, and they are delimited with the ']'.

		- IT IS IMPORTANT that this table never grow beyond the
		  value of CC_USER_OFFSET, because it would be impossible
		  to access array elements beyond this value.

		- This table's contents can be added to from version to version
		  because the indexes are never stored on disk.
*/

static char *char_classes[] =
{ 	
#				define CC_IDX_ALPHA 		0	/* a */
	"@]",
#				define CC_IDX_HEXDIG		1	/* h */
	":/U]",
#				define CC_IDX_DIGIT0		2	/* n */
	"#//0]",
#				define CC_IDX_PRINT_7BIT	3	/* o */
	"+]",
#				define CC_IDX_PRINT			4	/* p */
	"*]",
#				define CC_IDX_NM_1ST		5	/* q */
	"&]",
#				define CC_IDX_NM_ANY		6	/* r */
	"%]",
#				define CC_IDX_ANY_7BIT		7	/* s */
	" +]",
#				define CC_IDX_ANY			8	/* t */
	" *]",
#				define CC_IDX_ALPHANUM		9	/* x */
	"#@]",
#				define CC_IDX_DIGITS		10	/* z */
	"#]",

/* ---------------------------------------- internal classes for dates */

#				define CC_IDX_PM_UC			11	/* am/pm upper case */
	"ap/U//A]",
#				define CC_IDX_PM_LC			12	/* am/pm lower case */
	"ap/L//A]",
#				define CC_IDX_AL_UC			13	/* alpha lower case */
	"@/U//_]",
#				define CC_IDX_AL_LC			14	/* am/pm lower case */
	"@/L//_]",
#				define CC_IDX_0_1			15	/* digits 0-1 (months) */
	"01//0]",
#				define CC_IDX_0_2			16	/* digits 0-2 (years,hours) */
	"0-2//0]",
#				define CC_IDX_0_3			17	/* digits 0-3 (days) */
	"0-3//0]",
#				define CC_IDX_0_5			18	/* digits 0-6 (mins,secs) */
	"0-5//0]", 
#				define CC_IDX_0_1_sp		19	/* digits 0-1 (months) */
	"01]",
#				define CC_IDX_0_3_sp		20	/* digits 0-3 (days) */
	"0-3]",
#				define CC_IDX_0_5_sp		21	/* digits 0-6 (mins,secs) */
	"0-5]", 
};

static STATUS f_stparse();
static bool f_dtparse();

static i4  f_stchval();

static STATUS cc_error_status;	/* if static function f_stchval finds an
								   error in the definition of the character
								   class it posts the error status here */


/* return status bit values from f_stchval() */
# define GOOD_CHAR 	0x01	/* input character was a good character */
# define DEF_CHAR 	0x02	/* input character was the default character */
# define SHORTEN	0x04	/* the buffer was shortened, because
							   we posted a single byte char where there
							   was a double byte char */
# define CC_ERR 	0x40	/* error in character class mask */


/*{
** Name:	f_string	- This procedure actually formats strings
**				  with the various C formats and T format.
**
** Description:
**	This procedure formats a given string with a given C or T format.
**
** Inputs:
**	cb		Points to the current ADF_CB control block.
**	value		Points to the actual value to format.
**
**	fmt		Points to the FMT structure defining the format.
**
**	display		Points to a longtext value in which to place the
**			resulting formatted value.
**	change_cntrl    Boolean to set whether control chars are converted to
**				'.'
**
** Outputs:
**	display		Contains the formatted value.
**
**	Returns:
**		OK
**
** History:
**	22-dec-86 (grant)	Taken from rxstring.c in report directory.
**	01-jan-87 (grant)	Added T format;
**	27-mar-87 (bab)		Reverse the formatted string as the last
**				step for right-to-left formats.
**	17-apr-1989 (danielt)
**				added change_cntrl flag
**	20-aug-91 (tom) added support for string format templates
**	11-jan-2006 (kibro01) b117247/b117248
**		Added initialisation for wrkspc to avoid memory
**		overwrites and errors in f_colfmt
**
*/

STATUS
f_string(cb, value, fmt, display, change_cntrl)
ADF_CB		*cb;
DB_DATA_VALUE	*value;
FMT		*fmt;
DB_DATA_VALUE	*display;
bool		change_cntrl;
{
	/* internal declarations */

	i4		width;		/* column width */
	DB_TEXT_STRING	*disp;
	u_char		*string;
	u_char		*t;
	u_char		*out;
	i4		i;
	i4		length;
	i4		clrlen;
	i4		buflen;
	FM_WORKSPACE	wrkspc;
	bool		reverse;
	
	/* start of routine */

	disp = (DB_TEXT_STRING *) display->db_data;

	width = fmt->fmt_width;

	if (fmt->fmt_type == F_B)
	{
	    disp->db_t_count = width;
	    MEfill((u_i2) width, ' ', disp->db_t_text);
	    return OK;
	}

	/* set up string pointer and string length */
	f_strlen(value, &string, &length);
	buflen = (width == 0) ? display->db_length - DB_CNTSIZE : width;

	switch(fmt->fmt_type)
	{
	case F_ST:		/* string templates */
		out = disp->db_t_text;
		for (i = 0; i < width; i++)
		{
			if (fmt->fmt_emask[i] & EC_CONSTCHAR)
			{
				CMcpychar(&fmt->fmt_cmask[i*2], out);
				CMnext(out);
			}
			else
			{
				/* first copy to the output either the string or
				   a space if there is no string left */
				if (length <= 0)
				{
					*out = ' ';
				}
				else
				{
					CMcpychar(string, out);
					CMbytedec(length, string);
					CMnext(string);
				}
				/* this routine will fix the case if needed, or replace space 
				   or bad character with default character if needed */
				if (f_stchval(out, fmt->fmt_cmask[i*2], fmt->fmt_ccary) 
							& GOOD_CHAR)
				{
					/* error?? */
					/* we should probably mark the data changed bit maybe */
				}
				CMnext(out);
			}
		}
		disp->db_t_count = out - disp->db_t_text;
		return OK;

	case F_T:
	    /* convert control chars to \c & \ddd */
	    buflen = f_strT(&string, &length, disp->db_t_text, buflen);
	    if (width == 0)
	    {
		width = buflen;
	    }
	    break;

	case F_C:
	    /* convert control chars to blanks */
	    buflen = f_strCformat(&string, &length, disp->db_t_text, buflen, 
							change_cntrl);
	    if (width == 0)
	    {
		width = f_lentrmwhite(disp->db_t_text, buflen);

		/* blank pad C0 formatted data. this
		   fixes bug 33558. (steveh)        */
		if (width < buflen)
		{
			MEfill(buflen-width,' ',(PTR)(disp->db_t_text+width));
		}
	    }
	    break;

	case F_CF:
	case F_CFE:
	case F_CJ:
	case F_CJE:
	/* internal format used by unloaddb() */
	case F_CU:
	case F_Q:
	    /* Initialise all we need for f_colfmt (kibro01) b117247/b117248 */
	    wrkspc.fmws_text = string;
	    wrkspc.fmws_count = length;
	    wrkspc.fmws_left = width;
	    wrkspc.fmws_inquote = F_NOQ;
	    wrkspc.fmws_incomment = FALSE;
	    wrkspc.fmws_multi_buf = NULL;
	    wrkspc.fmws_multi_buf = NULL;
	    wrkspc.fmws_multi_size = DB_MAXSTRING + FMT_OVFLOW;
	    wrkspc.fmws_cprev = NULL;

	    if (width == 0)
	    {
	        width = f_lentrmwhite(string, length);
	    }

	    f_colfmt(&wrkspc, disp, width, fmt->fmt_type, FALSE, FALSE);
	    break;
	}

	disp->db_t_count = width;

	if (fmt->fmt_sign == FS_PLUS)
	{
	    f_strrgt(disp->db_t_text, buflen, disp->db_t_text, width);
	}
	else if (fmt->fmt_sign == FS_MINUS)
	{
	    f_strlft(disp->db_t_text, buflen, disp->db_t_text, width);
	}
	else if (fmt->fmt_sign == FS_STAR)
	{
	    f_strctr(disp->db_t_text, buflen, disp->db_t_text, width);
	}
	else if (width > buflen)
	{
	    MEfill(width-buflen, ' ', (PTR)(disp->db_t_text+buflen));
	}

	if (fmt_isreversed(cb, fmt, &reverse) == OK)
	{
	    if (reverse)
	    {
			f_revrsbuf(width, TRUE, disp->db_t_text);
	    }
	}
	/*
	** if not OK, does nothing at present since this routine
	** wants to always return OK
	*/

	return OK;
}

/*{
** Name:	f_stcheck	- Check a template and return mask and array sizes
**
** Description:
**			This routine checks a string format template, and returns
**			error status, or sizes for allocation calculation. 
**			It is called only by fmt_areasize.
**
** Inputs:
**	FMT  *fmt		- ptr to fmt struct to check
**
** Outputs:
**	i4  *len		- ptr to caller's variable to contain length of 
**						mask required
**	i4	 *cclen		- ptr to caller's variable to contain number of elements
**						in the user character class array for this template
**
**	Returns:
**		STATUS - Error status or OK
**
** History:
**	7-27-91	(tom) - created
**
*/

STATUS
f_stcheck(fmtstr, len, cclen)
	char *fmtstr;
	i4 *len;
	i4 *cclen;
{
	/* since this is just a check (called from fmt_areasize) we 
	   will just build these masks and arrays locally, and just
	   return their sizes to the caller for areasize calculation */
	char cmask[(MAX_FMTMASK+1)*2];	/* *2 because of possible double byte */
	char emask[MAX_FMTMASK+1];	
	char *cc_ary[MAX_CCIDX];

	return f_stparse(fmtstr, cmask, emask, len, cc_ary, cclen);
}

/*{
** Name:	f_stsetup	- Check a template for setfmt, and setup the masks.
**
** Description:
**			This routine checks a string format template, and returns
**			error status. It is called by f_setfmt.
**
** Inputs:
**	FMT  *fmt		- ptr to fmt struct to check
**							of char classes defined
**
** Outputs:
**
**	char *mask_area		- caller's area (alloced after calling fmt_areasize)
**						  filled in with the masks and character class array.
**
**	The following elements of the FMT structure are set:
**		.fmt_width
**		.fmt_ccsize
**		.fmt_emask
**		.fmt_cmask
**		.fmt_ccary
**
**	Returns:
**		STATUS - Error status or OK
**
** History:
**	7-27-91	(tom) - created
**
*/

STATUS
f_stsetup(fmt, mask_area)
	FMT *fmt;
	char *mask_area;
{
	char cmask[(MAX_FMTMASK+1)*2];	/* *2 because of possible double byte */
	char emask[MAX_FMTMASK+1];	
	char *cc_ary[MAX_CCIDX];
	i4 width;
	i4 ccsize;
	STATUS retval;

	retval = f_stparse(fmt->fmt_var.fmt_template, cmask, emask, &width, 	
			cc_ary, &ccsize);

	if (retval != OK)
		return retval;

	fmt->fmt_width = width;
	fmt->fmt_ccsize = ccsize;
	fmt->fmt_cmask = mask_area;
	fmt->fmt_emask = mask_area + 2 * width;
	fmt->fmt_ccary 
		= (char**) ME_ALIGN_MACRO(fmt->fmt_emask + width, sizeof(char*));
	MEcopy((PTR) cmask, (u_i2) (2 * width), (PTR) fmt->fmt_cmask);
	MEcopy((PTR) emask, (u_i2) width, (PTR) fmt->fmt_emask);
	MEcopy((PTR) cc_ary, (u_i2) (ccsize * sizeof(char*)),
		(PTR) fmt->fmt_ccary); 

	return OK;
}


/*{
** Name:	f_dtcheck	- Check a date tplt for input masking, return mask size
**
** Description:
**			This routine checks a date format template, and returns
**			error status, or sizes for allocation calculation. 
**			It is called only by fmt_areasize.
**
** Inputs:
**	FMT  *fmt		- ptr to fmt struct to check
**
** Outputs:
**	i4 *len		- ptr to caller's variable to contain length of mask
**
**	Returns:
**		bool - True for success, False for failure
**
** History:
**	7-27-91	(tom) - created
**
*/

bool
f_dtcheck(fmtstr, len)
	char *fmtstr;
	i4 *len;
{
	/* since this is just a check (called from fmt_areasize) we 
	   will just build these masks and arrays locally, and just
	   return their sizes to the caller for areasize calculation */
	char emask[MAX_FMTMASK+1];	
	char cmask[(MAX_FMTMASK+1)*2];	/* *2 because of possible double byte */

	return f_dtparse(fmtstr, cmask, emask, len);
}

/*{
** Name:	f_dtsetup	- Check a template for setfmt, and setup the masks.
**
** Description:
**			This routine checks a date format template, and returns
**			error status. It is called by f_setfmt.
**
** Inputs:
**	FMT  *fmt		- ptr to fmt struct to check
**	char *mask_area		- ptr to area to place masks, and array
**							of char classes defined
**
** Outputs:
**	mask_area is filled in with the masks.
**	
**	The following members of the fmt structure are set.
**
**		.fmt_width
**		.fmt_ccsize (set to 0)
**		.fmt_emask
**		.fmt_cmask
**		.fmt_ccary	(set to NULL)
**
**	Returns:
**		VOID - it is not an error for this to fail.. it just means that
**				the date template contains items which preclude 
**				input masking to be applied. In this case the mask structures
**				are not setup.
**
** History:
**	7-27-91	(tom) - created
**
*/

VOID
f_dtsetup(fmt, mask_area)
	FMT *fmt;
	char *mask_area;
{
	char cmask[(MAX_FMTMASK+1)*2];	/* *2 because of possible double byte */
	char emask[MAX_FMTMASK+1];	
	i4 width;

	/* NOTE: f_dtcheck should have been called from fmt_areasize..
	   if it failed there it will fail here.. which is really ok,
	   it just means that the date template can't really be used 
	   for input masking. */
	if (f_dtparse(fmt->fmt_var.fmt_template, cmask, emask, &width) == TRUE)
	{
		/* dates don't require local character classes */
		fmt->fmt_ccary = NULL;
		fmt->fmt_ccsize = 0;

		fmt->fmt_width = width;
		fmt->fmt_cmask = mask_area;
		fmt->fmt_emask = mask_area + 2 * width;
		MEcopy(cmask, 2 * width, fmt->fmt_cmask);
		MEcopy(emask, width, fmt->fmt_emask);
	}
}


/*{
** Name:	f_stparse	- Parse a template that's been processed by setfmt 
**
** Description:
**			This routine parses a string format template, and returns
**			in thc caller's buffers 2 mask arrays, a character class
**			array and some lengths.
**
** Inputs:
**	char *t			- pointer to fmt struct
**
** Outputs:
**	char *cmask 	- mask array to describe the content control
**	char *emask		- mask array to describe the entry control
**	char *cc_ary[]	- array of pointers to the character classes
**	i4 *len		- filled in with the length of the  mask
**	i4 *cc_size	- filled in with the size of the user character class
**					  array
**
**	Returns:
**		STATUS - Error status or OK.
**
** History:
**	7-25-91	(tom) - created
**
*/


static STATUS
f_stparse(t, cmask, emask, len, cc_ary, cc_ary_size)
	register char *t;	/* the template */
	char *cmask;
	char *emask;
	i4 *len;
	char *cc_ary[];
	i4 *cc_ary_size;
{
	register char *cm = cmask;
	register char *em = emask;
	i4 cc_idx = 0;
	char str[3];
	bool seen_def = FALSE;
	i4 user_idx;
	char user_let[NUM_USER_LETTER];
	i4	tmp;


	/* initialize the user letter array,
	   this is used to remember the cc_ary index of a given letter */
	for (user_idx = 0; user_idx < NUM_USER_LETTER; user_idx++)
	{
		user_let[user_idx] = 0;
	}

	for (; *t && *t != '\'' && *t != '"'; CMnext(t), em++, cm += 2)
	{
		if (em - emask >= MAX_FMTMASK)
		{
			tmp = MAX_FMTMASK;
			return afe_error(FEadfcb(), E_FM3000_MASKSIZE, 2,
				sizeof(tmp), (PTR)&tmp);
		}

		*em = 0;

		switch (*t)
		{
		case '\\':			/* note: setfmt guards agains \EOS */
			*em = EC_CONSTCHAR;
			t++;
			CMcpychar(t, cm); 
			break;
	
		case ' ':				/* spaces are just considered constant */
			*em = EC_CONSTCHAR;
			*cm = *t;
			break;

		case '[':				/* start of char class defintion */
			if (cc_idx < MAX_CCIDX)
			{
				if (!CMdbl1st(t+1) && t[2] == '=')
				{
					switch (t[1])
					{
					case 'I':
						*em |= EC_MANDATORY;
					case 'i':
						user_let[0] = cc_idx + CC_USER_OFFSET;
						break;

					case 'J':
						*em |= EC_MANDATORY;
					case 'j':
						user_let[1] = cc_idx + CC_USER_OFFSET;
						break;

					case 'K':
						*em |= EC_MANDATORY;
					case 'k':
						user_let[2] = cc_idx + CC_USER_OFFSET;	
						break;

					case 'L':
						*em |= EC_MANDATORY;
					case 'l':
						user_let[3] = cc_idx + CC_USER_OFFSET;	
						break;

					case 'M':
						*em |= EC_MANDATORY;
					case 'm':
						user_let[4] = cc_idx + CC_USER_OFFSET;	
						break;

					default:
						/* triggering an error here actually precludes
						   the possibility of '=' being the 2nd character
						   in a character class definition.. doesn't seem
						   that bad really */
						return afe_error(FEadfcb(), E_FM3001_BAD_USER_CLASS, 2,
							1, (PTR) (t + 1));
					}
					t += 2;
				}

				/* assign the new character class to the array */
				cc_ary[cc_idx] = t + 1;

				for (; *t != ']'; CMnext(t))/* pass character class def */
				{
					if (*t == '\\') /* note: setfmt guards agains \EOS */
					{
						t++;
					}
					else if (*t == EOS)
					{
						return afe_error(FEadfcb(), E_FM3002_NO_CC_TERM, 0);
					}
				}
				*cm = cc_idx++ + CC_USER_OFFSET;	/* post index */

				/* call character validate routine.. it also 
				   validates the character class definition */
				str[0] = EOS;
				if (f_stchval(str, *cm, cc_ary) & CC_ERR)
				{
					return afe_error(FEadfcb(), cc_error_status, 0);
				}
			}
			else	/* greater than MAX_CCIDX */
			{
				tmp = MAX_CCIDX;
				return afe_error(FEadfcb(), E_FM300D_MAX_CCIDX, 2,
					sizeof(tmp), (PTR)&tmp);
			}
			break;

		case 'A':
			*em |= EC_MANDATORY;
		case 'a': 
			*cm = CC_IDX_ALPHA; 
			break;

		case 'H':
			*em |= EC_MANDATORY;
		case 'h': 
			*cm = CC_IDX_HEXDIG;
			break;

		case 'N':
			*em |= EC_MANDATORY;
		case 'n': 
			*cm = CC_IDX_DIGIT0; 
			break;
			
		case 'O':
			*em |= EC_MANDATORY;
		case 'o': 
			*cm = CC_IDX_PRINT_7BIT; 
			break;

		case 'P':
			*em |= EC_MANDATORY;
		case 'p': 
			*cm = CC_IDX_PRINT; 
			break;

		case 'Q':
			*em |= EC_MANDATORY;
		case 'q': 
			*cm = CC_IDX_NM_1ST; 
			break;

		case 'R':
			*em |= EC_MANDATORY;
		case 'r': 
			*cm = CC_IDX_NM_ANY; 
			break;

		case 'S':
			*em |= EC_MANDATORY;
		case 's': 
			*cm = CC_IDX_ANY_7BIT; 
			break;

		case 'T':
			*em |= EC_MANDATORY;
		case 't': 
			*cm = CC_IDX_ANY; 
			break;

		case 'X':
			*em |= EC_MANDATORY;
		case 'x': 
			*cm = CC_IDX_ALPHANUM; 
			break;

		case 'Z':
			*em |= EC_MANDATORY;
		case 'z': 
			*cm = CC_IDX_DIGITS; 
			break;
			
							/* the user definable letters */
		case 'I':
			*em |= EC_MANDATORY;
		case 'i':
			user_idx = 0;
			goto USER_LETTER;	

		case 'J':
			*em |= EC_MANDATORY;
		case 'j':
			user_idx = 1;
			goto USER_LETTER;	

		case 'K':
			*em |= EC_MANDATORY;
		case 'k':
			user_idx = 2;
			goto USER_LETTER;	

		case 'L':
			*em |= EC_MANDATORY;
		case 'l':
			user_idx = 3;
			goto USER_LETTER;	

		case 'M':
			*em |= EC_MANDATORY;
		case 'm':
			user_idx = 4;

USER_LETTER:	/* common code to process user letter, user_idx setting */

			if (user_let[user_idx] == 0)	/* check that it is set */
			{
				return afe_error(FEadfcb(), E_FM300E_USR_CLAS_UNDEF, 2, 
					1, (PTR)t);
			}
			*cm = user_let[user_idx];		/* post index */
			break;

		default:
			
			return afe_error(FEadfcb(), E_FM300F_BAD_ST_CHAR, 2,
				CMbytecnt(t), (PTR)t);
		}

		/* if it's not a constant character, then we have seen a definition */
		if ( !(*em & EC_CONSTCHAR) )
		{
			seen_def = TRUE;
		}
	}

	if (seen_def == FALSE)
	{
		return afe_error(FEadfcb(), E_FM3010_NO_DEF, 0);
	}

	*len = em - emask;
	*cc_ary_size = cc_idx;
	
	return OK;
}



/*{
** Name:	fmt_stvalid	- Validate string against mask.
**
** Description:
**		Validate a string that the user would like to place at a
**		particular position into a string template controlled 
**		display. This is called by the window manager specific code.
**		In most cases it is only a single character being placed,
**		however it may be more than one if the user is pasting.
**		In addition we also force the case of the character
**		according to the mask, or we may substitute a constant
**		character if a particular position is invalid.
**		Because constant characters can be double byte, and could
**		be replacing single byte character the buffer could get
**		longer. Or shorter due to the opposite case.
**
** Inputs:
**	FMT *fmt		- fmt struct
**	char *string	- pointer to the string to validate, could contain
**					  double byte characters, null terminated.
**					  Must be 2 times MAX_FMTMASK wide so we can 
**					  lengthen if necessary (by posting double byte
**					  constant characters)
**	i4  len		- length of string in bytes (not character positions)
**	i4  pos		- start position in mask in character positions 
**						(not byte positions)
**
** Outputs:
**	bool *warn		- caller is posted if there were any bad characters
**					  in the string.
**
**		The caller's string is massaged to conform to the template.
**		
**
**	Returns:
**		i4 - Number of good characters that were found. 
**
** History:
**	7-25-91	(tom) - created
**
*/

i4
fmt_stvalid(fmt, str, len, pos, warn)
	FMT *fmt;
	register char *str;
	register i4  len;
	register i4  pos;
	bool *warn;
{
	i4 good;
	i4 stat;
	char *s;

	*warn = FALSE;

	/* sanity check */
	if (pos < 0 || len <= 0 || fmt->fmt_emask == NULL)
	{
		return 0;
	}

	/* move through the string replacing constant characters,
	   or validating entry position characters */
	for (good = 0; len > 0 && pos < fmt->fmt_width; CMnext(str), pos++)
	{
		/* is it a constant charcter */
		if (fmt->fmt_emask[pos] & EC_CONSTCHAR)
		{
			/* we must test for the constant character and the input being of 
			   different lengths and shorten or lengthen the buffer accordingly,
			   NOTE: we need to copy the null terminator */
			switch ((CMdbl1st(&fmt->fmt_cmask[pos*2]) << 1) | CMdbl1st(str))
			{
			case 1:		/* input only is double byte */
				MEcopy(str+2, --len, str+1);	/* buffer is getting shorter */ 
				*warn = TRUE;
				break;

			case 2:		/* constant char only is double byte */
								/* buffer is getting longer */
				for (s = str + 1 + len; s >= str + 1;  s--)
					s[1] = s[0];
				len++;
				*warn = TRUE;
				break;
			default:	/* they are the same length, we warn if not exact */
				if (CMcmpcase(str, &fmt->fmt_cmask[pos*2]) != 0)
				{
					*warn = TRUE;
				}
			}

			CMcpychar(&fmt->fmt_cmask[pos*2], str);
		}
		else
		{
			/* do validation */
			stat = f_stchval(str, fmt->fmt_cmask[pos*2], fmt->fmt_ccary);
			if (stat & GOOD_CHAR)
			{
				good++;
			}
			else if (stat & SHORTEN)
			{
				*warn = TRUE;
				MEcopy(str+2, --len, str+1);
				/* since the default character cannot be a double byte
				   we cannot lengthen */
			}
			else
			{
				*warn = TRUE;
			}
		}
		CMbytedec(len, str);
	}
	return good;
}

/*{
** Name:	fmt_stchval	- Character validate against character class.
**
** Description:
**		Validate a character that the user would like to place at a
**		particular position in a string that is controlled by a string
**		template. In addition we also force the case of the character
**		according to the mask.
**		If it is user entry but is a space, as it would be 
**		in a deletion.. then we post the default character.
**
** Inputs:
**	char *ptr		- pointer to the character
**	i4 idx			- index into character class array
**	char *cc_ary[]	- pointer to user character class array definitions
**
** Outputs:
**		- the character may have its case changed, or it may be
**		  changed into the default character if it is not valid at all.
**		  In the worse case this would result in a double byte character
**		  being replaced with a single byte character in which case the
**		  caller must fix up his buffer.
**
**	Returns:
**		- various bits set in return status (see top of the file)
**
** History:
**	7-25-91	(tom) - created
**
*/

static i4
f_stchval(ptr, idx, cc_ary)
	char *ptr;
	i4 idx;
	char *cc_ary[];
{
	register char *cc;
	char match[2];
	bool upr = FALSE;
	bool lwr = FALSE; 
	i4 status = 0;
	char def_char = EOS;
	bool good_class = FALSE;

	/* determine if we are using the user defined or built-in char classes */
	if (idx >= CC_USER_OFFSET)
		cc = cc_ary[idx - CC_USER_OFFSET];		/* user defined */
	else
		cc = char_classes[idx];					/* built-in */

	/* loop through the character class string trying to 
	   match against the input character, note that the character
	   class defintion string is delimited with a ']' to support
	   them being pointed to in the format template */
	for ( ; *cc != ']'; CMnext(cc))
	{

		switch (*cc)
		{
		case '/':		/* test for specification to force case */
			switch (cc[1])
			{
			case 'U':
			case 'u':
				if (upr == TRUE)		/* test for multiple */
				{
					cc_error_status = E_FM3003_DUP_UPR;
					return CC_ERR;
				}
				else if (lwr == TRUE)	/* test for conflicting */
				{
					cc_error_status = E_FM3005_BOTH_UPR_LWR;
					return CC_ERR;
				}
				upr = TRUE;
				cc++;
				break;

			case 'L':
			case 'l':
				if (lwr == TRUE) 		/* test for multiple */
				{
					cc_error_status = E_FM3004_DUP_LWR;
					return CC_ERR;
				}
				else if (upr == TRUE) 	/* test for conflicting */
				{
					cc_error_status = E_FM3005_BOTH_UPR_LWR;
					return CC_ERR;
				}
				lwr = TRUE;
				cc++;
				break;

			case '/':	/* or specification of default char */

				if (def_char != EOS)	/* test for multiple */
				{
					cc_error_status = E_FM3006_DUP_DEF;
					return CC_ERR; 
				}
				cc += 2;

				/* disallow default character to be control or double byte */
				if (CMcntrl(cc) || CMdbl1st(cc))
				{
					cc_error_status = E_FM3007_BAD_DEF_CHAR;
					return CC_ERR;
				}
				def_char = *cc;

				/* some of our callers are interested in if the
				   character tested is the default character */ 
				if (def_char == ptr[0])
				{
					status |= DEF_CHAR;
				}

				break;

			default:	
				cc_error_status = E_FM3008_BAD_SLASH_CHAR;
				return CC_ERR;
			}
			break;

		case '#':				/* CM digit */
			good_class = TRUE;
			if (CMdigit(ptr))
				status |= GOOD_CHAR;
			break;

		case '@':				/* CM alpha */
			good_class = TRUE;
			if (CMalpha(ptr))
				status |= GOOD_CHAR;
			break;

		case '*':				/* CM printable */
			good_class = TRUE;
			if (CMprint(ptr))
				status |= GOOD_CHAR;
			break;

		case '+':				/* CM printable and 7bit only */
			good_class = TRUE;
			if (CMprint(ptr) && ptr[0] < 0x7F)
				status |= GOOD_CHAR;
			break;

		case '&':				/* CM name character (1st) */
			good_class = TRUE;
			if (CMnmstart(ptr))
				status |= GOOD_CHAR;
			break;

		case '%':				/* CM name character (any) */
			good_class = TRUE;
			if (CMnmchar(ptr))
				status |= GOOD_CHAR;
			break;

		case ':':				/* CM hex digit  */
			good_class = TRUE;
			if (CMhex(ptr))
				status |= GOOD_CHAR;
			break;

		default:
			if (*cc == '\\')			/* escape */
			{
				++cc;
				if (CMcntrl(cc))	/* musn't be control */
				{
					cc_error_status = E_FM3009_CTL_CHAR;
					return CC_ERR;
				}
			}
			/* at this point it must be either space, alpha or digit */ 
			else if (  !CMspace(cc) 
					&& !CMalpha(cc) 
					&& !CMdigit(cc)
					)
			{
				cc_error_status = E_FM300A_MUST_ESCAPE;
				return CC_ERR;
			}

			CMcpychar(cc, match);			/* we have a character to match */

			good_class = TRUE;

			if (cc[CMbytecnt(cc)] == '-')	/* is it a range spec */
			{
				cc += CMbytecnt(cc) + 1;
				if (*cc == '\\')
				{
					++cc;
					if (CMcntrl(cc))	/* musn't be control */
					{
						cc_error_status = E_FM3009_CTL_CHAR;
						return CC_ERR;
					}
				}
				/* at this point it must be either space, alpha or digit */ 
				else if (  !CMspace(cc)
						&& !CMalpha(cc)
						&& !CMdigit(cc)
						)
				{
					cc_error_status = E_FM300A_MUST_ESCAPE;
					return CC_ERR;
				}

				/* disallow backwards specifications e.g. [Z-A], [A-A] */
				if (CMcmpnocase(match, cc) >= 0)
				{
					cc_error_status = E_FM300B_BAD_RANGE;
					return CC_ERR;
				}

				/* is the input within the range */
				if (  CMcmpnocase(ptr, match)  >= 0 
				   && CMcmpnocase(ptr, cc)     <= 0
				   )
				{
					status |= GOOD_CHAR;
				}
			}
			else if (CMcmpnocase(ptr, match) == 0)	 /* is it a match */
			{
				status |= GOOD_CHAR;
			}
			break;
		}
	}

	/* we didn't match so post space or default character */
	if (!(status & GOOD_CHAR))
	{
		/* if the input character is doublebyte then we 
		   are shortening the buffer */
		if (CMdbl1st(ptr))
		{
			status |= SHORTEN;
		}

		if (def_char == EOS)		/* if not set.. post a space */
		{
			*ptr = ' ';
		}
		else
		{
			*ptr = def_char;
		}
	}

	/* force case if necessary */
	if (upr == TRUE)
	{
		CMtoupper(ptr, ptr);
	}
	else if (lwr == TRUE)
	{
		CMtolower(ptr, ptr);
	}

	if (good_class == FALSE) /* if there were no good character class specs */
	{
		cc_error_status = E_FM300C_BAD_CLASS;
		return CC_ERR;
	}

	return status;
}

/*{
** Name:	fmt_stposition	- Return closest entry position to given target.
**
** Description:
**		Return the position closest to the caller's desired target 
**		position in a string controlled by a string template.
**		If the user's position would land on a constant character
**		then place it after the desired position, if we hit the
**		end of the line then move backwards till we find a location.
**
** Inputs:
**	FMT *fmt		- pointer to the format struct
**	i4 target		- desired target column
**	i4 dir			- desired direction of movement 
**					  (1 = forward, -1 = backward)
**
** Outputs:
**
**	Returns:
**		Column to move to.
**
** History:
**	7-25-91	(tom) - created
**
*/

i4
fmt_stposition(fmt, target, dir)
	register FMT *fmt;
	register i4  target;
	register i4  dir;
{
	i4 trips;

	/* keep target in bounds */
	if (target >= fmt->fmt_width - 1)		/* this could happen */
		target = fmt->fmt_width - 1;
	else if (target < 0)		/* this shouldn't ever happen */
		target = 0;

	/* keep moving the target until it finds a 
	   non-constant character to rest on */
	for (trips = 0; fmt->fmt_emask[target] & EC_CONSTCHAR; )
	{
		target += dir;

		/* reverse directions when we hit the ends */
		if (target <= 0 || target >= fmt->fmt_width)
		{
			/* keep from endless looping in the case
			   where the mask is nothing but constant characrters
			   which should be caught by setfmt */
			if (trips++ > 2) 
				break;
			dir = -dir;
			target += dir;
		}
	}
	return target;
}


/*{
** Name:	f_inst	- convert display into DBV for string template field
**
** Description:
**		Given a display value, return a DBV. Strip constant characters.
**		Depending on the datatype of the 'value' parameter we behave
**		differently. Two cases:
**
**		1: (abs(value->db_datatype) == DB_VCH_TYPE) -- varchar	
**			- we are being called to strip constant characters,
**			  and place good characters into db_data.
**
**		2: (abs(value->db_datatype) == DB_DTE_TYPE)	-- date
**			- we are being called to see if the display contains 
**			  null date, that is, no entry positions are filled in.
**			  (NOTE: only the string template code knows whether
**			  this is true.)
**
** Inputs:
**	FMT *fmt				- pointer to the format struct
**	DB_DATA_VALUE *display	- display value long text string
**
** Outputs:
**	DB_DATA_VALUE *value	- result data value
**	bool *is_null			- ptr to caller's bool to be able to tell
**							  if the display represented a null value
**							  (this is used only for dates.)
**
**	Returns:
**		OK
**
** History:
**	7-25-91	(tom) - created
**
*/

STATUS
f_inst(cb, fmt, display, value, is_null)
	ADF_CB *cb;
	FMT *fmt;
	DB_DATA_VALUE *display;
	DB_DATA_VALUE *value;
	bool *is_null;
{
	register i4  i;
	i4 stat;
	i4 bump;
	bool seen_good = FALSE;
	u_char *buf;
	u_char tmp_buf[(MAX_FMTMASK+1) * 2];/* *2 because of possible double byte */

	DB_TEXT_STRING *disp;
	register u_char *in;
	register i4  input_len;

	DB_TEXT_STRING *val;
	register u_char *out;
	register i4  output_len;

	disp = (DB_TEXT_STRING *) display->db_data;
	in = disp->db_t_text;
	input_len = disp->db_t_count;

	*is_null = FALSE;

	/* sanity check, result value must be a varchar type or date */
	switch (abs(value->db_datatype))
	{
	case DB_VCH_TYPE:	/* being called to convert via string template */
		val = (DB_TEXT_STRING *) value->db_data;
		out = buf = val->db_t_text;
		output_len = value->db_length;
		break;

	case DB_DTE_TYPE:	/* being called to see if the date is null,
						   we will form string into local buffer and discard */
		out = buf = tmp_buf;
		output_len = MAX_FMTMASK; /* should be plenty big enough */
		break;

	default:
		/* attempt to null it out.. could get data and convert it */
		return adc_getempty(cb,value);
	}

	for (i = 0; i < fmt->fmt_width && input_len > 0 && output_len > 0; i++)
	{
		bump = CMbytecnt(in);	/* remember how big the input was */

		/* if it's not a constant char */
		if ( !(fmt->fmt_emask[i] & EC_CONSTCHAR) )	
		{
			/* if the character is good, but not the default character
			   then we say we have seen a good character */
			stat = f_stchval(in, fmt->fmt_cmask[i*2], fmt->fmt_ccary);
			if ((stat & GOOD_CHAR) && !(stat & DEF_CHAR))
			{
				seen_good = TRUE;
			}

			/* test for double byte character causing buffer overrun */
			if (output_len - CMbytecnt(in) < 0)
			{
				break;
			}

			CMcpychar(in, out);		/* it's ok to copy it */

			CMbytedec(output_len, out);
			CMnext(out);
		}

		in += bump;				/* bump the input pointers */
		input_len -= bump;

	}

	/* trim white space, this should be CM'ified but we can't null terminate
	   because it may be a completly full buffer */
	while (out > buf)
	{
		if (out[-1] == ' ')			/* single byte space */
			out--; 
		else if (  CMspace(out-2)
				&& out - 2 >= buf 
			    && out[-2] != ' '	/* double byte space? */
				)
			out -= 2;
		else
			break;
	}

	/* if the type is nullable, and either the text is 0 length, or we
	   haven't seen any good characters then mark it as nullable */
	if (  AFE_NULLABLE_MACRO(value->db_datatype)
	   && (  out - buf == 0 
		  || seen_good == FALSE
		  )
	   )
	{
		*is_null = TRUE;
		return adc_getempty(cb, value);
	}
	else if (buf != tmp_buf)	/* only post if it's not a date */
	{
		val->db_t_count = out - buf;
		AFE_CLRNULL(value);
	}

	return OK;
}

/*{
** Name:	fmt_stmand	- check string for mandatory input positions
**
** Description:
**		Given a display value, check string for mandatory input positions.
**		Called from window manager specific code.
**	
**
** Inputs:
**	FMT *fmt		- pointer to the format struct
**	str				- string to check
**	len				- length of string 
**
** Outputs:
**
**	Returns:
**		bool	TRUE if mandatory positions are filled with good characters
**				note: can be the default character as long as it's also good
**				else FALSE
**
** History:
**	8-23-91	(tom) - created
**
*/

bool
fmt_stmand(fmt, str, len)
	FMT *fmt;
	char *str;
	i4 len;
{
	register i4  i;

	/* see if we are actively input masking, 
	   if not then mandatory status is satisfied */
	if (fmt->fmt_emask == NULL)
		return TRUE; 

	for (i = 0; i < fmt->fmt_width; i++)
	{
		/* if it's a mandatory position in the mask */
		if (fmt->fmt_emask[i] & EC_MANDATORY)	
		{
			if (  len-- <= 0		/* we exhausted input */
			   || !(f_stchval(&str[i], fmt->fmt_cmask[i*2], fmt->fmt_ccary) 
					& GOOD_CHAR)  /* or character isn't good */
			   )
			{
				return FALSE; 
			}
		}
	}
	return TRUE;
}

/*{
** Name:	f_dtparse	- derive string template structures from a date format
**
** Description:
**		Given a date format template, derive appropriate string template
**		internal structures.
**	
** Inputs:
**	char *t		- pointer to fmt struct
**
** Outputs:
**	char *cmask		- mask array to describe the content control
**	char *emask		- mask array to describe the entry control
**	i4 *len		- filled in with the length of the  mask
**	char *cc_ary[]	- array of pointers to the character classes
**	i4 *cc_size	- filled in with the size of the user character class
**					  array
**
**	Returns:
**		bool		- TRUE if it is approved for input masking
**					  else FALSE
**
** History:
**	7-25-91	(tom) - created
**
*/

static bool 
f_dtparse(t, cmask, emask, len)
	char *t;
	char *cmask;
	char *emask;
	i4 *len;
{
	register char *cm = cmask;
	register char *em = emask;
	i4 length;
	i4 fwidth;
	i4 zwidth;
	char zfill_digit;
	char class;
	i4 value;
	char *p;
	char word[100];
	char buf[10];
	int dbl_chars = 0;	/* length calculations are based on emask,
				** which count number of 'characters' and not
				** bytes. A double byte char needs two bytes
				** for representation and also occupies 2
				** display position. Need to count number of
				** double byte chars ecountered and adjust
				** length accordingly.
				*/

	*len = 0;

	for (zwidth = fwidth = 0; *t && *t != '\'' && *t != '"'; )
	{
		switch(*t)
		{
		case('\\'):
			t++;
			*em++ = EC_CONSTCHAR;
			CMcpychar(t, cm); 
			CMnext(t);
			cm += 2;
			zwidth = fwidth = 0;
			break;

		case('|'):
			return FALSE;

		case('0'):
			for (zwidth = 0; *t == '0'; )
			{
				t++;
				fwidth++;
				zwidth++;
			}
			break;

		case('1'):
			if (*(t+1) == '6')				/* 24 hour time */
			{
				if (zwidth > 0)
					return FALSE;

				t += 2;
				*em++ = 0;
				*cm++ = CC_IDX_0_2;
				cm++;
				*em++ = 0;
				*cm++ = CC_IDX_DIGIT0;
				cm++;
			}
			else if (  *(t+1) == '9' 		/* 4 digit year */
					&& *(t+2) == '0' 
					&& *(t+3) == '1'
					)
			{
				if (zwidth > 0)
					return FALSE;

				t += 4;
				*em++ = 0;
				*cm++ = CC_IDX_0_2;
				cm++;
				*em++ = 0;
				*cm++ = CC_IDX_DIGIT0;
				cm++;
				*em++ = 0;
				*cm++ = CC_IDX_DIGIT0;
				cm++;
				*em++ = 0;
				*cm++ = CC_IDX_DIGIT0;
				cm++;
			}
			else
			{
												/* 2 digit year */
				zfill_digit = zwidth ? CC_IDX_DIGIT0 : CC_IDX_DIGITS;
	PUT_2_DIGITS:
				t++;

				if (++fwidth < 2 || zwidth > 1)
					return FALSE;

				if (zwidth == 0)/* if there was no zero fill it must have been
						space filled.. so backup, cause we interpreted a
						space as a constant character. */
				{
					em[-1] = 0;
					cm[-2] = zfill_digit;
				}
				else
				{
					*em++ = 0;
					*cm++ = zfill_digit;
					cm++;
				}
				*em++ = 0;
				*cm++ = CC_IDX_DIGIT0;
				cm++;
			}
			zwidth = 0;
			fwidth = (CMspace(t)) ? -1 : 0;	
			break;

		case('4'):							/* 2 digit hour (12 hour) */
		case('2'):							/* 2 digit month */
			zfill_digit = zwidth ? CC_IDX_0_1 : CC_IDX_0_1_sp;
			goto PUT_2_DIGITS;

		case('3'):							/* 2 digit day */
			zfill_digit = zwidth ? CC_IDX_0_3 : CC_IDX_0_3_sp;
			goto PUT_2_DIGITS;


		case('5'):							/* 2 digit min, sec */
		case('6'):
			zfill_digit = zwidth ? CC_IDX_0_5 : CC_IDX_0_5_sp;
			goto PUT_2_DIGITS;

		case ' ':
			t++;
			fwidth++;
			*em++ = EC_CONSTCHAR;
			*cm++ = ' ';
			cm++;
			break;

		default:

			if (CMalpha(t))
			{
				length = f_getword(&t, 0, word);
				if (f_dkeyword(word, &class, &value))
				{
					switch(class)
					{
					case(F_MON):		/* month of year */
						if (length == F_monlen)
						{
							/* disallow full length month names */
							return FALSE;	
						}
						else
						{
							goto DO_A_WORD;
						}
						break;

					case(F_DOW):		/* day of week */
						if (length == F_dowlen)
						{
							/* disallow full length day names */
							return FALSE;	
						}
						else
						{
					DO_A_WORD:
							/* this assumes that the month names do not
							   contain double byte characters */
							for (p = word; *p; p++)
							{
								*em++ = EC_MANDATORY;
								*cm++ = CMupper(p) 
										? CC_IDX_AL_UC : CC_IDX_AL_LC;
								cm++;
								/* could get a little fancier and scan the
								   tables on the fly to create a customized
								   character class */
							}
						}
						break;

					case(F_PM): 	/* am/pm indicator */
								/* this assumes that am/pm indicators do
								   not contain double byte characters */
						p = buf;
						length = f_formword(
							length == F_pmlen ? F_Fullpm : F_Abbrpm,
							1, word, &p); 
						*em++ = EC_MANDATORY;
						*cm++ = CMupper(buf) ? CC_IDX_PM_UC : CC_IDX_PM_LC;   
						cm++;
						for (p = buf; --length; )
						{
							*em++ = EC_CONSTCHAR;
							*cm++ = *++p;
							cm++;
						}
						break;

					case(F_ORD): 		/* disallow ordinals */ 
					case(F_TIUNIT): 	/* disallow time units */
						return FALSE; 	
					}
				}
				else	/* else it's just a constant word */
				{
					for (p = word; *p; CMnext(p))
					{
						*em++ = EC_CONSTCHAR;
						if (CMdbl1st(p))
							dbl_chars++;
						CMcpychar(p, cm);
						cm += 2;
					}
				}

				zwidth = 0;
				fwidth = (CMspace(t)) ? -1 : 0;
			}
			else
			{
				*em++ = EC_CONSTCHAR;
				if (CMdbl1st(t))
					dbl_chars++;
				CMcpychar(t, cm);
				CMnext(t);
				cm += 2;
				zwidth = fwidth = 0;
			}
			break;
		}
	}

	/* post length of mask */
	*len = em - emask + dbl_chars;

	return TRUE; 
}

