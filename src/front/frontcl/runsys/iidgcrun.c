# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<eqlang.h>
# include	<eqrun.h>

# ifdef DGC_AOS

/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	iidgcrun.c
** Purpose:	Runtime (forms) translation layer for DG AOS/VS COBOL programs.
**
** Defines:
**	IIXACTCLM()		IIactclm
**	IIXACTCOMM()		IIactcomm
**	IIXACTFLD()		IIactfld
**	IIXNMUACT()		IInmuact
**	IIXACTSCRL()		IIactscrl
**	IIXADDFRM()		IIaddfrm
**	IIXCLRSCR()		IIchkfrm
**	IIXCLRFLDS()		IIclrflds
**	IIXCLRSCR()		IIclrscr
**	IIXDISPFRM()		IIdispfrm
**	IIXPRMPTIO()		IIprmptio
**	IIXENDFORMS()		IIendforms
**	IIXENDFRM()		IIendfrm
**	IIXENDMU()		IIendmu
**	IIXFRGPSETIO()		IIFRgpsetio
**	IIXFLDCLEAR()		IIfldclear
**	IIXFNAMES()		IIfnames
**	IIXFORMINIT()		IIforminit
**	IIXFORMS()		IIforms
**	IIXFRAEALERTEREVENT()	IIFRaeAlerterEvent
**	IIXFRAFACTFLD()		IIFRafActFld
**	IIXFRITISTIMEOUT()	IIFRitIsTimeout
**	IIXFRGPCONTROL()	IIFRgpcontrol
**	IIXFRRERESENTRY()	IIFRreResEntry
**	IIXFRSHELP()		IIfrshelp
**	IIXFRTOACT()		IIFRtoact
**	IIXFSINQIO()		IIfsinqio
**	IIXNFRSKACT()		IInfrskact
**	IIXFSSETIO()		IIfssetio
**	IIXFSETIO()		IIfsetio
**	IIXGETOPER()		IIgetoper
**	IIXGTQRYIO()		IIgtqryio
**	IIXHELPFILE()		IIhelpfile
**	IIXINITMU()		IIinitmu
**	IIXIQSET()		IIiqset
**	IIXINQSET()		IIinqset
**	IIXIQFSIO()		IIiqfsio
**	IIXMESSAGE()		IImessage
**	IIXMUONLY()		IImuonly
**	IIXPRNSCR()		IIprnscr
**	IIXPUTOPER()		IIputoper
**	IIXREDISP()		IIredisp
**	IIXRESCOL()		IIrescol
**	IIXRESFLD()		IIresfld
**	IIXRESMU()		IIresmu
**	IIXRESNEXT()		IIresnext
**	IIXGETFLDIO()		IIgetfldio
**	IIXRETVAL()		IIretval
**	IIXRUNFORM()		IIrunform
**	IIXRUNMU()		IIrunmu
**	IIXPUTFLDIO()		IIputfldio
**	IIXSLEEP()		IIsleep
**	IIXSTFSIO()		IIstfsio
**	IIXVALFLD()		IIvalfld
**	IIXNESTMU()		IInestmu
**	IIXRUNNEST()		IIrunnest
**	IIXENDNEST()		IIendnest
**	IIXTBACT()		IItbact
**	IIXTBCACLMACT()		IITBcaClmAct
**	IIXTBCECOLEND()		IITBceColEnd
**	IIXTBINIT()		IItbinit
**	IIXTBSETIO()		IItbsetio
**	IIXTBSMODE()		IItbsmode
**	IIXTCLRCOL()		IItclrcol
**	IIXTCLRROW()		IItclrrow
**	IIXTCOLRET()		IItcolret
**	IIXTCOPUTIO()		IItcoputio
**	IIXTCOLVAL()		IItcolval
**	IIXTDATA()		IItdata
**	IIXTDATEND()		IItdatend
**	IIXTDELROW()		IItdelrow
**	IIXTFILL()		IItfill
**	IIXTHIDECOL()		IIthidecol
**	IIXTINSERT()		IItinsert
**	IIXTSCROLL()		IItscroll
**	IIXTUNEND()		IItunend
**	IIXTUNLOAD()		IItunload
**	IIXTVALROW()		IItvalrow
**	IIXTVALVAL() 		IItvalval
**
-*
** Notes:
**	This file is one of two files making up the runtime interface for
**	COBOL.   The files are:
**		iidgclbq.c	- libqsys directory
**		iidgcrun.c	- runsys directory
**	See complete notes on the runtime layer in iidgclbq.c.
**
**
** History:
**      09-feb-89 (sylviap)
**		Created from iiufrunt.c.
**      24-apr-89 (sylviap)
**		Changed var_ptr to be char * from i4  *.
**      23-may-89 (sylviap)
**		Fixed IIfrshelp parameters.
**	15-may-89 (teresal)
**		Added 6.4 calls IIFRafActFld, IITBcaClmAct, IIFRreResEntry.
**	10-aug-89 (teresal)
**		Added 6.4 call IITBceColEnd.
**      22-nov-89 (sylviap)
**              Changed calls to IIXACTSCRL from IIXACTSCR.
**                               IIXPUTOPER from IIXOUTOPER.
**	14-aug-1990 (barbara)
**		Added call IIFRsaSetAttrio for support of per value
**		attribute setting on LOADTABLE and INSERTROW (via WITH
**		clause).
**	21-apr-91 (teresal)
**		Add activate event call.
**      10-apr-1993 (kathryn)
**          Remove references to obsolete 3.0 routine: IIfmdatio. We no longer
**	    support rel. 3.0 calls. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



