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
#include        <adf.h>
#include        <ft.h>
#include        <fmt.h>
#include        <frame.h>

/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	iiufrunt.c
** Purpose:	Runtime (forms) translation layer for UNIX EQUEL/F77 programs.
**
** Defines:
**	iiaccl_()		IIactclm
**	iiactc_()		IIactcomm
**	iiactf_()		IIactfld
**	iinmua_()		IInmuact
**	iiacts_()		IIactscrl
**	iiaddf_()		IIaddfrm
**	iichkf_()		IIchkfrm
**	iiclrf_()		IIclrflds
**	iiclrs_()		IIclrscr
**	iidisp_()		IIdispfrm
**	iiprmp_()		IIprmptio
**	iienfo_()		IIendforms
**	iiendf_()		IIendfrm
**	iiendm_()		IIendmu
**	iifgps_()		IIFRgpsetio
**	iifldc_()		IIfldclear
**	iifnam_()		IIfnames
**	iifomi_()		IIforminit
**	iiform_()		IIforms
**	iifraf_()		IIFRafActFld
**	iifrit_()		IIFRitIsTimeout
**	iifrgp_()		IIFRgpcontrol
**	iifrre_()		IIFRreResEntry
**	iifrsa_()		IIFRsaSetAttrio
**	iifrsh_()		IIfrshelp
**	iifrto_()		IIFRtoact
**	iifsin_()		IIfsinqio
**	iinfrs_()		IInfrskact
**	iifsse_()		IIfssetio
**	iifset_()		IIfsetio
**	iigeto_()		IIgetoper
**	iigtqr_()		IIgtqryio
**	iihelp_()		IIhelpfile
**	iiinit_()		IIinitmu
**	iiiqse_()		IIiqset
**	iiinqs_()		IIinqset
**	iiiqfs_()		IIiqfsio
**	iimess_()		IImessage
**	iimuon_()		IImuonly
**	iiprns_()		IIprnscr
**	iiputo_()		IIputoper
**	iiredi_()		IIredisp
**	iiresc_()		IIrescol
**	iiresf_()		IIresfld
**	iigfld_()		IIFRgotofld
**	iifrgo_()		IIFRgotofld
**	iiresm_()		IIresmu
**	iiresn_()		IIresnext
**	iigetf_()		IIgetfldio
**	iiretv_()		IIretval
**	iirfpa_()		IIrf_param
**	iirunf_()		IIrunform
**	iirunm_()		IIrunmu
**	iiputf_()		IIputfldio
**	iitbca_()		IITBcaClmAct
**	iisfpa_()		IIsf_param
**	iislee_()		IIsleep
**	iistfs_()		IIstfsio
**	iivalf_()		IIvalfld
**	iinest_()		IInestmu
**	iirunn_()		IIrunnest
**	iiendn_()		IIendnest
**	iifsqd_()               IIFRsqDescribe
**	iifsqe_()               IIFRsqExecute
**	iifrae_()		IIFRaeAlerterEvent
**
-*
** Notes:
**	This file is one of four files making up the runtime interface for
**	F77.   The files are:
**		iiuflibq.c	- libqsys directory
**		iiufutils.c	- libqsys directory
**		iiufrunt.c	- runsys directory
**		iiuftb.c	- runsys directory
**	See complete notes on the runtime layer in iiuflibq.c.
**
**
** History:
**	06-jun-86	- Written for 4.0 UNIX EQUEL/FORTRAN (bjb)
**	01-oct-86 (cmr)	- Modified for 5.0 UNIX EQUEL/FORTRAN
**	2/19/87	(daveb)	-- Hide fortran details in iiufsys.h by using
**			  FLENTYPE and FSTRLEN for string lengths.
**			  Add wrappers for apollo's different way of
**			  creating C names from the fortran compiler.
**	15-jun-87 (cmr) - use NO_UNDSCR (defined in iiufsys.h) for
**			  for compilers that do NOT append an underscore
**			  to external symbol names.
**	1-jun-88 (markd) - Added LEN_BEFORE_VAR case for FORTRAN compilers
**			   which pass the string length before the pointer
**			   to the string.
**      10-jun-88 (markd) - Added dummy variable so formname can be correctly 
**			    passed from Symmetry Compiler.
**      10-oct-88(russ)  - Added the STRUCT_VAR case for compilers which 
**	15-may-89(teresa) - Added function calls for 6.4
**	14-aug-1990 (barbara)
**	    Added interface for IIFRsaSetAttrio for per value attribute
**	    setting on INSERTROW and LOADTABLE (via WITH clause).
**	04-sep-90 (kathryn)-Added functions for F77 Dynamic SQL SQLDA support.
**	    (iifsqd & iifsqe).
**	05-sep-90 (kathryn)
**  	    Integrate Porting changes from 6202p code line:
**      20-apr-90 (kathryn)
**              Changed iiaddf_ to call IIaddform rather than IIaddfrm.
**              IIaddfrm is a 3.0 compatibility wrapper for IIaddform
**              and should only be called by ii30 routines.
**              Also removed function iifmda_ - this function is never
**              invoked as the formdata statement was only applicable to
**              releases prior to 3.0.
**      06-jul-90 (fredb) - Added cases for HP3000 MPE/XL (hp9_mpe) which
**              needs these routines.
**	12-sep-90 (barbara)
**	    Incorporated part of Russ's mixed case changes here using the
**	    same code as NO_UNDSCR.  The other redefines for external
**	    symbols are in separate files.
**	11-sep-90(teresa) - Modified I/O calls to accept new decimal 
**			    data type DB_DEC_CHR_TYPE (53) which is a decimal 
**			    number encoded as a character string. For  
**			    completeness, all current I/O routines have been 
**			    updated. 
**	14-sep-90 (barbara)
**	    Undid my last change.  It won't work.
**      19-sep-90 (kathryn)
**          Changed iifrsa_() and iistfs_(), Not to pass "isvar" to the
**          forms system. "isvar" indicates whether var_ptr was passed in
**          by reference or by value. F77 always passes by reference.
**	25-sep-90 (teresal)
**	    For completeness, updated iifrsa_ call to handle new decimal
**	    data type DB_DEC_CHR_TYPE eventhough this call currently 
**	    shouldn't be passed this data type.
**	26-oct-1990 (barbara)
**	    Took out a change by hand that tools into_trunk tool couldn't
**	    detect.
**	21-apr-1991 (teresal)
**	    Add new FRS call IIFRaeAlerterEvent.
**	30-Jul-1992 (fredv)
**	    For m88_us5, we don't want to redefine these symbols because
**	    m88_us5 FORTRAN compiler is NO_UNDSCR and MIXEDCASE_FORTRAN.
**	    If we do that, symbols in iiufrunM.o will be all screwed up.
**	    This is a quick and dirty change for this box for 6.4. The
**	    NO_UNSCR and MIXEDCASE_FORTRAN issue should be revisited in
**	    6.5. Me and Rudy both have some idea what should be the change
**	    to make this FORTRAN stuff a lot cleaner.
**	08/19/93 (dkh) - Added IIgfld_() to support resume
**			 nextfield/previousfield
**	03/03/94 (smc) - Changed fldname to iifldname as the former is
**			 is a #define in frame.h.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**      11-jan-1996 (toumi01; from 1.1 axp_osf port)
**          Added kchin's change (from 6.4) for axp_osf
**          05-Aug-1993 (kchin) bug# 54293
**          Changed type of argument of iiaddf_() from i4  to PTR.  Using
**          a i4  will truncate pointer address if it is 8 bytes long (such
**          as PTR on Alpha OSF/1).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-Sep-2003 (bonro01)
**	    Modified iiaddf_() for i64_hpu to replace "i4 *" casts with
**	    "void *" casts.  The parameter passed to iiaddf_() is either
**	    a pointer or a pointer-to-a-pointer, so I changed it to
**	    "void" to eliminate any association with a 4-byte int.
**	    Redefined the deref_form variable from "i4 *" to "void **"
**	    to prevent truncation of the pointer value on 64-bit platforms.
**	24-Apr-2007 (kibro01) b118115
**	    Add IIFRgotofld's abbreviation: iifrgo
**	24-Aug-2009 (kschendel) 121804
**	    Prototype a couple of function calls to keep aux-info straight.
*/

