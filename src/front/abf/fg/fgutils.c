/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<ug.h>
# include	<st.h>
# include	<me.h>
# include	<cv.h>
# include	<cm.h>
# include	<lo.h>
# include	<si.h>
# include	<ooclass.h>
# include	<abclass.h>
# include	<metafrm.h>
# include	<erfg.h>
# include	<ug.h>
# include	"framegen.h"

# define	HIDDEN	"iih_"
# define	HID_LEN 4	/* STlength(HIDDEN) */

# define	LRMKSZ	600	/* Size of "long remarks" field */
# define	SRMKSZ	60	/* Size of "short remarks" field */

static	TAGID fgmtag = 0;	/* TAG for dynamic memory */
static	char  srbuff[SRMKSZ+2];	/* short remarks buffer */
static	char  lrbuff[LRMKSZ+1];	/* long remarks buffer */
static	const char _optim[] = ERx("optimistic");

FUNC_EXTERN IITOKINFO	*IIFGftFindToken();

GLOBALREF	i4 IIFGhc_hiddenCnt;		/* initialized in fginit.c */

/**
** Name:	fgutils.c -	Code generation utilities
**
** Description:
**	This file defines:
**
**	IIFGts_tblsrch		Find a table in a metastructure
**	IIFGpt_pritbl		Find primary table for a section
**	IIFGti_tblindx		Get low and high table inidices for a section
**	IIFGkc_keycols		Get the key columns for a table
**	IIFGnkc_nullKeyCols	Get the NULLable key columns for a table
**	IIFGolc_optLockCols	Get the optimistic locking columns for a table
**	IIFGgm_getmem		Allocate (tagged) dynamic memory.
**	IIFGhn_hiddenname	Construct a hidden name
**	IIFGrm_relmem		Release all dynamic memory
**	IIFGga_getassign	Get assignment string for a column
**	IIFGgd_getdefault	Get default value string for a datatype
**	IIFGft_frametype	Get the text name for a VQ frame type.
**	IIFGwe_writeEscCode	Write out text buffer of User_escape code.
**	IIFGbuf_pad		Concat 2 buffers to newly allocated buffer.
**	IIFGmr_makeRemarks	Construct the long and short remarks for a form
**	IIFGnkf_nullKeyFlags	Tell if iiNullKeyFlag variables are needed.
**	IIFGmj_MasterJoinCol	Find primary column for master/detail join.
**	IIFGlccLowerCaseCopy	Allocate and create lower case copy of string
**	IIFGmsnMenuStyleName	Get Menu style name from mask
**	IIFGmsmMenuStyleMask	Get Menu style mask from name
**
** History:
**	4/28/89	(Mike S)	Initial version
**	5/15/89 (Mike S)	Change function names to agree with coding 
**				standard.
**	3/16/90 (pete)		Added IIFGnkf_nullKeyFlags(),
**					& IIFGmj_MasterJoinCol.
**	3/20/90 (pete)		Added IIFGnkc_nullKeyCols().
**	3/1/91  (pete)		Added IIFGlccLowerCaseCopy().
**	3/91	(Mike S)	Added Menu-style routines
**	6-dec-92 (blaise)	Added IIFGolc_optLockCols().
**	09-sep-93 (cmr)		Deleted IIFGolc_optLockCol().
**	15-feb-94 (blaise)	Reinstated IIFGolc_optLockCols() - it shouldn't
**				have been deleted (bug #57546)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	24-Aug-2009 (kschendel) 121804
**	    Need ug.h to satisfy gcc 4.3.
**/

FUNC_EXTERN char *IIFGgd_getdefault();
char *IIFGft_frametype();
STATUS IIFGwe_writeEscCode();

/*{
** Name:	IIFGts_tblsrch	- Find a table in a metaframe.
**
** Description:
**	Search for a table by name in a metaframe.
**
** Inputs:
**	mf		METAFRAME *		metaframe to search
**	table_name	char *			name of table
**
** Outputs:
**	none
**
**	Returns:
**			MFTAB * 		pointer to table structure
**						NULL if table not found
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	4/28/89	(Mike S) 	Initial version
*/
MFTAB 
*IIFGts_tblsrch( mf, table_name )
METAFRAME 	*mf;
char		*table_name;
{
	MFTAB **tptr = mf->tabs;
	i4 i=0;

	for ( ;i < mf->numtabs; i++, tptr++ )
		if ( !STcompare((*tptr)->name, table_name) )
			return (*tptr);
	
	return (NULL);
}

