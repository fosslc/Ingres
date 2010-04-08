/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <me.h>
# include       <generr.h>
# include	"iivmssd.h"
# include       "erls.h"

# ifdef	VMS

/**
+*  Name: iivlibq.c - VMS interface to LIBQ to handle character data.
**
**  Description:
**	These routines (for VMS non-C environments ONLY) are called from an 
**	embedded program when character data is being sent/received in 
**	database statements.  Their purpose is to transform a VMS 
**	data structure (i.e., a descriptor) into a format that LIBQ/ADH
**	knows about.  Data copying is avoided if at all possible for
**	performance reasons.
**
**	In 6.0, LIBQ/ADH has a convention regarding character types from 
**	embedded programs as follows:
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
**    6.0 interface routines (input):
**	IIxputdomio	- Interface to IIputdomio
**	IIxnotrimio	- Interface to IIputdomio when no_trim
**	IIxwritio	- Interface to IIwritio
**	IIxvarstrio	- Interface to IIvarstrio
**	IIxeqstio	- Interface to IIeqstio
**   6.1 interface routines (input):
**	IIxLQprvProcValio - Interface to IILQprvProcValio
**   6.0 interface routines (output):
**	IIxeqiqio	- Interface to IIeqiqio
**	IIxgetdomio	- Interface to IIgetdomio
**	IIxcsGetio	- Interface to IIcsGetio
**   6.4 interface routine (input):
**	IIxLQssSetSqlio - Interface to IILQssSetSqlio
**   6.5 interface routine (input):
**	IIxLQlpd_LoPutData - Interface to IILQlpd_LoPutData
**   6.4 interface routine (output):
**      IIxLQisInqSqlio - Interface to IILQisInqSqlio
**   6.5 interface routine (output):
**	IIxLQlgd_LoGetData - Interface to IILQlgd_LoGetData
**	IIxsqGdInit	- Interface to IIsqGdInit
**   Pre-6.0 interface routines (output)
**	IIxeqinq	- Interface to IIeqiqio
**	IIxretdom	- Interface to IIgetdomio
**   Pre-6.0 interface routines (input)
**	IInotrim	- Interface to IIputdomio
**
**  Notes:
-*
**  History:
**	12-may-1987	- written (bjb)
**	01-aug-1989	- Shut ranlib up (GordonW)
**	20-dec-1989 (barbara)
**	    Updated for Phoenix/Alerters
**	16-may-1990 (teresal)
**	    Fixed typo for IIxLQegEvGetio call
**	26-feb-1991 (kathryn)
**	    Added IIxLQssSetSqlio and IIxLQisInqSqlio.
**	25-apr-1991 (teresal)
**	    Removed IILQegEvGetio.
**	01-dec-1992 (kathryn)
**	    Added IIxLQlpd_LoPutData and IIxLQlgd_LoGetData
**	17-dec-1992 (larrym)
**	    Added IIxsqGdInit (SQLSTATE support)
**	26-jul-1993 (lan)
**	    Added IIxG4... for EXEC 4GL.
**	29-jul-1993 (lan)
**	    Removed IIxG4 (moved them to iivrunt.c).
**	31-jan-1994 (teresal)
**	    Modify IIxsqGdInit() to look for a CHA type rather than
**	    a CHR type - SQLSTATE fix for bug 59015.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      13-feb-03 (chash01) x-integrate change#461908
**          compiler complains about uninitialized variable bufptr,
**          set it to (char *) 0.  Thios is in _LoGetData. Note for
**          possible memory leak in this routine
**/

/* External data */
FUNC_EXTERN	char	*IIsd(), *IIsd_no();
FUNC_EXTERN	void	IIsd_undo(), IIsd_lenptr(), IIsd_convert(), IIsd_fill();


