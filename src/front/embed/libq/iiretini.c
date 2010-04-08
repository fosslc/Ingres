/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<iirowdsc.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<erlq.h>

/**
+*  Name: iiretinit.c - Initialize a RETRIEVE/SELECT statement.
**
**  Defines:
**	IIretinit 	- Initialize a RETRIEVE.
**
**  Example:
**	## int	 i;
**	## float f;
**	## char *c;
**	## retrieve (i = t.icol, f = t.fcol, c = t.ccol)
**	## {
**		Code;
**	## }
**  Generates:
**	int i;
**	float f;
**	char *c;
**
**	IIwritedb("retrieve(i=t.icol,f=t.fcol,c=t.ccol)");
**	IIretinit((char *)0,0);
**	[if (IIerrtest() != 0) goto IIrtE1;]  -- not for singleton ESQL select
**  IIrtB1:
**	while (IInextget() != 0) {
**	    IIretdom(1,DB_INT_TYPE,sizeof(i4),&i);
**	    IIretdom(1,DB_FLT_TYPE,sizeof(f4),&f);
**	    IIretdom(1,DB_CHR_TYPE,0,c);
**	    if (IIerrtest() != 0) goto IIrtB1;
**	    {
**		Code;
**	    }
**	} 
**	IIflush((char *)0,0);
**  IIrtE1:
**
-*
**  History:
**	08-oct-1986	- Modified to use ULC. (ncg)
**	26-mar-1987	- Removed II_gettupdesc, and modified code to call
**			  IIrdDescribe. (ncg)
**	26-aug-1987 	- Modified to use GCA. (ncg)
**	08-dec-1989	- Updated for Phoenix/Alerters. (bjb)
**	26-mar-1990	- Push current session id to track errors in switching
**			  loops and sessions.  Bug 9159. (barbara)
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
**	11-jan-1990 (barbara)
**	    Fixed bug 35231 (see comments in IIretinit header)
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
+*  Name: IIretinit - Mark the start of a RETRIEVE.
**
**  Description:
**	This routine is called after RETRIEVE query has been written to the
**	DBMS.  If okay, the query expects to be followed by the data returning
**	from the DBMS.  This sets up the structures to process the results
**	in order to assign the DB values into host language variables.
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
** 	1. If an error occurs during this routine then the retrieve flags are
**	   set to INITERR so that IInextget() will fail and no data will
**	   attempt to be retrieved.  This was added to allow ESQL singleton 
**	   SELECT queries not to include the call to IIerrtest (which bypass 
**	   the IIflush call).
**	2. The DML (query language) in case LIBQ's record of this is changed
**	   by another query nested inside a SELECT/RETRIEVE loop.  Nesting may
**	   occur illegally (a query) or legally (the GET EVENT statement).
**	   IInextget() always restores ii_lq_dml from the saved value.  In
**	   this way, the parent SELECT/RETRIEVE will do the correct error
**	   handling, i.e. assign values to the SQLCA if ii_lq_dml is DB_SQL.
**	
**  History:
**	08-oct-1986	- Modified for 6.0. (ncg)
**	24-may-1988	- Sets tuple reading descriptor for INGRES/NET. (ncg)
**	12-dec-1988	- Generic error processing. (ncg)
**	22-may-1989	- Modified globals for multiple connects. (bjb)
**	08-dec-1989	- Changed name of where we save dml. (bjb)
**	26-mar-1990	- Push current session id to track errors in switching
**			  loops and sessions.  Bug 9159. (barbara)
**	11-jan-1990 (barbara)
**	    Reset ii_rd_flags AFTER test for local (LIBQ) error instead of
**	    BEFORE.  If the local error is that current select is nested
**	    inside another select, we must leave the NESTERR flag set,
**	    otherwise IIflush/IIqry_end of the parent select will not flush
**	    the parent select's data. (bug 35231)
**	15-mar-1991 (teresal)
**	    Changed interface to IIdbg_info to pass II_G_DEBUG flag.
**	    This is part of adding last query text feature. (teresal)
**	04-apr-1991 (kathryn)   Bug 36849
**	    Don't push session id (IILQsepSessionPush) for singleton select.
*/

