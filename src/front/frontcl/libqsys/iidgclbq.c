# include 	<compat.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<eqlang.h>
# include	<eqrun.h>
# include	"erls.h"

# ifdef DGC_AOS

/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:	iidgclbq.c
** Purpose:	Runtime translation layer for Data General ESQL/COBOL programs 
** 		only
**
** Defines:			Maps to:
**	IIXERRTEST		IIerrtest
**	IIXEXIT			IIexit
**	IIXFLUSH		IIflush
**	IIXNGOPEN		IIngopen
**	IIXNEXTGET		IInextget
**	IIXNEXEC		IInexec
**	IIXPARRET		IIparret
**	IIXPARSET		IIparset
**	IIXGETDOMIO		IIgetdomio
**	IIXRETINIT		IIretinit
**	IIXPUTDOMIO		IIputdomio
**	IIXNOTRIMIO		IIputdomio
**	IIXSEXEC		IIsexec
**	IIXSYNCUP		IIsyncup
**	IIXTUPGET		IItupget
**	IIXUTSYS		IIutsys 
**	IIXWRITIO		IIwritio
**	IIXCSCLOSE		IIcsClose
**	IIXCSDELETE		IIcsDelete
**	IIXCSERPLACE		IIcsERplace
**	IIXCSREPLACE		IIcsReplace
**	IIXCSOPEN		IIcsOpen
**	IIXCSQUERY		IIcsQuery
**	IIXCSRETRIEVE		IIcsRetrieve
**	IIXCSGETIO		IIcsGetio
**	IIXCSERETRIEVE		IIcsERetrieve
**	IIXEXDEFINE		IIexDefine
**	IIXEXEXEC		IIexExec
**	IIXPUTCTRL		IIputctrl
**	IIXXACT			IIxact
**	IIXCSRDO		IIcsRdO
**	IIXLQPRIPROCINIT	IILQpriProcInit
**	IIXLQPRSPROCSTATUS	IILQprsProcStatus
**	IIXLQPRSPROCVALIO	IILQprsProcValio	
**	IIXSETERR		IIseterr
**	IIXSQCONNECT		IIsqConnect
**	IIXSQDISCONNECT		IIsqDisconnect
**	IIXSQFLUSH		IIsqFlush
**	IIXSQINIT		IIsqInit
**	IIXSQLCDINIT		IIsqlcdInit
**	IIXSQGDINIT		IIsqGdInit
**	IIXSQSTOP		IIsqStop
**	IIXSQPRINT		IIsqPrint
**	IIXSQUSER		IIsqUser
**	IIXSQTPC		IIsqTPC
**	IIXLQSIDSESSID		IILQsidSessID
**	IIXSQXEXIMMED		IIsqExImmed
**	IIXSQEXSTMT		IIsqExStmt
**	IIXCSDAGET		IIcsDaGet
**	IIXSQMODS		IIsqMods
**	IIXSQPARMS		IIsqParms
**	IIXLQESEVSTAT		IILQesEvStat
**	IIXLQSSSETSQLIO		IILQssSetSqlio
**	IIXLQISINQSQLIO		IILQisInqSqlio
**	IIXLQSHSETHANDLER	IILQshSetHandler
**	IIXLQLED_LOENDDATA	IILQled_LoEndData
-*
-*
** Notes:
** 0)	Files in DG COBOL runtime layer:
**		iidgclbq.c	- libqsys directory
**		iidgcrun.c	- runsys directory
**
** 1)   Purpose of DG COBOL runtime layer:
**	The above files contain routines which map subroutine calls from a 
**	AOS/VS COBOL program to our runtime system.
**
** 2)   Parameter passing by reference:
**	The DG COBOL compiler passes everything by reference, so this 
**	translation layer also serves to dereference numerics and strings 
**      where necessary.
**	Sometimes an argument may be a numeric zero (a null pointer) or a 
**	string pointer.  (See IIflush, IIeqiqio, IIeqstio, IIgetdomio ). 
**	This layer tests for a non-zero (sent by reference) before deciding 
**	whether to send the pointer or a zero.
**
** 3)   Null-terminated input strings:
**	This layer assumes that strings and string variables which are
**	SENDING data to INGRES/forms system have had a null concatenated 
**	
**	For IIsqPrepare and IIsqExImmed, length of the variable string is
**	passed.  If the query string is sent via a COBOL PIC X and NOT
**	null terminated, then length > 0.  If the query string is null
**	terminated, then length is 0.  IIput_new dynamically allocates
**	memory to copy the query into and then null terminates before
**	sending it off to LIBQ.
**		
** 4)   DG COBOL does not support the GIVING clause.  For all function calls,
**	the layer sets ret_val to the return function value.  Ret_val (IIRES)
**	is passed back as a parameter.
**
** 5)	#ifdef DG_LIKE_INGRES.  This appears around new calls that reflect
**	INGRES-only functionality.  When and if this functionality shows 
**	up in the DG dbms, these ifdefs can be removed.
**
** History:
**	09-feb-89 (sylviap)   
**		Created from iiuflibq.c
**	24-apr-89 (sylviap)   
**		Changed var_ptr to be char * from i4  *.
**	11-may-89 (sylviap)   
**		Fixed call to IIexExec.  Should not return a value.
**		Fixed call to IIretinit.  Passes a char *.
**	08-sep-89 (sylviap)   
**		Bug 7872 - Passes 0 to IIsyncup, IIflush, IIretinit, IIsqFlush 
**		if file_name is a pointer to zero.
**	29-sep-89 (barbara)
**		Integrated porting changes.
**	20-dec-89 (barbara)
**		Updated for Phoenix/Alerters.  Added IILQesEvStat and
**		IILQegEvGetio.  Also added 6.3 calls IILQsidSessID and
**		IIsqTPC.
**	26-feb-1991 (kathryn) Added:
**		IILQssSetSqlio, IILQisInqSqlio IILQssSetHandler.
**	25-apr-1991 (teresal)
**		Removed IILQegEvGetio.
**	14-oct-1992 (lan)
**		Added IILQled_LoEndData.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      09-oct-2007 (huazh01)
**          Call IILQprsProcStatus() with correct number of parameter.
**          This fixes bug 119273.
*/


