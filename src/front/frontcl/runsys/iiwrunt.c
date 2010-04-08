/* 
** Copyright (c) 2004 Ingres Corporation  
*/ 
# include <compat.h>
# include <cm.h>
# include <gl.h>
# include <sl.h>
# include <st.h>
# include <iicommon.h>
# include <fe.h>
# include <eqrun.h>

# ifdef	NT_GENERIC

/**
+*  Name: iiwrunt.c - Windows interface to RUNTIME to handle character names.
**
**  Description:
**	These routines (for Windows non-C environments ONLY) are called from an 
**	embedded program when character data is being sent/received in 
**	database statements.  Their purpose is to transform a Windows 
**	data structure (i.e., a descriptor) into a format that RUNTIME/ADH
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
**	In general, in 6.0, non-C Windows descriptor character data is sent as 
**	DB_CHA_TYPE because we require it to be blank trimmed on input and 
**	blank padded on output.  However, there are some exceptions.  See the 
**	table in iivmsstr.c for a complete mapping of Windows string descriptors to 
**	internally understood descriptions.
**
**	The pre-6.0 interface routines are also contained in this file.
**
**  Defines: 
**    New Interface for 6.0:
**	IIxfssetio	- Interface to IIfssetio
**	IIxputfldio	- Interface to IIputfldio
**	IIxstfsio	- Interface to IIstfsio
**	IIxprmptio	- Interface to IIprmptio
**	IIxfsinqio	- Interface to IIfsinqio
**	IIxgtqryio	- Interface to IIgtqryio
**	IIxgetfldio	- Interface to IIxgetfldio
**	IIxiqfsio	- Interface to IIiqfsio
**    New Interface for 6.1:
**	IIxFRgpsetio	- Interface to IIFRgpsetio
**    Pre 6.0 interface:
** 	IIxretfield( isvar, type, len, addr, field );
**	IIxfrsinq( isvar, type, len, addr, object );
**	IIxiqfrs( isvar, type, len, addr, frsflag, object, row );
**	IIxdoprompt( prompt, retbuf )
**	IIxneprompt( prompt, retbuf )
**	IIxrf_param( tlist, argv )
**	IIxsf_param( tlist, argv )
**
**  Notes:
** 	These routines should not be ported to non-Windows environments.  
-*
**  History:
**      22-Sep-2003 (fanra01)
**          Bug 110927
**          Created based on VMS version iivrunt.c
**/

/* External data */
FUNC_EXTERN	char	*IIsd(), *IIsd_no();
FUNC_EXTERN	void	IIsd_undo(), IIsd_lenptr(), IIsd_convert(), IIsd_fill();

/*
** IIxfssetio - Interface to IIfssetio
*/
void
IIxfssetio(name, ind, isref, type, length, data)
char	*name;
i2	*ind;
i4	isref, type, length;
PTR	data;
{
    i4	llen;

    if (type != DB_CHR_TYPE)
    {
        IIfssetio( name, ind, isref, type, length, data );
        return;
    }
    if (data != NULL)
    {
        llen = STlength( data );
    }
    IIfssetio( name, ind, isref, type, llen, data );
}

/*
** IIxputfldio - Interface to IIputfldio
*/
void
IIxputfldio(name, ind, isref, type, length, data)
char	*name;
i2	*ind;
i4	isref, type, length;
PTR	data;
{
    i4	llen;

    if (type != DB_CHR_TYPE)
    {
        IIputfldio( name, ind, isref, type, length, data );
        return;
    }
    if (data != NULL)
    {
        llen = STlength( data );
    }
    IIputfldio( name, ind, isref, type, llen, data );
}

/*
** IIxstfsio - Interface to IIstfsio
*/
void
IIxstfsio(attr, name, row, ind, isref, type, length, data)
i4	attr;
char	*name;
i4	row;
i2	*ind;
i4	isref, type, length;
PTR	data;
{
    i4	llen;

    if (type != DB_CHR_TYPE)
    {
        IIstfsio( attr, name, row, ind, isref, type, length, data );
        return;
    }
    if (data != NULL)
    {
        llen = STlength( data );
    }
    IIstfsio( attr, name, row, ind, isref, type, llen, data );
}

