# include	<compat.h>
# include	<cm.h>
# include	<er.h>
# include	<me.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<eqrun.h>
# include	"erls.h"

/*
**  Copyright (c) 2004 Ingres Corporation
*/

# ifdef VMS

# include	"iivmssd.h"

/**
+*  Name: iivmsstr.c - Routines to manage VMS string descriptors.
**
**  Description:
**	These routines are called from an embedded program and from the 
**	interface between an embedded program and LIBQ/RUNTIME when non-C 
**	character data/names are being processed.  They convert between VMS 
**	character data descriptors and data descriptions understood by INGRES, 
**	copying data where necessary.
**
**	In 6.0, all object names (input) are converted to C strings.  PARAM 
**	statement character (input and output) data is also converted 
**	to/from C strings.  Conversion to and from C strings requires data 
**	copying.
**
**	In 6.0, because of ADH/ADF conversion, the runtime system can
**	process character data which is not null-terminated.  Therefore, 
**	character I/O variables (except those coming through PARAM statements)
**	can now be passed directly into LIBQ and RUNTIME.  On I/O statements, 
**	these routines will point the caller at the data and length fields of
**	the VMS descriptor and only in the case of dynamic descriptors is
**	copying necessary.
**
**	The following table describes the transformation between VMS string
**	descriptors (only when used in non-param I/O statements) and internal 
**	data:
**
**	From/To VMS 		   To/From LIBQ
**	--------------	-----------------------------------------------------
**	descriptor 	type		len		data ptr
**	-------------	-------------	--------------	---------------------
**
**	Static string	
**	 (input/output)	DB_CHA_TYPE	sd_len		sd_ptr
**	 (input/notrim)	DB_CHR_TYPE	sd_len		sd_ptr
**
**	Dynamic str	
**	 (input)	DB_CHA_TYPE	sd_len		sd_ptr
**	 (input/notrim)	DB_CHR_TYPE	sd_len		sd_ptr
**	 (output)	DB_VCH_TYPE	sd_len		temp buffer (w/copying)
**
**	Varying string	
**	 (input)	DB_CHA_TYPE	sd_ptr.len	sd_ptr.data
**	 (input/notrim) DB_CHR_TYPE	sd_ptr.len	sd_ptr.data
**	 (output)	DB_VCH_TYPE	sd_len 		sd_ptr
**	----------------------------------------------------------------------
**
**  Defines:	
**	IIsd 			- Copy descriptor data to C string, trimming.
**	IIsd_no 		- Same, with no blank trimming.
**	IIsd_undo		- Copy C string back to descriptor.
**	IIsd_lenptr		- Get length and data pointer of descriptor
**	IIsd_convert		- Map descriptor into type, len & data ptrs.
**	IIsd_fill		- Map data into descriptor
**  Locals:
**	II_sdesc		- Actual conversion from descriptor to C string.
**	II_sdcopy		- Copy descriptor into a buffer
**	II_sderr		- Report error in managing descriptor
**
**  Notes:
**  	Do not port this file to another environment! If you do, you will be
**  	personally whipped by Neil !
-*
**  History:
**	26-feb-1985	- Rewritten from IIstring.c for Equel 
**			  Rewrite (ncg)
**	25-jun-1985	- Fixed bug 6033 to control tarshing of current
**			  data (ncg)
**	12-sep-1985	- Added IIsd and IIsd_no (ncg)
**	12-may-1987	- Added IIsd_convert and IIsd_fill; removed
**			  IIsd_reg, IIsd_notrim and IIsd_keepnull.
**	12-dec-1988	- Generic error processing. (ncg)
**	01-aug-1989	- Shut ranlib up (GordonW)
**	05-dec-1990 (barbara)
**	    Fixed bug 8680.  Allow users to insert using uninitialized dynamic
**	    strings.   See IIsd_lenptr().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# define	MAXvmsSTRING	4095

char	*IIsd(), *IIsd_no();
void	IIsd_undo(), IIsd_lenptr(), IIsd_convert(), IIsd_fill();

/* Locals */
char	*II_sdesc();
void	II_sderr();
i4	IIsd_copy();

/*
** IIsd - Convert SDESC into blanked trimmed C string.
*/
char	*
IIsd( sd  )
register char	*sd;
{
    return II_sdesc( II_TRIM, (SDESC *)sd );
}

/*
** IIsd_no - Convert SDESC into non-trimmed C string.
*/
char	*
IIsd_no( sd )
register char	*sd;
{
    return II_sdesc( 0, (SDESC *)sd );
}


/*
**  IIsd_undo -- Convert C string to host string
**
**	Takes the given C string and copies it to the string area
**	described by the string descriptor provided.
**
**	Truncation and/or blank fill takes place as needed.
**
**	Parameters:
**		cstr	-- pointer to C string.
**		sd	-- pointer to string descriptor.
*/