/*
** The first group of routines are an interface between the 6.0 generated
** INPUT calls and the runtime code.
*/
/*
** IIxputdomio - Interface to IIputdomio
*/
void
IIxputdomio(ind, isref, type, len, data)
i2	*ind;
i4	isref;
i4	type;
i4	len;
PTR	data;
{
    i4		llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)			/* Shouldn't happen */
    {
	IIputdomio(ind, isref, type, len, data);
	return;
    }
    IIsd_lenptr( data, &llen, &ldata );
    IIputdomio( ind, isref, DB_CHA_TYPE, llen, ldata );
}

/*
** IIxnotrimio - Interface to IIputdomio where blanks are not trimmed
*/
void
IIxnotrimio(ind, isref, type, len, data)
i2	*ind;
i4	isref;
i4	type;
i4	len;
PTR	data;
{
    i4		llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)			/* Shouldn't happen */
    {
	IIputdomio(ind, isref, type, len, data);
	return;
    }
    IIsd_lenptr( data, &llen, &ldata );
    IIputdomio( ind, isref, DB_CHR_TYPE, llen, ldata );
}

/*
** IIxwritio - Interface to IIwritio
**	       Note that this routine will never be called with anything
**	       other than character-type data.
*/
void
IIxwritio(trim, ind, isref, type, length, data)
i4	trim; 
i2	*ind; 
i4	isref, type, length;
PTR	data;
{
    i4	llen;
    PTR	ldata;

    IIsd_lenptr( data, &llen, &ldata );
    IIwritio( trim, ind, isref, DB_CHA_TYPE, llen, ldata );
}

/*
** IIxvarstrio -- Interface to IIvarstrio
*/
void
IIxvarstrio(ind, isref, type, length, data)
i2	*ind;
i4	isref, type, length;
PTR	data;
{
    i4	llen;
    PTR	ldata;

    if (type != DB_CHR_TYPE)		/* Shouldn't happen */
    {
	IIvarstrio( ind, isref, type, length, data );
	return;
    }

    IIsd_lenptr( data, &llen,  &ldata );
    IIvarstrio( ind, isref, DB_CHA_TYPE, llen, ldata );
}

/*
** IIxeqstio -- interface to IIeqstio
*/
void
IIxeqstio(obj, ind, isref, type, length, data)
char	*obj;
i2	*ind;
i4	isref, type, length;
PTR	data;
{
    i4  llen;
    PTR	ldata;

    if (type != DB_CHR_TYPE)
    {
	IIeqstio(obj, ind, isref, type, length, data);
	return;
    }

    /* 
    ** Actually, we don't yet support setting character objects yet, so
    ** this piece of code won't be used.
    */
    IIsd_lenptr( data, &llen, &ldata );
    IIeqstio( obj, ind, isref, DB_CHA_TYPE, llen, ldata );
}

/*
** IIxLQprvProcValio -- interface to IILQprvProcValio
*/
VOID
IIxLQprvProcValio(pname, pbyref, ind, isref, type, length, data)
char		*pname;
i4		pbyref;
i2		*ind;
i4		isref, type, length;
PTR		data;
{
    i4  llen;
    PTR	ldata;

    if (type != DB_CHR_TYPE)
    {
	IILQprvProcValio(pname, pbyref, ind, isref, type, length, data);
	return;
    }

    IIsd_lenptr(data, &llen,  &ldata);
    IILQprvProcValio(pname, pbyref, ind, isref, DB_CHA_TYPE, llen, ldata);
}

/*
** The following routines are an interface between the 6.0-generated OUTPUT 
** calls and the runtime code.
*/
/*
** IIxeqiqio -- Interface to C IIeqiqio.
*/
void
IIxeqiqio(ind, isref, type, length, addr, obj)
i2	*ind;
i4	isref, type, length;
PTR	addr;
char	*obj;
{
    char	buf[DB_MAXSTRING +2];
    i4		ltype, llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	IIeqiqio( ind, isref, type, length, addr, obj );
	return;
    }
    IIsd_convert( addr, &ltype, &llen, &ldata, buf );
    IIeqiqio( ind, isref, ltype, llen, ldata, obj );
    IIsd_fill( addr, ind, ldata );
}