/*
** IIxFRsaSetAttrio - Interface to IIFRsaSetAttrio
*/
void
IIxFRsaSetAttrio(attr, name, ind, isref, type, length, data)
i4      attr;
char    *name;
i2      *ind;
i4      isref, type, length;
PTR     data;
{
    i4  llen;

    if (type != DB_CHR_TYPE)
    {
        IIFRsaSetAttrio( attr, name, ind, isref, type, length, data );
        return;
    }
    if (data != NULL)
    {
        llen = STlength( data );
    }
    IIFRsaSetAttrio( attr, name, ind, isref, type, llen, data );
}

/*
** IIxFRgpsetio - Interface to IIFRgpsetio
*/
void
IIxFRgpsetio(param, ind, isref, type, length, data)
i4	param;
i2	*ind;
i4	isref, type, length;
PTR	data;
{
    i4	llen;

    if (type != DB_CHR_TYPE)
    {
        IIFRgpsetio( param, ind, isref, type, length, data );
        return;
    }
    if (data != NULL)
    {
        llen = STlength( data );
    }
    IIFRgpsetio( param, ind, isref, type, llen, data );
}

/*
** IIxprmptio -- Interface to IIprmptio
**
** History:
**      22-Sep-2003 (fanra01)
**          Bug 110905
**          Bug 110927
**          Added to correctly return space terminated prompt strings.
*/
VOID
IIxprmptio(noecho, prstring, nullind, isvar, type, length, data)
i4	noecho;
char	*prstring;
i2	*nullind;
i4	isvar;
i4	type;
i4	length;
PTR	data;
{
    char	buf[DB_MAXSTRING +2];
    i4		ltype, llen;
    PTR		ldata;
    ldata = buf;

    IIprmptio(noecho, prstring, nullind, isvar, type, length, ldata);
    llen = STlength( ldata );
    if (data != NULL)
    {
        MEfill( length, ' ', data );
    }
    if (llen != 0)
    {
        CMcopy( ldata, llen, data );
    }
}

/*
** IIxfsinqio - interface to IIfsinqio
*/
void
IIxfsinqio(ind, isref, type, length, addr, name)
i2	*ind;
i4	isref, type, length;
PTR	addr;
char	*name;
{
    char	buf[DB_MAXSTRING +2];
    i4		llen;
    i4  	ltype;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
        IIfsinqio( ind, isref, type, length, addr, name );
        return;
    }
    ldata = buf;
    IIfsinqio( ind, isref, type, length, ldata, name );
    llen = STlength( ldata );
    if (addr != NULL)
    {
        MEfill( length, ' ', addr );
    }
    if (llen != 0)
    {
        CMcopy( ldata, llen, addr );
    }
}

/* 
** IIxgtqryio - Interface to IIgtqryio
*/
IIxgtqryio(ind, isref, type, length, addr, name)
i2	*ind;
i4	isref, type, length;
PTR	addr;
char	*name;
{
    char	buf[DB_MAXSTRING +2];
    i4		llen;
    i4  	ltype;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
        IIgtqryio( ind, isref, type, length, addr, name );
        return;
    }
    ldata = buf;
    IIgtqryio( ind, isref, type, length, ldata, name );
    llen = STlength( ldata );
    if (addr != NULL)
    {
        MEfill( length, ' ', addr );
    }
    if (llen != 0)
    {
        CMcopy( ldata, llen, addr );
    }
}

/*
** IIxgetfldio - Interface to IIgetfldio
*/
void
IIxgetfldio(ind, isref, type, length, addr, name)
i2	*ind;
i4	isref, type, length;
PTR	addr;
char	*name;
{
    char	buf[DB_MAXSTRING +2];
    i4		llen;
    i4  	ltype;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
        IIgetfldio( ind, isref, type, length, addr, name );
        return;
    }
    ldata = buf;
    IIgetfldio( ind, isref, type, length, ldata, name );
    llen = STlength( ldata );
    if (addr != NULL)
    {
        MEfill( length, ' ', addr );
    }
    if (llen != 0)
    {
        CMcopy( ldata, llen, addr );
    }
}

/*
** IIxiqfsio - Interface to IIiqfsio
*/
void
IIxiqfsio(ind, isref, type, length, addr, attr, name, row)
i2	*ind;
i4	isref, type, length;
PTR	addr;
i4	attr;
char	*name;
i4	row;
{
    char	buf[DB_MAXSTRING +2];
    i4		llen;
    i4  	ltype;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	IIiqfsio( ind, isref, type, length, addr, attr, name, row );
	return;
    }
    ldata = buf;
    IIiqfsio( ind, isref, type, length, ldata, attr, name, row );
    llen = STlength( ldata );
    if (addr != NULL)
    {
        MEfill( length, ' ', addr );
    }
    if (llen != 0)
    {
        CMcopy( ldata, llen, addr );
    }
}