void
IIsd_undo( cstr, sd )
char	*cstr;
SDESC	*sd;
{
    i4	clength;

    clength = STlength( cstr );

    switch ( sd->sd_class )
    {
      case DSC_S: /* Static string descriptor - pad with blanks */
	STmove( cstr, ' ', sd->sd_len, sd->sd_ptr );
	break;

      case DSC_D: /* Dynamic string descriptor - system will create */
	lib$scopy_r_dx( &clength, cstr, sd );
	break;

      case DSC_VS: /* Varying string descriptor (first check max length) */
	{
	    char *p;

	    p = sd->sd_ptr;
	    clength = CMcopy( cstr, min(clength,sd->sd_len), p +2 );
	    *(i2 *)p = clength;
	}
	break;

      default:
	II_sderr( ERx("IIsd_undo"), sd->sd_class );
	break;
    }
}

/*
** IIsd_lenptr - Returns length and data pointer of a string descriptor.
**
**	This routine is called by the "IIx" routines that interface
**	between an embedded program and LIBQ/RUNTIME when character data
**	is being used for input.  It is also called by II_sdesc().
**
** History:	
**	05-dec-1990 (barbara)
**	    Fixed bug 8680.  Allow users to insert using uninitialized dynamic
**	    strings. BASIC strings declared as 'declare string s' are
**	    represented on VMS as dynamic strings.  Previously, uninitialized
**	    dynamic strings were sent as null pointers; this fix sends
**	    them as "empty" strings.
*/
void
IIsd_lenptr( sd, len, ptr )
register SDESC	*sd;
i4		*len;
char		**ptr;
{
    switch ( sd->sd_class )
    {
      case DSC_S:
      case DSC_D:
      {
	static char buf[1] = {'\0'};
	*len = sd->sd_len;
	*ptr = sd->sd_ptr;
	if (*ptr == (char *)0)
	    *ptr = buf;
      }
      break;
      case DSC_VS:
	*len = (*(i2 *)sd->sd_ptr);
	*ptr = sd->sd_ptr +2;
	break;
      default:
	{
	    static char buf[4] = {'?', '?', '?', '?'};

	    *len = 4;
	    *ptr = buf;
	    II_sderr( ERx("IIsd_lenptr"), sd->sd_class );
	}
	break;
    }
}


/*{
+*  Name: IIsd_convert - Convert from VMS descriptor to internal type.
**
**  Description:
**	This routine is called from the "IIx" layer  between an embedded
**	program and LIBQ/RUNTIME when an output statement is being
**	executed.  It is called in conjunction with IIsd_fill(), e.g.
**	    IIsd_convert();
**	    IIgetdomio();
**	    IIsd_fill();
**
**	IIsd_convert maps a VMS descriptor to the appropriate type, length 
**	and data-ptr that can be used by the runtime code to get data.  See 
**	the header notes for a table describing how the VMS descriptors get 
**	mapped on input and output.  If the string descriptor passed in is 
**	dynamic, this routine sets up the 'data' parameter to point at the 
**	passed in 'buf'.  Otherwise, 'data' will be set to point to the same 
**	place sd_ptr points at.
**
**  Inputs:
**	sd		Pointer to VMS string desciptor
**	    .sd_class	DSC_D, DSC_S, DSC_VS
**	buf		Pointer to caller's buffer assumed to be 
**			DB_MAXSTRING + DB_CNTSIZE
**
**  Outputs:
**	type		Internal character type for getting data
**	len		Maximum length of data
**	data		Will point to area where data is to be put by runtime
**
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	13-may-1987 - text (bjb)
*/
void
IIsd_convert(sd, type, len, data, buf)
SDESC	*sd;
i4	*type;
i4	*len;
PTR	*data;
char	*buf;
{
    switch (sd->sd_class)
    {
      case DSC_S:		/* Static string descriptor */
    	*len = sd->sd_len;
	*data = sd->sd_ptr;
	*type = DB_CHA_TYPE;
	break;

      case DSC_D:		/* Dynamic string descriptor */
	*len = DB_MAXSTRING + DB_CNTSIZE;
	*data = buf;
	*type = DB_VCH_TYPE;
	break;

      case DSC_VS:		/* Varying string descriptor */
	*len = sd->sd_len + DB_CNTSIZE;
	*type = DB_VCH_TYPE;
	*data = sd->sd_ptr;
	break;
    }
}

