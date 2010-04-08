/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include	<iilibq.h>

/**
+*  Name: iisync.c - Sync up after sending a query to the DBMS.
**
**  Defines:
**	IIsyncup - Send query to the DBMS.
**
**  Example:
**	## range of e is emp
**  Generates:
**	IIwritedb("range of e is emp");
**	IIsyncup((char *)0, 0);
-*
**  History:
**	08-oct-1986	- Modified to use ULC. (ncg)
**	26-aug-1987 	- Modified to use GCA. (ncg)
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
+*  Name: IIsyncup - Send query to DBMS and read results (till end).
**
**  Description:
**	This routine sends the query to the DBMS, which executes the query
**	and returns the results.  In most cases, the results will just
**	be response information.
**
**  Inputs:
**	file_nm	- char * - File name for debugging.
**	line_no	- i4	 - Line number for debugging.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    Implicitly processes errors from the DBMS.
-*
**  Side Effects:
**	
**  History:
**	22-nov-1983	- Fixed response information before reading results
**			  so it won't be left around (B1696). (lictman)
**	08-oct-1986	- Modified to use ULC. (ncg)
**	26-aug-1987 	- Modified to use GCA. (ncg)
**	15-mar-1991	- Changed IIdbg_info interface to pass II_G_DEBUG
**			  flag as part of adding last query text. (teresal)
*/

void
IIsyncup(file_nm, line_no)
char	*file_nm;
i4	line_no;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    if ( file_nm )  IIdbg_info( IIlbqcb, file_nm, line_no );

    if (IIlbqcb->ii_lq_curqry & II_Q_INQRY)
	IIcgc_end_msg(IIlbqcb->ii_lq_gca);	/* Send query to DBMS */

    /*
    ** End off query and flush results - even if no query it will still
    ** do the right cleanup
    */
    IIqry_end();
}