/*
** IIbreak
**	- ## endretrieve
**	- Generated to break retrieve loop
*/
void
IIXBREAK()
{
    IIbreak();
}


/*
** IIerrtest
**	- Generated as test in retrieve loop
*/
void
IIXERRTEST(ret_val)
i4	*ret_val;
{
    *ret_val = IIerrtest();
    return; 
}

/*
** IIexit
** 	- ## exit
*/
void
IIXEXIT()
{
    IIexit();
}

/*
** IIflush
**	- Clean up at completion of retrieve
*/
void
IIXFLUSH( file_name, line_num )
char	*file_name;
i4	*line_num;
{
    /* B7872 - if a ptr to zero, then pass 0 */
    if (*(char **)file_name == (char *)0)
        IIflush( 0, 0 );
    else
	IIflush( file_name, *line_num );
}
/* 
** IIingopen 
**	- ## ingres <dbname> flags
** 	- Sends a variable number of arguments, up to a maximum of 14
**	  character string args plus language argument.
**	- For COBOL we assume an extra argument, which is a
**	  count representing the number of string args.  This is present
**	  on all calls which have a varying number of arguments.
**	  (See which calls go through gen_var_args in code generator.)
*/
void
IIXINGOPEN( count, lan, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, 
	    arg9, arg10, arg11, arg12, arg13, arg14 )