# if defined(NO_UNDSCR) && !defined(m88_us5)
# define	iiaccl_	iiaccl
# define	iiactc_	iiactc
# define	iiactf_	iiactf
# define	iiacts_	iiacts
# define	iiaddf_	iiaddf
# define	iichkf_	iichkf
# define	iiclrf_	iiclrf
# define	iiclrs_	iiclrs
# define	iidisp_	iidisp
# define	iiendf_	iiendf
# define	iiendm_	iiendm
# define	iiendn_	iiendn
# define	iienfo_	iienfo
# define	iifldc_	iifldc
# define	iifnam_	iifnam
# define	iifomi_	iifomi
# define	iiform_	iiform
# define	iifraf_ iifraf
# define	iifrit_ iifrit
# define	iifrre_ iifrre
# define	iifrsa_ iifrsa
# define	iifrsh_	iifrsh
# define	iifrto_ iifrto
# define	iifset_	iifset
# define	iifsin_	iifsin
# define	iifsse_	iifsse
# define	iigetf_	iigetf
# define	iigeto_	iigeto
# define	iigtqr_	iigtqr
# define	iihelp_	iihelp
# define	iiinit_	iiinit
# define	iiinqs_	iiinqs
# define	iiiqfs_	iiiqfs
# define	iiiqse_	iiiqse
# define	iimess_	iimess
# define	iimuon_	iimuon
# define	iinest_	iinest
# define	iinfrs_	iinfrs
# define	iinmua_	iinmua
# define	iiprmp_	iiprmp
# define	iiprns_	iiprns
# define	iiputf_	iiputf
# define	iiputo_	iiputo
# define	iiredi_	iiredi
# define	iiresc_	iiresc
# define	iiresf_	iiresf
# define	iigfld_ iigfld
# define	iifrgo_ iifrgo
# define	iiresm_	iiresm
# define	iiresn_	iiresn
# define	iiretv_	iiretv
# define	iirfpa_	iirfpa
# define	iirunf_	iirunf
# define	iirunm_	iirunm
# define	iirunn_	iirunn
# define	iitbca_	iitbca
# define	iisfpa_	iisfpa
# define	iislee_	iislee
# define	iistfs_	iistfs
# define	iivalf_	iivalf
# define	iifrgp_	iifrgp
# define	iifgps_	iifgps
# define	iifsqd_ iifsqd
# define	iifsqe_ iifsqe
# define	iifrae_ iifrae
# endif


