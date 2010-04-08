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
+*  Name: iivrunt.c - VMS interface to ABF/RUNTIME to handle character data.
**
**  Description:
**	These routines (for VMS non-C environments ONLY) are called from an 
**	embedded program when character data is being sent/received in 
**	EXEC 4GL statements.  Their purpose is to transform a VMS 
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
**	In general, in release 6, non-C VMS descriptor character data is sent 
**	as DB_CHA_TYPE because we require it to be blank trimmed on input and 
**	blank padded on output.  However, there are some exceptions.  See the 
**	table in iivmsstr.c for a complete mapping of VMS string descriptors to 
**	internally understood descriptions.
**
**  Defines: 
**	IIxG4bpByrefParam( ind, isref, type, length, data, name )
**	IIxG4gaGetAttr( ind, isref, type, length, data, name )
**	IIxG4ggGetGlobal( ind, isref, type, length, data, name, gtype )
**	IIxG4grGetRow( rowind, isref, rowtype, rowlen, rowptr, array, index )
**	IIxG4i4Inq4GL( ind, isref, type, length, data, object, code )
**	IIxG4rvRetVal( ind, isref, type, length, data )
**	IIxG4s4Set4GL( object, code, ind, isref, type, length, data )
**	IIxG4saSetAttr( name, ind, isref, type, length, data )
**	IIxG4sgSetGlobal( name, ind, isref, type, length, data )
**	IIxG4vpValParam( name, ind, isref, type, length, data )
**
**  Notes:
** 	These routines should not be ported to non-VMS environments.  
-*
**  History:
**	09-15-1993 (kathryn) 
**	    These routines are new to release 6.5.  Moved from iivrunt.c 
**	    so that 3GL forms applications may link without including the 
**	    ABF runtime library.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* External data */
FUNC_EXTERN	void	IIsd_lenptr(), IIsd_convert(), IIsd_fill();

/*
** IIxG4bpByrefParam - interface to C IIG4bpByrefParam.
*/
i4
IIxG4bpByrefParam( ind, isref, type, length, data, name )
i2	*ind;
i4	isref;
i4	type;
i4	length;
PTR	data;
char	*name;
{
    char	buf[DB_MAXSTRING + DB_CNTSIZE];
    i4		ltype, llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	IIG4bpByrefParam( ind, isref, type, length, data, name );
	return;
    }
    IIsd_convert( data, &ltype, &llen, &ldata, buf );
    IIG4bpByrefParam( ind, isref, ltype, llen, ldata, name );
    IIsd_fill( data, ind, ldata );
}

/*
** IIxG4gaGetAttr - interface to C IIG4gaGetAttr.
*/
i4
IIxG4gaGetAttr( ind, isref, type, length, data, name )
i2	*ind;
i4	isref;
i4	type;
i4	length;
PTR	data;
char	*name;
{
    char	buf[DB_MAXSTRING + DB_CNTSIZE];
    i4		ltype, llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	IIG4gaGetAttr( ind, isref, type, length, data, name );
	return;
    }
    IIsd_convert( data, &ltype, &llen, &ldata, buf );
    IIG4gaGetAttr( ind, isref, ltype, llen, ldata, name );
    IIsd_fill( data, ind, ldata );
}

/*
** IIxG4ggGetGlobal - interface to C IIG4ggGetGlobal.
*/
i4
IIxG4ggGetGlobal( ind, isref, type, length, data, name, gtype )
i2	*ind;
i4	isref;
i4	type;
i4	length;
PTR	data;
char	*name;
i4	gtype;
{
    char	buf[DB_MAXSTRING + DB_CNTSIZE];
    i4		ltype, llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	IIG4ggGetGlobal( ind, isref, type, length, data, name, gtype );
	return;
    }
    IIsd_convert( data, &ltype, &llen, &ldata, buf );
    IIG4ggGetGlobal( ind, isref, ltype, llen, ldata, name, gtype );
    IIsd_fill( data, ind, ldata );
}

/*
** IIxG4grGetRow - interface to C IIG4grGetRow.
*/
i4
IIxG4grGetRow( rowind, isref, rowtype, rowlen, rowptr, array, index )
i2	*rowind;
i4	isref;
i4	rowtype;
i4	rowlen;
PTR	rowptr;
i4	array;
i4	index;
{
    char	buf[DB_MAXSTRING + DB_CNTSIZE];
    i4		ltype, llen;
    PTR		ldata;

    if (rowtype != DB_CHR_TYPE)
    {
	IIG4grGetRow( rowind, isref, rowtype, rowlen, rowptr, array, index );
	return;
    }
    IIsd_convert( rowptr, &ltype, &llen, &ldata, buf );
    IIG4grGetRow( rowind, isref, ltype, llen, ldata, array, index );
    IIsd_fill( rowptr, rowind, ldata );
}