/*{
** Name:	IIFGpt_pritbl	- Find primary table in a section
**
** Description:
**	Find a primary table in the specified metaframe with a matching 
**	section number.
**
** Inputs:
**	mf		METAFRAME *		metaframe to search
**	section		i4			section number
**
** Outputs:
**	none
**
**	Returns:
**			MFTAB * 		pointer to table structure
**						NULL if table not found
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	5/4/89	(Mike S) 	Initial version
*/
MFTAB 
*IIFGpt_pritbl( mf, section )
METAFRAME 	*mf;
i4		section;
{
	MFTAB **tptr = mf->tabs;
	i4 i=0;

	for ( ;i < mf->numtabs; i++, tptr++ )
	{
		if ( ((*tptr)->tabsect == section) && 
		     ((*tptr)->usage <= TAB_DISPLAY) )
			return (*tptr);
	}
	
	return (MFTAB *) (NULL);
}

/*{
** Name:	IIFGti_tblindx	- Get high and low table indices for section
**
** Description:
**	The tables used in a section are assumed to be contiguous in the table
**	array.  We return the low and high indices.  If no tables are found for
**	the section, both indices are returned as -1.
**	section number.
**
** Inputs:
**	mf		METAFRAME *		metaframe to search
**	section		i4			section number
**
** Outputs:
**	low		i4 *			low index
**	high		i4 *			high index
**
**	Returns:	none
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	5/8/89	(Mike S) 	Initial version
*/
VOID 
IIFGti_tblindx( mf, section, low, high )
METAFRAME 	*mf;
i4		section;
i4		*low;
i4		*high;
{
	MFTAB **tptr = mf->tabs;
	i4 i=0;

	*low = *high = -1;
	for ( ;i < mf->numtabs; i++, tptr++ )
	{
		if ( (*tptr)->tabsect == section )
		{
			if (*low < 0)
				*low = i;
			*high = i;
		}
		else if (*low >= 0)
		{
			return;
		}
	}
	return;
}
/*{
** Name:	IIFGkc_keycols	- Get key columns for a table
**
** Description:
**	Search the table structure and pick out the columns wich form a
**	unique key
**
** Inputs:
**	table		MFTAB *			table structure
**	cols		i4 [DB_GW2_MAXCOLS]	column index array
**
** Outputs:
**	none
**
**	Returns:
**			i4			number of columns returned
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	4/28/89	(Mike S) 	Initial version
**	8/30/89	(Mike S) 	Change calling sequence
*/
i4
IIFGkc_keycols( table, cols )
MFTAB	*table;
i4	cols[];
{
	i4 	total = 0;
	i4	i = 0;
	MFCOL 	**colptr = table->cols;

	for ( ; i < table->numcols; i++, colptr++)
	{
		if ( ((*colptr)->flags & COL_UNIQKEY) != 0)
			cols[total++] = i;
	}
	return (total);
}

/*{
** Name:	IIFGnkc_nullKeyCols	- Get NULLable key columns for a table
**
** Description:
**	Search the table structure and pick out the NULLable columns
**	which are part of the unique key, and the nullable columns which are
**	being used for optimistic locking.
**
** Inputs:
**	table		MFTAB *			table structure
**	cols		i4 [DB_GW2_MAXCOLS]	column index array
**
** Outputs:
**	none
**
**	Returns:
**			i4		number of NULLable key columns returned
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	3/21/90 (pete)		Initial version (cloned from Mike's
**						IIFGkc_keycols() )
**	16-feb-94 (blaise)
**		Updated code to also handle nullable optimistic locking columns.
*/
i4
IIFGnkc_nullKeyCols( table, cols )
MFTAB	*table;
i4	cols[];
{
	i4 	total = 0;
	i4	i = 0;
	MFCOL 	**colptr = table->cols;
	IITOKINFO	*p_iitok;
	bool	optimistic = FALSE;	/* Are we using optimistic locking? */

	/*
	** Are we using optimistic locking? Get the value of the $locks_held
	** substitution variable
	*/
	p_iitok = IIFGftFindToken(ERx("$locks_held"));
	if (p_iitok != (IITOKINFO *)NULL &&
		STbcompare(p_iitok->toktran, 0, _optim, 0, TRUE) == 0)
	{
		optimistic = TRUE;
	}


	for ( ; i < table->numcols; i++, colptr++)
	{
		if ((*colptr)->type.db_datatype < 0) /* if column is nullable */
		{
			/*
			** if ((this is a key column) OR
			** (frame uses optimistic locking AND
			** column is being used for optimistic locking))...
			*/
			if ( (((*colptr)->flags & COL_UNIQKEY) != 0) ||
			(optimistic && ((*colptr)->flags & COL_OPTLOCK)) )
			{
				cols[total++] = i;
			}
		}
	}
	return (total);
}

