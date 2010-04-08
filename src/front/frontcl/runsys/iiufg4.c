# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<eqlang.h>
# include	<eqrun.h>
# if defined(UNIX) || defined(hp9_mpe)
# include	<iiufsys.h>
# include	<iisqlda.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	iiufg4.c
** Purpose:	Runtime (forms) translation layer for UNIX EQUEL/F77 programs.
**
** Defines:
**	iig4ac_()		IIG4acArrayClear
**	iig4rr_()		IIG4rrRemRow
**	iig4ir_()		IIG4irInsRow
**	iig4dr_()		IIG4drDelRow
**	iig4sr_()		IIG4srSetRow
**	iig4gr_()		IIG4grGetRow
**	iig4gg_()		IIG4ggGetGlobal
**	iig4sg_()		IIG4sgSetGlobal
**	iig4ga_()		IIG4gaGetAttr
**	iig4sa_()		IIG4saSetAttr
**	iig4ch_()		IIG4chkobj
**	iig4ud_()		IIG4udUseDscr
**	iig4fd_()		IIG4fdFillDscr
**	iig4ic_()		IIG4icInitCall
**	iig4vp_()		IIG4vpValParam
**	iig4bp_()		IIG4bpByrefParam
**	iig4rv_()		IIG4rvRetVal
**	iig4cc_()		IIG4ccCallComp
**	iig4i4_()		IIG4i4Inq4GL
**	iig4s4_()		IIG4s4Set4GL
**	iig4se_()		IIG4seSendEvent
**
-*
** Notes:
**	This file is one of five files making up the runtime interface for
**	F77.   The files are:
**		iiuflibq.c	- libqsys directory
**		iiufutils.c	- libqsys directory
**		iiufrunt.c	- runsys directory
**		iiuftb.c	- runsys directory
**		iiufg4.c	- runsys directory
**	See complete notes on the runtime layer in iiuflibq.c.
**
**
** History:
**	18-aug-1993 (lan)
**	    Created for EXEC 4GL.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# if defined(NO_UNDSCR) && !defined(m88_us5)
# define	iig4ac_ iig4ac
# define	iig4rr_ iig4rr
# define	iig4ir_ iig4ir
# define	iig4dr_ iig4dr
# define	iig4sr_ iig4sr
# define	iig4gr_ iig4gr
# define	iig4gg_ iig4gg
# define	iig4sg_ iig4sg
# define	iig4ga_ iig4ga
# define	iig4sa_ iig4sa
# define	iig4ch_ iig4ch
# define	iig4ud_ iig4ud
# define	iig4fd_ iig4fd
# define	iig4ic_ iig4ic
# define	iig4vp_ iig4vp
# define	iig4bp_ iig4bp
# define	iig4rv_ iig4rv
# define	iig4cc_ iig4cc
# define	iig4i4_ iig4i4
# define	iig4s4_ iig4s4
# define	iig4se_ iig4se
# endif


/*
** IIG4acArrayClear
**	EXEC 4GL CLEAR ARRAY
*/
i4
iig4ac_( array )
i4	*array;
{
    return IIG4acArrayClear( *array );
}

/*
** IIG4rrRemRow
**	EXEC 4GL REMOVEROW
*/
i4
iig4rr_( array, index )
i4	*array;
i4	*index;
{
    return IIG4rrRemRow( *array, *index );
}

/*
** IIG4irInsRow
**	EXEC 4GL INSERTROW
*/
i4
iig4ir_( array, index, row, state, which )
i4	*array;
i4	*index;
i4	*row;
i4	*state;
i4	*which;
{
    return IIG4irInsRow( *array, *index, *row, *state, *which );
}

/*
** IIG4drDelRow
**	EXEC 4GL SETROW DELETED
*/
i4
iig4dr_( array, index )
i4	*array;
i4	*index;
{
    return IIG4drDelRow( *array, *index );
}

/*
** IIG4srSetRow
**	EXEC 4GL SETROW
*/
i4
iig4sr_( array, index, row )
i4	*array;
i4	*index;
i4	*row;
{
    return IIG4srSetRow( *array, *index, *row );
}