/*
** IIxG4i4Inq4GL - interface to C IIG4i4Inq4GL.
*/
i4
IIxG4i4Inq4GL( ind, isref, type, length, data, object, code )
i2	*ind;
i4	isref;
i4	type;
i4	length;
PTR	data;
i4	object;
i4	code;
{
    char	buf[DB_MAXSTRING + DB_CNTSIZE];
    i4		ltype, llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	IIG4i4Inq4GL( ind, isref, type, length, data, object, code );
	return;
    }
    IIsd_convert( data, &ltype, &llen, &ldata, buf );
    IIG4i4Inq4GL( ind, isref, ltype, llen, ldata, object, code );
    IIsd_fill( data, ind, ldata );
}

/*
** IIxG4rvRetVal - interface to C IIG4rvRetVal.
*/
i4
IIxG4rvRetVal( ind, isref, type, length, data )
i2	*ind;
i4	isref;
i4	type;
i4	length;
PTR	data;
{
    char	buf[DB_MAXSTRING + DB_CNTSIZE];
    i4		ltype, llen;
    PTR		ldata;

    if (type != DB_CHR_TYPE)
    {
	IIG4rvRetVal( ind, isref, type, length, data );
	return;
    }
    IIsd_convert( data, &ltype, &llen, &ldata, buf );
    IIG4rvRetVal( ind, isref, ltype, llen, ldata );
    IIsd_fill( data, ind, ldata );
}

/*
** IIxG4s4Set4GL - interface to C IIG4s4Set4GL.
*/
i4
IIxG4s4Set4GL( object, code, ind, isref, type, length, data )
i4	object;
i4	code;
i2	*ind;
i4	isref;
i4	type;
i4	length;
PTR	data;
{
    i4  llen;
    PTR	ldata;

    if (type != DB_CHR_TYPE)
    {
	IIG4s4Set4GL( object, code, ind, isref, type, length, data );
	return;
    }
    IIsd_lenptr( data, &llen, &ldata );
    IIG4s4Set4GL( object, code, ind, isref, DB_CHA_TYPE, llen, ldata );
}

/*
** IIxG4saSetAttr - interface to C IIG4saSetAttr.
*/
i4
IIxG4saSetAttr( name, ind, isref, type, length, data )
char	*name;
i2	*ind;
i4	isref;
i4	type;
i4	length;
PTR	data;
{
    i4  llen;
    PTR	ldata;

    if (type != DB_CHR_TYPE)
    {
	IIG4saSetAttr( name, ind, isref, type, length, data );
	return;
    }
    IIsd_lenptr( data, &llen, &ldata );
    IIG4saSetAttr( name, ind, isref, DB_CHA_TYPE, llen, ldata );
}

/*
** IIxG4sgSetGlobal - interface to C IIG4sgSetGlobal.
*/
i4
IIxG4sgSetGlobal( name, ind, isref, type, length, data )
char	*name;
i2	*ind;
i4	isref;
i4	type;
i4	length;
PTR	data;
{
    i4  llen;
    PTR	ldata;

    if (type != DB_CHR_TYPE)
    {
	IIG4sgSetGlobal( name, ind, isref, type, length, data );
	return;
    }
    IIsd_lenptr( data, &llen, &ldata );
    IIG4sgSetGlobal( name, ind, isref, DB_CHA_TYPE, llen, ldata );
}

/*
** IIxG4vpValParam - interface to C IIG4vpValParam.
*/
i4
IIxG4vpValParam( name, ind, isref, type, length, data )
char	*name;
i2	*ind;
i4	isref;
i4	type;
i4	length;
PTR	data;
{
    i4  llen;
    PTR	ldata;

    if (type != DB_CHR_TYPE)
    {
	IIG4vpValParam( name, ind, isref, type, length, data );
	return;
    }
    IIsd_lenptr( data, &llen, &ldata );
    IIG4vpValParam( name, ind, isref, DB_CHA_TYPE, llen, ldata );
}

# else
static	i4	ranlib_dummy;
# endif /* VMS */