/*{
+*  Name: IIsd_fill - Complete a VMS descriptor with length and data.
**
**  Description:
**	IIsd_fill is called from the "IIx" layer  between an embedded
**	program and LIBQ/RUNTIME when an output statement is being
**	executed.  It must be called in conjunction with IIsd_convert(), e.g.
**	    IIsd_convert();
**	    IIgetdomio();
**	    IIsd_fill();
**
**	If the data descriptor is dynamic, this routine copies from the
**	caller's passed in buffer to the descriptor, using a VMS function.
**
**  Inputs:
**	sd		- Pointer to VMS string descriptor
**	    .sd_class	- If this field = DSC_D, then assign 'data' to
**			  user's string by calling a VMS function, which will 
**			  copy and set the length.  For all other classes of 
**			  descriptors do nothing because IIsd_convert will have 
**			  arranged to have data put directly into .sd_ptr.
**	ind		- Pointer to embedded null indicator.  If this has
**			  the value of DB_EMB_NULL, don't copy data.
**	data		- Pointer to data
**
**  Outputs:
**	sd		- Pointer to VMS string descriptor
**	    .sd_len	- Will be filled if dynamic descriptor
**	    .sd_ptr	- Data will have been copied here if dynamic descriptor
**	Returns:
**	    Noting
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	15-may-1987 - written (bjb)
*/
void
IIsd_fill(sd, ind, data)
SDESC	*sd;
i2	*ind;
PTR	data;
{
    switch (sd->sd_class)
    {
      /* Dynamic string descriptors are the only ones that require copying */
      case DSC_D:
	if (!ind || *ind != DB_EMB_NULL)
	{
	    DB_TEXT_STRING	*dbtxt;

	    dbtxt = (DB_TEXT_STRING *)data;
	    lib$scopy_r_dx( &dbtxt->db_t_count, dbtxt->db_t_text, sd );
	}
	break;
      /* 
      ** For other kinds of descriptors, assume that data was retrieved/put
      ** directly into area pointed at by sd_ptr.
      */
      case DSC_S:
      case DSC_VS:
	break;
    }
}
	

/*
** II_sdesc - Real conversion routine from VMS string descriptor to C string.
**
** Parameters:	flag --	What to do with converted string.
**		sd   -- Host string descriptor.
*/

char	*
II_sdesc( flag, sd )
i4		flag;
register SDESC	*sd;
{
/* 
** Logic on size is to supply at least enough room for one name (usually < 20)
** plus space for data <= 2000.
** BUG 6033 - But in case the data does not fit, and we start from the beginning
** 	      of the buffer, do not trash data of current call. Therefore 
**	allow up to 5000 to prevent trashing current data. 
** Example:
**	putform f (f1=c700, f2=c2000)
**	
**	used to give string table:
**
**		'f1' 'c700' 'f2' c2000 does not fit so wrap around and TRASH
**		previous data including 'f2'.
**
** 	If we have 5000 bytes then we cannot trash data of current call as the
**	only way we wrap around to the start is if the previous data is beyond
**	the middle.
*/
#   define 		STRTABSIZE	2*MAXvmsSTRING +2
    static		char	*str_buf = (char *)0;
    static		char	*str_ptr = (char *)0;

    register i4	slen;
    i4	 		sd_len;
    register char 	*sptr;
    char	 	*sd_ptr;
    char 		locbuf[MAXvmsSTRING +1];

    if ( sd == NULL )
	return (char *)0;
    if (str_buf == NULL)	/* Alloc a buffer */
    {
	if ( MEcalloc(1, STRTABSIZE, (i4 *)&str_buf) != OK )
	{
	    /* Fatal error */
	    IIlocerr(GE_NO_RESOURCE, E_LS0101_SDALLOC, 0, 0 );
	    IIabort();
	}
	str_ptr = str_buf;			/* Reset roving pointer */
    }
    IIsd_lenptr( sd, &sd_len, &sd_ptr );	
    slen = II_sdcopy( flag, sd_ptr, sd_len, locbuf );  /* Copy and 0 term */
    /*
    ** Logic is if that is enough room not to trash previous data,
    ** then fit it in.  Otherwise readjust to start of buffer.  Note
    ** that when there are multiple args to a single call, these
    ** are usually very small anyway.
    */
    sptr = str_ptr;
    if (sptr - str_buf + slen > STRTABSIZE)
	sptr = str_buf;
    slen = CMcopy( locbuf, (u_i2)slen, sptr );
    str_ptr = sptr + slen;
    return sptr;
}

/*
** II_sdcopy - Copy string descriptor data into a C buffer.
**
** Parameters:	flag 	-- Leave nulls or not (Cobol).
**		src	-- Source data from host.
**		dlen	-- Length to copy to destination.
**		dbuf	-- Destination C buffer.
*/

i4
II_sdcopy( flag, src, dlen, dbuf )
i4		flag;
char		*src;			/* Source string ptr */
i4		dlen;			/* Destination length and buffer */
char		dbuf[];
{

    /*
    ** C doesn't like nulls embedded in strings so we must mask them out!
    ** Data is same size as storage area and may be blank padded.
    ** Copy to destination buffer, null terminate.
    */
    dlen = CMcopy( src, min(dlen, MAXvmsSTRING), dbuf );
    dbuf[dlen] = '\0';
    if ( flag & II_TRIM )
	return STtrmwhite(dbuf) +1;	      /* Add null after trimmed data */
    else
	return dlen +1;
}


void
II_sderr( who, class )
char	*who;
char	class;
{
    i4		nclass;
    char	sclass[5];

    nclass = class;
    CVna(nclass, sclass);
    IIlocerr(GE_DATA_EXCEPTION, E_LS0102_BADSTRING, 0, 2, who, sclass);
    /* IIabort(); - Can recover from this */
}

# else
static	i4	ranlib_dummy;
# endif /* VMS */
