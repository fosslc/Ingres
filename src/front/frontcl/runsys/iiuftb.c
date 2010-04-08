# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include 	<eqrun.h>
# if defined(UNIX) || defined(hp9_mpe)
# include	<iiufsys.h>
 
/*
** Copyright (c) 2004 Ingres Corporation
*/
 
/*
+* Filename:	iiuftb.c
** Purpose:	Runtime translation layer for Unix EQUEL/F77 programs only
**
** Defines:			Maps to:
**	iitbac_			IItbact
**	iitbin_			IItbinit
**	iitbse_			IItbsetio
**	iitbsm_			IItbsmode
**	iitclc_			IItclrcol
**	iitclr_			IItclrrow
**	iitcog_			IItcogetio
**	iitcop_			IItcoputio
**	iitcol_			IItcolval
**	iitdat_			IItdata
**	iitdte_			IItdatend
**	iitdel_			IItdelrow
**	iitfil_			IItfill
**	iithid_			IIthidecol
**	iitins_			IItinsert
**	iitrcp_			IItrc_param
**	iitscp_			IItsc_param
**	iitscr_			IItscroll
**	iitune_			IItunend
**	iitunl_			IItunload
**	iitvlr_			IItvalrow
**	iitval_			IItvalval
**	iitbce_			IITBceColEnd
-*
** Notes:
**	This file is one of four which make up the F77 runtime interface.
** The files are:
**		iiuflibq.c	libqsys directory
**		iiufutil.c	libqsys directory
**		iiufrunt.c	runsys directory
**		iiuftb.c	runsys directory.
**
**	For a full explanation of the runtime interface for Unix F77,
** see notes in iiuflibq.c.
**
**
** History:
**	12-jun-86	- Written for 4.0 UNIX EQUEL/FORTRAN (bjb) 
**	01-oct-86 (cmr)	- Modified for 5.0 UNIX EQUEL/FORTRAN
**	2/19/87	(daveb)	-- Add wrappers for apollo's different way of
**			  creating C names from the fortran compiler.
**	15-jun-87 (cmr) - use NO_UNDSCR (defined in iiufsys.h) for
**			  for compilers that do NOT append an underscore
**			  to external symbol names.
**	1-jun-88 (markd) - Added LEN_BEFORE_VAR case for FORTRAN compilers
**			   which pass the length of a string before the
** 			   pointer to the string.
**      10-oct-88(russ)  - Added the STRUCT_VAR case for compilers which 
**                         pass strings as a structure.
**	19-jun-89 (rexl) - Correct typos and mismappings for IItclrcol,
**			   IItclrrow, and IItcolval.
**	10-aug-89(teresa)- Added 6.4 function call.
**
**      06-jul-90 (fredb) - Added cases for HP3000 MPE/XL (hp9_mpe) which
**                          needs these routines.
**	12-sep-90 (barbara)
**	    Incorporated part of Russ's mixed case changes here using the
**	    same code as NO_UNDSCR.  The other redefines for external
**	    symbols are in separate files.
**	11-sep-90(teresa)- Modified I/O calls to accept new decimal
**                         data type DB_DEC_CHR_TYPE (53) which is a decimal
**                         number encoded as a character string. For
**                         completeness, all current I/O routines have been
**                         updated.
**	14-sep-90 (barbara)
**	    Undid my last change.  It won't work.
**	26-oct-90 (barbara)
**	    Edited trunk by hand to back out a change that merge tools
**	    didn't pick up.
**	30-Jul-1992 (fredv)
**	    For m88_us5, we don't want to redefine these symbols because
**	    m88_us5 FORTRAN compiler is NO_UNDSCR and MIXEDCASE_FORTRAN.
**	    If we do that, symbols in iiuftbM.o will be all screwed up.
**	    This is a quick and dirty change for this box for 6.4. The
**	    NO_UNSCR and MIXEDCASE_FORTRAN issue should be revisited in
**	    6.5. Me and Rudy both have some idea what should be the change
**	    to make this FORTRAN stuff a lot cleaner.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
 
# if defined(NO_UNDSCR) && !defined(m88_us5)
# define	iitbac_		iitbac
# define	iitbin_		iitbin
# define	iitbse_		iitbse
# define	iitbsm_		iitbsm
# define	iitclc_		iitclc
# define	iitclr_		iitclr
# define	iitcog_		iitcog
# define	iitcop_		iitcop
# define	iitcol_		iitcol
# define	iitdat_		iitdat
# define	iitdel_		iitdel
# define	iitdte_		iitdte
# define	iitfil_		iitfil
# define	iithid_		iithid
# define	iitins_		iitins
# define	iitrcp_		iitrcp
# define	iitscp_		iitscp
# define	iitscr_		iitscr
# define	iitune_		iitune
# define	iitunl_		iitunl
# define	iitval_		iitval
# define	iitvlr_		iirvlr
# define	iitbce_		iitbce
# endif
 
 
/*
** IItbact
**	- sets up data set for loading/unloading of a linked table field
*/
i4
iitbac_( form_name, tbl_fld_name, load_flg )
char	**form_name;			/* name of form */
char	**tbl_fld_name;			/* name of table field */
i4	*load_flg;			/* load/unload flag */
{
    return IItbact( *form_name, *tbl_fld_name, *load_flg );
}
 