/*{
** Name:	IIFGolc_optLockCols - Get optimistic locking columns
**
** Description:
**	Search the table structure and pick out the columns which are being
**	used for optimistic locking
**
** Inputs:
**	table		MFTAB *			table structure
**	cols		i4 [DB_GW2_MAXCOLS]	column index array
**	start		i4			where to start filling in the
**						array
**
** Outputs:
**	none
**
**	Returns:
**			i4			number of columns returned
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	3-dec-92 (blaise)
**		Initial version (also cloned from MikeS's IIFGkc_keycols)
*/
i4
IIFGolc_optLockCols(table, cols, start)
MFTAB	*table;
i4	cols[];
i4	start;
{
	i4 	total = start;
	i4	i = 0;
	MFCOL 	**colptr = table->cols;

	for ( ; i < table->numcols; i++, colptr++)
	{
		if ( ((*colptr)->flags & COL_OPTLOCK) != 0)
			cols[total++] = i;
	}
	return (total-start);
}

/*{
** Name:	IIFGgm_getmem	- Get dynamic memory
**
** Description:
**	Get tagged dynamic memory.  If necessary, allocate a tag.
**	or column name.
**
** Inputs:
**	size		u_i4 	number of bytes to get.
**	clear		bool	whether to clear memory.
**
** Outputs:
**	none
**
**	Returns:
**			PTR	memory allocated (NULL on failure)
**
**	Exceptions:
**		none
**
** Side Effects:
**		Allocates dynamic memory
**		May get a new memory tag
**
** History:
**	5/9/89	(Mike S)	Initial version
*/
PTR
IIFGgm_getmem( size, clear )
u_i4 size;
bool  clear;
{
	/* Get a tag, if we don't have one */
	if (fgmtag == 0) fgmtag = FEgettag();

	return( (char *)FEreqmem(fgmtag, size, clear, NULL) );
}

/*{
** Name:	IIFGhn_hiddenname	- Construct a hidden name
**
** Description:
**	Prepend "iih_" to a name to construct a unique hidden field
**	or column name. But if resulting name would be longer than
**	18 characters, then build an 18 character name as:
**
**		iih_  +  n  +  first_part_of_column_name
**		 ^	 ^	^
**		 |	 |	|
**		 |	 |	leading portion of tbl.col name so total length 
**		 |	 |	of name being built = 18 characters. Length of
**		 |	 |	part will vary based on length of "n".
**		 |	 |
**		 |	 a number that starts with 1 for first long-named hidden
**		 |	 and increments by 1 for each subsequent hidden with
**		 |	 a long name. A variable length string.
**		 |
**		 a fixed length string. all hiddens begin with this.
**
** Inputs:
**	name		char *	non-hidden name
**
** Outputs:
**	none
**
**	Returns:
**			char * pointer to hidden name
**
**	Exceptions:
**		none
**
** Side Effects:
**		Allocates dynamic memory
**
** History:
**	5/2/89	(Mike S)	Initial version
**	6/18/90 (Pete)		Changed to not make a hidden name longer than
**				18 characters (the limit on Gateways).
*/
char *
IIFGhn_hiddenname( name )
char *name;
{
#define FG_MAX_GTWNAME	18	/* max tbl.column length on Gateway */

	char *hname;
	char char_hidcnt[FG_MAX_GTWNAME + 1];
	char name_extract[FG_MAX_GTWNAME + 1];
	i4  nm_len, hidcnt_len, extract_len;

	nm_len = STlength(name);
	
	if ((nm_len + HID_LEN) > FG_MAX_GTWNAME)
	{
	    /* "iih_" + column name would be too long */

	    CVna (++IIFGhc_hiddenCnt, char_hidcnt);
	    hidcnt_len = STlength(char_hidcnt);

	    hname = (char *)IIFGgm_getmem((u_i4)(FG_MAX_GTWNAME + 1),
						(bool)FALSE);
	    if (hname != NULL)
	    {
	        extract_len = FG_MAX_GTWNAME - hidcnt_len - HID_LEN;
	        STlcopy(name, name_extract, extract_len);
	        STpolycat(3, HIDDEN, char_hidcnt, name_extract, hname);
	    }
	}
	else
	{
	    /* it's ok to just prepend "iih_" to name */
	    hname = (char *)IIFGgm_getmem(
			(u_i4)(STlength(name)+sizeof(HIDDEN)), (bool)FALSE);

	    if (hname != NULL)
	    {
		STcopy(HIDDEN, hname);
		STcat(hname, name);
	    }
	}

	return (hname);
}