i4	*count, *lan;
char	*arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7, *arg8,
        *arg9, *arg10,*arg11,*arg12,*arg13,*arg14;
{
# define 	ING_ARGS_MAX	14
    char	*v_args[ING_ARGS_MAX];
    i4  	i;

    for (i = 0; i < ING_ARGS_MAX; i++)
  	v_args[i] = (char *)0;		/* initialize arg array */

  /* 
  ** Enter this switch statement according to argument count.
  ** Fill v_args, starting at last arg and dropping through
  ** all subsequent ones until first arg
  */
    switch( *count - 1 ) {
	case 14:  v_args[13] = arg14;
	case 13:  v_args[12] = arg13;	
	case 12:  v_args[11] = arg12;	
	case 11:  v_args[10] = arg11;	
	case 10:  v_args[ 9] = arg10;	
	case  9:  v_args[ 8] = arg9;	
	case  8:  v_args[ 7] = arg8;	
	case  7:  v_args[ 6] = arg7;	
	case  6:  v_args[ 5] = arg6;	
	case  5:  v_args[ 4] = arg5;	
	case  4:  v_args[ 3] = arg4;	
	case  3:  v_args[ 2] = arg3;	
	case  2:  v_args[ 1] = arg2;	
	case  1:  v_args[ 0] = arg1;	
    }

    IIingopen( *lan, v_args[0], v_args[1], v_args[2], v_args[3], 
	v_args[4], v_args[5], v_args[6], v_args[7], v_args[8], v_args[9],
	v_args[10], v_args[11], v_args[12], v_args[13]); 
}

/*
** IInextget
**	- Checks to see if there's another tuple in retrieve loop
*/
void
IIXNEXTGET(ret_val)
i4	*ret_val;
{
    *ret_val = IInextget();
    return; 
}

/*
** IInexec
**	- Checks if repeat query successfully executed.
*/
void
IIXNEXEC(ret_val)
i4	*ret_val;
{
    *ret_val = IInexec();
    return; 
}

/*
** IIeqiqio
**	- ## inquire_equel statement
**	- Called once for each object being inquired about
**	- We must blank pad user var if result var is a string
*/
 
void
IIXEQIQIO( indflag, indptr, isvar, type, len, var_ptr, inq_name )
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;
char	*inq_name;

{
   if ( !*indflag )
      indptr = (i2 *)0;

   if (*type == DB_CHR_TYPE)
   {
      IIeqiqio( indptr, 1, DB_CHA_TYPE, *len, var_ptr, inq_name );
   }
   else
   {
      IIeqiqio( indptr, 1, *type, *len, var_ptr, inq_name );
   }
}


/*
** IIeqstio
**	- ## set_equel command
*/
void
IIXEQSTIO( setname, indflag, indptr, isvar, type, len, var_ptr )
char	*setname;		/* Object to set		*/
i4	*indflag;		/* 0 = no null indicator	*/
i2	*indptr;		/* null indicator pointer	*/
i4	*isvar;			/* Always 1 for F77		*/
i4	*type;			/* Type of user variable	*/
i4	*len;			/* Sizeof data, or strlen	*/
char	*var_ptr;		/* Variable containing value	*/
{
     if ( !*indflag )
 	indptr = (i2 *)0;
 
     if (*type == DB_CHR_TYPE)
     	IIeqstio( setname, indptr,  1, DB_CHR_TYPE, *len, var_ptr );
     else
     	IIeqstio( setname, indptr, 1, *type, *len, var_ptr );
}
     
/*
** IIgetdomio
**	- Transfers data from retrieve statement into host-vars
** 	- When the receiving variable is a string, we must blank pad
*/
void
IIXGETDOMIO( indflag, indptr, isvar, type, len, var_ptr )
i4	*indflag;		/* 0 = no null indicator	*/
i2	*indptr;		/* null indicator pointer	*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of object		*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;		/* pointer to the object	*/
{

    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
    {
	IIgetdomio( indptr, 1, DB_CHA_TYPE, *len, var_ptr );
    }
    else
    {
	IIgetdomio( indptr, 1, *type, *len, var_ptr );
    }
}
		
/*
** IIretinit
** 	- Set up for retrieve loop
*/
void
IIXRETINIT( file_name, line_num )
char	*file_name;
i4	*line_num;
{
    /* B7872 - if a ptr to zero, then pass 0 */
    if (*(char **)file_name == (char *)0)
       IIretinit( 0, 0 );
    else
       IIretinit( file_name, *line_num );
}

