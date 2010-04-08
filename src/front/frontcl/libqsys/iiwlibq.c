/* 
** Copyright (c) 2004 Ingres Corporation  
*/ 
# include <compat.h>
# include <cm.h>
# include <gl.h>
# include <sl.h>
# include <st.h>
# include <iicommon.h>
# include <me.h>
# include <generr.h>
# include "iivmssd.h"
# include "erls.h"

# ifdef	NT_GENERIC

/**
+*  Name: iiwlibq.c - Windows interface to LIBQ to handle character data.
**
**  Description:
**	These routines (for Windows non-C environments ONLY) are called from an
**	embedded program when character data is being sent/received in 
**	database statements.  Their purpose is to transform a Windows
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
**	In general, in 6.0, non-C Windows descriptor character data is sent as 
**	DB_CHA_TYPE because we require it to be blank trimmed on input and 
**	blank padded on output.  However, there are some exceptions.  See the 
**	table in iivmsstr.c for a complete mapping of Windows string descriptors to 
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
**      22-Sep-2003 (fanra01)
**          Bug 110906
**          Bug 110925
**          Bug 110926
**          Bug 110960
**          Created based on the VMS version iivlibq.c
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
    IIputdomio( ind, isref, type, len, data );
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
    IIputdomio( ind, isref, type, len, data );
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
    IIwritio( trim, ind, isref, type, length, data );
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
    IIvarstrio( ind, isref, type, length, data );
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
    if (data != NULL)
    {
        llen = STlength( data );
    }
    IIeqstio( obj, ind, isref, type, llen, data );
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

    if (data != NULL)
    {
        llen = STlength( data );
    }
    IILQprvProcValio(pname, pbyref, ind, isref, type, llen, data);
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
    i4		llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
        IIeqiqio( ind, isref, type, length, addr, obj );
        return;
    }
    ldata = buf;
    IIeqiqio( ind, isref, type, length, ldata, obj );
    llen = STlength( ldata );
    if (addr != NULL)
    {
        MEfill( length, ' ', addr );
    }
    if ((ind == NULL) || (*ind != DB_EMB_NULL))
    {
        if (llen != 0)
        {
            CMcopy( ldata, llen, addr );
        }
    }
}

/*
** IIxparret
** 	- Called when the target list for RET-type statements is parameterized.
**	- IIxouttrans (a special F77 version) will be called from IIparret to
**	  put data into host vars pointed at by argv vector.
**
**	  History:
**      22-Sep-2003 (fanra01)
**          Bug 110960
**          Added to handle returned string parameters.
*/
IIxparret( format, argv )
char	*format;		/* User's target list		*/
char	*argv[];		/* Pointers to user's variables	*/
{
    void	IIxouttrans();

    IIparret( format, argv, IIxouttrans );
}

/*
** IIxparset
**	- Called when the target list for SET-type statements is parameterized.
**	- IIxintrans (a special F77 version) will be called from IIparset to
**	  get data from host vars pointed at by argv vector.
**
**	  History:
**      22-Sep-2003 (fanra01)
**          Bug 110960
**          Added to handle string parameters.
*/
void
IIxparset( format, argv )
char	*format;
char	*argv[];
{
    char	*IIxintrans();

    IIparset( format, argv, IIxintrans );
}