/*
** IIactclm
**	- ## activate column <tablefldname> <colname>
**	- Old style call (pre 8.0)
*/
void
IIXACTCLM( ret_val, table, column, intr )
char	*table;			/* Name of tablefield */
char	*column;		/* Name of column */
i4	*intr;			/* Display loop interrupt value */
i4	*ret_val;		/* Return value */
{
   *ret_val = IIactclm ( table, column, *intr );
   return; 
}


/*
** IITBcaClmAct
**	- ## activate [before/after] column <tablefldname> <colname>
**	- 8.0 version
*/
void
IIXTBCACLMACT( ret_val, table, column, entact, intr )
char	*table;			/* Name of tablefield */
char	*column;		/* Name of column */
i4	*entact;		/* Entry activation indicator */
i4	*intr;			/* Display loop interrupt value */
i4	*ret_val;		/* Return value */
{
   *ret_val = IITBcaClmAct ( table, column, *entact, *intr );
   return; 
}


/*
** IIactcomm
** 	- ## activate command <control-key>
*/
void
IIXACTCOMM( ret_val, ctrl, intr_val )
i4	*ctrl;				/* control value	*/
i4	*intr_val;			/* user interrupt value */
i4  	*ret_val;
{
    *ret_val = IIactcomm( *ctrl, *intr_val );
    return; 
}

/*
** IIactfld
**	- ## activate field <fldname>
**	- Old style call (pre 8.0)
*/
void
IIXACTFLD(ret_val,  fld_name, intr_val )
char    *fld_name;			/* name of the field to activate */
i4	*intr_val;			/* user's interrupt value	*/
i4      *ret_val;
{
    *ret_val = IIactfld( fld_name, *intr_val );
    return; 
}


/*
** IIFRafActFld
**	- ## activate [before/after] field <fldname>
**	- Version 8.0
*/
void
IIXFRAFACTFLD(ret_val,  fld_name, entact, intr_val )
char    *fld_name;			/* name of the field to activate */
i4	*entact;			/* Entry activation indicator */
i4	*intr_val;			/* user's interrupt value	*/
i4      *ret_val;
{
    *ret_val = IIFRacActFld( fld_name, *entact, *intr_val );
    return; 
}