/*
** IIactclm
**	- ## activate column <tablefldname> <colname>
**	- Old style (pre 8.0) needed for backward compatibility.
*/
i4
iiaccl_( table, column, intr )
char	**table;		/* Name of tablefield */
char	**column;		/* Name of column */
i4	*intr;			/* Display loop interrupt value */
{
    return IIactclm ( *table, *column, *intr );
}

/*
** IITBcaClmAct
**	- ## activate column [before/after] <tablefldname> <colname>
**	- Version 8.0
*/
i4
iitbca_( table, column, entact, intr )
char	**table;		/* Name of tablefield */
char	**column;		/* Name of column */
i4	*entact;		/* Entry activation indicator */
i4	*intr;			/* Display loop interrupt value */
{
    return IITBcaClmAct ( *table, *column, *entact, *intr );
}

/*
** IIactcomm
** 	- ## activate command <control-key>
*/
i4
iiactc_( ctrl, intr_val )
i4	*ctrl;				/* control value	*/
i4	*intr_val;			/* user interrupt value */
{
    return IIactcomm( *ctrl, *intr_val );
}

/*
** IIactfld
**	- ## activate field <fldname>
**	- Old style (pre 8.0) needed for backward compatibility.
*/
i4
iiactf_( fld_name, intr_val )
char    **fld_name;			/* name of the field to activate */
i4	*intr_val;			/* user's interrupt value	*/
{
    return IIactfld( *fld_name, *intr_val );
}

