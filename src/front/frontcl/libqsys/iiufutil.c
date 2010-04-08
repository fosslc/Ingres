# include <compat.h>
# include <st.h>
# include	<gl.h>
# include	<sl.h>
# include	<me.h>
# include	<iicommon.h>
# include <generr.h>
# include <eqrun.h>
# if defined(UNIX) || defined(hp9_mpe) && !defined(NT_GENERIC)
# include <iiufsys.h>
# include "erls.h"
/*
** Copyright (c) 2004, 2008 Ingres Corporation
*/

/*
+* Filename:	iiufutil.c
** Purpose:	Utility routines to implement PARAM statements in F77
**		These routines are only required on UNIX.
**
** Defines:
**	IIxintrans()		- Return pointer to copy of user's INPUT data
**	IIxouttrans()		- Copy OUTPUT data to user's data area 
**	IInum()			- Return pointer to numeric data passed in
**	IIstr()			- Given a string, return address of descriptor
**	IIsd()			- Given a string, null terminate and trim.
**	IIsdno()		- Given a string, null terminate (no trim).
**	IIps()		        - Dereference a reference to structure pointer.
**	IIslen()		- Given a string, return its length.
**	IIsadr()		- Given a string construct, return an address
**				  to the string portion.
-*
** Notes:
** 0)	This file is one of four files making up the F77 runtime layer.
**	These files are:
**		iiuflibq.c 	- libqsys directory
**		iiufutils.c	- libqsys directory
**	        iiufrunt.c	- runsys directory
**		iiuftb.c	- runsys directory
**	See comments at the top of iiuflibq.c for a detailed explanation
**	on the F77 runtime layer.
**
** 1)	PARAM statements need special runtime treatment in F77 because
**	there is no FORTRAN instruction to assign the address of
**	a variable to the PARAM argument vector.  Therefore, to set
**	up their PARAM argument lists, F77 EQUEL programs make use of the 
**	functions
**	    IInum( numeric_arg ) 
**	    IIstr( string_arg )
**	which return addresses of their respective arguments.
**
** 2)   When the PARAM arg_list is used in an EQUEL statement, our
**	runtime routines call the translation routines defined in
**	this file (IIxintrans, IIxouttrans) to access a copy of user data 
**	or to copy it to the user data area, respectively.
**
** 3)   Some FORTRAN compilers pass the string length before the address
** 	of the string.  IIstr_, IIsd_ and IIsdno_ are ifdef'ed for this
**	case.
**
** History:
**	17-jun-86	- writlten (bjb)
**	01-sep-86	- modified for 5.0 UNIX EQUEL/FORTRAN
**	2/19/87	(daveb)	-- Hide fortran details in iiufsys.h by using
**			  FLENTYPE and FSTRLEN for string lengths.
**			  Add wrappers for apollo's different way of
**			  creating C names from the fortran compiler.
**	15-jun-87 (cmr) - use NO_UNDSCR (defined in iiufsys.h) for
**			  for compilers that do NOT append an underscore
**			  to external symbol names.
**	17-mar-88 (russ)- using II_sdesc for EQUEL/ADA strings.  Any changes
**                        here need to be reflected in front/ada_unix/eqdef.a.
**      1-jun-88 (markd) - Added LEN_BEFORE_VAR case for FORTRAN compilers
**			   which pass the pointer to the string length before
**			   the pointer to the string.
**      10-oct-88(russ)  - Added the STRUCT_VAR case for compilers which 
**                         pass strings as a structure.
**	12-dec-88(neil)  - Generic error processing.
**      26-jun-90(fredb) - Added hp9_mpe to define allowing this file to be
**                         compiled.  MPE/XL Fortran/77 needs these routines.
**	12-sep-90 (barbara)
**	    Incorporated part of Russ's mixed case changes here using
**	    the same code as NO_UNDSCR.  The other redefines for external
**	    symbols are in separate files.
**	14-sep-90 (barbara)
**	    Undid last change.  It won't work.
**	30-Jul-1992 (fredv)
**	    For m88_us5, we don't want to redefine these symbols because
**	    m88_us5 FORTRAN compiler is NO_UNDSCR and MIXEDCASE_FORTRAN.
**	    If we do that, symbols in iiufutlM.o will be all screwed up.
**	    This is a quick and dirty change for this box for 6.4. The
**	    NO_UNSCR and MIXEDCASE_FORTRAN issue should be revisited in
**	    6.5. Me and Rudy both have some idea what should be the change
**	    to make this FORTRAN stuff a lot cleaner.
**	25-Aug-1993 (fredv)
**	    Included <me.h>.
**	17-dec-1993 (donc) Bug 56875
**	    Add IIps. Prepare ... into :sqlda statements in UNIX F77 need
**	    to now pass a dereferenced pointer to the sqlda to IIsqPrepare.
**      02-aug-1995 (morayf)
**              Added iips_ define for donc change of 17-dec-1993.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-apr-2001 (mcgem01)
**          Implement Embedded Fortran Support on Windows.
**  22-Sep-2003 (fanra01)
**      Bug 110906
**      Disabled definition for windows platform as there are windows
**      specific calls.
**      20-Jun-2008 (hweho01)
**          Added function prototype for II_sdesc.
**	24-Aug-2009 (kschendel) 121804
**	    Fix return from IIxouttrans, IIretparam needs a value.
*/