/* 
** IInmuact
** 	- ## activate menuitem <itemname>
*/
void
IIXNMUACT( ret_val, menu_name, exp, val, act, intr )
char	*menu_name;		/* Name of menu item		*/
char	*exp;			/* Used within form system	*/
i4	*val;			/* Ordinal position in list of items */
i4	*act;			/* activation flag		*/
i4	*intr;			/* User's interrupt value	*/
i4      *ret_val;
{
    *ret_val = IInmuact( menu_name, exp, *val, *act, *intr );
    return; 
}

/*
** IIactscrl
**	- activate scroll[up/down] <tablefldname>
*/
void
IIXACTSCRL( ret_val, tab_name, mode, intr )
char	*tab_name;
i4	*mode;
i4	*intr;			/* Display loop interrupt value */
i4      *ret_val;
{
    *ret_val = IIactscrl( tab_name, *mode, *intr );
    return; 
}


/* 
** IIaddfrm
** 	- ## addform formname
*/
void
IIXADDFRM( ex_form )
i4	**ex_form;			/* external compiled frame structure */
{
    IIaddform( *ex_form );
}


/*
** IIchkfrm
**	- generated for validate
*/
void
IIXCHKFRM(ret_val)
i4  	*ret_val;
{
    *ret_val = IIchkfrm();
    return; 
}


/*
** IIclrflds
**	- ## clear field all
*/
void
IIXCLRFLDS()
{
    IIclrflds();
}

/*
** IIclrscr
**	- ## clear screen
*/
void
IIXCLRSCR()
{
    IIclrscr();
}

/*
** IIdispfrm
**	- ## display <formname>
*/
void
IIXDISPFRM( ret_val, form_name, mode )
char    *form_name;		/* form name	*/
char    *mode;			/* display mode */		
i4      *ret_val;
{
    *ret_val = IIdispfrm( form_name, mode );
    return; 
}


/*
** IIprmptio
** 	- ## prompt (<message> <response>)
*/
void
IIXPRMPTIO( echo, prompt, indflag, indptr, isvar, type, len, response )
i4	*echo;
char    *prompt;		/* prompt string		*/
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
char    *response;		/* response buffer		*/
{

    if ( !*indflag )
	indptr = (i2 *)0;

    IIprmptio( *echo, prompt, indptr, 1, DB_CHA_TYPE, *len, response );
}

/*
** IIenforms (IIendforms in VMS)
**	- ## endform
*/

void
IIXENDFORMS()
{
    IIendforms();
}

/*
** IIendfrm_
**	- end frame
*/

void
IIXENDFRM()
{
    IIendfrm();
}

/* 
** IIendmu
** 	- Clean up after adding user menuitems
*/
void
IIXENDMU(ret_val)
i4	*ret_val;
{
    *ret_val = IIendmu();
    return; 
}


/* 
** IIfldclear
**	- ## clear field <fieldname>
*/
void
IIXFLDCLEAR( fldname )
char	*fldname;
{
    IIfldclear( fldname );
}


/*
** IIfnames
** 	- Generated for ## Formdata statement
*/
void
IIXFNAMES(ret_val)
i4	*ret_val;
{
    *ret_val = IIfnames();
    return; 
}


/* 
** IIforminit
** 	- ## forminit <formname>
*/
void
IIXFORMINIT( form_name )
char    *form_name;			/* name of the form */
{
    IIforminit(form_name );
}


/*
** IIforms
** 	- ## forms
*/
/*ARGSUSED*/
void
IIXFORMS( lan )
i4     *lan;			/* which host language? */
{
    IIforms( *lan );
}

/*
** IIFRitIsTimeout
**	- generated for empty display loops.  If timeout ended display
**	  loop, validating (IIchkfrm) is skipped.
*/

void
IIXFRITISTIMEOUT(ret_val)
i4	*ret_val;
{
    *ret_val = IIFRitIsTimeout();
    return; 
}

/*
** IIfrshelp
**	- ## help_frs(subject = string, file = string)
*/
void
IIXFRSHELP( type, subject, filename )
i4	*type;			/* Type of help - currently always 0 */
char	*subject;
char	*filename;
{
    IIfrshelp( *type, subject, filename );
}