/*
** IIFRafActFld
**	- ## activate [before/after] field <fldname>
**	- Version 8.0
*/
i4
iifraf_( fld_name, entact, intr_val )
char    **fld_name;			/* name of the field to activate */
i4	*entact;			/* Entry activation indicator */
i4	*intr_val;			/* user's interrupt value	*/
{
    return IIFRafActFld( *fld_name, *entact, *intr_val );
}

/* 
** IInmuact
** 	- ## activate menuitem <itemname>
*/
i4
iinmua_( menu_name, exp, val, act, intr )
char	**menu_name;		/* Name of menu item		*/
char	*exp;			/* Used within form system	*/
i4	*val;			/* Ordinal position in list of items */
i4	*act;			/* activation flag		*/
i4	*intr;			/* User's interrupt value	*/
{
    return IInmuact( *menu_name, exp, *val, *act, *intr );
}

/*
** IIactscrl
**	- activate scroll[up/down] <tablefldname>
*/
i4
iiacts_( tab_name, mode, intr )
char	**tab_name;
i4	*mode;
i4	*intr;			/* Display loop interrupt value */
{
    return IIactscrl( *tab_name, *mode, *intr );
}


/* 
** IIaddfrm
** 	- ## addform formname
*/
void
iiaddf_( ex_form )
PTR	*ex_form;			/* external compiled form name	*/
{
	void   **deref_form;
	deref_form = (void *)(*ex_form);

# ifdef DOUBLEDEREF
	IIaddform((void *)(*deref_form));  /* some machines pass structure */
					  /* so pointer must be dereferenced */
# else
	IIaddform((void *)(*ex_form));
# endif
}


/*
** IIchkfrm
**	- generated for validate
*/
i4
iichkf_()
{
    return IIchkfrm();
}


/*
** IIclrflds
**	- ## clear field all
*/
iiclrf_()
{
    IIclrflds();
}

/*
** IIclrscr
**	- ## clear screen
*/
iiclrs_()
{
    IIclrscr();
}

/*
** IIdispfrm
**	- ## display <formname>
*/
i4
iidisp_( form_name, mode )
char    **form_name;		/* form name	*/
char    **mode;			/* display mode */		
{
    return IIdispfrm( *form_name, *mode );
}


/*
** IIprmptio
** 	- ## prompt (<message> <response>)
*/
void
iiprmp_( echo, prompt, indflag, indptr, isvar, type, len, response )
i4	*echo;
char    **prompt;		/* prompt string		*/
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
char    **response;		/* response buffer		*/
{

    if ( !*indflag )
	indptr = (i2 *)0;

    IIprmptio( *echo, *prompt, indptr, 1, DB_CHA_TYPE, *len, *response );
}

/*
** IIenforms (IIendforms in VMS)
**	- ## endform
*/

void
iienfo_()
{
    IIendforms();
}

/*
** IIendfrm_
**	- end frame
*/

void
iiendf_()
{
    IIendfrm();
}

/* 
** IIendmu
** 	- Clean up after adding user menuitems
*/
i4
iiendm_()
{
    return IIendmu();
}


/* 
** IIfldclear
**	- ## clear field <fieldname>
*/
iifldc_( iifldname )
char	**iifldname;
{
    IIfldclear( *iifldname );
}


/*
** IIfnames
** 	- Generated for ## Formdata statement
*/
i4
iifnam_()
{
    return IIfnames();
}


/* 
** IIforminit
** 	- ## forminit <formname>
*/
void
iifomi_( form_name )
char    **form_name;			/* name of the form */
{
    IIforminit( *form_name );
}


/*
** IIforms
** 	- ## forms
*/
/*ARGSUSED*/
void
iiform_( lan )
i4     *lan;			/* which host language? */
{
    IIforms( hostC );
}

/*
** IIFRitIsTimeout
**	- generated for empty display loops.  If timeout ended display
**	  loop, validating (IIchkfrm) is skipped.
*/

i4
iifrit_()
{
    return IIFRitIsTimeout();
}

/*
** IIfrshelp
**	- ## help_frs(subject = string, file = string)
*/
FUNC_EXTERN bool IIfrshelp();