/*
** IItbinit
**	- initialize a table field with a data set
**	- ## inittable formname tablefldname
*/
i4
iitbin_( form_name, tbl_fld_name, tf_mode )
char	**form_name;			/* name of form */
char	**tbl_fld_name;			/* name of table field */
char    **tf_mode;			/* mode for table field */
{
    return IItbinit( *form_name, *tbl_fld_name, *tf_mode );
}
 
/*
** IItbsetio
**	- set up table field structures before doing row I/O
**	- used on most row statements
*/
i4
iitbse_( mode, form_name, tbl_fld_name, row_num )
i4	*mode;				/* command mode */
char    **form_name;			/* form name */
char    **tbl_fld_name;			/* table-field name */
i4	*row_num;			/* row number */
{
    return IItbsetio( *mode, *form_name, *tbl_fld_name, *row_num );
}
 
 
/*
** IItbsmode
**	- set up the mode scrolling a table field
*/
void
iitbsm_( mode )
char    **mode;			/* up/down/to scroll mode */
{
    IItbsmode( *mode );
}
 
/*
** IItclrcol
**	- clear a specific column in a specified row
**	- ## clearrow formname tablename 1 (col1, col2)
*/
void
iitclc_( col_name )
char    **col_name;			/* name of column to clear */
{
    IItclrcol( *col_name );
}
 
 
/*
** IIclrrow
**	- clear a specified row (or current row) in a tablefield
**	- ## clearrow formname tablename 3
*/
void
iitclr_()
{
    IItclrrow();
}
 
 
/*
** IItcogetio
**	- ## getrow formname tablefldname (var = column, ...) 
**	- also generated for OUT clauses on ## deleterow statements.
*/
 
void
iitcog_( indflag, indptr, isvar, type, len, var_ptr, col_name )
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
i4	*var_ptr;		/* pointer to retrieval variable */
char	**col_name;		/* name of column to retrieve	*/
{
 
    if ( !*indflag )
	indptr = (i2 *)0;
 
    if (*type == DB_CHR_TYPE)
    {
	IItcogetio( indptr, 1, DB_CHA_TYPE, *len, *(char **)var_ptr, *col_name );
    }
    else if (*type == DB_DEC_CHR_TYPE)
    {
	IItcogetio( indptr, 1, *type, *len, *(char **)var_ptr, *col_name );
    }
    else
    {
	IItcogetio( indptr, 1, *type, *len, var_ptr, *col_name );
    }
}
 