/*
** IIfsinqio
**	- Old-style inquire_frs statement
**	- ## inquire_frs 'object' (var = <frs_const>, ...)
**	     e.g., ## inquire_frs 'form' <formname> (var = 'mode')
**	- If receiving var is a string, blank pad.
*/

void
IIXFSINQIO( isvar, type, len, var_ptr, obj_name )
i4	    *isvar;			/* Always 1 for F77		*/
i4	    *type;			/* type of variable		*/
i4	    *len;			/* sizeof data, or strlen	*/
char	    *var_ptr;			/* pointer to inquiring var	*/
char	    *obj_name;			/* name of object to inquire	*/
{

    if (*type == DB_CHR_TYPE)
    {
	IIfsinqio( 1, DB_CHA_TYPE, *len, var_ptr, obj_name );
    }
    else
    {
        IIfsinqio( 1, *type, *len, var_ptr, obj_name );
    }
}

/*
** IIFRtoact
**	- ## activate timeout
*/
void
IIXFRTOACT( ret_val, val, intr )
i4	*val;		/* Timeout value - unused presently */
i4	*intr;		/* Display loop interrupt value */
i4      *ret_val;
{
	*ret_val = IIFRtoact( *val, *intr );
        return; 
}

/*
** IInfrskact
**	- ## activate frskeyN command
*/
void
IIXNFRSKACT( ret_val, frsnum, exp, val, act, intr )
i4	*frsnum;		/* Frskey number		*/
char	*exp;			/* Explanation string/var	*/
i4	*val;			/* Validate Value		*/
i4	*act;			/* activation flag		*/
i4	*intr;			/* Display loop interrupt value */
i4      *ret_val;
{
    	*ret_val = IInfrskact( *frsnum, exp, *val, *act, *intr );
        return; 
}


/* 
** IIfssetio
** 	- old-style set_frs data set command
**	  ## set_frs 'object' (frsconst = var/val, ...)
**	     e.g. ## set_frs 'form' f ('mode' = 'update')
*/
void
IIXFSSETIO( name, indflag, indptr, isvar, type, len, var_ptr )
char    *name;			/* the field name		*/
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
char    *var_ptr;
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
	IIfssetio( name, indptr, 1, *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE,
		   *len, var_ptr );
    else
	IIfssetio( name, indptr, 1, *type, *len, var_ptr );
}


/*
** IIfsetio
** 	- ## initialize statement
*/
void
IIXFSETIO( ret_val, name )
char    *name;			/* name of form to set up */
i4	*ret_val;
{
    if (*name == NULL)
       *ret_val = IIfsetio( NULL );
    else
       *ret_val = IIfsetio( name );
    return; 
}


/*
** IIgetoper
** 	- ## getoper(field)
*/
void
IIXGETOPER( one )
i4     *one;			/* Always has value of 1 */
{
    IIgetoper( 1 );
}

/*
** IIgtqryio
**	- ## getoper on a field for query mode.
*/
void
IIXGTQRYIO( indflag, indptr, isvar, type, len, var_ptr, qry_fld_name )
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;		/* variable for query op	*/
char    *qry_fld_name;		/* name of field for query	*/
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
	IIgtqryio( indptr, 1, DB_CHA_TYPE, *len, var_ptr, qry_fld_name);
    else
	IIgtqryio( indptr, 1, *type, *len, var_ptr, qry_fld_name );
}


/*
** IIhelpfile
** 	- ## helpfile <subject> <filename>
*/
void
IIXHELPFILE( subject, file_name )
char    *subject;			/* help subject */
char    *file_name;			/* name of the help file */
{
    IIhelpfile( subject, file_name );
}


