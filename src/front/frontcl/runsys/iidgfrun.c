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
+* Filename:	iidgfrun.c
** Purpose:	Runtime (forms) translation layer for DG AOS/VS F77 programs.
**
** Defines:
**	IIACTCLM()		IIactclm
**	IIACTCOMM()		IIactcomm
**	IIACTFLD()		IIactfld
**	IINMUACT()		IInmuact
**	IIACTSCRL()		IIactscrl
**	IIADDFRM()		IIaddfrm
**	IICLRSCR()		IIchkfrm
**	IICLRFLDS()		IIclrflds
**	IICLRSCR()		IIclrscr
**	IIDISPFRM()		IIdispfrm
**	IIPRMPTIO()		IIprmptio
**	IIENDFORMS()		IIendforms
**	IIENDFRM()		IIendfrm
**	IIENDMU()		IIendmu
**	IIFRGPSETIO()		IIFRgpsetio
**	IIFLDCLEAR()		IIfldclear
**	IIFNAMES()		IIfnames
**	IIFORMINIT()		IIforminit
**	IIFORMS()		IIforms
**      IIFRAEALERTEREVENT()    IIFRaeAlerterEvent
**      IIFRAFACTFLD()          IIFRafActFld
**	IIFRITISTIMEOUT()	IIFRitIsTimeout
**	IIFRGPCONTROL()		IIFRgpcontrol
**      IIFRRERESENTRY()        IIFRreResEntry
**	IIFRSASETATTRIO()	IIFRsaSetAttrio
**	IIFRSHELP()		IIfrshelp
**	IIFRTOACT()		IIFRtoact
**	IIFSINQIO()		IIfsinqio
**	IINFRSKACT()		IInfrskact
**	IIFSSETIO()		IIfssetio
**	IIFSETIO()		IIfsetio
**	IIGETOPER()		IIgetoper
**	IIGTQRYIO()		IIgtqryio
**	IIHELPFILE()		IIhelpfile
**	IIINITMU()		IIinitmu
**	IIIQSET()		IIiqset
**	IIINQSET()		IIinqset
**	IIIQFSIO()		IIiqfsio
**	IIMESSAGE()		IImessage
**	IIMUONLY()		IImuonly
**	IIPRNSCR()		IIprnscr
**	IIPUTOPER()		IIputoper
**	IIREDISP()		IIredisp
**	IIRESCOL()		IIrescol
**	IIRESFLD()		IIresfld
**	IIRESMU()		IIresmu
**	IIRESNEXT()		IIresnext
**	IIGETFLDIO()		IIgetfldio
**	IIRETVAL()		IIretval
**	IIRUNFORM()		IIrunform
**	IIRUNMU()		IIrunmu
**	IIPUTFLDIO()		IIputfldio
**	IISLEEP()		IIsleep
**	IISTFSIO()		IIstfsio
**	IIVALFLD()		IIvalfld
**	IINESTMU()		IInestmu
**	IIRUNNEST()		IIrunnest
**	IIENDNEST()		IIendnest
**	IITBACT()		IItbact
**      IITBCACLMACT()          IITBcaClmAct
**      IITBCECOLEND()          IITBceColEnd
**	IITBINIT()		IItbinit
**	IITBSETIO()		IItbsetio
**	IITBSMODE()		IItbsmode
**	IITCLRCOL()		IItclrcol
**	IITCLRROW()		IItclrrow
**	IITCOLRET()		IItcolret
**	IITCOPUTIO()		IItcoputio
**	IITCOLVAL()		IItcolval
**	IITDATA()		IItdata
**	IITDATEND()		IItdatend
**	IITDELROW()		IItdelrow
**	IITFILL()		IItfill
**	IITHIDECOL()		IIthidecol
**	IITINSERT()		IItinsert
**	IITSCROLL()		IItscroll
**	IITUNEND()		IItunend
**	IITUNLOAD()		IItunload
**	IITVALROW()		IItvalrow
**	IITVALVAL() 		IItvalval
**
-*
** Notes:
**	This file is one of two files making up the runtime interface for
**	F77   The files are:
**		iidgflbq.c	- libqsys directory
**		iidgfrun.c	- runsys directory
**	See complete notes on the runtime layer in iidgflbq.c.
**
**
** History:
**      22-may-89 (sylviap)
**		Created from iidgcrun.c.
**	24-oct-89 (sylviap)
**		Added 6.4 calls IIFRafActFld, IITBcaClmAct, IIFRreResEntry and
**		IITBceColEnd.
**      22-nov-89 (sylviap)
**              Changed calls to IIACTSCRL from IIACTSCR.
**                               IIPUTOPER from IIOUTOPER.
**	14-aug-1990 (barbara)
**		Added interface for IIFRsaSetAttrio for per value attribute
**	setting on INSERTROW and LOADTABLE (via WITH clause).
**	11-sep-90 (teresal) 
**		Modified I/O calls to accept new decimal 
**              data type DB_DEC_CHR_TYPE (53) which is a decimal 
**              number encoded as a character string. For 
**              completeness, all current I/O routines have been 
**              updated. 
**      19-sep-90 (kathryn)
**          Changed iifrsa_() and iistfs_(), Not to pass "isvar" to the
**          forms system. "isvar" indicates whether var_ptr was passed in
**          by reference or by value. F77 always passes by reference.
**	21-apr-91 (teresal)
**	    Add activate event call.
**      10-apr-1993 (kathryn)
**          Remove references to obsolete 3.0 routine: IIfmdatio. We no longer
**          support rel. 3.0 calls.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



/*
** IIactclm
**	- ## activate column <tablefldname> <colname>
**      - Old style call (pre 6.4)
*/
i4
IIACTCLM( t_table, t_column, t_intr )
u_i4	t_table;			/* Name of tablefield */
u_i4	t_column;			/* Name of column */
u_i4	t_intr;			/* Display loop interrupt value */
{
	char	**table;			
	char	**column;	
	i4	*intr;	

   /* double pointer because of DG commonformat */
   table = (char **)(2 * t_table);
   column = (char **)(2 * t_column);
   intr = (i4 *)(2 * t_intr);

   return IIactclm ( *table, *column, *intr );
}
 
