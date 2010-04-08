/*
** Copyright (c) 1995, 2005 Ingres Corporation
**
**
*/

# include <compat.h>
# include <gl.h>
# include <er.h>
# include <si.h>
# include <cs.h>
# include <di.h>
# include <pc.h>
# include <tr.h>
# include <iicommon.h>
# include  <ulf.h>
# include  <dbdbms.h>
#include    <adf.h>
# include  <dmf.h>
# include  <dm.h>
# include  <lg.h>
# include  <lk.h>
# include  <dmp.h>
# include  <dm0c.h>
# include  <dmfjsp.h>

/*
**  dmfio.c - DMF I/O module.
**
**  Description:
**      This file contains the function calls for output to stdout and stderr.
**      These functions have been moved from dmfjsp.
**  History:
**      24-Oct-95 (fanra01)
**          Created.
**	23-sep-1996 (canor01)
**	    Updated.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-oct-2005 (mutma03/devjo01)
**	    Add dmf_write_msg_with_args
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
*/

/*{
** Name: dmfWriteMsgFcn	- Write message to error output.
**
** Description:
**      This routine will lookup a standard message and write the test of the
**	message to the error output.
**
**	It will also log the error message in the error log.
**
**	We also call TRdisplay to echo the message to the trace file. If
**	TRset_file was not called, this TRdisplay is a no-op, otherwise it
**	sends the message to the appropriate trace log opened by TRset_file.
**
**	This function is called by way of the dmf_write_msg and 
**	dmf_write_msg_with_args macros, which supply __FILE__ and __LINE__.
**
** Inputs:
**      err_code                        Code for message to display.
**	FileName			Pointer to filename of caller
**	LineNumber			Source line number within that file.
**	numparams			Number of following parameters.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-dec-1986 (Derek)
**          Created for Jupiter.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    30-apr-1991 (bryanp)
**		Change ULE_LOOKUP to ULE_LOG; messages written through this
**		subroutine will now be logged to the error log as well, as they
**		are generally error messages which we want to keep a record of.
**		Call TRdisplay so that utility trace logs are more complete.
**      16-nov-2005 (devjo01)
**	    Split portion of code into write_msg_common.
**	08-Sep-2008 (jonj)
**	    SIR 120874: Condensed from dmf_write_msg(), dmf_write_msg_with_args(),
**	    now called via those (new) macros to identify caller's file/line.
**	    Deprecated write_msg_common().Use new form of uleFormat().
**	29-Oct-2008 (jonj)
**	    SIR 120874: Add non-variadic macros and functions for
**	    compilers that don't support them.
**	04-Nov-2008 (jonj)
**	    Changed form of dmfWriteMsgFcn to take DB_ERROR *dberr
**	    as first arg.
**	    Add dmfWriteMsgNV non-variadic function.
**	14-Nov-2008 (jonj)
**	    Delete dmf_write_msg variant functions; all code now uses
**	    dmfWriteMsg.
[@history_template@]...
*/
VOID
dmfWriteMsgFcn(
DB_ERROR	*dberr,
i4		err_code,
PTR		FileName,
i4		LineNumber,
i4		numparams,
...)
{
#define NUM_ER_ARGS	4
    char		message[ER_MAX_LEN+1];
    i4			l_message;
    DB_STATUS		local_err_code;
    DB_STATUS		status;
    DB_ERROR		localDBerror, *DBerror;
    va_list		ap;
    ER_ARGUMENT         er_args[NUM_ER_ARGS];
    i4			i;

    /*
    ** If missing dberr, or err_code
    ** overrides what's in dberr, use localDBerror,
    ** caller's file name and line number, and err_code.
    */
    if ( !dberr || err_code )
    {
        DBerror = &localDBerror;
	DBerror->err_file = FileName;
	DBerror->err_line = LineNumber;
	DBerror->err_code = err_code;
	DBerror->err_data = 0;

	/* Fill caller's dberr with that used */
	if ( dberr )
	    *dberr = *DBerror;
    }
    else
        DBerror = dberr;
        

    va_start( ap, numparams );
    for ( i = 0; i < numparams && i < NUM_ER_ARGS; i++ )
    {
	er_args[i].er_size = (i4) va_arg( ap, i4 );
	er_args[i].er_value = (PTR) va_arg( ap, PTR );
    }
    va_end( ap );

    status = uleFormat(DBerror, 0, NULL, ULE_LOG, NULL, message, sizeof(message),
	&l_message, &local_err_code, i,
	er_args[0].er_size, er_args[0].er_value,
	er_args[1].er_size, er_args[1].er_value,
	er_args[2].er_size, er_args[2].er_value,
	er_args[3].er_size, er_args[3].er_value );

    if (status == E_DB_OK)
    {
	SIwrite(l_message, message, &i, stderr);
	SIwrite(1, "\n", &i, stderr);

	TRdisplay("%t\n", l_message, message);
    }
    else
	SIprintf("Can't find message text for error: %x\n", err_code);
    SIflush(stderr);
}

/* Non-variadic function form, insert __FILE__, __LINE__ manually */
VOID
dmfWriteMsgNV(
DB_ERROR	*dberr,
i4		err_code,
i4		numparams,
...)
{
    va_list		ap;
    i4			ps[NUM_ER_ARGS];
    PTR			pv[NUM_ER_ARGS];
    i4			i;

    va_start( ap, numparams );
    for ( i = 0; i < numparams && i < NUM_ER_ARGS; i++ )
    {
	ps[i] = (i4) va_arg( ap, i4 );
	pv[i] = (PTR) va_arg( ap, PTR );
    }
    va_end( ap );

    dmfWriteMsgFcn( dberr,
		    err_code,
		    __FILE__,
		    __LINE__,
		    i,
		    ps[0], pv[0],
		    ps[1], pv[1],
		    ps[2], pv[2],
		    ps[3], pv[3] );
}


/*{
** Name: dmf_put_line	- Write message to standard output.
**
** Description:
**	Write line to standard output
**
**	We also call TRdisplay to echo the message to the trace file. If
**	TRset_file was not called, this TRdisplay is a no-op, otherwise it
**	sends the message to the appropriate trace log opened by TRset_file.
**
** Inputs:
**      err_code                        Code for message to display.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-dec-1986 (Derek)
**          Created for Jupiter.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    30-apr-1991 (bryanp)
**		Call TRdisplay so that utility trace logs are more complete.
[@history_template@]...
*/
i4
dmf_put_line(
PTR		arg,
i4		length,
char		*buffer)
{
    i4	count;

	SIwrite(length, buffer, &count, stdout);
	SIwrite(1, "\n", &count, stdout);
	SIflush(stdout);

	TRdisplay("%t\n", length, buffer);
}