/*{
** Name:	IIFGrm_relmem	- Release dynamic memory
**
** Description:
**	Release dynamic memory.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**		none
**
**	Exceptions:
**		none
**
** Side Effects:
**		Release dynamic memory.
**
** History:
**	5/2/89	(Mike S) Initial version
*/
VOID
IIFGrm_relmem()
{
	if (fgmtag != 0)
		FEfree(fgmtag);
}

/*{
** Name:	IIFGga_getassign	- Get assignment string for a column
**
** Description:
**	If the assignment string is defined, get it.  Otherwise, get the 
**	default for the column's type.
**
** Inputs:
**	column		MFCOL *			column pointer
**
** Outputs:
**	none
**
**	Returns:
**			 char * 		assignment string
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	5/4/89	(Mike S) 	Initial version
**	1/11/90 (pete)		Skip over leading "=" sign, if any.
*/
char *
IIFGga_getassign( column )
MFCOL		*column;
{
	if ( (column->info != NULL) && (*column->info != EOS) )
	{
		char *equals = ERx("=");

		/* skip leading blanks */
		column->info =STskipblank(column->info, STlength(column->info));

		/* if first nonblank character is "=" then skip it too */
		if ( CMcmpcase(column->info, equals) == 0)
			CMnext(column->info);

		return (column->info);
	}
	else 
		return(IIFGgd_getdefault(column->type.db_datatype));
}

/*{
** Name:	IIFGgd_getdefault	- Get default string for a datatype
**
** Description:
**	If the type is nullable, return "NULL".  Otherwise return "0" or a null
**	4GL string ('');
**
** Inputs:
**	type		DB_DT_ID	datatype
**
** Outputs:
**	none
**
**	Returns:
**			 char * 		default string
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	5/19/89	(Mike S) 	Initial version
**	7/09/06 (gupsh01)	Add date time types.
*/
char *
IIFGgd_getdefault( type )
DB_DT_ID	type;
{
	if (type < 0)
		return ("NULL");
	else
	{
		switch(type)
		{
		    case DB_CHR_TYPE:
		    case DB_TXT_TYPE:
		    case DB_DTE_TYPE:
		    case DB_ADTE_TYPE:
		    case DB_TMWO_TYPE:
		    case DB_TMW_TYPE:
		    case DB_TME_TYPE:
		    case DB_TSWO_TYPE:
		    case DB_TSW_TYPE:
		    case DB_TSTMP_TYPE:
		    case DB_INYM_TYPE:
		    case DB_INDS_TYPE:
		    case DB_CHA_TYPE:
		    case DB_VCH_TYPE:
			return("''");

		    default:
			return("0");
		}
	}
}

/*{
** Name:	IIFGft_frametype - return the text name of this VQ frame type.
**
** Description:
**	Return the frame type for this VQ frame, based on the "class" entry
**	in the USER_FRAME structure pointed to by the METAFRAME.
**
** Inputs:
**	OOID class	The class to be translated into a string frame type.
**
** Outputs:
**
**	Returns:
**		Pointer to string = text name of VQ frame type. Zero
**		length string if frame type is not a known VQ frame type.
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	6/2/89 (pete)	Written.
*/
char *
IIFGft_frametype(class)
OOID class;
{
	switch (class)
	{
	    case OC_MUFRAME:
		return ERget(F_FG0001_MenuFrame);

	    case OC_BRWFRAME:
		return ERget(F_FG0002_BrowseFrame);

	    case OC_APPFRAME:
		return ERget(F_FG0003_AppendFrame);

	    case OC_UPDFRAME:
		return ERget(F_FG0004_UpdateFrame);

	    default:
		return ERx("");
	}
}