/*
** IIputdomio
**	- Transfers data to backend in database write statements
**	- Generated on ## append and ## replace
**	- Trims if len > 0.
*/
void
IIXPUTDOMIO( indflag, indptr, isvar, type, len, var_ptr )
i4	*indflag;		/* 0 = no null indicator	*/
i2	*indptr;		/* null indicator pointer	*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of object		*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;		/* pointer to the object	*/
{

    if ( !*indflag )
	indptr = (i2 *)0;

    if (*type == DB_CHR_TYPE)
	IIputdomio( indptr, 1, DB_CHA_TYPE, *len, var_ptr );
    else
	IIputdomio( indptr, 1, *type, *len, var_ptr);
}
/*
** IInotrimio
**	- Transfers data to backend in database write statements
**	- Generated on ## append and ## replace
**	- No trimming takes place
*/
void
IIXNOTRIMIO( indflag, indptr, isvar, type, len, var_ptr )
i4	*indflag;		/* 0 = no null indicator	*/
i2	*indptr;		/* null indicator pointer	*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of object		*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;		/* pointer to the object	*/
{
    if ( !*indflag )
	indptr = (i2 *)0;

    IIputdomio( indptr, 1, DB_CHR_TYPE, *len, var_ptr );
}

/*
** IIsexec
**	- Resets status flag during repeat query loop
*/
void
IIXSEXEC()
{
    IIsexec();
}

/*
** IIsyncup
**	- Generated after IIwrite calls to synch up with backend
*/
void
IIXSYNCUP( file_name, line_num )
char	*file_name;
i4	*line_num;
{
    /* B7872 - if a ptr to zero, then pass 0 */
    if (*(char **)file_name == (char *)0)
       IIsyncup( 0, 0 );
    else
       IIsyncup( file_name, *line_num );
}

/*
** IItupget
**	- Generated in parameterized retrieves to get data into host_var.
*/
void
IIXTUPGET(ret_val)
i4	*ret_val;
{
    *ret_val = IItupget();
    return; 
}

/*
** IIutsys
**	- ## call <ingres subsystem>  (parameters)
*/
void
IIXUTSYS( flag, arg, argstr )
i4	*flag;
char	*arg;			/* Program/argument name */
char	*argstr;		/* Value of argument name */
{

    IIutsys( *flag, arg, argstr ); 
}


/*
** IIwritio
**	- Writes a string to the backend
**	- Generated for most statements
*/
void
IIXWRITIO( trim, indflag, indptr, isvar, type, len, string )
i4	*trim;			/* trim flag, 0 = no trim	*/
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of data			*/
i4	*len;			/* sizeof data, or strlen	*/
char	*string;		/* actual string		*/
{
    if ( !*indflag )
	indptr = (i2 *)0;

    IIwritio( *trim, indptr, *isvar, *type, *len, string );
}

/*
** IIcsClose
**	- Close cursor
*/
void
IIXCSCLOSE( cursor_name, cid1, cid2 )
char	 *cursor_name;
i4	 *cid1;			/* cursor id 1			*/
i4	 *cid2;			/* cursor id 2			*/
{
    IIcsClose( cursor_name, *cid1, *cid2 );
}

/*
** IIcsDelete
**	- Delete cursor
*/
void
IIXCSDELETE( table_name, cursor_name, cid1, cid2 )
char	*table_name;
char	*cursor_name;
i4	*cid1;			/* cursor id 1			*/
i4	*cid2;			/* cursor id 2			*/
{
    IIcsDelete( table_name, cursor_name, *cid1, *cid2 );
}

/*
** IIcsERplace
**	- Cursor UPDATE/REPLACE end
*/
void
IIXCSERPLACE( cursor_name, cid1, cid2 )
char	*cursor_name;
i4	 *cid1;			/* cursor id 1			*/
i4	 *cid2;			/* cursor id 2			*/
{
    IIcsERplace( cursor_name, *cid1, *cid2 );
}

/*
** IIcsReplace 
**	- Cursor UPDATE/REPLACE begin
*/
void
IIXCSREPLACE( cursor_name, cid1, cid2 )
char	*cursor_name;
i4	 *cid1;			/* cursor id 1			*/
i4	 *cid2;			/* cursor id 2			*/
{
    IIcsReplace( cursor_name, *cid1, *cid2 );
}