/*
** IITBcaClmAct
**      - ## activate [before/after] column <tablefldname> <colname>
**      - 6.4 version
*/
void
IITBCACLMACT( table, column, entact, intr )
u_i4    t_table;                 /* Name of tablefield */
u_i4    t_column;                /* Name of column */
u_i4    t_entact;                /* Entry activation indicator */
u_i4    t_intr;                  /* Display loop interrupt value */
{
	char   **table;                 
	char   **column;               
	i4    *entact;               
	i4    *intr;                

   /* double pointer because of DG commonformat */
   table = (char **)(2 * t_table);
   column = (char **)(2 * t_column);
   entact = (i4 *)(2 * t_entact);
   intr = (i4 *)(2 * t_intr);

   return IITBcaClmAct ( *table, *column, *entact, *intr );
}
	

/*
** IIactcomm
** 	- ## activate command <control-key>
*/
i4
IIACTCOMM( t_ctrl, t_intr_val )
u_i4	t_ctrl;				/* control value	*/
u_i4	t_intr_val;			/* user interrupt value */
{
	i4	*ctrl;				
	i4	*intr_val;		

   /* double pointer because of DG commonformat */
   ctrl = (i4 *)(2 * t_ctrl);
   intr_val = (i4 *)(2 * t_intr_val);

   return IIactcomm( *ctrl, *intr_val );
}

/*
** IIactfld
**	- ## activate field <fldname>
*/
i4
IIACTFLD(t_fld_name, t_intr_val )
u_i4   t_fld_name;			/* name of the field to activate */
u_i4	t_intr_val;			/* user's interrupt value	*/
{
	char    **fld_name;			
	i4	*intr_val;		

   /* double pointer because of DG commonformat */
   intr_val = (i4 *)(2 * t_intr_val);
   fld_name = (char **)(2 * t_fld_name);

   return IIactfld( *fld_name, *intr_val );
}



/*
** IIFRafActFld
**      - ## activate [before/after] field <fldname>
**      - Version 6.4
*/
void
IIFRAFACTFLD(fld_name, entact, intr_val )
u_i4     t_fld_name;                      /* name of the field to activate */
u_i4     t_entact;                        /* Entry activation indicator */
u_i4     t_intr_val;                      /* user's interrupt value       */
{
	char    **fld_name;                      
	i4     *entact;                       
	i4     *intr_val;                    

   /* double pointer because of DG commonformat */
   fld_name = (char **)(2 * t_fld_name);
   entact = (i4 *)(2 * t_entact);
   intr_val = (i4 *)(2 * t_intr_val);

   return IIFRacActFld( *fld_name, *entact, *intr_val );
}
	 

/* 
** IInmuact
** 	- ## activate menuitem <itemname>
*/
i4
IINMUACT( t_menu_name, t_exp, t_val, t_act, t_intr )
u_i4	t_menu_name;		/* Name of menu item		*/
u_i4	t_exp;			/* Used within form system	*/
u_i4	t_val;			/* Ordinal position in list of items */
u_i4	t_act;			/* activation flag		*/
u_i4	t_intr;			/* User's interrupt value	*/
{
	char	**menu_name;		
	char	**exp;		
	i4	*val;	
	i4	*act;
	i4	*intr;	

   /* double pointer because of DG commonformat */
   menu_name = (char **)(2 * t_menu_name);
   exp = (char **)(2 * t_exp);
   val = (i4 *)(2 * t_val);
   act = (i4 *)(2 * t_act);
   intr = (i4 *)(2 * t_intr);

   return IInmuact( *menu_name, *exp, *val, *act, *intr );
}

/*
** IIactscrl
**	- activate scroll[up/down] <tablefldname>
*/
i4
IIACTSCRL( t_tab_name, t_mode, t_intr )
u_i4	t_tab_name;
u_i4	t_mode;
u_i4	t_intr;			/* Display loop interrupt value */
{
	char	**tab_name;
	i4	*mode;
	i4	*intr;			/* Display loop interrupt value */

   /* double pointer because of DG commonformat */
   tab_name = (char **)(2 * t_tab_name);
   mode = (i4 *)(2 * t_mode);
   intr = (i4 *)(2 * t_intr);

   return IIactscrl( *tab_name, *mode, *intr );
}


/* 
** IIaddfrm
** 	- ## addform formname
*/
void
IIADDFRM( t_ex_form )
u_i4	t_ex_form;			/* external compiled frame structure */
{
	i4	***ex_form;

   /* double pointer because of DG commonformat */
   ex_form = (i4 **)(2 * t_ex_form);

   IIaddform( **ex_form );
}


/*
** IIchkfrm
**	- generated for validate
*/
i4
IICHKFRM()
{
    return IIchkfrm();
}


/*
** IIclrflds
**	- ## clear field all
*/
void
IICLRFLDS()
{
    IIclrflds();
}

/*
** IIclrscr
**	- ## clear screen
*/
void
IICLRSCR()
{
    IIclrscr();
}

/*
** IIdispfrm
**	- ## display <formname>
*/
i4
IIDISPFRM( t_form_name, t_mode )
u_i4    t_form_name;		/* form name	*/
u_i4    t_mode;			/* display mode */		
{
	char    **form_name;		/* form name	*/
	char    **mode;			/* display mode */		

   /* double pointer because of DG commonformat */
   form_name = (char **)(2 * t_form_name);
   mode = (char **)(2 * t_mode);

    return IIdispfrm( *form_name, *mode );
}


/*
** IIprmptio
** 	- ## prompt (<message> <response>)
*/
void
IIPRMPTIO( t_echo, t_prompt, t_indflag, t_indptr, t_isvar, t_type, 
	   t_len, response )
u_i4	t_echo;
u_i4   t_prompt;		/* prompt string		*/
u_i4	t_indflag;		/* 0 = no null pointer		*/
u_i4	t_indptr;		/* null pointer			*/
u_i4	t_isvar;		/* by value or reference	*/
u_i4	t_type;			/* type of user variable	*/
u_i4	t_len;			/* sizeof data, or strlen	*/
u_i4   response;		/* response buffer		*/
				/* F77 passes the pointer to 
				** variable so don't double 
				** address	
				*/
{
	i4	*echo;
	char    **prompt;
	i4	*indflag;
	i2	*indptr;
	i4	*isvar;
	i4	*type;
	i4	*len;


   /* double pointer because of DG commonformat */
   echo = (i4 *)(2 * t_echo);
   prompt = (char **)(2 * t_prompt);
   indflag = (i4 *)(2 * t_indflag);
   indptr = (i2 *)(2 * t_indptr);
   isvar = (i4 *)(2 * t_isvar);
   type = (i4 *)(2 * t_type);
   len = (i4 *)(2 * t_len);

    if ( !*indflag )
	indptr = (i2 *)0;

    IIprmptio( *echo, *prompt, indptr, 1, DB_CHA_TYPE, *len, (char *)response );
}