/*{
** Name:	IIFGwe_writeEscCode - Write out text buffer of User_escape code.
**
** Description:
**	Write a text buffer that contains embedded newline characters to
**	a stream file. Prepend each line of text written with a buffer of
**	whitespace characters. Assumes every line in MFESC ends in a '\n',
**	even the last line.
**		Turn 'text' strings like: line1;\nline2;\nline3;\nEOS
**		Into:	line1;
**			line2;
**			line3;
**
** Inputs:
**	char	*text		text buffer to process.
**	char	*p_wbuf		whitespace to prepend to front of generated code
**	char	*p_extra_wbuf	optional additional whitespace buffer
**	FILE	*outfp;		file to write output to
**
** Outputs:
**	Writes to FILE *outfp.
**
**	Returns:
**		FAIL if write error occurs. OK otherwise.
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	6/6/89	(pete)	Written.
**	6/13/89 (pete)	Added "p_extra_wbuf".
**	12/16/92 (twai) Bug #48045.  Change i++ to CMbyteinc(i, text) to
**		handle double byte increment.
*/
STATUS
IIFGwe_writeEscCode(text, p_wbuf, p_extra_wbuf, outfp)
register char	*text;	/* text buffer with embedded newlines to write out */
char	*p_wbuf;	/* whitespace to tack to front of generated code */
char	*p_extra_wbuf;	/* optional additional whitespace buffer */
FILE *outfp;		/* file to write output to */
{
	i4	junk;		/* SIwrite() requires this */
	char buf[RECSIZE +1];	/* this gets built up prior to SIwrite() */
	char *p_buf ;
	i4  wbuf_len = STlength(p_wbuf) + STlength(p_extra_wbuf) ;
	register i4  i ;	/* count number of chars in each buf */

	p_buf = &buf[0] + wbuf_len;
	i = wbuf_len;

	STcopy (p_wbuf, buf);
	STcat (buf, p_extra_wbuf); /* Every buf written will have this
				   ** whitespace buffer prepended to it.
				   */
	/* copy text to buffer; stop when hit '\n' or EOS */
	while ( *text != EOS)
	{
	    /* new line; build up buffer till next newline, then write it */

	    CMcpychar(text, p_buf);
	    CMbyteinc(i, text);

	    if (*text == '\n')
	    {
	    	/* write out everything in buf, including '\n' */
	    	if (SIwrite ( i, buf, &junk, outfp) != OK)
	    	{
		    IIFGerr(E_FG0023_Write_Error, FGFAIL, 0);	/*returns FAIL*/
	    	}

		/* reinitialize buf for new line */
	    	p_buf = &buf[0] + wbuf_len;  /* buf, right after whitespace */
	    	i = wbuf_len ;

		/* point to next char to be copied in text */
	    	CMnext(text);
	    }
	    else
	    {
	    	CMnext(text);
	    	CMnext(p_buf);
	    }
	}

	return OK;
}

/*{
** Name:  IIFGbuf_pad - cat one buffer to end of another buf & allocate new buf.
**
** Description:
**	Take one buffer + another buffer and allocate a buffer big enough
**	to hold the concatenation of both buffers and concatenate them
**	both into the newly allocated buffer. Uses the memory tag set
**	up by IIFGgm_getmem, which is freed when 'framegen' returns.
**
** Inputs:
**	char *p_wbuf;	basic buffer to tack more onto
**	char *xtra;	extra buffer to tack on after 'p_wbuf'
**
** Outputs:
**
**	Returns:
**		pointer to newly allocated buffer with concatenation
**		result. (NULL if an error occurred allocating memory).
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	6/20/89 (pete)	Initial Version.
*/
char
*IIFGbuf_pad(p_wbuf, xtra)
char *p_wbuf;	/* basic whitespace buffer to tack more onto */
char *xtra;	/* extra whitespace to tack on after 'p_wbuf' */
{
	char *tmp;

        tmp = (char *)IIFGgm_getmem(
			(u_i4)(STlength(p_wbuf) + STlength(xtra) + 1), FALSE);

	if (tmp != NULL)
            STpolycat (2, p_wbuf, xtra, tmp);

	return tmp;
}