/*
** IIcsOpen
**	- Cursor OPEN begin
*/
void
IIXCSOPEN( cursor_name, cid1, cid2 )
char	*cursor_name;
i4	 *cid1;			/* cursor id 1			*/
i4	 *cid2;			/* cursor id 2			*/
{
    IIcsOpen( cursor_name, *cid1, *cid2 );
}

/*
** IIcsQuery
**	- Cursor OPEN end
*/
void
IIXCSQUERY( cursor_name, cid1, cid2 )
char	*cursor_name;
i4	 *cid1;			/* cursor id 1			*/
i4	 *cid2;			/* cursor id 2			*/
{
    IIcsQuery( cursor_name, *cid1, *cid2 );
}

/*
** IIcsRetrieve
**	- Cursor FETCH begin
*/
void
IIXCSRETRIEVE(ret_val,  cursor_name, cid1, cid2 )
char	*cursor_name;
i4	 *cid1;			/* cursor id 1			*/
i4	 *cid2;			/* cursor id 2			*/
i4	 *ret_val;		/* function value */
{
    *ret_val = IIcsRetrieve( cursor_name, *cid1, *cid2 );
    return; 
}

/*
** IIcsGetio
**	- Cursor FETCH data into user variable
*/
void
IIXCSGETIO( indflag, indptr, isvar, type, len, var_ptr )
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of data			*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;		/* pointer to user space	*/
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if ( *type == DB_CHR_TYPE )
	IIcsGetio( indptr, 1, DB_CHA_TYPE, *len, var_ptr );
    else
	IIcsGetio( indptr, 1, *type, *len, var_ptr );
}

/*
** IIcsERetrieve 
**	- Cursor FETCH end
*/
void
IIXCSERETRIEVE()
{
    IIcsERetrieve();
}

/*
** IIexDefine
**	- Repeat Query Definition
*/
void
IIXEXDEFINE( type, name, qid1, qid2 )
i4	 *type;			/* type of repeat definition	*/
char	*name;			/* name of query		*/
i4	 *qid1;			/* query id 1			*/
i4	 *qid2;			/* query id 2			*/
{
    IIexDefine( *type, name, *qid1, *qid2 );
}

/*
** IIexExec
**	- Repeat Query Execution
*/
void
IIXEXEXEC( type, name, qid1, qid2 )
i4	 *type;			/* type of repeat definition	*/
char	 *name;			/* name of query		*/
i4	 *qid1;			/* query id 1			*/
i4	 *qid2;			/* query id 2			*/
{
    _VOID_ IIexExec( *type, name, *qid1, *qid2 );
}

/*
** IIputctrl
**	- Place control character in the DBMS query string
*/
void
IIXPUTCTRL( ctrl )
i4	*ctrl;
{
    IIputctrl ( *ctrl );
}


/*
** IIxact
**	- Begin Transaction, Commit Transaction, End Transaction
*/
void
IIXXACT( flag )
i4	*flag;
{
    IIxact ( *flag );
}


/*
** IIvarstrio
**	- Send variable to the BE as string
*/
void
IIXVARSTRIO( indflag, indptr, isvar, type, len, var_ptr )
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of data			*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;		/* pointer to user space	*/
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if ( *type == DB_CHR_TYPE )
	IIvarstrio( indptr, 1, DB_CHA_TYPE, *len, var_ptr );
    else
	IIvarstrio( indptr, 1, *type, *len, var_ptr );
}


/*
** IIcsRdO
**	- Open cursor for Repeat Query readonly operations
*/
void
IIXCSRDO( flag, str )
i4	*flag;
char	*str;
{
    IIcsRdO( *flag, ERx("for readonly") );
}

/*
** IILQpriProcInit
**	- Create/Execute/Drop Procedure
*/
void
IIXLQPRIPROCINIT( type, name )
i4	*type;
char	*name;
{
    IILQpriProcInit( *type, name );
}


/*
** IILQprsProcStatus
**	- EXECUTE PROCEDURE control flow
*/
void
IIXLQPRSPROCSTATUS(ret_val)
i4	*ret_val;
{
    *ret_val = IILQprsProcStatus(0);
    return;	 
}