/*
** IIinitmu
**	- Generated as part of display loop
*/
void
IIXINITMU(ret_val)
i4  	*ret_val;
{
    *ret_val = IIinitmu();
    return; 
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
void
IIXIQSET( ret_val, count, objtype, row, arg1, arg2, arg3, arg4 )
i4	*count; 		/* Number of string args */
i4	*objtype;
i4	*row;
char    *arg1, *arg2, *arg3, *arg4;	    
i4	*ret_val;
{
# define	IQ_ARGS_MAX	4		/* Max string args */
    char	*v_args[IQ_ARGS_MAX];
    i4		i;

    for (i = 0; i <IQ_ARGS_MAX; i++)
	v_args[i] = (char *)0;		/* initialize */
    switch ( *count - 2 ) {
      case 4:	v_args[3] = arg4;
      case 3:	v_args[2] = arg3;
      case 2:	v_args[1] = arg2;
      case 1:	v_args[0] = arg1;
    }
    *ret_val = IIiqset( *objtype, *row, v_args[0], v_args[1], v_args[2], v_args[3] );
    return; 
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
void
IIXINQSET( ret_val, count, arg1, arg2, arg3, arg4, arg5 )
i4	*count;
char    *arg1, *arg2, *arg3, *arg4, *arg5;	    
i4	*ret_val;
{
# define	INQ_ARGS_MAX	5
    char	*v_args[INQ_ARGS_MAX];
    i4		i;

    for (i = 0; i < INQ_ARGS_MAX; i++)
	v_args[i] = (char *)0;
    switch ( *count ) {
      case 5:	v_args[4] = arg5;
      case 4:	v_args[3] = arg4;
      case 3:	v_args[2] = arg3;
      case 2:	v_args[1] = arg2;
      case 1:	v_args[0] = arg1;
    }
    *ret_val = IIinqset( v_args[0], v_args[1], v_args[2], v_args[3], v_args[4] );
    return; 
}

/*
** IIiqfsio
**	- ## inquire_frs 'object' <formname> (var = frsconst)
**	- If receiving var is a string, copy and blank pad.
*/
void
IIXIQFSIO( indflag, indptr, isvar, type, len, var_ptr, attr, name, row )
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;		/* Pointer to inquiring var	*/
i4	*attr;			/* Object type			*/
char	*name;			/* Table name			*/
i4	*row;			/* Row number			*/
{

    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
    	if (*name == NULL)
	   IIiqfsio( indptr, 1, DB_CHA_TYPE, *len, var_ptr, *attr, 
	   NULL, *row );
	else
	   IIiqfsio( indptr, 1, DB_CHA_TYPE, *len, var_ptr, *attr, 
           name, *row );
    }
    else
    {
    	if (*name == NULL)
	   IIiqfsio( indptr, 1, *type, *len, var_ptr, *attr, NULL, *row );
	else
	   IIiqfsio( indptr, 1, *type, *len, var_ptr, *attr, name, *row );
    }
}

/*
** IImessage
**	- ## message
*/
void
IIXMESSAGE( msg_str )
char    *msg_str;			/* the message string */
{
    IImessage( msg_str );
}


/* 
** IImuonly
**	- Generated in ## submenu loop
*/
void
IIXMUONLY()
{
    IImuonly();
}


/*
** IIprnscr
**	- ## printscreen [(file = filename)]
*/
void
IIXPRNSCR( name )
char	*name;
{
    if (*name == NULL)
	IIprnscr( NULL );
    else
	IIprnscr( name );

}


/*
** IIputoper
**	- generated for putoper function
*/
void
IIXPUTOPER( one )
i4	*one;
{
    IIputoper( 1 );
}


/*
** IIredisp
*/
void
IIXREDISP()
{
    IIredisp();
}


/*
** IIrescol
**	- ## resume column <tablefield> <column>
*/
void
IIXRESCOL( tabname, colname )
char	*tabname;
char	*colname;
{
    IIrescol( tabname, colname);
}


/*
** IIresfld
** 	- ## resume <fldname>
*/
void
IIXRESFLD( fld_name )
char    *fld_name;			/* field name */
{
    IIresfld( fld_name );
}


/*
** IIresmu
**	- ## resume menu
*/
void
IIXRESMU()
{
    IIresmu();
}


/*
** IIresnext
** 	- ## resume
*/
i4
IIXRESNEXT()
{
    IIresnext();
}


/*
** IIFRreResEntry
**	- ## resume entry
*/
void
IIXFRRERESENTRY()
{
    IIFRreResEntry();
}


/*
** IIgetfldio
** 	- ## getform I/O
**	- If receiving variable is a string, copy and blank pad.
*/

void
IIXGETFLDIO( indflag, indptr, isvar, type, len, var_ptr, fld_name )
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;		/* pointer to retrieval var	*/
char    *fld_name;		/* name of field to retrieve	*/
{

    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
	IIgetfldio( indptr, 1, DB_CHA_TYPE, *len, var_ptr, fld_name );
    }
    else
    {
	IIgetfldio( indptr, 1, *type, *len, var_ptr, fld_name );
    }
}


