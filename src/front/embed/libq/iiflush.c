/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<iisqlca.h>
# include	<iilibq.h>

/**
+*  Name: iiflush.c - Flush any remaining data that returns from a RETRIEVE
**		      loop.
**
**  Defines:
**	IIflush	- Flush data returning after RETRIEVE.
**
**  Example:
**	For a RETRIEVE example see iiretinit.c
-*
**  History:
**	08-oct-1986	- Modified to use 6.0. (ncg)
**	26-mar-1990	- Loop-structured statements must push/pop a session
**			  id in order to catch inadvertent session switching
**			  in loops. Bug 9159.  (bjb)
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
+*  Name: IIflush - Flush data at the end of a RETRIEVE.
**
**  Description:
**	This routine is invoked at the end of a RETRIEVE loop, by the
**	preprocessor.  If the RETRIEVE was successful then this will just
**	fall out (probably read the final response information).  If the
**	RETRIEVE was not okay, then this may read error information or may
**	skip valid rows returing from the DBMS.
** 
**	Note that because the RETRIEVE statement may span another query (even
**	though that query would have been flagged as an error) it may not
**	be true that II_Q_INQRY is set, so set it.  The only way this should
**	be called is at the end of a RETRIEVE.  A RETRIEVE with a start up
**	error in it may drop below IIflush (see the IIerrtest call description
**	in iiretinit.c) via the preprocessor.  A singleton SELECT though does
**	not have the IIerrtest call, and falls through to this routine.
**	Note that in this case IIqry_end has already been called, but no
**	harm is done calling it again.
**
**  Inputs:
**	file_nm	- char * - File name for debugging.
**	line_no	- i4	 - Line number for debugging.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	1. Resets global RETRIEVE information.
**	2. Saves away row count just in case it is set to zero in IIqry_end.
**	
**  History:
**	09-mar-1985	- Skip reading the response block if normal exit. (lin)
**	08-oct-1986	- Modified to use ULC. (ncg)
**	15-mar-1991	- Changed IIdbg_info interface to pass II_G_DEBUG
**			  flag as part of adding last query text. (teresal)
**	04-apr-1991 	- When singleton select turn off II_L_SINGLE flag
**			  otherwise Pop session id (IILQspSessionPop).
**			  (kathryn) Bug 36849
**	15-dec-1992 (larrym)
**	     added support for BYREF parameters in database procedures.
*/

void
IIflush(file_nm, line_no)
char	*file_nm;
i4	line_no;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    i4		rows;

    if ( file_nm )  IIdbg_info( IIlbqcb, file_nm, line_no );

    /*
    ** If there was a nested query (even though in error) the INQRY
    ** flag will have been turned off by the ending of the bad nested
    ** nested query, so we turn it back on here.
    */
    if (IIlbqcb->ii_lq_retdesc.ii_rd_flags & II_R_NESTERR)
	IIlbqcb->ii_lq_curqry |= II_Q_INQRY;

    /* End off query */
    rows = IIlbqcb->ii_lq_rowcount;	/* Attempt to get number looped */

    IIqry_end();

    if (IIlbqcb->ii_lq_rowcount == 0)
	IIlbqcb->ii_lq_rowcount = rows;

    /* Reset global information */
    IIlbqcb->ii_lq_flags &= ~II_L_RETRIEVE;
    IIlbqcb->ii_lq_flags &= ~II_L_DBPROC;   /* in the case of BYREF params */
    IIlbqcb->ii_lq_retdesc.ii_rd_colnum = 0;
    IIlbqcb->ii_lq_retdesc.ii_rd_flags = II_R_DEFAULT;

    /* Pop loop id */
    if (IIlbqcb->ii_lq_flags & II_L_SINGLE)
	IIlbqcb->ii_lq_flags &= ~II_L_SINGLE;
    else
    	IILQspSessionPop( thr_cb );

}