/*
** IIG4grGetRow
**	EXEC 4GL GETROW
*/
i4
iig4gr_( rowind, isvar, rowtype, rowlen, rowptr, array, index )
i2	*rowind;
i4	*isvar;
i4	*rowtype;
i4	*rowlen;
i4	*rowptr;
i4	*array;
i4	*index;
{
    return IIG4grGetRow( rowind, 1, *rowtype, *rowlen, *rowptr, *array, *index);
}

/*
** IIG4ggGetGlobal
*/
i4
iig4gg_( ind, isvar, type, length, data, name, gtype )
i2	*ind;
i4	*isvar;
i4	*type;
i4	*length;
i4	*data;
i4	*name;
i4	*gtype;
{
    if (*type == DB_CHR_TYPE)
	return IIG4ggGetGlobal( ind, 1, DB_CHA_TYPE,
	       *length, *(char **)data, *(char **)name, *gtype );
    else if (*type == DB_DEC_CHR_TYPE)
	return IIG4ggGetGlobal( ind, 1, *type,
	       *length, *(char **)data, *(char **)name, *gtype );
    else
	return IIG4ggGetGlobal( ind, 1, *type, *length, data, name, *gtype );
}

/*
** IIG4sgSetGlobal
*/
i4
iig4sg_( name, ind, isvar, type, length, data )
i4	*name;
i2	*ind;
i4	*isvar;
i4	*type;
i4	*length;
i4	*data;
{
    if (*type == DB_CHR_TYPE)
	return IIG4sgSetGlobal( *(char **)name, ind, 1,
	       *length == 0 ? DB_CHR_TYPE : DB_CHA_TYPE, *length,
	       *(char **)data );
    else if (*type == DB_DEC_CHR_TYPE)
	return IIG4sgSetGlobal( *(char **)name, ind, 1,
	       *type, *length, *(char **)data );
    else
	return IIG4sgSetGlobal( *(char **)name, ind, 1,
	       *type, *length, data );
}

/*
** IIG4gaGetAttr
*/
i4
iig4ga_( ind, isvar, type, length, data, name )
i2	*ind;
i4	*isvar;
i4	*type;
i4	*length;
i4	*data;
i4	*name;
{
    if (*type == DB_CHR_TYPE)
	return IIG4gaGetAttr( ind, 1, DB_CHA_TYPE,
	       *length, *(char **)data, *(char **)name );
    else if (*type == DB_DEC_CHR_TYPE)
	return IIG4gaGetAttr( ind, 1, *type,
	       *length, *(char **)data, *(char **)name );
    else
	return IIG4gaGetAttr( ind, 1, *type, *length, data, name );
}

/*
** IIG4saSetAttr
*/
i4
iig4sa_( name, ind, isvar, type, length, data )
i4	*name;
i2	*ind;
i4	*isvar;
i4	*type;
i4	*length;
i4	*data;
{
    if (*type == DB_CHR_TYPE)
	return IIG4saSetAttr( *(char **)name, ind, 1,
	       *length == 0 ? DB_CHR_TYPE : DB_CHA_TYPE, *length,
	       *(char **)data );
    else if (*type == DB_DEC_CHR_TYPE)
	return IIG4saSetAttr( *(char **)name, ind, 1,
	       *type, *length, *(char **)data );
    else
	return IIG4saSetAttr( *(char **)name, ind, 1,
	       *type, *length, data );
}

/*
** IIG4chkobj
*/
i4
iig4ch_( object, access, index, caller )
i4	*object;
i4	*access;
i4	*index;
i4	*caller;
{
    return IIG4chkobj( *object, *access, *index, *caller );
}

/*
** IIG4udUseDscr
*/
i4
iig4ud_( language, direction, sqlda )
i4	*language;
i4	*direction;
i4	*sqlda;
{
    return IIG4udUseDscr( *language, *direction, *sqlda );
}