/*
** IIretval
**	- return value
*/
void
IIXRETVAL(ret_val)
i4	*ret_val;
{
    *ret_val = IIretval();
    return; 
}




/* IIrunform
** 	- Generated for loop display
*/
void
IIXRUNFORM(ret_val)
i4	*ret_val;
{
    *ret_val = IIrunform();
    return; 
}


/*
** IIrunmu_
**	- IIrunmu
**	- call frame driver to display a nested menu.
**	  if single_item is set then allow reentry to form for the user,
**	  otherwise treat as a multi-item prompt.
*/

void
IIXRUNMU( single_item )
i4	*single_item;		/* single-entry boolean */
{
    IIrunmu( *single_item );
}

/*
** IIputfldio
**	## initialize(fld = var, ...)
*/
void
IIXPUTFLDIO( name, indflag, indptr, isvar, type, len, var_ptr )
char	*name;			/* the field name		*/
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
char    *var_ptr;		/* ptr to data for setting	*/
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
	IIputfldio( name, indptr, 1, *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE,
		    *len, var_ptr );
    else
	IIputfldio( name, indptr, 1, *type, *len, var_ptr );
}




/*
** IIsleep
**	## sleep
*/
void
IIXSLEEP( seconds )
i4	    *seconds;			/* number of seconds to sleep */
{
    IIsleep( *seconds );
}


/* 
** IIstfsio
**	- ## set_frs statement
*/
void
IIXSTFSIO( attr, name, row, indflag, indptr, isvar, type, len, var_ptr )
i4	*attr;
char	*name;
i4	*row;
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
	IIstfsio( *attr, name, *row, 1, *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE,
		  *len, var_ptr );
    else
	IIstfsio( *attr, name, *row, *isvar, *type, *len, var_ptr );
}

/* 
** IIFRsaSetAttrio
**   -	loadtable/insertrow with clause for per value attribute setting.
**	e.g.,
**	exec frs loadtable f t (col1 = val) with (reverse(col1) = 1);
*/
void
IIXFRSASETATTRIO( attr, name, indflag, indptr, isvar, type, len, var_ptr )
i4	*attr;
char	*name;
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
	IIFRsaSetAttrio( *attr, name, 1, *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE,
		  *len, var_ptr );
    else
	IIFRsaSetAttrio( *attr, name, *isvar, *type, *len, var_ptr );
}

/* 
** IIvalfld
** 	- ## validate
*/
void
IIXVALFLD( ret_val, fld_name )
char    *fld_name;			/* name of the field */
i4	*ret_val;
{
    *ret_val = IIvalfld( fld_name );
    return; 
}

/*
** IInestmu
** 	- ## validate
*/
void
IIXNESTMU(ret_val)
i4	*ret_val;
{
    *ret_val = IInestmu();
    return; 
}

/*
** IIrunnest
** 	- ## validate
*/
void
IIXRUNNEST(ret_val)
i4	*ret_val;
{
    *ret_val = IIrunnest();
    return; 
}