/*{
** Name: IIFGmr_makeRemarks	Construct the long and short remarks for a form
**
** Description:
**	Format remarks for a form which tell which application and frame
**	it's attached to. We'd like to get both the frame name and application 
**	name into the short remarks.  If that doesn't fit, we'll settle for the 
**	frame name.  Both always fit in the long remarks.
**
**	Each call to this routine overwrites the same buffers.
**
** Inputs:
**	frame	char *	Name of frame which form was constructed for.
**	appl	char *	Name of application which form was constructed for.
**
** Outputs:
**	srmk	char **	Short remark
**	lrmk	char **	Long remark
**
**	Returns:
**		none
**
**	Exceptions:
**		none
**
** Side Effects:
**	none.
**
** History:
**	8/2/89 (Mike S)	Initial version
*/
VOID
IIFGmr_makeRemarks(frame, appl, srmk, lrmk)
char	*frame;
char	*appl;
char 	**srmk;
char	**lrmk;
{
	/* Format the longer version of the short remarks */
	IIUGfmt(srbuff, SRMKSZ+2, ERget(S_FG0040_ShortRemark1), 2, 
	   	(PTR)frame, (PTR)appl);

	/*
	** If it's too big, format the shorter version.  IIUGfmt will
	** truncate it for us
	*/
	if (STlength(srbuff) > SRMKSZ)
		IIUGfmt(srbuff, SRMKSZ+1, ERget(S_FG0041_ShortRemark2), 1,
			(PTR)frame);

	/* Format the long remarks */
	IIUGfmt(lrbuff, LRMKSZ+1, ERget(S_FG0042_LongRemark), 2, 
		(PTR)frame, (PTR)appl);

	/* Return the string pointers */
	*srmk = srbuff;
	*lrmk = lrbuff;
}

/*{
** Name:	IIFGdq_DoubleUpQuotes - double-up any single quotes in a string.
**
** Description:
**	Scan through an input string and double up any single quotes you
**	find and write the results to an output string.
**
** Inputs:
**	char *rem;	scan this string for single quotes.
**	char *fix_rem;	write "rem" to here, and double up single quotes.
**			this string is assumed to be long enough. Output
**			string is guaranteed to be terminated by EOS.
**
** Outputs:
**	writes to second argument.
**
**	Returns:
**		VOID
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	1/09/90	(pete)	Initial Version.
*/
VOID
IIFGdq_DoubleUpQuotes( rem, fix_rem)
register char *rem;	/* short remark string -- may contain ' chars */
register char *fix_rem;
{
	char *single_quote = ERx("'");

	while ((rem != NULL) && (*rem != EOS))
	{
	    if ( CMcmpcase(rem, single_quote) == 0)
	    {
		/* we've hit a single quote character */
		CMcpychar(rem, fix_rem);	/* copy once */
		CMnext(fix_rem);		/* increment target */
		CMcpyinc(rem, fix_rem);		/* copy again */
	    }
	    else
	    {
		/* != "'"; copy and increment (*fix_rem++ = *rem++;) */
		CMcpyinc (rem, fix_rem);
	    }
	}

	*fix_rem = EOS;	/* null terminate */
}

/*{
** Name:	IIFGnkf_nullKeyFlags - Tell if iiNullKeyFlag vars needed.
**
** Description:
**	For applications run on Ingres before version 6.3/03, Ingres chokes on
**	a WHERE clause like:
**		"WHERE ((:iih_key IS NULL) AND (tbl.key IS NULL))
**			OR tbl.key = :iih_key)"
**	so we have to create indicator ("Flag") variables in the 4gl & set them
**	according to whether "iih_key" above is NULL, and then use a where
**	clause like:
**		"WHERE ((:iiNullKeyFlag1 = 1) AND (tbl.key IS NULL))
**			OR tbl.key = :iih_key)"
**	which 6.3/02 Ingres thinks is ok.
**	Since we have to worry about interoperability (whew, what a word!),
**	e.g. a 6.3/03 FE running against a 6.3/02 BE over a network, we always
**	use the indicator variable technique. However, if we knew which version
**	of Ingres the application is targeted for, we would not have to create
**	and set all these Flag variables (one for each nullable key), and could
**	simply generate the first of the two WHERE clauses above.
**	For now, this humble routine just returns TRUE.
**NOTE: BE SURE TO TEST GENERATED WHERE CLAUSES CAREFULLY IF THIS IS
**EVER CHANGED TO ALSO RETURN FALSE (they couldn't be tested thoroughly
**when this work was first done cause Ingres choked on the syntax).
**
** Inputs:
**	NONE.
**
** Outputs:
**
**	Returns:
**		bool
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	3/16/90	(pete)	Initial Version.
*/
bool
IIFGnkf_nullKeyFlags()
{
	return TRUE;
}