/*
** IIenforms (IIendforms in VMS)
**	- ## endform
*/

void
IIENDFORMS()
{
    IIendforms();
}

/*
** IIendfrm_
**	- end frame
*/

void
IIENDFRM()
{
    IIendfrm();
}

/* 
** IIendmu
** 	- Clean up after adding user menuitems
*/
i4
IIENDMU()
{
    return IIendmu();
}


/* 
** IIfldclear
**	- ## clear field <fieldname>
*/
void
IIFLDCLEAR( t_fldname )
u_i4	t_fldname;
{
	char	**fldname;

   /* double pointer because of DG commonformat */
   fldname = (char **)(2 * t_fldname);

   IIfldclear( *fldname );
}


/*
** IIfnames
** 	- Generated for ## Formdata statement
*/
i4
IIFNAMES()
{
    return IIfnames();
}


/* 
** IIforminit
** 	- ## forminit <formname>
*/
void
IIFORMINIT( t_form_name )
u_i4    t_form_name;			/* name of the form */
{
	char    **form_name;			

   /* double pointer because of DG commonformat */
   form_name = (char **)(2 * t_form_name);

   IIforminit(*form_name );
}


/*
** IIforms
** 	- ## forms
*/
void
IIFORMS( t_lan )
u_i4    t_lan;			/* which host language? */
{
	i4    *lan;

   /* double pointer because of DG commonformat */
   lan = (i4 *)(2 * t_lan);

   IIforms( *lan );
}

/*
** IIFRitIsTimeout
**	- generated for empty display loops.  If timeout ended display
**	  loop, validating (IIchkfrm) is skipped.
*/

i4
IIFRITISTIMEOUT()
{
    return IIFRitIsTimeout();
}

/*
** IIfrshelp
**	- ## help_frs(subject = string, file = string)
*/
void
IIFRSHELP( t_type, t_subject, t_filename )
u_i4	t_type;			/* Type of help - currently always 0 */
u_i4	t_subject;
u_i4	t_filename;
{
	i4	*type;
	char	**subject;
	char	**filename;

   /* double pointer because of DG commonformat */
   type = (i4 *)(2 * t_type);
   subject = (char **)(2 * t_subject);
   filename = (char **)(2 * t_filename);

   IIfrshelp( *type, *subject, *filename );
}


/*
** IIfsinqio
**	- Old-style inquire_frs statement
**	- ## inquire_frs 'object' (var = <frs_const>, ...)
**	     e.g., ## inquire_frs 'form' <formname> (var = 'mode')
**	- If receiving var is a string, blank pad.
*/

void
IIFSINQIO( t_isvar, t_type, t_len, var_ptr, t_obj_name )
u_i4	    t_isvar;			/* Always 1 for F77		*/
u_i4	    t_type;			/* type of variable		*/
u_i4	    t_len;			/* sizeof data, or strlen	*/
u_i4	    var_ptr;			/* pointer to inquiring var	*/
					/* F77 passes the pointer to 
					** variable so don't double 
					** address			*/
u_i4	    t_obj_name;			/* name of object to inquire	*/
{
	i4	    *isvar;		
	i4	    *type;	
	i4	    *len;
	char	    **obj_name;

   /* double pointer because of DG commonformat */
   isvar = (i4 *)(2 * t_isvar);
   type = (i4 *)(2 * t_type);
   len = (i4 *)(2 * t_len);
   obj_name = (char **)(2 * t_obj_name);


    if (*type == DB_CHR_TYPE)
    {
	IIfsinqio( 1, DB_CHA_TYPE, *len, (char *)var_ptr, *obj_name );
    }
    else
    {
        var_ptr = (2 * var_ptr);
        IIfsinqio( 1, *type, *len, var_ptr, *obj_name );
    }
}

/*
** IIFRtoact
**	- ## activate timeout
*/
i4
IIFRTOACT( t_val, t_intr )
u_i4	t_val;		/* Timeout value - unused presently */
u_i4	t_intr;		/* Display loop interrupt value */
{
	i4	*val;
	i4	*intr;

   /* double pointer because of DG commonformat */
   val = (i4 *)(2 * t_val);
   intr = (i4 *)(2 * t_intr);

   return IIFRtoact( *val, *intr );
}

/*
** IInfrskact
**	- ## activate frskeyN command
*/
i4
IINFRSKACT( t_frsnum, t_exp, t_val, t_act, t_intr )
u_i4	t_frsnum;		/* Frskey number		*/
u_i4	t_exp;			/* Explanation string/var	*/
u_i4	t_val;			/* Validate Value		*/
u_i4	t_act;			/* activation flag		*/
u_i4	t_intr;			/* Display loop interrupt value */
{
	i4	*frsnum;
	char    **exp;
	i4	*val;
	i4	*act;
	i4	*intr;


   /* double pointer because of DG commonformat */
   frsnum = (i4 *)(2 * t_frsnum);
   exp = (char **)(2 * t_exp);
   val = (i4 *)(2 * t_val);
   act = (i4 *)(2 * t_act);
   intr = (i4 *)(2 * t_intr);

   return IInfrskact( *frsnum, *exp, *val, *act, *intr );
}


/* 
** IIfssetio
** 	- old-style set_frs data set command
**	  ## set_frs 'object' (frsconst = var/val, ...)
**	     e.g. ## set_frs 'form' f ('mode' = 'update')
*/
void
IIFSSETIO( t_name, t_indflag, t_indptr, t_isvar, t_type, t_len, var_ptr )
u_i4   t_name;			/* the field name		*/
u_i4	t_indflag;		/* 0 = no null pointer		*/
u_i4	t_indptr;		/* null pointer			*/
u_i4	t_isvar;		/* by value or reference	*/
u_i4	t_type;			/* type of user variable	*/
u_i4	t_len;			/* sizeof data, or strlen	*/
u_i4   var_ptr;	        /* F77 passes the pointer to 
				** variable so don't double 
				** address			*/
{
	char        **name;
	i4	    *indflag;
	i2	    *indptr;
	i4	    *isvar;		
	i4	    *type;	
	i4	    *len;

   /* double pointer because of DG commonformat */
   name = (char **)(2 * t_name);
   indflag = (i4 *)(2 * t_indflag);
   indptr = (i2 *)(2 * t_indptr);
   isvar = (i4 *)(2 * t_isvar);
   type = (i4 *)(2 * t_type);
   len = (i4 *)(2 * t_len);


    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
	IIfssetio( *name, indptr, 1, *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE,
		   *len, (char *)var_ptr );
    }
    else
    {
        var_ptr = (2 * var_ptr);
	IIfssetio( *name, indptr, 1, *type, *len, var_ptr );
    }
}