/*
** IIendnest
** 	- ## validate
*/
void
IIXENDNEST(ret_val)
i4	*ret_val;
{
    *ret_val = IIendnest();
    return; 
}

/*
** IIFRgpcontrol
**	- POPUP forms control
*/
void
IIXFRGPCONTROL( state, modifier )
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
IIXFRGPSETIO( id, indflag, indptr, isvar, type, len, var_ptr )
i4	*id;			/* internal clause id		*/
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
	IIFRgpsetio( *id, indptr, 1, *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE,
		     *len, var_ptr );
    else
	IIFRgpsetio( *id, indptr, 1, *type, *len, var_ptr );
}

/*
** IItbact
**	- sets up data set for loading/unloading of a linked table field
*/
void
IIXTBACT( ret_val, form_name, tbl_fld_name, load_flg )
char	*form_name;			/* name of form */
char	*tbl_fld_name;			/* name of table field */
i4	*load_flg;			/* load/unload flag */
i4	*ret_val;
{
    *ret_val = IItbact( form_name, tbl_fld_name, *load_flg );
    return; 
}

/*
** IItbinit
**	- initialize a table field with a data set
**	- ## inittable formname tablefldname
*/
void
IIXTBINIT( ret_val, form_name, tbl_fld_name, tf_mode )
char	*form_name;			/* name of form */
char	*tbl_fld_name;			/* name of table field */
char    *tf_mode;			/* mode for table field */
i4	*ret_val;
{
    *ret_val = IItbinit( form_name, tbl_fld_name, tf_mode );
    return; 
}

/*
** IItbsetio
**	- set up table field structures before doing row I/O
**	- used on most row statements
*/
void
IIXTBSETIO( ret_val, mode, form_name, tbl_fld_name, row_num )
i4	*mode;				/* command mode */
char    *form_name;			/* form name */
char    *tbl_fld_name;			/* table-field name */
i4	*row_num;			/* row number */
i4	*ret_val;
{
    *ret_val = IItbsetio( *mode, form_name, tbl_fld_name, *row_num );
    return; 
}


/*
** IItbsmode
**	- set up the mode scrolling a table field
*/
void
IIXTBSMODE( mode )
char    *mode;			/* up/down/to scroll mode */
{
    IItbsmode( mode );
}

/*
** IItclrcol
**	- clear a specific column in a specified row
**	- ## clearrow formname tablename 1 (col1, col2)
*/
void
IIXTCLRCOL( col_name )
char    *col_name;			/* name of column to clear */
{
    IItclrcol( col_name );
}


/*
** IIclrrow
**	- clear a specified row (or current row) in a tablefield
**	- ## clearrow formname tablename 3
*/
void
IIXTCLRROW()
{
    IItclrrow();
}


/*
** IItcogetio
**	- ## getrow formname tablefldname (var = column, ...) 
**	- also generated for OUT clauses on ## deleterow statements.
*/

void
IIXTCOGETIO( indflag, indptr, isvar, type, len, var_ptr, col_name )
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;		/* pointer to retrieval variable */
char	*col_name;		/* name of column to retrieve	*/
{

    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
	IItcogetio( indptr, 1, DB_CHA_TYPE, *len, var_ptr, col_name );
    }
    else
    {
	IItcogetio( indptr, 1, *type, *len, var_ptr, col_name );
    }
}

/*
** IItcoputio
**	- ## putrow formname tablefldname (colname = var, ...)
**	- Also generated for IN clauses on ## deleterow statements.
*/
void
IIXTCOPUTIO( name, indflag, indptr, isvar, type, len, var_ptr )
char	*name;			/* the field name		*/
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;		/* ptr to data for setting	*/
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
	IItcoputio( name, indptr, 1, *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE,
		    *len, var_ptr);
    else
	IItcoputio( name, indptr, 1, *type, *len, var_ptr );
}


/*
** IItcolval
**	- ## validrow formname tablefldname row (col1, col2)
*/
void
IIXTCOLVAL( colname )
char	*colname;
{
    IItcolval( colname );
}