/*{
** Name:  IIFGmj_MasterJoinCol -  Find primary column for master-detail join.
**
** Description:
**	Find the column in the master table for a master-detail join.
**
** Inputs:
**	METAFRAME *mf   metaframe
**	MFTAB *detbl	Detail table
**	i4 detcol	Detail column index
**
** Outputs:
**
**	Returns:
**		(MFCOL *)	Pointer to Primary table col, or NULL if error.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	3/16/90	(pete)	Initial Version (code extracted from "masterjcol" in
**			"fgqclaus.c").
*/
MFCOL
*IIFGmj_MasterJoinCol (mf, detbl, detcol)
METAFRAME *mf;
MFTAB *detbl;           /* Detail table */
i4  detcol;             /* Detail column index */
{
        i4  i;
        MFJOIN *joinptr;

        /*
        ** Search all joins in metastructure for a match.
        */
        for (i = 0; i < mf->numjoins; i++)
        {
            joinptr = mf->joins[i];
            if ( joinptr->type == JOIN_MASDET &&
                detbl == mf->tabs[joinptr->tab_2] &&
                joinptr->col_2 == detcol )
                return mf->tabs[joinptr->tab_1]->cols[joinptr->col_1];
        }

        /* No join found -- return NULL, which means ERROR */
	return (MFCOL *) NULL;
}

/*{
** Name:        IIFGlccLowerCaseCopy - Allocate & make lower case copy of string
**
** Description:
**
** Inputs:
**	char	*string;	string to make lower case copy of.
**	TAGID	tag;		memory tag
**
** Outputs:
**      Returns:
**              (char *)     -  pointer to the memory with lower case copy
**				of string
**
** History:
**      1-mar-1991 (pete)
**		Initial Version.
*/
char *
IIFGlccLowerCaseCopy(string, tag)
char	*string;
TAGID	tag;
{
        char *p = NULL;
	i4 size = 0;

        if (string != (char*)NULL)
	{
            size = STlength(string) + 1;

            p =  (char*)FEreqmem((u_i4)tag, (u_i4)size, TRUE, (STATUS*) NULL);

	    if (p != (char*)NULL)
	    {
                STcopy(string, p);
		CVlower(p);
	    }
	}

        return (p);
}

/*{
** Name:	IIFGmsnMenuStyleName - Translate menu style mask to name
**      	IIFGmsmMenuStyleMask - Translate menu style name to mask
**
** Description:
**	The menu style used for a frame or application is stored in a 
**	4-bit mask, allowing 16 styles.  Thes routines translate between the
**	mask value and the menu style name.
**
**	We should never see illegal values at this level.  If we do,
**	we'll translate them to the origianl sinle-line style.
**
** Inputs:
**	mask	i4	mask value 	(for IIFGmsnMenuStyleName)
**	name	char *  style name	(for IIFGmsmMenuStyleMask)
**
** Outputs:
**	none
**
**	Returns:
**		char *	style name	(for IIFGmsnMenuStyleName)
**		i4	mask value	(for IIFGmsmMenuStyleMask)
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	3/91 (Mike S) initial version
*/

/* Message ID of menu style names; 0 means unused mask pattern */
static const	ER_MSGID ms_names[APC_MS_CHOICES] =
{
	F_FG003D_SingleLine,
	F_FG0040_TableField,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
} ;	
	
char *
IIFGmsnMenuStyleName(mask)
i4 mask;
{
	i4 normal;		/* Normalized version of mask */
	ER_MSGID msgid;

	normal = (mask & APC_MS_MASK) >> APC_MS_OFFSET;
	msgid = ms_names[normal];

	/* If we got a bad mask somehow, default to style 0 */
	if (msgid == 0)
		msgid = ms_names[0];
	return ERget(msgid);
}

i4
IIFGmsmMenuStyleMask(name)
char	*name;
{
	i4 i;

	for (i = 0; i < APC_MS_CHOICES; i++)
	{
		ER_MSGID id = ms_names[i];

		if (id == 0)
			continue;

		if (STbcompare(name, 0, ERget(id), 0, TRUE) == 0)
			return (i4)i << APC_MS_OFFSET;
	}

	/* No match; default to style 0 */
	return 0;
}