/*
** IIxgetdomio -- Interface to IIgetdomio.
*/
IIxgetdomio( ind, isref, type, length, addr )
i2	*ind;
i4	isref, type, length;
PTR	addr;
{
    char	buf[DB_MAXSTRING +2];
    i4		ltype, llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	_VOID_ IIgetdomio( ind, isref, type, length, addr );
	return;
    }
    IIsd_convert( addr, &ltype, &llen, &ldata, buf );
    _VOID_ IIgetdomio( ind, isref, ltype, llen, ldata );
    IIsd_fill( addr, ind, ldata );
}

/*
** IIxcsGetio -- Interface to IIcsGetio
*/
void
IIxcsGetio( ind, isref, type, length, addr )
i2	*ind;
i4	isref, type, length;
PTR	addr;
{
    char	buf[DB_MAXSTRING +2];
    i4		ltype, llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	IIcsGetio( ind, isref, type, length, addr );
	return;
    }
    IIsd_convert( addr, &ltype, &llen, &ldata, buf );
    IIcsGetio( ind, isref, ltype, llen, ldata );
    IIsd_fill( addr, ind, ldata );
}

/*
** IIxLQssSetSqlio -- interface to IILQssSetSqlio
*/
void
IIxLQssSetSqlio(attr, ind, isref, type, length, data)
i4	attr;
i2	*ind;
i4	isref, type, length;
PTR	data;
{
    i4  llen;
    PTR	ldata;

    if (type != DB_CHR_TYPE)
    {
	IILQssSetSqlio(attr, ind, isref, type, length, data);
	return;
    }
    IIsd_lenptr( data, &llen, &ldata );
    IILQssSetSqlio( attr, ind, isref, DB_CHA_TYPE, llen, ldata );
}

/*
** IIxLQlpd_LoPutData -- interface to IILQlpd_LoPutData
*/
VOID
IIxLQlpd_LoPutData(isvar, type, len, data, seglen, dataend)
i4      isvar,type;
i4 len;
PTR     data;
i4 seglen;
i4	dataend;
{
    i4  llen;
    PTR ldata;

    if (type != DB_CHR_TYPE)
    {
        IILQlpd_LoPutData(isvar, type, len, data, seglen, dataend);
        return;
    }
    IIsd_lenptr( data, &llen, &ldata );
    IILQlpd_LoPutData( isvar, DB_CHA_TYPE, llen, ldata, seglen, dataend);
}


/*
** IIxLQInqSqlio -- Interface to C IILQisInqSqlio.
*/
void
IIxLQisInqSqlio( ind, isref, type, len, addr, attr )
i2	*ind;
i4	isref, type, len;
PTR	addr;
i4	attr;
{
    char	buf[DB_MAXSTRING + DB_CNTSIZE];
    i4		ltype, llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	IILQisInqSqlio( ind, isref, type, len, addr, attr );
	return;
    }
    IIsd_convert( addr, &ltype, &llen, &ldata, buf );
    IILQisInqSqlio( ind, isref, ltype, llen, ldata, attr );
    IIsd_fill( addr, ind, ldata );
}