void
iifrsh_( type, subject, filename )
i4	*type;			/* Type of help - currently always 0 */
char	**subject;
char	**filename;
{
    (void) IIfrshelp( *type, *subject, *filename );
}


/*
** IIfsinqio
**	- Old-style inquire_frs statement
**	- ## inquire_frs 'object' (var = <frs_const>, ...)
**	     e.g., ## inquire_frs 'form' <formname> (var = 'mode')
**	- If receiving var is a string, blank pad.
*/

void
iifsin_( isvar, type, len, var_ptr, obj_name )
i4	    *isvar;			/* Always 1 for F77		*/
i4	    *type;			/* type of variable		*/
i4	    *len;			/* sizeof data, or strlen	*/
i4	    *var_ptr;			/* pointer to inquiring var	*/
char	    **obj_name;			/* name of object to inquire	*/
{

    if (*type == DB_CHR_TYPE)
    {
	IIfsinqio( 1, DB_CHA_TYPE, *len, *(char **)var_ptr, *obj_name );
    }
    else
    {
        IIfsinqio( 1, *type, *len, var_ptr, *obj_name );
    }
}

/*
** IIFRtoact
**	- ## activate timeout
*/
i4
iifrto_( val, intr )
i4	*val;		/* Timeout value - unused presently */
i4	*intr;		/* Display loop interrupt value */
{
	return IIFRtoact( *val, *intr );
}

/*
** IInfrskact
**	- ## activate frskeyN command
*/
i4
iinfrs_( frsnum, exp, val, act, intr )
i4	*frsnum;		/* Frskey number		*/
char	**exp;			/* Explanation string/var	*/
i4	*val;			/* Validate Value		*/
i4	*act;			/* activation flag		*/
i4	*intr;			/* Display loop interrupt value */
{
    	return IInfrskact( *frsnum, *exp, *val, *act, *intr );
}


/* 
** IIfssetio
** 	- old-style set_frs data set command
**	  ## set_frs 'object' (frsconst = var/val, ...)
**	     e.g. ## set_frs 'form' f ('mode' = 'update')
*/
iifsse_( name, indflag, indptr, isvar, type, len, var_ptr )
char    **name;			/* the field name		*/
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
i4      *var_ptr;
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
	IIfssetio( *name, indptr, 1, *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE,
		   *len, *(char **)var_ptr );
    else
	IIfssetio( *name, indptr, 1, *type, *len, var_ptr );
}


/*
** IIfsetio
** 	- ## initialize statement
*/
i4
iifset_( name )
char    **name;			/* name of form to set up */
{
    return IIfsetio( *name );
}


/*
** IIgetoper
** 	- ## getoper(field)
*/
void
iigeto_( one )
i4     *one;			/* Always has value of 1 */
{
    IIgetoper( 1 );
}

/*
** IIgtqryio
**	- ## getoper on a field for query mode.
*/
void
iigtqr_( indflag, indptr, isvar, type, len, var_ptr, qry_fld_name )
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
i4	*var_ptr;		/* variable for query op	*/
char    **qry_fld_name;		/* name of field for query	*/
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
	IIgtqryio( indptr, 1, DB_CHA_TYPE, *len, *(char **)var_ptr, *qry_fld_name);
    else if (*type == DB_DEC_CHR_TYPE)
	IIgtqryio( indptr, 1, *type, *len, *(char **)var_ptr, *qry_fld_name);
    else
	IIgtqryio( indptr, 1, *type, *len, var_ptr, *qry_fld_name );
}


/*
** IIhelpfile
** 	- ## helpfile <subject> <filename>
*/
FUNC_EXTERN bool IIhelpfile();
void
iihelp_( subject, file_name )
char    **subject;			/* help subject */
char    **file_name;			/* name of the help file */
{
    (void) IIhelpfile( *subject, *file_name );
}


/*
** IIinitmu
**	- Generated as part of display loop
*/
i4
iiinit_()
{
    return IIinitmu();
}