/*
** IILQprvProcValio
**	- EXECUTE PROCEDURE requiring i/o
*/
void
IIXLQPRVPROCVALIO( indflag, indptr, isvar, type, len, var_ptr )
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of data			*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;		/* pointer to user space	*/
{
    if ( !*indflag )
	indptr = (i2 *)0;

    if ( *type == DB_CHR_TYPE )
	IILQprvProcValio( indptr, 1, DB_CHA_TYPE, *len, var_ptr );
    else
	IILQprvProcValio( indptr, 1, *type, *len, var_ptr );
}

/*
** IIseterr
**	- user defined error function
*/
void
IIXSETERR( func )
i4	*func;
{
    IIseterr( func );
}

/*
** IIsqConnect
**	- error handling layer between ESQL program and IIingopen
** 	- Sends a variable number of arguments, up to a maximum of 15
**	  character string args plus language argument.
**	- we assume an extra first argument, which is a
**	  count representing the number of string args.  This is present
**	  on all calls which have a varying number of arguments.
**	  (See which calls go through gen_var_args in code generator.)
*/
void
IIXSQCONNECT( count, lan, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, 
	    arg9, arg10, arg11, arg12, arg13, arg14, arg15 )
i4	*count, *lan; 
char	*arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7, *arg8,
        *arg9, *arg10,*arg11,*arg12,*arg13, *arg14, *arg15;
{
# define 	ING_ARGS_MAX	15
    char	*v_args[ING_ARGS_MAX];
    i4  	i;

    for (i = 0; i < ING_ARGS_MAX; i++)
  	v_args[i] = (char *)0;		/* initialize arg array */

  /* 
  ** Enter this switch statement according to argument count.
  ** Fill v_args, starting at last arg and dropping through
  ** all subsequent ones until first arg
  */
    switch( *count - 1 ) {
	case 15:  v_args[14] = arg15;
	case 14:  v_args[13] = arg14;
	case 13:  v_args[12] = arg13;	
	case 12:  v_args[11] = arg12;	
	case 11:  v_args[10] = arg11;	
	case 10:  v_args[ 9] = arg10;	
	case  9:  v_args[ 8] = arg9;	
	case  8:  v_args[ 7] = arg8;	
	case  7:  v_args[ 6] = arg7;	
	case  6:  v_args[ 5] = arg6;	
	case  5:  v_args[ 4] = arg5;	
	case  4:  v_args[ 3] = arg4;	
	case  3:  v_args[ 2] = arg3;	
	case  2:  v_args[ 1] = arg2;	
	case  1:  v_args[ 0] = arg1;	
    }

    IIsqConnect( *lan, v_args[0], v_args[1], v_args[2], v_args[3], 
	v_args[4], v_args[5], v_args[6], v_args[7], v_args[8], v_args[9],
	v_args[10], v_args[11], v_args[12], v_args[13], v_args[14]); 
}

/*
** IIsqDisconnect
** 	- close up any open cursors before calling IIexit
*/
void
IIXSQDISCONNECT()
{
    IIsqDisconnect();
}


/*
** IIsqFlush
**	- processes sqlca  and resets error handling
*/
void
IIXSQFLUSH( file_name, lineno )
char	*file_name;
i4	 *lineno;
{
    /* B7872 - if a ptr to zero, then pass 0 */
    if (*(char **)file_name == (char *)0)
        IIsqFlush( 0, 0 );
    else
    	IIsqFlush( file_name, *lineno );
}

/*
** IIsqInit
**	- error handling layer between ESQL program and libq routines
*/
void
IIXSQINIT( sqlca )
i4	*sqlca;
{
    IIsqInit( sqlca );
}

/*
** IIsqlcdInit
**	- SQLCODE handling layer between ESQL program and libq routines
*/
void
IIXSQLCDINIT( sqlca, sqlcode )
i4	*sqlca;
i4	*sqlcode;
{
    IIsqlcdInit( sqlca, sqlcode );
}