/*
** IIxretfield - Pre-6.0 interface to C IIgetfldio.
*/
void
IIxretfield( isref, type, length, addr, obj )
i4	isref, type, length;
PTR	addr;
char	*obj;
{
    char	buf[DB_MAXSTRING +2];
    i4		llen;
    i4  	ltype;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
        _VOID_ IIgetfldio( (i2 *)0, isref, type, length, addr, obj );
        return;
    }
    ldata = buf;
    _VOID_ IIgetfldio( (i2 *)0, isref, type, length, ldata, obj );
    llen = STlength( ldata );
    if (addr != NULL)
    {
        MEfill( length, ' ', addr );
    }
    if (llen != 0)
    {
        CMcopy( ldata, llen, addr );
    }
}

/*
** IIxfrsinq - Pre-6.0 interface to C IIfsinqio.
*/

void
IIxfrsinq( isref, type, length, addr, obj )
i4	isref, type, length;
PTR	addr;
char	*obj;
{
    char	buf[DB_MAXSTRING +2];
    i4		llen;
    i4  	ltype;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
        _VOID_ IIfsinqio( (i2 *)0, isref, type, length, addr, obj );
        return;
    }
    ldata = buf;
    _VOID_ IIfsinqio( (i2 *)0, isref, ltype, length, ldata, obj );
    llen = STlength( ldata );
    if (addr != NULL)
    {
        MEfill( length, ' ', addr );
    }
    if (llen != 0)
    {
        CMcopy( ldata, llen, addr );
    }
}

/*
** IIxiqfrs - Pre-6.0 generated call, interface to IIiqfsio.
*/

void
IIxiqfrs( isref, type, len, addr, frsflg, obj, row )
i4	isref, type, len;
PTR	addr;
i4	frsflg;
char	*obj;
i4	row;
{
    char	buf[DB_MAXSTRING +2];
    i4		llen;
    i4  	ltype;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
        _VOID_ IIiqfsio( (i2 *)0, isref, type, len, addr, frsflg, obj, row );
        return;
    }
    ldata = buf;
    _VOID_ IIiqfsio( (i2 *)0, isref, ltype, len, ldata, frsflg, obj, row );
    llen = STlength( ldata );
    if (addr != NULL)
    {
        MEfill( len, ' ', addr );
    }
    if (llen != 0)
    {
        CMcopy( ldata, llen, addr );
    }
}

/*
** IIxdoprompt - Pre-6.0 generated call, interface to C IIprmptio.
*/

void
IIxdoprompt( prompt, retbuf )
char	*prompt;
char	*retbuf;
{
    i4	llen;
    PTR	ldata;

    if (retbuf != NULL)
    {
        llen = STlength( retbuf );
    }
    IIprmptio( FALSE, prompt, (i2 *)0, 1, DB_CHA_TYPE, llen, ldata );
}

/*
** IIxneprompt - Interface to C IIneprompt.
*/

void
IIxneprompt( prompt, retbuf )
char	*prompt;
char	*retbuf;
{
    i4	llen;
    PTR	ldata;

    if (retbuf != NULL)
    {
        llen = STlength( retbuf );
    }
    IIprmptio( TRUE, prompt, (i2 *)0, 1, DB_CHA_TYPE, llen, retbuf );
}

/*
** IIxrf_param - Pre 6.0 interface to C IIrf_param (but still used in 6.0).
*/

i4
IIxrf_param( tl, argv )
char	*tl;				/* Really string descriptor */
char	*argv[];
{
    void IIxouttrans();

    /* Call C param utility, but pass an argument translator */
    return IIrf_param( IIsd(tl), argv, IIxouttrans );
}


/*
** IIxsf_param - Pre-6.0 interface to C IIsf_param (but still used in 6.0).
*/

i4
IIxsf_param( tl, argv )
char	*tl;				/* Really string descriptor */
char	*argv[];
{
    void IIxintrans();

    /* Call C param utility, but pass an argument translator */
    return IIsf_param( IIsd(tl), argv, IIxintrans );
}
# else
static	i4	ranlib_dummy;
# endif /* NT_GENERIC */