/*
** IIiqset
**	- Function called for ## inquire_frs and ## set_frs commands
**	- ## inquire_frs 'table' <formname> (var = column) 
**	     will generate:
**		if (IIiqset(3,0,'f',0) .ne. 0) then
**		  call IIiqfrs(1, 30, 4, var, 29, 0, 0)
**	- Sends two i4  arguments followed by a variable number of string 
**	  arguments, up to a maximum of 4
**	- count refers to number of string args.
*/
i4
iiiqse_( count, objtype, row, arg1, arg2, arg3, arg4 )
i4	*count; 		/* Number of string args */
i4	*objtype;
i4	*row;
char    **arg1, **arg2, **arg3, **arg4;	    
{
# define	IQ_ARGS_MAX	4		/* Max string args */
    char	*v_args[IQ_ARGS_MAX];
    i4		i;

    for (i = 0; i <IQ_ARGS_MAX; i++)
	v_args[i] = (char *)0;		/* initialize */
    switch ( *count - 2 ) {
      case 4:	v_args[3] = *arg4;
      case 3:	v_args[2] = *arg3;
      case 2:	v_args[1] = *arg2;
      case 1:	v_args[0] = *arg1;
    }
    return IIiqset( *objtype, *row, v_args[0], v_args[1], v_args[2], v_args[3] );
}

/*
** IIinqset
**	- Function called for old-style ## inquire_frs and ## set_frs commands 
**	- ## inquire_frs 'field' <frmname> <fldname> (modevar = 'mode') 
**	     will generate:
**		if (IIinqset('field', 'frmname', 'fldname', 0) .ne. 0) then
**		  call IIxfrsinq(1, 32, 0, modevar, 'mode')
**	- Sends a variable number of arguments, up to a maximum of 5
**	  character string args, preceded by a count.
*/
i4
iiinqs_( count, arg1, arg2, arg3, arg4, arg5 )
i4	*count;
char    **arg1, **arg2, **arg3, **arg4, **arg5;	    
{
# define	INQ_ARGS_MAX	5
    char	*v_args[INQ_ARGS_MAX];
    i4		i;

    for (i = 0; i < INQ_ARGS_MAX; i++)
	v_args[i] = (char *)0;
    switch ( *count ) {
      case 5:	v_args[4] = *arg5;
      case 4:	v_args[3] = *arg4;
      case 3:	v_args[2] = *arg3;
      case 2:	v_args[1] = *arg2;
      case 1:	v_args[0] = *arg1;
    }
    return IIinqset( v_args[0], v_args[1], v_args[2], v_args[3], v_args[4] );
}

/*
** IIiqfsio
**	- ## inquire_frs 'object' <formname> (var = frsconst)
**	- If receiving var is a string, copy and blank pad.
*/
void
iiiqfs_( indflag, indptr, isvar, type, len, var_ptr, attr, name, row )
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
i4	*var_ptr;		/* Pointer to inquiring var	*/
i4	*attr;			/* Object type			*/
char	**name;			/* Table name			*/
i4	*row;			/* Row number			*/
{

    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
	IIiqfsio( indptr, 1, DB_CHA_TYPE, *len, *(char **)var_ptr, *attr, *name, *row );
    }
    else if (*type == DB_DEC_CHR_TYPE)
    {
	IIiqfsio( indptr, 1, *type, *len, *(char **)var_ptr, *attr, *name, *row );
    }	
    else
    {
	IIiqfsio( indptr, 1, *type, *len, var_ptr, *attr, *name, *row );
    }
}

/*
** IImessage
**	- ## message
*/
void
iimess_( msg_str )
char    **msg_str;			/* the message string */
{
    IImessage( *msg_str );
}


/* 
** IImuonly
**	- Generated in ## submenu loop
*/
void
iimuon_()
{
    IImuonly();
}


/*
** IIprnscr
**	- ## printscreen [(file = filename)]
*/
void
iiprns_( name )
char	**name;
{
	IIprnscr( *name );
}


/*
** IIputoper
**	- generated for putoper function
*/
void
iiputo_( one )
i4	*one;
{
    IIputoper( 1 );
}


/*
** IIredisp
*/
iiredi_()
{
    IIredisp();
}