# define MAXunxSTRING	4095

# if defined(NO_UNDSCR) && !defined(m88_us5)
/* compiler does NOT append an underscore to calls from fortran */
# define	iinum_		iinum
# define	iips_		iips
# define	iistr_		iistr
# define	iisd_		iisd
# define	iisdno_		iisdno
# define	iislen_		iislen
# define	iisadr_		iisadr
# endif

/* Locals */
char  *II_sdesc( int, char *, int);

/*
** SDESC: string descriptor used in F77 implementation of param statements.
**     User calls IIstr to assign a string var pointer to the argument vector.
**     However, the runtime output routines need the length as well as the 
**     address of the string variable in order to transfer data.
**     Therefore IIstr returns the address of a dynamically allocated string 
**     descriptor instead of the address of the user's string variable.
*/
typedef struct
{
    char	*sd_ptr;		/* pointer to user's str var */
    i4		sd_len;			/* len of user's str var  */
} SDESC;


/*
+* Procedure:	IIxintrans
** Purpose:	Return pointer to copy of user's data (re PARAM stmt).
** Parameters:
**	type	- i4	- data type
**	len	- i4	- data size
**	arg	- *char - data pointer from argv list
** Return Values:
-*	Pointer to real data
** Notes:
**	- Get at data being sent TO database via a param statement.
**	- Called from our runtime routines (NOT from FORTRAN program).
**	- Called once for each argument pointer in user's argv list 
**	  (corresponding to each item in his format list).  
**	- Pointers to user's arguments of type char are really pointers 
**	  to string descriptors allocated via user's IIstr call.
**	- Copy string data so that we can null terminate it.
*/
char *
IIxintrans( type, len, arg )
i4	type;
i4	len;
char	*arg;				/* Pointer from user's argv */
{
    static	char	buf[DB_MAXSTRING +1];	/* For copy of user's string */

    if (type != DB_CHR_TYPE )
	return arg;
    {
	SDESC 	*sd;
	sd = (SDESC *)arg;
	MEcopy( sd->sd_ptr, sd->sd_len, buf );
	buf[sd->sd_len] = '\0';
	if (len >= 0)		    /* negative len means no trim */
	    STtrmwhite( buf );
	return buf;
    }
}


/*
+* Procedure:	IIxouttrans
** Purpose:	Move result data of PARAM statement to user's variable
** Parameters:
**	type	- i4	- data type of user var
**	len	- i4	- size of user var
**	data	- *char	- pointer to data from LIBQ/FRS
**	addr	- *char - pointer to user's var or string descriptor
** Return Values:
-*	i4 always TRUE
** Notes:
**	- Assign data coming FROM database via a param call.
**	- Called from our runtime routines (NOT from FORTRAN program)
**	  for each argument pointer in user's argv list corresponding to 
**	  each item in his target list).  
**	- pointers to user's arguments of type char are really pointers to 
**	  dynamically allocated string descriptors.
**	- The return value is always TRUE, see IIretparam() in frame/runtime.
*/
i4
IIxouttrans( type, len, data, addr )
i4	type;
i4	len;
char	*data;
char	*addr;		/* host address (or temporary string descriptor */
{
    if (type != DB_CHR_TYPE)
	MEcopy( data, len, addr ); 
    else
    {
	SDESC	*sd;
	sd = (SDESC *)addr;
	STmove( data, ' ', sd->sd_len, sd->sd_ptr );  /* move str to host var */
    }
    return (1);
}

/*
+* Procedure:	IInum
** Purpose:	Returns address of its numeric argument 
** Parameters:
**	ivar	- i4	- Pointer to user's numeric var
** Return Values:
-*	Pointer to numeric var
** Notes:
**	- Called from F77 EQUEL program in setting up argv list 
**	  for numeric variables for PARAM statements.
**	- Easy to return pointer to numeric data since F77 data is
**	  passed by reference.
**
*/
i4  *
iinum_( ivar )
i4	*ivar;
{
    return ivar;
}