/*
** IIfsetio
** 	- ## initialize statement
*/
i4
IIFSETIO( t_name )
u_i4    t_name;			/* name of form to set up */
{
	char	**name;

    /* double pointer because of DG commonformat */
    name = (char **)(2 * t_name);

    if (*name == NULL)
       return IIfsetio( NULL );
    else
       return IIfsetio( *name );
}


/*
** IIgetoper
** 	- ## getoper(field)
*/
void
IIGETOPER( t_one )
u_i4    t_one;			/* Always has value of 1 */
{
    IIgetoper( 1 );
}

/*
** IIgtqryio
**	- ## getoper on a field for query mode.
*/
void
IIGTQRYIO( t_indflag, t_indptr, t_isvar, t_type, t_len, 
	   var_ptr, t_qry_fld_name )
u_i4	t_indflag;		/* 0 = no null pointer		*/
u_i4	t_indptr;		/* null pointer			*/
u_i4	t_isvar;		/* by value or reference	*/
u_i4	t_type;			/* type of user variable	*/
u_i4	t_len;			/* sizeof data, or strlen	*/
u_i4	var_ptr;		/* variable for query op	*/
				/* F77 passes the pointer to 
				** variable so don't double 
				** address			*/
u_i4   t_qry_fld_name;		/* name of field for query	*/
{
	i4	    *indflag;
	i2	    *indptr;
	i4	    *isvar;		
	i4	    *type;	
	i4	    *len;
	char        **qry_fld_name;

   /* double pointer because of DG commonformat */
   indflag = (i4 *)(2 * t_indflag);
   indptr = (i2 *)(2 * t_indptr);
   isvar = (i4 *)(2 * t_isvar);
   type = (i4 *)(2 * t_type);
   len = (i4 *)(2 * t_len);
   qry_fld_name = (char **)(2 * t_qry_fld_name);

    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
	IIgtqryio( indptr, 1, DB_CHA_TYPE, *len, (char *)var_ptr, *qry_fld_name);
    }
    else if (*type == DB_DEC_CHR_TYPE)
    {
	IIgtqryio( indptr, 1, *type, *len, (char *)var_ptr, *qry_fld_name);
    }	
    else
    {
        var_ptr = (2 * var_ptr);
	IIgtqryio( indptr, 1, *type, *len, var_ptr, *qry_fld_name );
    }
}


/*
** IIhelpfile
** 	- ## helpfile <subject> <filename>
*/
void
IIHELPFILE( t_subject, t_file_name )
u_i4    t_subject;			/* help subject */
u_i4    t_file_name;			/* name of the help file */
{
	char	    **subject;	
	char        **file_name;

   /* double pointer because of DG commonformat */
   subject = (char **)(2 * t_subject);
   file_name = (char **)(2 * t_file_name);

   IIhelpfile( *subject, *file_name );
}