/*
** IIrescol
**	- ## resume column <tablefield> <column>
*/
void
iiresc_( tabname, colname )
char	**tabname;
char	**colname;
{
    IIrescol( *tabname, *colname);
}


/*
** IIresfld
** 	- ## resume <fldname>
*/
void
iiresf_( fld_name )
char    **fld_name;			/* field name */
{
    IIresfld( *fld_name );
}


/*
** IIFRgotofld
**	- ## resume nextfield/previousfield
*/
void
iigfld_( dir )
i4	*dir;
{
    IIFRgotofld( *dir );
}
void
iifrgo_( dir )
i4	*dir;
{
    IIFRgotofld( *dir );
}


/*
** IIresmu
**	- ## resume menu
*/
void
iiresm_()
{
    IIresmu();
}


/*
** IIFRreResEntry
**	- ## resume entry
*/
void
iifrre_()
{
    IIFRreResEntry();
}


/*
** IIresnext
** 	- ## resume
*/
i4
iiresn_()
{
    IIresnext();
}


/*
** IIgetfldio
** 	- ## getform I/O
**	- If receiving variable is a string, copy and blank pad.
*/

void
iigetf_( indflag, indptr, isvar, type, len, var_ptr, fld_name )
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
i4	*var_ptr;		/* pointer to retrieval var	*/
char    **fld_name;		/* name of field to retrieve	*/
{

    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
	IIgetfldio( indptr, 1, DB_CHA_TYPE, *len, *(char **)var_ptr, *fld_name );
    }
    else if (*type == DB_DEC_CHR_TYPE)
    {
	IIgetfldio( indptr, 1, *type, *len, *(char **)var_ptr, *fld_name );
    }
    else
    {
	IIgetfldio( indptr, 1, *type, *len, var_ptr, *fld_name );
    }
}


/*
** IIretval
**	- return value
*/
i4
iiretv_()
{
    return IIretval();
}


/*
** IIrf_param
** 	- PARAM(target_list, arg_vector) within an output-type of forms
** 	  statement, e.g. 
** 	  ## getform <formname> (param (tlist, arg))
** 	- Data will be put into host_var by IIxouttrans (defined in iifutil.c)
*/
void
iirfpa_( format, argv )
char    **format;			/* format for param getform */
char    *argv[];			/* arg vector for retrieval */
{
    i4    IIxouttrans();

    IIrf_param( *format, argv, IIxouttrans );
}


/* IIrunform
** 	- Generated for loop display
*/
i4
iirunf_()
{
    return IIrunform();
}


/*
** IIrunmu_
**	- IIrunmu
**	- call frame driver to display a nested menu.
**	  if single_item is set then allow reentry to form for the user,
**	  otherwise treat as a multi-item prompt.
*/

void
iirunm_( single_item )
i4	*single_item;		/* single-entry boolean */
{
    IIrunmu( *single_item );
}

/*
** IIputfldio
**	## initialize(fld = var, ...)
*/
void
iiputf_( name, indflag, indptr, isvar, type, len, var_ptr )
char	**name;			/* the field name		*/
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
i4      *var_ptr;		/* ptr to data for setting	*/
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
	IIputfldio( *name, indptr, 1, *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE,
		    *len, *(char **)var_ptr );
    else if (*type == DB_DEC_CHR_TYPE)
	IIputfldio( *name, indptr, 1, *type, *len, *(char **)var_ptr );
    else
	IIputfldio( *name, indptr, 1, *type, *len, var_ptr );
}



/*
** IIsf_param
** 	- PARAM(target_list, arg_vector) within an input-type of forms
** 	  statement, e.g. 
** 	  ## putform <formname> (param (tlist, arg))
** 	- Data will be put into host_var by IIxinttrans (defined in iifutil.c)
*/
void
iisfpa_( format, argv )
char    **format;			/* format for param getform */
char    *argv[];			/* arg vector for retrieval */
{
    char    *IIxintrans();

    IIsf_param( *format, argv, IIxintrans );
}

/*
** IIsleep
**	## sleep
*/
void
iislee_( seconds )
i4	    *seconds;			/* number of seconds to sleep */
{
    IIsleep( *seconds );
}


