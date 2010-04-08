# include <compat.h>
# include <cm.h>
# include <er.h>
# include <me.h>
# include <st.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <generr.h>
# include <eqrun.h>
# include "erls.h"

/* 
** Copyright (c) 2004 Ingres Corporation  
*/ 

# ifdef NT_GENERIC

# define MAXunxSTRING        4095

# include "iivmssd.h"

# ifndef FSTRLEN
	typedef		i4	FLENTYPE;
#	define		FSTRLEN(l)	(l)
# endif


/**
+*  Name: iiw32str.c - Routines to manage Windows string descriptors.
**
**  Description:
**	These routines are called from an embedded program and from the 
**	interface between an embedded program and LIBQ/RUNTIME when non-C 
**	character data/names are being processed.  They convert between Windows 
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
**	the Windows descriptor and only in the case of dynamic descriptors is
**	copying necessary.
**
**	The following table describes the transformation between Windows string
**	descriptors (only when used in non-param I/O statements) and internal 
**	data:
**
**	From/To Windows 		   To/From LIBQ
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
**      22-Sep-2003 (fanra01)
**          Bug
**          Created based on the VMS version iivmsstr.c
**/

char	*IIsd(), *IIsd_no();
void	IIsd_undo(), IIsd_lenptr(), IIsd_convert(), IIsd_fill();

/* Locals */
char	*II_sdesc();
void	II_sderr();
i4	IIsd_copy();
/*
** II_sdesc - Real convertion routine from user string to C string.
**
** Parameters:	flag --	What to do with converted string.
**		str  -- Host string.
**		len  -- Length of the string.
*/

char *
II_sdesc( flag, str, len  )
i4	flag;
char	*str;
i4	len;
{
/*
** Logic on size is to supply at least enough room for one name (usually < 20)
** plus space for data <= 2000.
** But in case the data does not fit, and we start from the beginning
** of the buffer, do not trash data of current call. Therefore
** allow up to 5000 to prevent trashing current data.
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
#   define 		STRTABSIZE	2*MAXunxSTRING +2
    static		char	*str_buf = (char *)0;
    static		char	*str_ptr = (char *)0;

    i2			slen;
    char	 	*sptr;
    char 		locbuf[MAXunxSTRING +1];

    if ( str == NULL )
	return (char *)0;
    if (str_buf == NULL)	/* Alloc a buffer */
    {
	str_buf = (char *)MEreqmem(0, STRTABSIZE, TRUE, NULL);
	if ( str_buf == NULL )
	{
	    IIlocerr(GE_NO_RESOURCE, E_LS0101_SDALLOC, 0, 0 );
	    IIabort();
	}
	str_ptr = str_buf;			/* Reset roving pointer */
    }
    slen = STlcopy( str, locbuf, (i2)len );	/* Copy and 0 term */
    if (flag & II_TRIM)
       slen = STtrmwhite(locbuf);
    slen++;		/* to account for the null */
    /*
    ** Logic is if that is enough room not to trash previous data,
    ** then fit it in.  Otherwise readjust to start of buffer.  Note
    ** that when there are multiple args to a single call, these
    ** are usually very small anyway.
    */
    sptr = str_ptr;
    if ( str_ptr - str_buf + slen > STRTABSIZE )
	sptr = str_buf;
    MEcopy( locbuf, (u_i2)slen, sptr );
    str_ptr = sptr + slen;
    return sptr;
}

/*
+* Procedure:	IIslen
** Purpose:	Return the length of a string in FORTRAN
** Parameters:
**	svar	- *char	- pointer to user's string variable
**	slen	- i4	- F77 supplied length of string var
** Return Values:
-*	integer length of string, not including the null terminator
** Notes:
**	- The FORTRAN preprocessor generates calls to IIslen for
**	  all string variables or string constants requiring that
**	  a length also be passed to the runtime interface.
**	- Previously, a 0 length was passed for strings.
*/

i4
IIslen( svar, slen )
char	*svar;
FLENTYPE	slen;		/* f77 Compiler-supplied length */
{
    return FSTRLEN (slen);
}

/*
+* Procedure:	IIsd
** Purpose:	Convert string into blank trimmed C string.
** Parameters:
**	svar	- *char	- pointer to user's string variable
**	slen	- i4	- F77 supplied length of string var
** Return Values:
-*	Pointer into our temp buffer where the string was converted.
*/

char *
IIsd( svar, slen )
    char	*svar;
    FLENTYPE	slen;		/* f77 Compiler-supplied length */
{
    return II_sdesc(II_TRIM, svar, FSTRLEN(slen));
}


/*
+* Procedure:	IIsdno
** Purpose:	Convert string into C string -- no trimming.
** Parameters:
**	svar	- *char	- pointer to user's string variable
**	slen	- i4	- F77 supplied length of string var
** Return Values:
-*	Pointer into our temp buffer where the string was converted.
*/

char *
IIsdno( svar, slen )
    char	*svar;
    FLENTYPE	slen;		/* f77 Compiler-supplied length */
{
    return II_sdesc(0, svar, FSTRLEN(slen));
}

/*
+* Procedure:	IIsadr
** Purpose:	To hide varying string passing methods used by different
**		F77 compilers.
** Parameters:
**	svar	- *char	- pointer to user's string variable
**	slen	- i4	- F77 supplied length of string var
** Return Values:
-*	pointer to string.
** Notes:
**	- The FORTRAN preprocessor generates calls to IIsadr for
**	  all string variables or string constants not hidden by
**	  a call to IIsd or IIsdno.
**	- The runtime interface will receive the string as
**			char	**str;
**	  However, most portions of the interface layer receive
**	  the arguement as
**			i4	*var_ptr;
**	  and if (*type == DB_CHR_TYPE)
**	  must cast the pointer as (... *(char **)var_ptr, ...)
**	- Use of this function, with IIslen, allows most routines
**	  in the runtime interface to ignore differences in string
**	  passing techinques used by the various F77 compilers.
*/

char *
IIsadr( svar, slen )
char	*svar;
FLENTYPE	slen;		/* f77 Compiler-supplied length */
{
    return svar;
}

# else  /* NT_GENERIC */
static	i4	ranlib_dummy;
# endif /* NT_GENERIC */