/*
** IIxLQlgd_LoGetData -- interface to IILQlgd_LoGetData
**
*/
VOID
IIxLQlgd_LoGetData(isvar, type, len, data, maxlen, seglen, dataend)
i4      isvar,type;
i4 len;
PTR     data;
i4 maxlen;
i4 *seglen;
i4  	*dataend;
{
    static i4	bufsz;    
    char        buf[DB_MAXSTRING + DB_CNTSIZE];
    char        *bufptr = (char *)0;
    i4          ltype, llen;
    PTR 	ldata;

    if (type != DB_CHR_TYPE)
    {
        IILQlgd_LoGetData(isvar, type, len, data, maxlen, seglen, dataend);
        return;
    }
    else 
    {
	/* Called from program loop so only allocate when absolutely necessary.
	** Users may specify maximum length of segment to retrieve which may be
	** of any size. If maxlen is not specified then for dynamic string
	** descriptors DB_MAXSTRING will be assumed. Only dynamic strings
	** require a copy buffer.
	*/
	if ((maxlen >  DB_MAXSTRING + DB_CNTSIZE) && 
		((SDESC *)data)->sd_class == DSC_D)
	{
	    if (bufptr == (char *)0 || bufsz < maxlen)
	    {
		if (bufptr != (char *)0)
		    MEfree(bufptr);

                /* FIXME: POSSIBLE MEMORY LEAK HERE */
                /* Where is MEfree(bufptr)?         */
	        if ((bufptr = MEreqmem((u_i4)0,maxlen, TRUE, (STATUS*)0)) 
			== (char *)0)
			IIlocerr(GE_NO_RESOURCE, E_LS0101_SDALLOC, 0, 0 );
		bufsz = maxlen;
	    }
	       IIsd_convert( data, &ltype, &llen, &ldata, bufptr);
	}
	else
            IIsd_convert( data, &ltype, &llen, &ldata, buf);
        IILQlgd_LoGetData(isvar, ltype, llen, ldata, maxlen, seglen, dataend);
        IIsd_fill( data, 0, ldata );
    }
}


/*
** IIxsqGdInit	- Interface to IIsqGdInit (SQLSTATE support)
*/
VOID
IIxsqGdInit(type, data)
i4	type;
PTR     data;
{
    i4          ltype, dummylen;
    PTR 	ldata;

    if (type != DB_CHA_TYPE)	/* Shouldn't happen */
    {
        IIsqGdInit( type, data );
        return;
    }
    IIsd_lenptr( data, &dummylen, &ldata );
    IIsqGdInit( DB_CHA_TYPE, ldata);

}

/*
** The following three routines map pre-6.0 generated calls to the 6.0 runtime 
** code.  (A null pointer is passed to LIBQ for the null indicator.)
*/
/*
** IInotrim - Interface to IIsetdomio where blank padding is not to be trimmed.
*/
void
IInotrim( isref, type, len, data )
i4	isref, type, len;
PTR	data;
{
    i4		llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)			/* Shouldn't happen */
    {
	IIputdomio( (i2 *)0, isref, type, len, data );
	return;
    }
    IIsd_lenptr( data, &llen, &ldata );
    IIputdomio( (char *)0, isref, DB_CHR_TYPE, llen, ldata );
}

/*
** IIxeqinq -- Pre-6.0 generated call, interface to C IIeqiqio.
*/
void
IIxeqinq( isref, type, len, addr, obj )
i4	isref, type, len;
PTR	addr;
char	*obj;
{
    char	buf[DB_MAXSTRING + DB_CNTSIZE];
    i4		ltype, llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	IIeqiqio( (i2 *)0, isref, type, len, addr, obj );
	return;
    }
    IIsd_convert( addr, &ltype, &llen, &ldata, buf );
    IIeqiqio( (i2 *)0, isref, ltype, llen, ldata, obj );
    IIsd_fill( addr, (i2 *)0, ldata );
}

/*
** IIxretdom - Pre-6.0 generated call, interface to C IIgetdomio.
*/
void
IIxretdom( isref, type, len, addr )
i4	isref, type, len;
PTR	addr;
{
    char	buf[DB_MAXSTRING + DB_CNTSIZE];
    i4		ltype, llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	IIgetdomio( (i2 *)0, isref, type, len, addr );
	return;
    }
    IIsd_convert( addr, &ltype, &llen, &ldata, buf );
    IIgetdomio( (i2 *)0, isref, ltype, llen, ldata );
    IIsd_fill( addr, (i2 *)0, ldata );
}

# else
static	i4	ranlib_dummy;
# endif /* VMS */
