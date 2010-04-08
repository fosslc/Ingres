/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<eqrun.h>

# ifdef	VMS

/**
+*  Name: iivtbacc.c - VMS interface to TBACC to handle character names/data.
**
**  Description:
**	These routines (for VMS non-C environments ONLY) are called from an 
**	embedded program when character data is being sent/received in 
**	database statements.  Their purpose is to transform a VMS 
**	data structure (i.e., a descriptor) into a format that TBACC/ADH
**	knows about.  Data copying is avoided if at all possible for
**	performance reasons.
**
**	In 6.0, ADH has a convention regarding character types from embedded
**	programs as follows:
**	
**	DB_CHR_TYPE 
**	w/len of zero (Input)	Assume to be null terminated (input) and 
**				take len = STlength.
**	  "    "   "  (Output)	Assume to have len of DB_MAXSTRING.  Null
**				terminate.
**
**	DB_CHR_TYPE		
**	w/len > 0     (Input)	Use given length.  Don't trim blanks. 
**	"      "   "  (Output)	Use given length.  Null terminate.
**
**	DB_CHA_TYPE 	
**	w/len > 0     (Input)	Don't assume null termination.  Trim blanks.
**	"     "    "  (Output)	Blank pad out to length.  Don't null terminate.
**
**	DB_VCH_TYPE
**		      (Input)	No trimming.
**		      (Output)	Don't blank pad.
**
**	In general, in 6.0, non-C VMS descriptor character data is sent as 
**	DB_CHA_TYPE because we require it to be blank trimmed on input and 
**	blank padded on output.  However, there are some exceptions.  See the 
**	table in iivmsstr.c for a complete mapping of VMS string descriptors to 
**	internally understood descriptions.
**
**	The pre-6.0 interface routines are also contained in this file.
**
**  Defines: 
**    New Interface for 6.0:
**	IIxtcoputio	- Interface to IItcoputio
**	IIxtcogetio	- Interface to IItcogetio
**    Pre 6.0 interface:
** 	IIxtcolret( isvar, type, len, addr, column )
**	IIxtrc_param( tlist, argv )
**	IIxtsc_param( tlist, argv )
**
**  Notes:
** 	These routines should not be ported to non-VMS environments.  
-*
**  History:
**	27-feb-1985	- Written (ncg)
**	13-may-1987	- Extended to include input interface (bjb)
**	01-aug-1989	- Shut up ranlib (GordonW)
**	01-dec-1989	- Fixed Bug #8966. 'ind' arg to IIxtcoputio was
**			  not indirected. (barbara)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* External data */
FUNC_EXTERN	char	*IIsd(), *IIsd_no();
FUNC_EXTERN	void	IIsd_undo(), IIsd_lenptr(), IIsd_convert(), IIsd_fill();

/*
** IIxtcoputio - Interface to IItcoputio
*/
void
IIxtcoputio(colname, ind, isref, type, length, data)
char	*colname;
i2	*ind;
i4	isref, type, length;
PTR	data;
{
    i4	llen;
    PTR	ldata;

    if (type != DB_CHR_TYPE)
    {
	_VOID_ IItcoputio( colname, ind, isref, type, length, data );
	return;
    }
    IIsd_lenptr( data, &llen, &ldata );
    _VOID_ IItcoputio( colname, ind, isref, DB_CHA_TYPE, llen, ldata );
}

/*
** IIxtcogetio - Interface to IItcogetio
*/
void
IIxtcogetio(ind, isref, type, length, addr, colname)
i2	*ind;
i4	isref, type, length;
PTR	addr;
char	*colname;
{
    char	buf[DB_MAXSTRING +2];
    i4		llen;
    i4  	ltype;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	_VOID_ IItcogetio( ind, isref, type, length, addr, colname );
	return;
    }
    IIsd_convert( addr, &ltype, &llen, &ldata, buf );
    _VOID_ IItcogetio( ind, isref, ltype, llen, ldata, colname );
    IIsd_fill( addr, ind, ldata );
}

/*
** IIxtcolret - Pre-6.0 generated call.  Interface to C IItcogetio.
*/
void
IIxtcolret( isref, type, len, addr, colname )
i4	isref, type, len;
PTR	addr;
char	*colname;
{
    char	buf[DB_MAXSTRING +2];
    i4		llen;
    i4  	ltype;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	_VOID_ IItcogetio( (i2 *)0, isref, type, len, addr, colname );
	return;
    }
    IIsd_convert( addr, &ltype, &llen, &ldata, buf );
    _VOID_ IItcogetio( (i2 *)0, isref, ltype, llen, ldata, colname );
    IIsd_fill( addr, (i2 *)0, ldata );
}


/*
** IIxtrc_param - Pre-6.0 interface to C IItrc_param (but still in use in 6.0).
*/

i4
IIxtrc_param( tl, argv )
char	*tl;				/* Really string descriptor */
char	*argv[];
{
    void IIxouttrans();

    /* Call C param utility, but pass an argument translator */
    return IItrc_param( IIsd(tl), argv, IIxouttrans );
}


/*
** IIxtsc_param - Pre-60. interface to C IItsc_param (but still in use in 6.0).
*/

i4
IIxtsc_param( tl, argv )
char	*tl;				/* Really string descriptor */
char	*argv[];
{
    void IIxintrans();

    /* Call C param utility, but pass an argument translator */
    return IItsc_param( IIsd(tl), argv, IIxintrans );
}
# else
static	i4	ranlib_dummy;
# endif /* VMS */