/*
/*
** IIsqGdInit
**	- SQLSTATE handling layer between ESQL program and libq routines
*/
void
IIXSQGDINIT( op_type, sqlstate )
i4	*op_type;
char	*sqlstate;
{
    IIsqGdInit( *op_type, sqlstate );
}

/*
/*
** IIsqStop
**	- handles SQL STOP command 
*/
void
IIXSQSTOP( sqlca )
i4	*sqlca;
{
    IIsqStop( sqlca );
}

/*
** IIsqPrint
**	- in-house method of dumping error messages 
*/
void
IIXSQPRINT( sqlca )
i4	*sqlca;
{
    IIsqPrint( sqlca );
}

/*
** IIsqUser
**	- simulates and saves a "-u" flag for use by IIsqConnect
*/
void
IIXSQUSER( uname )
char	*uname;
{
    IIsqUser( uname );
}

#ifdef DG_LIKE_INGRES
/*
** IIsqTPC
**      - Passes in transaction id on connection made for Two-Phase Commit
*/
void
IIXSQTPC( highxid, lowxid )
i4  	*highxid, *lowxid;
{
    IIsqTPC( *highxid, *lowxid );
}
#endif /* DG_LIKE_INGRES */

/*
** IILQsidSessID
**	- Pass Session ID on CONNECT statement
*/
void
IIXLQSIDSESSID( sessid )
i4	*sessid;
{
    IILQsidSessID( *sessid );
}

/*
** IIsqExImmed
**	- EXECUTE IMMEDIATE query
*/
void
IIXSQEXIMMED( qry_str, length )
char	*qry_str;
i4	*length;
{
    char	*IIput_null();
    char	*new_qry;

    /* If length is zero, then query is already null terminated */
    if (*length == 0)
    {
    	IIsqExImmed( qry_str );
    }
    else
    {
	/* need to null terminate */
    	new_qry = IIput_null (qry_str, *length);
    	IIsqExImmed( new_qry );
    }
}

/*
** IIsqExStmt
**	- EXECUTE statement [USING ...]
*/
void
IIXSQEXSTMT( qry_str, using )
char	*qry_str;
i4	 *using;
{
    IIsqExStmt( qry_str, *using );
}


/*
** IIsqDaIn
**	- send descriptor of variables via IIputdomio
*/
void
IIXSQDAIN( lang, sqd )
i4      *lang;
PTR 	*sqd;
{
    IIsqDaIn( *lang, sqd );
}

/*
** IIsqPrepare
**	- Prepare and describe a statement
*/
void
IIXSQPREPARE( lang, stmt_str, sqd, using_fl, qry_str, length )
i4      *lang;
char	*stmt_str;
PTR 	*sqd;
i4      *using_fl;
char	*qry_str;
i4	*length;
{
    char	*IIput_null();
    char	*new_qry;

    /* If length is zero, then query is already null terminated */
    if (*length == 0)
    	new_qry = qry_str;
    else
	/* need to null terminate */
    	new_qry = IIput_null (qry_str, *length);

    if (*sqd == NULL)
       IIsqPrepare( *lang, stmt_str, NULL, *using_fl, new_qry );
    else
       IIsqPrepare( *lang, stmt_str, *sqd, *using_fl, new_qry );
}

/*
** IIsqDescribe
**	- Describe a statement into an SQLDA
*/
void
IIXSQDESCRIBE( lang, stmt_str, sqd, using_fl)
i4      *lang;
char	*stmt_str;
PTR 	*sqd;
i4      *using_fl;
{
    IIsqDescribe( *lang, stmt_str, sqd, *using_fl);
}


/*
** IIcsDaGet
**	- Fetch values out via an SQLDA
*/
void
IIXCSDAGET( lang, sqd )
i4      *lang;
PTR 	*sqd;
{
    IIcsDaGet( *lang, sqd );
}

/*
** IIsqMods
**	- Singleton Select or Gateway CONNECT WITH clause
*/
void
IIXSQMODS( flag )
i4	*flag;
{
    IIsqMods( *flag );
}


/*
** IIsqParms
**	- Gateway WITH clause on CONNECT
*/
void
IIXSQPARMS( op_type, name, type, var_ptr, val )
i4	*op_type;
char	*name;
i4	*type;
char	*var_ptr;
i4	*val;
{
	IIsqParms( *op_type, name, *type, var_ptr, *val );
}