/*
** IIinitmu
**	- Generated as part of display loop
*/
i4
IIINITMU()
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
IIIQSET( t_count, t_objtype, t_row, t_arg1, t_arg2, t_arg3, t_arg4 )
u_i4	t_count; 		/* Number of string args */
u_i4	t_objtype;
u_i4	t_row;
u_i4   t_arg1, t_arg2, t_arg3, t_arg4;	    
{
# define	IQ_ARGS_MAX	4		/* Max string args */
    char	*v_args[IQ_ARGS_MAX];
    i4		i;
    i4	        *count; 		        /* Number of string args */
    i4	        *objtype;
    i4	        *row;
    char        **arg1, **arg2, **arg3, **arg4;	    

   /* double pointer because of DG commonformat */
   count = (i4 *)(2 * t_count);
   objtype = (i4 *)(2 * t_objtype);
   row = (i4 *)(2 * t_row);
   arg1 = (char **)(2 * t_arg1);
   arg2 = (char **)(2 * t_arg2);
   arg3 = (char **)(2 * t_arg3);
   arg4 = (char **)(2 * t_arg4);

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
IIINQSET( t_count, t_arg1, t_arg2, t_arg3, t_arg4, t_arg5 )
u_i4	t_count;
u_i4   t_arg1, t_arg2, t_arg3, t_arg4, t_arg5;	    
{
# define	INQ_ARGS_MAX	5
    char	*v_args[INQ_ARGS_MAX];
    i4		i;
    i4	        *count; 		        /* Number of string args */
    char        **arg1, **arg2, **arg3, **arg4, **arg5;	    

   /* double pointer because of DG commonformat */
   count = (i4 *)(2 * t_count);
   arg1 = (char **)(2 * t_arg1);
   arg2 = (char **)(2 * t_arg2);
   arg3 = (char **)(2 * t_arg3);
   arg4 = (char **)(2 * t_arg4);
   arg5 = (char **)(2 * t_arg5);

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
IIIQFSIO( t_indflag, t_indptr, t_isvar, t_type, t_len, var_ptr, 
	  t_attr, t_name, t_row )
u_i4	t_indflag;		/* 0 = no null pointer		*/
u_i4	t_indptr;		/* null pointer			*/
u_i4	t_isvar;		/* by value or reference	*/
u_i4	t_type;			/* type of user variable	*/
u_i4	t_len;			/* sizeof data, or strlen	*/
u_i4	var_ptr;		/* Pointer to inquiring var	*/
				/* F77 passes the pointer to 
				** variable so don't double 
				** address			*/
u_i4	t_attr;			/* Object type			*/
u_i4	t_name;			/* Table name			*/
u_i4	t_row;			/* Row number			*/
{
	i4         *attr;
	i4         *row;
	char        **name;
	i4	    *indflag;
	i2	    *indptr;
	i4	    *isvar;		
	i4	    *type;	
	i4	    *len;

   /* double pointer because of DG commonformat */
   name = (char **)(2 * t_name);
   indflag = (i4 *)(2 * t_indflag);
   indptr = (i2 *)(2 * t_indptr);
   isvar = (i4 *)(2 * t_isvar);
   type = (i4 *)(2 * t_type);
   len = (i4 *)(2 * t_len);
   attr = (i4 *)(2 * t_attr);
   row = (i4 *)(2 * t_row);


    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
    	if (*name == NULL)
	   IIiqfsio( indptr, 1, DB_CHA_TYPE, *len, (char *)var_ptr, *attr, 
	   NULL, *row );
	else
	   IIiqfsio( indptr, 1, DB_CHA_TYPE, *len, (char *)var_ptr, *attr, 
           *name, *row );
    }
    else if (*type == DB_DEC_CHR_TYPE)
    {
    	if (*name == NULL)
	   IIiqfsio( indptr, 1, *type, *len, (char *)var_ptr, *attr, 
	   NULL, *row );
	else
	   IIiqfsio( indptr, 1, *type, *len, (char *)var_ptr, *attr, 
           *name, *row );
    }
    else
    {
        var_ptr = (2 * var_ptr);
    	if (*name == NULL)
	   IIiqfsio( indptr, 1, *type, *len, var_ptr, *attr, NULL, *row );
	else
	   IIiqfsio( indptr, 1, *type, *len, var_ptr, *attr, *name, *row );
    }
}

/*
** IImessage
**	- ## message
*/
void
IIMESSAGE( t_msg_str )
u_i4    t_msg_str;			/* the message string */
{
    char        **msg_str;

   /* double pointer because of DG commonformat */
   msg_str = (char **)(2 * t_msg_str);

   IImessage( *msg_str );
}


/* 
** IImuonly
**	- Generated in ## submenu loop
*/
void
IIMUONLY()
{
    IImuonly();
}


/*
** IIprnscr
**	- ## printscreen [(file = filename)]
*/
void
IIPRNSCR( t_name )
u_i4	t_name;
{
    char        **name;

   /* double pointer because of DG commonformat */
   name = (char **)(2 * t_name);

    if (*name == NULL)
	IIprnscr( NULL );
    else
	IIprnscr( *name );

}


/*
** IIputoper
**	- generated for putoper function
*/
void
IIPUTOPER( t_one )
u_i4	t_one;
{
    IIputoper( 1 );
}


/*
** IIredisp
*/
void
IIREDISP()
{
    IIredisp();
}


/*
** IIrescol
**	- ## resume column <tablefield> <column>
*/
void
IIRESCOL( t_tabname, t_colname )
u_i4	t_tabname;
u_i4	t_colname;
{
	char	**tabname;
	char	**colname;

   /* double pointer because of DG commonformat */
   tabname = (char **)(2 * t_tabname);
   colname = (char **)(2 * t_colname);

   IIrescol( *tabname, *colname);
}


/*
** IIresfld
** 	- ## resume <fldname>
*/
void
IIRESFLD( t_fld_name )
u_i4    t_fld_name;			/* field name */
{
	char	**fld_name;

   /* double pointer because of DG commonformat */
   fld_name = (char **)(2 * t_fld_name);

   IIresfld( *fld_name );
}


/*
** IIresmu
**	- ## resume menu
*/
void
IIRESMU()
{
    IIresmu();
}


/*
** IIresnext
** 	- ## resume
*/
i4
IIRESNEXT()
{
    IIresnext();
}

/*
** IIFRreResEntry
**      - ## resume entry
*/
void
IIFRRERESENTRY()
{
    IIFRreResEntry();
}



/*
** IIgetfldio
** 	- ## getform I/O
**	- If receiving variable is a string, copy and blank pad.
*/

void
IIGETFLDIO( t_indflag, t_indptr, t_isvar, t_type, t_len, var_ptr, t_fld_name )
u_i4	t_indflag;		/* 0 = no null pointer		*/
u_i4	t_indptr;		/* null pointer			*/
u_i4	t_isvar;		/* by value or reference	*/
u_i4	t_type;			/* type of user variable	*/
u_i4	t_len;			/* sizeof data, or strlen	*/
u_i4	var_ptr;		/* pointer to retrieval var	*/
				/* F77 passes the pointer to 
				** variable so don't double 
				** address			*/
u_i4   t_fld_name;		/* name of field to retrieve	*/
{
	char        **fld_name;
	i4	    *indflag;
	i2	    *indptr;
	i4	    *isvar;		
	i4	    *type;	
	i4	    *len;

   /* double pointer because of DG commonformat */
   fld_name = (char **)(2 * t_fld_name);
   indflag = (i4 *)(2 * t_indflag);
   indptr = (i2 *)(2 * t_indptr);
   isvar = (i4 *)(2 * t_isvar);
   type = (i4 *)(2 * t_type);
   len = (i4 *)(2 * t_len);


    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
	IIgetfldio( indptr, 1, DB_CHA_TYPE, *len, (char *)var_ptr, *fld_name );
    }
    else if (*type == DB_DEC_CHR_TYPE)
    {
	IIgetfldio( indptr, 1, *type, *len, (char *)var_ptr, *fld_name );
    }
    else
    {
        var_ptr = (2 * var_ptr);
	IIgetfldio( indptr, 1, *type, *len, var_ptr, *fld_name );
    }
}


/*
** IIretval
**	- return value
*/
i4
IIRETVAL()
{
    return IIretval();
}




/* IIrunform
** 	- Generated for loop display
*/
i4
IIRUNFORM()
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
IIRUNMU( t_single_item )
u_i4	t_single_item;		/* single-entry boolean */
{
	i4	    *single_item;

   /* double pointer because of DG commonformat */
   single_item = (i4 *)(2 * t_single_item);

   IIrunmu( *single_item );
}

/*
** IIputfldio
**	## initialize(fld = var, ...)
*/
void
IIPUTFLDIO( t_name, t_indflag, t_indptr, t_isvar, t_type, t_len, var_ptr )
u_i4	t_name;			/* the field name		*/
u_i4	t_indflag;		/* 0 = no null pointer		*/
u_i4	t_indptr;		/* null pointer			*/
u_i4	t_isvar;		/* by value or reference	*/
u_i4	t_type;			/* type of user variable	*/
u_i4	t_len;			/* sizeof data, or strlen	*/
u_i4   var_ptr;		/* ptr to data for setting	*/
				/* F77 passes the pointer to 
				** variable so don't double 
				** address			*/
{
	char        **name;
	i4	    *indflag;
	i2	    *indptr;
	i4	    *isvar;		
	i4	    *type;	
	i4	    *len;

   /* double pointer because of DG commonformat */
   name = (char **)(2 * t_name);
   indflag = (i4 *)(2 * t_indflag);
   indptr = (i2 *)(2 * t_indptr);
   isvar = (i4 *)(2 * t_isvar);
   type = (i4 *)(2 * t_type);
   len = (i4 *)(2 * t_len);

    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
	IIputfldio( *name, indptr, 1, *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE,
		    *len, (char *)var_ptr );
    }
    else if (*type == DB_DEC_CHR_TYPE)
    {
	IIputfldio( *name, indptr, 1, *type, *len, (char *)var_ptr );
    }
    else
    {
        var_ptr = (2 * var_ptr);
	IIputfldio( *name, indptr, 1, *type, *len, var_ptr );
    }
}