void
IIretinit(file_nm, line_no)
char	*file_nm;
i4	line_no;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    II_RET_DESC	*ret;
    STATUS	stat;
    char	errbuf1[20], errbuf2[20];
    char	*qrystr;

    if ( file_nm )  IIdbg_info( IIlbqcb, file_nm, line_no );

# ifdef II_DEBUG
    if (IIdebug)
	SIprintf(ERx("IIretinit\n"));
# endif

    qrystr = IIlbqcb->ii_lq_dml == DB_QUEL ? ERx("retrieve") : ERx("select");

    ret = &IIlbqcb->ii_lq_retdesc;
    IIlbqcb->ii_lq_svdml = IIlbqcb->ii_lq_dml;	/* See Side Effects */

    if (IIlbqcb->ii_lq_curqry & II_Q_INQRY)
	IIcgc_end_msg(IIlbqcb->ii_lq_gca);	/* Send query to DBMS */

    /* Was there a local error? */
    if (((IIlbqcb->ii_lq_curqry & II_Q_INQRY) == 0) ||
        (IIlbqcb->ii_lq_curqry & II_Q_QERR))
    {
	IIqry_end();			/* Will reset flags anyway */
	return;
    }

    /* Assume there will be an error before the end of the routine */
    ret->ii_rd_flags = II_R_INITERR;

    stat = IIqry_read( IIlbqcb, GCA_TDESCR );

    /* 
    ** If an error occurred during the setup for the RETRIEVE, cleanup
    ** any returning data, as the preprocessor will exit the RETRIEVE loop.
    */
    if (stat == FAIL || II_DBERR_MACRO(IIlbqcb))
    {
	/* Assign error number to make sure preprocessor skips loop */
	if (IIlbqcb->ii_lq_error.ii_er_num == IIerOK)
	{
	    IIlocerr(GE_LOGICAL_ERROR, E_LQ003A_RETINIT, II_ERR, 1, qrystr);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	}
	IIqry_end();
	return;			/* End of query */
    }

    if (IIrdDescribe(GCA_TDESCR, &ret->ii_rd_desc) != OK)
    {
	if (IIlbqcb->ii_lq_error.ii_er_num == IIerOK)
	{
	    IIlocerr(GE_LOGICAL_ERROR, E_LQ003B_RETDESC, II_ERR, 1, qrystr);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	}
	IIqry_end();
	return;
    }

    /* Check if PARAM query variable numbers match */
    if ((IIlbqcb->ii_lq_curqry & II_Q_PARAM) &&
       (ret->ii_rd_patts != ret->ii_rd_desc.rd_numcols))
    {
	CVna(ret->ii_rd_desc.rd_numcols, errbuf1);
	CVna(ret->ii_rd_patts, errbuf2);
	IIlocerr(GE_CARDINALITY, E_LQ003C_RETCOLS, II_ERR, 2, errbuf1, errbuf2);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	IIqry_end();
	return;
    }

    /* Setup global information */
    IIlbqcb->ii_lq_flags |= II_L_RETRIEVE;
    IIlbqcb->ii_lq_rowcount = 0;
    IIlbqcb->ii_lq_retdesc.ii_rd_colnum = II_R_NOCOLS;
    IIlbqcb->ii_lq_retdesc.ii_rd_flags = II_R_DEFAULT;

    /*
    ** Save session id associated with this select loop.  This helps LIBQ
    ** catch errors from inadvertently fetching another session's data
    ** in loop.   (bug #9159)
    */
    if (!(IIlbqcb->ii_lq_flags & II_L_SINGLE))
    	IILQsepSessionPush( thr_cb );
}