/* 
** IIstfsio
**	- ## set_frs statement
*/
void
iistfs_( attr, name, row, indflag, indptr, isvar, type, len, var_ptr )
i4	*attr;
char	**name;
i4	*row;
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
i4	*var_ptr;
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
	IIstfsio( *attr, *name, *row, indptr, 1,
	    *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE, *len, *(char **)var_ptr );
    else if (*type == DB_DEC_CHR_TYPE)
	IIstfsio( *attr, *name, *row, indptr, 1, *type, *len, 
	    *(char **)var_ptr ); 
    else
	IIstfsio( *attr, *name, *row, indptr, 1, *type, *len, var_ptr );
}

/*
** IIFRsaSetAttrio
**   -	loadtable/insertrow with clause for per value attribute setting.
**	e.g.,
**	exec frs loadtable f t (col1 = val) with (reverse(col1) = 1);
*/
void
iifrsa_( attr, name, indflag, indptr, isvar, type, len, var_ptr )
i4    *attr;
char  **name;
i4    *indflag;               /* 0 = no null pointer          */
i2    *indptr;                /* null pointer                 */
i4    *isvar;                 /* by value or reference        */
i4    *type;                  /* type of user variable        */
i4    *len;                   /* sizeof data, or strlen       */
i4    *var_ptr;
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
	IIFRsaSetAttrio( *attr, *name, indptr, 1, 
	    *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE, *len, *(char **)var_ptr );
    else if (*type == DB_DEC_CHR_TYPE)	
	IIFRsaSetAttrio( *attr, *name, indptr, 1, *type, *len,
	    *(char **)var_ptr );
    else
	IIFRsaSetAttrio( *attr, *name, indptr, 1, *type, *len, var_ptr );
}

/* 
** IIvalfld
** 	- ## validate
*/
i4
iivalf_( fld_name )
char    **fld_name;			/* name of the field */
{
    return IIvalfld( *fld_name );
}

/*
** IInestmu
** 	- ## validate
*/
i4
iinest_()
{
    return IInestmu();
}

/*
** IIrunnest
** 	- ## validate
*/
i4
iirunn_()
{
    return IIrunnest();
}

/*
** IIendnest
** 	- ## validate
*/
i4
iiendn_()
{
    return IIendnest();
}

/*
** IIFRgpcontrol
**	- POPUP forms control
*/
void
iifrgp_( state, modifier )
i4	*state;
i4	*modifier;
{
    IIFRgpcontrol( *state, *modifier );
}


/*
** IIFRgpsetio
** 	- POPUP forms
*/
void
iifgps_( id, indflag, indptr, isvar, type, len, var_ptr )
i4	*id;			/* internal clause id		*/
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
i4	*var_ptr;
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
	IIFRgpsetio( *id, indptr, 1, *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE,
		     *len, *(char **)var_ptr );
    else if (*type == DB_DEC_CHR_TYPE)
	IIFRgpsetio( *id, indptr, 1, *type, *len, *(char **)var_ptr );
    else
	IIFRgpsetio( *id, indptr, 1, *type, *len, var_ptr );
}

/*
** IIFRsqDescribe
**	-DESCRIBE a form/table into an SQLDA.
*/
void
iifsqd_(lang, is_form, form_name, table_name, mode, sqd)
i4      *lang;
i4      *is_form;
char    **form_name;
char    *table_name;
char    **mode;
IISQLDA *sqd;
{
	IIFRsqDescribe(*lang, *is_form, *form_name, table_name, *mode, sqd);
}

/*
** IIFRsqExecute
**	- Process the USING clause with an SQLDA.
*/
void
iifsqe_(lang, is_form, is_input, sqd)
i4      *lang;
i4      *is_form;
i4      *is_input;
IISQLDA *sqd;
{
	IIFRsqExecute(*lang, *is_form, *is_input, sqd);

}
/*
** IIFRaeAlerterEvent
**	- exec sql activate event
*/
i4
iifrae_( intr )
i4	*intr;		/* Display loop interrupt value */
{
	return IIFRaeAlerterEvent( *intr );
}

# endif /* UNIX */