/*
** IIsleep
**	## sleep
*/
void
IISLEEP( t_seconds )
u_i4	    t_seconds;			/* number of seconds to sleep */
{
	i4	    *seconds;

   /* double pointer because of DG commonformat */
   seconds = (i4 *)(2 * t_seconds);

   IIsleep( *seconds );
}


/* 
** IIstfsio
**	- ## set_frs statement
*/
void
IISTFSIO( t_attr, t_name, t_row, t_indflag, t_indptr, t_isvar, t_type, 
	  t_len, var_ptr )
u_i4	t_attr;
u_i4	t_name;
u_i4	t_row;
u_i4	t_indflag;		/* 0 = no null pointer		*/
u_i4	t_indptr;		/* null pointer			*/
u_i4	t_isvar;		/* by value or reference	*/
u_i4	t_type;			/* type of user variable	*/
u_i4	t_len;			/* sizeof data, or strlen	*/
u_i4	var_ptr; 		/* F77 passes the pointer to 
				** variable so don't double 
				** address			*/
{
	i4         *attr;
	i4         *row;
	char        **name;
	i4	    *indflag;
	i2	    *indptr;
	i4	    *isvar;		
	i4	    *type;	
	i4	    *len;

   /* double pointer because of DG commonformat */
   name = (char **)(2 * t_name);
   indflag = (i4 *)(2 * t_indflag);
   indptr = (i2 *)(2 * t_indptr);
   isvar = (i4 *)(2 * t_isvar);
   type = (i4 *)(2 * t_type);
   len = (i4 *)(2 * t_len);
   attr = (i4 *)(2 * t_attr);
   row = (i4 *)(2 * t_row);


    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
	IIstfsio( *attr, *name, *row, indptr, *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE,
		  *len, (char *)var_ptr );
    }
    else if (*type == DB_DEC_CHR_TYPE) 
    { 
	IIstfsio( *attr, *name, *row, indptr, 1, *type, *len, (char *)var_ptr ); 
    } 
    else
    {
        var_ptr = (2 * var_ptr);
	IIstfsio( *attr, *name, *row, indptr, 1, *type, *len, var_ptr );
    }
}

/*
** IIFRsaSetAttrio
**   -	loadtable/insertrow with clause for per value attribute setting.
**	e.g.,
**	exec frs loadtable f t (col1 = val) with (reverse(col1) = 1);
*/
void
IIFRSASETATTRIO( t_attr, t_name, t_indflag, t_indptr, t_isvar, t_type, 
	  t_len, var_ptr )
u_i4	t_attr;
u_i4	t_name;
u_i4	t_indflag;		/* 0 = no null pointer		*/
u_i4	t_indptr;		/* null pointer			*/
u_i4	t_isvar;		/* by value or reference	*/
u_i4	t_type;			/* type of user variable	*/
u_i4	t_len;			/* sizeof data, or strlen	*/
u_i4	var_ptr; 		/* F77 passes the pointer to 
				** variable so don't double 
				** address			*/
{
	i4         *attr;
	char        **name;
	i4	    *indflag;
	i2	    *indptr;
	i4	    *isvar;		
	i4	    *type;	
	i4	    *len;

   /* double pointer because of DG commonformat */
   name = (char **)(2 * t_name);
   indflag = (i4 *)(2 * t_indflag);
   indptr = (i2 *)(2 * t_indptr);
   isvar = (i4 *)(2 * t_isvar);
   type = (i4 *)(2 * t_type);
   len = (i4 *)(2 * t_len);
   attr = (i4 *)(2 * t_attr);


    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
	IIFRsaSetAttrio( *attr, *name, indptr, 1,
		*len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE, *len, (char *)var_ptr );
    }
    else if (*type == DB_DEC_CHR_TYPE) 
    { 
	IIFRsaSetAttrio(*attr, *name, indptr, 1, *type, *len,(char *)var_ptr); 
    } 
    else
    {
        var_ptr = (2 * var_ptr);
	IIFRsaSetAttrio( *attr, *name, indptr, 1, *type, *len, var_ptr );
    }
}

/* 
** IIvalfld
** 	- ## validate
*/
i4
IIVALFLD( t_fld_name )
u_i4    t_fld_name;			/* name of the field */
{
	char        **fld_name;

   /* double pointer because of DG commonformat */
   fld_name = (char **)(2 * t_fld_name);

   return IIvalfld( *fld_name );
}

/*
** IInestmu
** 	- ## validate
*/
i4
IINESTMU()
{
    return IInestmu();
}

/*
** IIrunnest
** 	- ## validate
*/
i4
IIRUNNEST()
{
    return IIrunnest();
}

/*
** IIendnest
** 	- ## validate
*/
i4
IIENDNEST()
{
    return IIendnest();
}

/*
** IIFRgpcontrol
**	- POPUP forms control
*/
void
IIFRGPCONTROL( t_state, t_modifier )
u_i4	t_state;
u_i4	t_modifier;
{
	i4	    *state;		
	i4	    *modifier;	

   /* double pointer because of DG commonformat */
   state = (i4 *)(2 * t_state);
   modifier = (i4 *)(2 * t_modifier);

   IIFRgpcontrol( *state, *modifier );
}


/*
** IIFRgpsetio
** 	- POPUP forms
*/
void
IIFRGPSETIO( t_id, t_indflag, t_indptr, t_isvar, t_type, t_len, var_ptr )
u_i4	t_id;			/* internal clause id		*/
u_i4	t_indflag;		/* 0 = no null pointer		*/
u_i4	t_indptr;		/* null pointer			*/
u_i4	t_isvar;		/* by value or reference	*/
u_i4	t_type;			/* type of user variable	*/
u_i4	t_len;			/* sizeof data, or strlen	*/
u_i4	var_ptr; 		/* F77 passes the pointer to 
				** variable so don't double 
				** address			*/
{
	i4         *id;
	i4	    *indflag;
	i2	    *indptr;
	i4	    *isvar;		
	i4	    *type;	
	i4	    *len;

   /* double pointer because of DG commonformat */
   id = (i4 *)(2 * t_id);
   indflag = (i4 *)(2 * t_indflag);
   indptr = (i2 *)(2 * t_indptr);
   isvar = (i4 *)(2 * t_isvar);
   type = (i4 *)(2 * t_type);
   len = (i4 *)(2 * t_len);

    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
	IIFRgpsetio( *id, indptr, 1, *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE,
		     *len, (char *)var_ptr );
    }
    else if (*type == DB_DEC_CHR_TYPE)
    {
	IIFRgpsetio( *id, indptr, 1, *type, *len, (char *)var_ptr );
    }
    else
    {
        var_ptr = (2 * var_ptr);
	IIFRgpsetio( *id, indptr, 1, *type, *len, var_ptr );
    }
}