/*
** IItcoputio
**	- ## putrow formname tablefldname (colname = var, ...)
**	- Also generated for IN clauses on ## deleterow statements.
*/
void
iitcop_( name, indflag, indptr, isvar, type, len, var_ptr )
char	**name;			/* the field name		*/
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
i4	*var_ptr;		/* ptr to data for setting	*/
{
    if ( !*indflag )
	indptr = (i2 *)0;
 
    if (*type == DB_CHR_TYPE)
	IItcoputio( *name, indptr, 1, DB_CHR_TYPE, *len, *(char **)var_ptr);
    else if (*type == DB_DEC_CHR_TYPE)
	IItcoputio( *name, indptr, 1, *type, *len, *(char **)var_ptr);
    else
	IItcoputio( *name, indptr, 1, *type, *len, var_ptr );
}
 
 
/*
** IItcolval
**	- ## validrow formname tablefldname row (col1, col2)
*/
void
iitcol_( colname )
char	**colname;
{
    IItcolval( *colname );
}
 
 
/*
** IItdata
**	- ## tabledata
*/
i4
iitdat_()
{
    return IItdata();
}
 
 
/*
** IItdatend
**	- Clean up after tabledata loop
*/
void
iitdte_()
{
    IItdatend();
}
 
 
/*
** IItdelrow
**	- ## deleterow formname tablename [row] [out/in clause]
*/
i4
iitdel_( in_list )
i4	*in_list;			/* IN-list boolean */
{
    return IItdelrow( *in_list );
}
 
 
/*
** IItfill
**	- generated by ## inittable statements
*/
void
iitfil_()
{
    IItfill();
}
 
 
/*
** IIhidecol
**	- ## inittable statement with hidden column clause
*/
void
iithid_( col_name, format )
char	**col_name;			/* column name		*/
char	**format;			/* col type format string */
{
    IIthidecol( *col_name, *format );
}
 
 
/*
** IItinsert
**	- ## insertrow formname tablefldname [row] [target list]
*/
i4
iitins_()
{
    return IItinsert();
}
 
/*
** IItrc_param
**	- PARAM(target_list, arg_vector) within an output-type of forms
**	  statement, e.g.
**	- ## getrow formname tablename (param (tlist, arg))
**	- Data will be transferred to vars referenced in arg_vector
**	  by IIxouttrans (defined in iifutil.c)
*/
 
void
iitrcp_( format, argv )
char	**format;			/* format for param getrow	*/
char	*argv[];			/* arg vector for retrieval	*/
{
    i4	IIxouttrans();
 
    IItrc_param(  *format, argv, IIxouttrans );
}
 
 
/*
** IItsc_param
**	- call IIsetparam() with IIxintrans() as function argument
**	- PARAM(target_list, arg_vector) within an input-type of forms
**	  statement, e.g.
**	- ## putrow formname tablename (param (tlist, arg))
**	- Data will be transferred from vars referenced in arg_vector
**	  by IIxintrans (defined in iifutil.c)
*/
 
void
iitscp_( format, argv )
char	**format;			/* format for param putrow	*/
char	*argv[];			/* arg vector for setting	*/
{
    char    *IIxintrans();
 
    IItsc_param( *format, argv, IIxintrans );
}
 
 
/*
** IItscroll
**	- set up calls to actual scrolling routines
*/
i4
iitscr_( in, rec_num )
i4	*in;			/* IN-list boolean		*/
i4	*rec_num;		/* record number for scroll TO	*/
{
    return IItscroll( *in, *rec_num );
}
 
 
/*
** IItunend
**	- generated to clean up after ## unloadtable
*/
void
iitune_()
{
    IItunend();
}
 
 
 
/*
** IItunload
**	- table unload
*/
FUNC_EXTERN bool IItunload();
i4
iitunl_()
{
    return IItunload();
}
 
/* 
** IItvalrow
**	- ##  validrow command
*/
void
iitvlr_()
{
    IItvalrow();
}

/* 
** IITBceColEnd
**	- indicates there are no more column values to set
*/
void
iitbce_()
{
    IITBceColEnd();
}
 
/*
** IItvalval
**	- ## validrow command
*/
i4
iitval_( one )
i4	*one;			/* Always has value of 1 */
{
    return IItvalval( 1 );
}
# endif /* UNIX */