/*
** IItdata
**	- ## tabledata
*/
void
IIXTDATA(ret_val)
i4	*ret_val;
{
    *ret_val = IItdata();
    return; 
}


/*
** IItdatend
**	- Clean up after tabledata loop
*/
void
IIXTDATEND()
{
    IItdatend();
}


/*
** IItdelrow
**	- ## deleterow formname tablename [row] [out/in clause]
*/
void
IIXTDELROW( ret_val, in_list )
i4	*in_list;			/* IN-list boolean */
i4	*ret_val;
{
    *ret_val = IItdelrow( *in_list );
    return; 
}


/*
** IItfill
**	- generated by ## inittable statements
*/
void
IIXTFILL()
{
    IItfill();
}


/*
** IIhidecol
**	- ## inittable statement with hidden column clause
*/
void
IIXTHIDECOL( col_name, format )
char	*col_name;			/* column name		*/
char	*format;			/* col type format string */
{
    IIthidecol( col_name, format );
}


/*
** IItinsert
**	- ## insertrow formname tablefldname [row] [target list]
*/
void
IIXTINSERT(ret_val)
i4	*ret_val;
{
    *ret_val = IItinsert();
    return; 
}


/*
** IItscroll
**	- set up calls to actual scrolling routines
*/
void
IIXTSCROLL( ret_val, in, rec_num )
i4	*in;			/* IN-list boolean		*/
i4	*rec_num;		/* record number for scroll TO	*/
i4	*ret_val;
{
    *ret_val = IItscroll( *in, *rec_num );
    return; 
}


/*
** IItunend
**	- generated to clean up after ## unloadtable
*/
void
IIXTUNEND()
{
    IItunend();
}



/*
** IItunload
**	- table unload
*/
void
IIXTUNLOAD(ret_val)
i4	*ret_val;
{
    *ret_val = IItunload();
    return; 
}

/* 
** IItvalrow
**	- ##  validrow command
*/
void
IIXTVALROW()
{
    IItvalrow();
}

/* 
** IITBceColEnd
**	- indicates there are no more columns values to be set.
*/
void
IIXTBCECOLEND()
{
    IITBceColEnd();
}

/*
** IItvalval
**	- ## validrow command
*/
void
IIXTVALVAL( ret_val, one )
i4	*one;			/* Always has value of 1 */
i4	*ret_val;
{
    *ret_val = IItvalval( 1 );
    return; 
}
/*
** IIFRsqExecute
**	- EXEC FRS PUTFORM :f1 USING :sqlda
**	- EXEC FRS GETFORM USING :sqlda
**	- EXEC FRS PUTROW :f1 :t1 :r1 USING :sqlda
**	- EXEC FRS UNLOADTABLE :f1 :t1 USING :sqlda
*/
void
IIXFRSQEXECUTE( lang, is_form, is_input, sqd )
i4	*lang;			/* language */
i4	*is_form;
i4	*is_input;			
PTR	*sqd;
{
    	IIFRsqExecute( *lang, *is_form, *is_input, sqd );
}

/*
** IIFRsqDescribe
**	- EXEC FRS DESCRIBE FORM f1 UPDATE INTO :sqlda
*/
void
IIXFRSQDESCRIBE( lang, is_form, fname, tname, mode, sqd )
i4	*lang;			/* language */
i4	*is_form;
char	*fname;			/* form name */
char	*tname;			/* table name */
char	*mode;			
PTR	*sqd;
{
    	IIFRsqDescribe( *lang, *is_form, fname, tname, mode, sqd );
}

/*
** IIFRaeAlerterEvent
**	- ## activate event
*/
void
IIXFRAEALERTEREVENT( ret_val, intr )
i4	*intr;		/* Display loop interrupt value */
i4      *ret_val;
{
	*ret_val = IIFRaeAlerterEvent( *intr );
        return; 
}

# endif /* DGC_AOS*/