/*
** IItbact
**	- sets up data set for loading/unloading of a linked table field
*/
u_i4
IITBACT( t_form_name, t_tbl_fld_name, t_load_flg )
u_i4	t_form_name;			/* name of form */
u_i4	t_tbl_fld_name;			/* name of table field */
u_i4	t_load_flg;			/* load/unload flag */
{
	char        **form_name;
	char        **tbl_fld_name;
	i4         *load_flg;

   /* double pointer because of DG commonformat */
   form_name = (char **)(2 * t_form_name);
   tbl_fld_name = (char **)(2 * t_tbl_fld_name);
   load_flg = (char **)(2 * t_load_flg);

   return IItbact( *form_name, *tbl_fld_name, *load_flg );
    return; 
}

/*
** IItbinit
**	- initialize a table field with a data set
**	- ## inittable formname tablefldname
*/
i4
IITBINIT( t_form_name, t_tbl_fld_name, t_tf_mode )
u_i4	t_form_name;			/* name of form */
u_i4	t_tbl_fld_name;			/* name of table field */
u_i4   t_tf_mode;			/* mode for table field */
{
	char        **form_name;
	char        **tbl_fld_name;
	char        **tf_mode;

   /* double pointer because of DG commonformat */
   form_name = (char **)(2 * t_form_name);
   tbl_fld_name = (char **)(2 * t_tbl_fld_name);
   tf_mode = (char **)(2 * t_tf_mode);

   return IItbinit( *form_name, *tbl_fld_name, *tf_mode );
}

/*
** IItbsetio
**	- set up table field structures before doing row I/O
**	- used on most row statements
*/
i4
IITBSETIO( t_mode, t_form_name, t_tbl_fld_name, t_row_num )
u_i4	t_mode;				/* command mode */
u_i4   t_form_name;			/* form name */
u_i4   t_tbl_fld_name;			/* table-field name */
u_i4	t_row_num;			/* row number */
{
	i4         *mode;
	char        **form_name;
	char        **tbl_fld_name;
	i4         *row_num;

   /* double pointer because of DG commonformat */
   mode = (i4 *)(2 * t_mode);
   form_name = (char **)(2 * t_form_name);
   tbl_fld_name = (char **)(2 * t_tbl_fld_name);
   row_num = (i4 *)(2 * t_row_num);

   return IItbsetio( *mode, *form_name, *tbl_fld_name, *row_num );
}


/*
** IItbsmode
**	- set up the mode scrolling a table field
*/
void
IITBSMODE( t_mode )
u_i4    t_mode;			/* up/down/to scroll mode */
{
	char        **mode;

   /* double pointer because of DG commonformat */
   mode = (char **)(2 * t_mode);

   IItbsmode( *mode );
}

/*
** IItclrcol
**	- clear a specific column in a specified row
**	- ## clearrow formname tablename 1 (col1, col2)
*/
void
IITCLRCOL( t_col_name )
u_i4    t_col_name;			/* name of column to clear */
{
	char        **col_name;

   /* double pointer because of DG commonformat */
   col_name = (char **)(2 * t_col_name);

   IItclrcol( *col_name );
}


/*
** IIclrrow
**	- clear a specified row (or current row) in a tablefield
**	- ## clearrow formname tablename 3
*/
void
IITCLRROW()
{
    IItclrrow();
}


/*
** IItcogetio
**	- ## getrow formname tablefldname (var = column, ...) 
**	- also generated for OUT clauses on ## deleterow statements.
*/

void
IITCOGETIO( t_indflag, t_indptr, t_isvar, t_type, t_len, var_ptr, t_col_name )
u_i4	t_indflag;		/* 0 = no null pointer		*/
u_i4	t_indptr;		/* null pointer			*/
u_i4	t_isvar;		/* by value or reference	*/
u_i4	t_type;			/* type of user variable	*/
u_i4	t_len;			/* sizeof data, or strlen	*/
u_i4	var_ptr;		/* pointer to retrieval variable */
				/* F77 passes the pointer to 
				** variable so don't double 
				** address			*/
u_i4	t_col_name;		/* name of column to retrieve	*/
{
	char        **col_name;
	i4	    *indflag;
	i2	    *indptr;
	i4	    *isvar;		
	i4	    *type;	
	i4	    *len;

   /* double pointer because of DG commonformat */
   col_name = (char **)(2 * t_col_name);
   indflag = (i4 *)(2 * t_indflag);
   indptr = (i2 *)(2 * t_indptr);
   isvar = (i4 *)(2 * t_isvar);
   type = (i4 *)(2 * t_type);
   len = (i4 *)(2 * t_len);


    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
	IItcogetio( indptr, 1, DB_CHA_TYPE, *len, (char *)var_ptr, *col_name );
    }
    else if (*type == DB_DEC_CHR_TYPE)
    {
	IItcogetio( indptr, 1, *type, *len, (char *)var_ptr, *col_name );
    }
    else
    {
        var_ptr = (2 * var_ptr);
	IItcogetio( indptr, 1, *type, *len, var_ptr, *col_name );
    }
}