/*
+* Procedure:	IIstr
** Purpose:	Return address of string descriptor for incoming string
** Parameters:
**	svar	- *char	- pointer to user's string variable
**	slen	- i4	- F77 supplied length of string var
** Return Values:
-*	Pointer to dynamically assigned string descriptor
** Notes:
**	- Called from F77 EQUEL program in setting up argv list 
**	  for string variables for PARAM statements.
**	- Instead of returning pointer to string var, we allocate
**	  a string descriptor and return pointer to that.
**	- String descriptor is necessary during execution of param
**	  statement because it contains string length.
**	- Allocation is as economically as possible by allocating
**	  space for 30 SD's at a time, and managing our own buffer
**	  of space.
*/

char *
# ifdef LEN_BEFORE_VAR   	/* Switch if compiler gives len before svar*/
    iistr_( slen, svar )
# else 
#	ifdef STRUCT_VAR
	    iistr_( svar )
#	else
	    iistr_( svar, slen )
#	endif 
# endif

# ifdef STRUCT_VAR
    struct x3b_fchar	*svar;
# else
    char	*svar;
    FLENTYPE	slen;		/* f77 Compiler-supplied length */
# endif
{
    register  SDESC 	*sd;
    static    i4     	sd_cnt = 0;
    static    SDESC	*sd_pool ZERO_FILL;
#   define    SD_MAX	30

    if (sd_cnt == 0)
    {
	sd_pool = (SDESC *)MEreqmem(0, SD_MAX * sizeof(*sd), TRUE, NULL);
	if (sd_pool == NULL)
	{
	    /* Fatal error */
	    IIlocerr(GE_NO_RESOURCE, E_LS0101_SDALLOC, 0, 0 );
	    IIabort();
	}
	sd_cnt = SD_MAX;
    }
    sd = sd_pool;
    sd_pool++;
    sd_cnt--;
# ifdef STRUCT_VAR
    sd->sd_ptr = svar->s;	/* Pointer to user's string var */
    sd->sd_len = svar->len;
# else
    sd->sd_ptr = svar;		/* Pointer to user's string var */
    sd->sd_len = FSTRLEN(slen);
# endif
    return (char *)sd;
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
# ifdef LEN_BEFORE_VAR   	/* Switch if compiler gives len before svar*/
    iisd_( slen, svar )
# else 
#	ifdef STRUCT_VAR
	    iisd_( svar )
#	else
	    iisd_( svar, slen )
#	endif 
# endif

# ifdef STRUCT_VAR
    struct x3b_fchar	*svar;
# else
    char	*svar;
    FLENTYPE	slen;		/* f77 Compiler-supplied length */
# endif
{
# ifdef STRUCT_VAR
    return II_sdesc(II_TRIM, svar->s, svar->len);
# else
    return II_sdesc(II_TRIM, svar, FSTRLEN(slen));
# endif
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
# ifdef LEN_BEFORE_VAR   	/* Switch if compiler gives len before svar*/
    iisdno_( slen, svar )
# else 
#	ifdef STRUCT_VAR
	    iisdno_( svar )
#	else
	    iisdno_( svar, slen )
#	endif 
# endif

# ifdef STRUCT_VAR
    struct x3b_fchar	*svar;
# else
    char	*svar;
    FLENTYPE	slen;		/* f77 Compiler-supplied length */
# endif
{
# ifdef STRUCT_VAR
    return II_sdesc(0, svar->s, svar->len);
# else
    return II_sdesc(0, svar, FSTRLEN(slen));
# endif
}

/*
+* Procedure:	IIps
** Purpose: 	Dereference the pointer for the input structure.	
** Parameters:
**	ps	- PTR	- F77 supplied pointer 
** Return Values:
-*	Pointer to the inputted pointer.
*/

PTR 
iips_( ps )
PTR	ps;
{
    PTR tmpptr = ps;
    return tmpptr;
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
# ifdef LEN_BEFORE_VAR   	/* Switch if compiler gives len before svar*/
    iislen_( slen, svar )
# else 
#	ifdef STRUCT_VAR
	    iislen_( svar )
#	else
	    iislen_( svar, slen )
#	endif 
# endif

# ifdef STRUCT_VAR
    struct x3b_fchar	*svar;
# else
    char	*svar;
    FLENTYPE	slen;		/* f77 Compiler-supplied length */
# endif
{
# ifdef STRUCT_VAR
    return svar->len;
# else
    return FSTRLEN (slen);
# endif
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
# ifdef LEN_BEFORE_VAR   	/* Switch if compiler gives len before svar*/
    iisadr_( slen, svar )
# else 
#	ifdef STRUCT_VAR
	    iisadr_( svar )
#	else
	    iisadr_( svar, slen )
#	endif 
# endif

# ifdef STRUCT_VAR
    struct x3b_fchar	*svar;
# else
    char	*svar;
    FLENTYPE	slen;		/* f77 Compiler-supplied length */
# endif
{
# ifdef STRUCT_VAR
    return svar->s;
# else
    return svar;
# endif
}

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

# endif /* UNIX */