/*
** IIxgetdomio -- Interface to IIgetdomio.
**
** History:
**      22-Sep-2003 (fanra01)
**          Bug 110926
**          Added to return field in a space padded buffer.
*/
void
IIxgetdomio( ind, isref, type, length, addr )
i2	*ind;
i4	isref, type, length;
PTR	addr;
{
    char	buf[DB_MAXSTRING +2];
    i4		llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
        IIgetdomio( ind, isref, type, length, addr );
        return;
    }
    ldata = buf;
    IIgetdomio( ind, isref, type, length, ldata );
    llen = STlength( ldata );
    if (addr != NULL)
    {
        MEfill( length, ' ', addr );
    }
    if ((ind == NULL) || (*ind != DB_EMB_NULL))
    {
        if (llen != 0)
        {
            CMcopy( ldata, llen, addr );
        }
    }
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
    i4		llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	IIcsGetio( ind, isref, type, length, addr );
	return;
    }
    ldata = buf;
    IIcsGetio( ind, isref, type, length, ldata );
    llen = STlength( ldata );
    if (addr != NULL)
    {
        MEfill( length, ' ', addr );
    }
    if ((ind == NULL) || (*ind != DB_EMB_NULL))
    {
        if (llen != 0)
        {
            CMcopy( ldata, llen, addr );
        }
    }
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

    if (type != DB_CHR_TYPE)
    {
        IILQssSetSqlio(attr, ind, isref, type, length, data);
        return;
    }
    if (data != NULL)
    {
        llen = STlength( data );
    }
    IILQssSetSqlio( attr, ind, isref, type, llen, data );
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
    IILQlpd_LoPutData(isvar, type, len, data, seglen, dataend);
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
    i4		llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
        IILQisInqSqlio( ind, isref, type, len, addr, attr );
        return;
    }
    ldata = buf;
    IILQisInqSqlio( ind, isref, type, len, ldata, attr );
    llen = STlength( ldata );
    if (addr != NULL)
    {
        MEfill( len, ' ', addr );
    }
    if ((ind == NULL) || (*ind != DB_EMB_NULL))
    {
        if (llen != 0)
        {
            CMcopy( ldata, llen, addr );
        }
    }
}

/*
** IIxLQlgd_LoGetData -- interface to IILQlgd_LoGetData
**
** History:
**      22-Sep-2003 (fanra01)
**          Bug 110925
**          Added to handle correct buffer termination.
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
    PTR end;
    
    if (type != DB_CHR_TYPE)
    {
        IILQlgd_LoGetData(isvar, type, len, data, maxlen, seglen, dataend);
        return;
    }
    else 
    {
        IILQlgd_LoGetData(isvar, type, len, data, maxlen, seglen, dataend);
        end = data + *seglen;
        MEfill( (maxlen - *seglen), ' ', end );
    }
    return;
}

/*
** IIxsqGdInit	- Interface to IIsqGdInit (SQLSTATE support)
*/
VOID
IIxsqGdInit(type, data)
i4	type;
PTR     data;
{
    IIsqGdInit( type, data );
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

    if (type != DB_CHR_TYPE)			/* Shouldn't happen */
    {
        IIputdomio( (i2 *)0, isref, type, len, data );
        return;
    }
    if (data != NULL)
    {
        llen = STlength( data );
    }
    IIputdomio( (char *)0, isref, type, llen, data );
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
    i4		llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
        IIeqiqio( (i2 *)0, isref, type, len, addr, obj );
        return;
    }
    ldata = buf;
    IIeqiqio( (i2 *)0, isref, type, len, ldata, obj );
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
** IIxretdom - Pre-6.0 generated call, interface to C IIgetdomio.
*/
void
IIxretdom( isref, type, len, addr )
i4	isref, type, len;
PTR	addr;
{
    char	buf[DB_MAXSTRING + DB_CNTSIZE];
    i4		llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
        IIgetdomio( (i2 *)0, isref, type, len, addr );
        return;
    }
    ldata = buf;
    IIgetdomio( (i2 *)0, isref, type, len, ldata );
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
** IIxouttrans - Reformat data on the way out of a Param 'get data'.
*/
void
IIxouttrans( type, len, data, addr )
i4	type, len;		/* Type and len sent from the param */
char	*data;
char	*addr;			/* Host address */
{
    if (type != DB_CHR_TYPE)
	MEcopy( data, len, addr ); 
    else
    {
	SDESC	*sd;
	sd = (SDESC *)addr;
	STmove( data, ' ', sd->sd_len, sd->sd_ptr );  /* move str to host var */
    }
}

/*
** IIxintrans - Format data before sending to a Param 'put data' call.
*/
char	*
IIxintrans( type, len, arg )
i4	type, len;		/* Type and len sent from the param format */
char	*arg;
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
** IIsqGdInit
**      - SQLSTATE handling layer between ESQL program and libq routines
**
**  History:
**      22-Sep-2003 (fanra01)
**          Bug 110906
**          Add IIsqGd call.
*/
void
IIsqGd( op_type, sqlstate )
i4      *op_type;
char	*sqlstate;
{
    IIsqGdInit( *op_type, IIsadr(sqlstate) );
}

# else
static	i4	ranlib_dummy;
# endif /* NT_GENERIC */