/*
** IItcoputio
**	- ## putrow formname tablefldname (colname = var, ...)
**	- Also generated for IN clauses on ## deleterow statements.
*/
void
IITCOPUTIO( t_name, t_indflag, t_indptr, t_isvar, t_type, t_len, var_ptr )
u_i4	t_name;			/* the field name		*/
u_i4	t_indflag;		/* 0 = no null pointer		*/
u_i4	t_indptr;		/* null pointer			*/
u_i4	t_isvar;		/* by value or reference	*/
u_i4	t_type;			/* type of user variable	*/
u_i4	t_len;			/* sizeof data, or strlen	*/
u_i4	var_ptr;		/* ptr to data for setting	*/
				/* F77 passes the pointer to 
				** variable so don't double 
				** address			*/
{
	char        **name;
	i4	    *indflag;
	i2	    *indptr;
	i4	    *isvar;		
	i4	    *type;	
	i4	    *len;

   /* double pointer because of DG commonformat */
   name = (char **)(2 * t_name);
   indflag = (i4 *)(2 * t_indflag);
   indptr = (i2 *)(2 * t_indptr);
   isvar = (i4 *)(2 * t_isvar);
   type = (i4 *)(2 * t_type);
   len = (i4 *)(2 * t_len);

    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
	IItcoputio( *name, indptr, 1, *len == 0 ? DB_CHR_TYPE : DB_CHA_TYPE,
		    *len, (char *)var_ptr);
    }
    else if (*type == DB_DEC_CHR_TYPE)
    {
	IItcoputio( *name, indptr, 1, *type, *len, (char *)var_ptr);
    }
    else
    {
        var_ptr = (2 * var_ptr);
	IItcoputio( *name, indptr, 1, *type, *len, var_ptr );
    }
}


/*
** IItcolval
**	- ## validrow formname tablefldname row (col1, col2)
*/
void
IITCOLVAL( t_colname )
u_i4	t_colname;
{
	char	**colname;

   /* double pointer because of DG commonformat */
   colname = (char **)(2 * t_colname);

   IItcolval( *colname );
}


/*
** IItdata
**	- ## tabledata
*/
i4
IITDATA()
{
    return IItdata();
}


/*
** IItdatend
**	- Clean up after tabledata loop
*/
void
IITDATEND()
{
    IItdatend();
}


/*
** IItdelrow
**	- ## deleterow formname tablename [row] [out/in clause]
*/
i4
IITDELROW( t_in_list )
u_i4	t_in_list;			/* IN-list boolean */
{
	i4	*in_list;			

   /* double pointer because of DG commonformat */
   in_list = (i4 *)(2 * t_in_list);

   return IItdelrow( *in_list );
}


/*
** IItfill
**	- generated by ## inittable statements
*/
void
IITFILL()
{
    IItfill();
}


/*
** IIhidecol
**	- ## inittable statement with hidden column clause
*/
void
IITHIDECOL( t_col_name, t_format )
u_i4	t_col_name;			/* column name		*/
u_i4	t_format;			/* col type format string */
{
	char	**col_name;			
	char	**format;		

   /* double pointer because of DG commonformat */
   col_name = (char **)(2 * t_col_name);
   format = (char **)(2 * t_format);

   IIthidecol( *col_name, *format );
}


/*
** IItinsert
**	- ## insertrow formname tablefldname [row] [target list]
*/
i4
IITINSERT()
{
    return IItinsert();
}


/*
** IItscroll
**	- set up calls to actual scrolling routines
*/
i4
IITSCROLL( t_in, t_rec_num )
u_i4	t_in;			/* IN-list boolean		*/
u_i4	t_rec_num;		/* record number for scroll TO	*/
{
	i4	*in;			
	i4	*rec_num;	

   /* double pointer because of DG commonformat */
   in = (i4 *)(2 * t_in);
   rec_num = (i4 *)(2 * t_rec_num);

   return IItscroll( *in, *rec_num );
}


/*
** IItunend
**	- generated to clean up after ## unloadtable
*/
void
IITUNEND()
{
    IItunend();
}



/*
** IItunload
**	- table unload
*/
i4
IITUNLOAD()
{
    return IItunload();
}

/* 
** IItvalrow
**	- ##  validrow command
*/
void
IITVALROW()
{
    IItvalrow();
}


/*
** IITBceColEnd
**      - indicates there are no more columns values to be set.
*/
void
IITBCECOLEND()
{
    IITBceColEnd();
}

/*
** IItvalval
**	- ## validrow command
*/
i4
IITVALVAL( t_one )
u_i4	t_one;			/* Always has value of 1 */
{
    return IItvalval( 1 );
}

/*
** IIFRsqExecute
**	- EXEC FRS PUTFORM :f1 USING :sqlda
**	- EXEC FRS GETFORM USING :sqlda
**	- EXEC FRS PUTROW :f1 :t1 :r1 USING :sqlda
**	- EXEC FRS UNLOADTABLE :f1 :t1 USING :sqlda
*/
void
IIFRSQEXECUTE( t_lang, t_is_form, t_is_input, t_sqd )
u_i4	t_lang;			/* language */
u_i4	t_is_form;
u_i4	t_is_input;			
u_i4	t_sqd;
{
	i4	*lang;		
	i4	*is_form;
	i4	*is_input;	
	PTR	*sqd;

   /* double pointer because of DG commonformat */
   lang = (i4 *)(2 * t_lang);
   is_form = (i4 *)(2 * t_is_form);
   is_input = (i4 *)(2 * t_is_input);
   sqd = (PTR *)(2 * t_sqd);

   IIFRsqExecute( *lang, *is_form, *is_input, sqd );
}

/*
** IIFRsqDescribe
**	- EXEC FRS DESCRIBE FORM f1 UPDATE INTO :sqlda
*/
void
IIFRSQDESCRIBE( t_lang, t_is_form, t_fname, t_tname, t_mode, t_sqd )
u_i4	t_lang;				/* language */
u_i4	t_is_form;
u_i4	t_fname;			/* form name */
u_i4	t_tname;			/* table name */
u_i4	t_mode;			
u_i4	t_sqd;
{
	i4	*lang;		
	i4	*is_form;
	char	**fname;	
	char	**tname;
	char	**mode;			
	PTR	*sqd;

   /* double pointer because of DG commonformat */
   lang = (i4 *)(2 * t_lang);
   is_form = (i4 *)(2 * t_is_form);
   fname = (char **)(2 * t_fname);
   tname = (char **)(2 * t_tname);
   mode = (char **)(2 * t_mode);
   sqd = (PTR *)(2 * t_sqd);

   IIFRsqDescribe( *lang, *is_form, *fname, *tname, *mode, sqd );
}

/*
** IIFRaeAlerterEvent
**	- exec sql activate event
*/
i4
IIFRAEALERTEREVENT( t_intr )
u_i4	t_intr;		/* Display loop interrupt value */
{
	i4	*intr;

   /* double pointer because of DG commonformat */
   intr = (i4 *)(2 * t_intr);

   return IIFRaeAlerterEvent( *intr );
}

# endif /* DGC_AOS*/