#ifdef DG_LIKE_INGRES
/*
** IILQesEvStat
**	- GET EVENT initializing/finalizing routine
*/
void
IIXLQESEVSTAT( flag, waitsecs )
i4	*flag, *waitsecs;
{
    IILQesEvStat( *flag, *waitsecs );
}

/*
** IILQssSetSqlio
**	- EXEC SQL SET_SQL(attribute = value)
*/
void
IIXLQSSSETSQLIO( attr, indflag, indptr, isvar, type, len, var_ptr )
i4	*attr;		/* Object to set		*/
i4	*indflag;		/* 0 = no null indicator	*/
i2	*indptr;		/* null indicator pointer	*/
i4	*isvar;			/* Always 1 for F77		*/
i4	*type;			/* Type of user variable	*/
i4	*len;			/* Sizeof data, or strlen	*/
char	*var_ptr;		/* Variable containing value	*/
{
     if ( !*indflag )
 	indptr = (i2 *)0;
 
     if (*type == DB_CHR_TYPE)
     	IILQssSetSqlio( *attr, indptr,  1, DB_CHR_TYPE, *len, var_ptr );
     else
     	IILQssSetSqlio( *attr, indptr, 1, *type, *len, var_ptr );
}

/*
** IILQisInqSqlio
**	- EXEC SQL INQUIRE_INGRES(var = attribute)
**	- Called once for each object being inquired about
**	- We must blank pad user var if result var is a string
*/
 
void
IIXLQISINQSQLIO( indflag, indptr, isvar, type, len, var_ptr, attr )
i4	*indflag;		/* 0 = no null pointer		*/
i2	*indptr;		/* null pointer			*/
i4	*isvar;			/* by value or reference	*/
i4	*type;			/* type of user variable	*/
i4	*len;			/* sizeof data, or strlen	*/
char	*var_ptr;
i4	*attr;			/* inquire_sql attribute code	*/

{
   if ( !*indflag )
      indptr = (i2 *)0;

   if (*type == DB_CHR_TYPE)
   {
      IILQisInqSqlio( indptr, 1, DB_CHA_TYPE, *len, var_ptr, *attr );
   }
   else
   {
      IILQisInqSqlio( indptr, 1, *type, *len, var_ptr, *attr );
   }
}
/*
** IILQshSetHandler
**	- EXEC SQL INQUIRE_INGRES (HANDLER = funcptr)
**	  Where HANLDER can be ERRORHANDLER, MESSAGEHANDLER or EVENTHANDLER.
*/
void
IIXLQSHSETHANDLER(hdlr, funcptr)
i4      *hdlr;
i4      *funcptr;
{
	if (*funcptr == 0)
		IILQshSetHandler(*hdlr, (i4 (*)())0);
	else
		IILQshSetHandler(*hdlr, funcptr);
}


#endif /* DG_LIKE_INGRES */

/*
** IIput_null
**	- Dynamically allocate space (if necessary) to null terminate string.
*/
char
*IIput_null (qry_str, length)
char	*qry_str;
i4	length;
{
	static 	int	bufsize = 0;	/* size of current buffer allocated */
	static  char	*dyn_buf;	/* pointer to memory allocated */


	if ((bufsize != 0) &&  (length > bufsize))
	      MEfree (dyn_buf);
	if (length > bufsize)
	{
	   bufsize = length + 1;
	   dyn_buf = (char *)MEreqmem(0, bufsize, TRUE, NULL);
	   if (dyn_buf == NULL)
	   {
	       /* Fatal error */
	       IIlocerr(GE_NO_RESOURCE, E_LS0101_SDALLOC, 0, 0 );
	       IIabort();
	   }
	}
	STlcopy (qry_str, dyn_buf, length);
	return (dyn_buf);
}
/*
** IILQled_LoEndData
**	EXEC SQL ENDDATA
*/
void
IIXLQLED_LOENDDATA()
{
    IILQled_LoEndData();
}
# endif /* DGC_AOS */