/*
** IIG4fdFillDscr
*/
i4
iig4fd_( object, language, sqlda )
i4	*object;
i4	*language;
i4	*sqlda;
{
    return IIG4fdFillDscr( *object, *language, *sqlda );
}

/*
** IIG4icInitCall
*/
i4
iig4ic_( name, type )
i4	*name;
i4	*type;
{
    return IIG4icInitCall( *(char **)name, *type );
}

/*
** IIG4vpValParam
*/
i4
iig4vp_( name, ind, isval, type, length, data )
i4	*name;
i2	*ind;
i4	*isval;
i4	*type;
i4	*length;
i4	*data;
{
    if (*type == DB_CHR_TYPE)
	return IIG4vpValParam( *(char **)name, ind, 1, DB_CHA_TYPE,
	       *length, *(char **)data );
    else if (*type == DB_DEC_CHR_TYPE)
	return IIG4vpValParam( *(char **)name, ind, 1, *type,
	       *length, *(char **)data );
    else
	return IIG4vpValParam( name, ind, 1, *type, *length, data );
}

/*
** IIG4bpByrefParam
*/
i4
iig4bp_( ind, isvar, type, length, data, name )
i2	*ind;
i4	*isvar;
i4	*type;
i4	*length;
i4	*data;
i4	*name;
{
    if (*type == DB_CHR_TYPE)
	return IIG4bpByrefParam( ind, 1, DB_CHA_TYPE,
	       *length, *(char **)data, *(char **)name );
    else if (*type == DB_DEC_CHR_TYPE)
	return IIG4bpByrefParam( ind, 1, *type,
	       *length, *(char **)data, *(char **)name );
    else
	return IIG4bpByrefParam( ind, 1, *type, *length, data, name );
}

/*
** IIG4rvRetVal
*/
i4
iig4rv_( ind, isval, type, length, data )
i2	*ind;
i4	*isval;
i4	*type;
i4	*length;
i4	*data;
{
    if (*type == DB_CHR_TYPE)
	return IIG4rvRetVal( ind, 1, DB_CHA_TYPE, *length, *(char **)data );
    else if (*type == DB_DEC_CHR_TYPE)
	return IIG4rvRetVal( ind, 1, *type, *length, *(char **)data );
    else
	return IIG4rvRetVal( ind, 1, *type, *length, data );
}

/*
** IIG4ccCallComp
*/
i4
iig4cc_()
{
    return IIG4ccCallComp();
}

/*
** IIG4i4Inq4GL
*/
i4
iig4i4_( ind, isvar, type, length, data, object, code )
i2	*ind;
i4	*isvar;
i4	*type;
i4	*length;
i4	*data;
i4	*object;
i4	*code;
{
    if (*type == DB_CHR_TYPE)
	return IIG4i4Inq4GL( ind, 1, DB_CHA_TYPE, *length, *(char **)data,
	       *object, *code );
    else if (*type == DB_DEC_CHR_TYPE)
	return IIG4i4Inq4GL( ind, 1, *type, *length, *(char **)data,
	       *object, *code );
    else
	return IIG4i4Inq4GL( ind, 1, *type, *length, data, *object, *code );
}

/*
** IIG4s4Set4GL
*/
i4
iig4s4_( object, code, ind, isvar, type, length, data )
i4	*object;
i4	*code;
i2	*ind;
i4	*isvar;
i4	*type;
i4	*length;
i4	*data;
{
    if (*type == DB_CHR_TYPE)
	return IIG4s4Set4GL( *object, *code, ind, 1,
	       *length == 0 ? DB_CHR_TYPE : DB_CHA_TYPE, *length,
	       *(char **)data );
    else if (*type == DB_DEC_CHR_TYPE)
	return IIG4s4Set4GL( *object, *code, ind, 1,
	       *type, *length, *(char **)data );
    else
	return IIG4s4Set4GL( *object, *code, ind, 1,
	       *type, *length, data );
}

/*
** IIG4seSendEvent
*/
i4
iig4se_( frame )
i4	*frame;
{
    return IIG4seSendEvent( *frame );
}

# endif /* UNIX */
