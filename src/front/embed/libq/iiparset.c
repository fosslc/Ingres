/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<er.h>
# include	<cm.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<eqrun.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<erlq.h>

/*{
+*  Name: IIparset - Send to the BE a string corresponding to the target list
**			of anything but a tupret.
**
**  Description:
**	This routine walks through the PARAM target list.  As it
**	processes each format specification (via IIParProc), it
**	sends the next element in the variable vector (argv) to the
**	DBMS.
**	See iiparproc.c for accepted format specifications:
**
**	Usage:
**		argv [0] = &double_raise;
**		IIparset("dom1 = i.ifield * (1+%f8)", argv, NULL);
**
**  Inputs:
**	string		- a string which contains the target list
**			  of a quel statement, where values from C variables
**			  to be plugged in are flagged by the construct
**			  '%<ingres_type>" a la SIprintf(). String is left
**			  unchanged.
**			  To escape the '%' mechanism use '%%'.
**	argv		- a vector of pointers to variables from which
**			  the values flagged by the '%' mechanism are taken.
**	transfunc	- A function pointer to transform non-C data
**			  (eg, VMS string descriptors).
**
**  Outputs:
**	Returns:
**	    FALSE on error, TRUE if it was a legal param string.
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	18-aug-1983 - Added %notrim for languages so users can
**		      prevent blank trimming. (lichtman)
**	22-nov-1983 - Fixed bug 1112 for illegal formats. (ncg)
**	10-oct-1986 - Modified for global-free 6.0 (mrw)
**	23-oct-1986 - Extracted out details of parsing param string (bjb)
**	26-feb-1986 - Added null indicator support (bjb)
**	12-dec-1988 - Generic error processing. (ncg)
**	24-mar-92 (leighb) DeskTop Porting Change:
**		      Added conditional function pointer prototypes.
**	18-jun-1997 (kosma01)
**	     AIX insists that the prototype for the transfunc use integers,
**	     since that is what is being passed.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

i4
IIparset(string, argv, transfunc)
char	*string;
char	**argv;
#ifdef	CL_HAS_PROTOS					 
char	*(*transfunc)(i4,i4,PTR);			 
#else							 
char	*(*transfunc)();
#endif							 
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    register char *b_st, *e_st;
    DB_DATA_VALUE dbv;
    register char **av;
    i2 		*indarg;
    char	*arg;
    i4		hosttype;
    i4		hostlen;
    i4		is_ind;		/* Flag for no indicator var */
    i4		stat;		/* Results of parsing target string */
    char	errbuf[20];	/* Null-terminated error text */

  /* If there was already an error on this query, just give up now. */
    if (IIlbqcb->ii_lq_curqry & II_Q_QERR)
	return FALSE;

    av = argv;
    for (b_st = e_st = string; *e_st;)
    {
	is_ind = FALSE;		/* Not yet seen an indicator variable spec */
	if (*e_st != '%')
	{
	    CMnext(e_st);
	    continue;
	}

	/* provide '%%' escape mechanism */
	if (e_st[1] == '%')
	{
	    II_DB_SCAL_MACRO(dbv, DB_QTXT_TYPE, e_st-b_st+1, b_st);
	    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv);
	    b_st = e_st = &e_st [2];
	    continue;
	}

      /* Send what we've seen so far. */
	II_DB_SCAL_MACRO( dbv, DB_QTXT_TYPE, e_st-b_st, b_st );
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv);
	CMnext(e_st);		/* Skip over the '%' */

      /* Parse next 'target' in param string */ 
	stat = IIParProc(e_st,&hosttype,&hostlen,&is_ind,errbuf,II_PARSET);
	if (stat < 0)			/* Error encountered? */
	{
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ001F_BADPARAM, II_ERR, 1, errbuf);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;	/* Stop later error proc'ing */
	    /*
	    ** Make sure that any query text already written down will
	    ** cause an error
	    */
	    II_DB_SCAL_MACRO(dbv, DB_QTXT_TYPE, 12, ERx("PARAM ERROR!"));
	    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv);
	    return FALSE;
	}
	else
	    e_st += stat;		/* Move along in target string */

	if (transfunc)		/* Allow entry point for non-C */
	    arg = (*transfunc)( hosttype, hostlen, *av );
	else
	    arg = *av;
	av++;			/* Next may be variable or indicator */
	if (is_ind == TRUE)
	{
	    /*
	    ** IIParProc found a valid indicator var description.
	    **	last av  -->  addr of data
	    **	this av  -->  addr of indicator var
	    */
	    if (transfunc)		/* Allow entry point for non-C */
		indarg = (i2 *)(*transfunc)( DB_INT_TYPE, sizeof(i2), *av );
	    else
		indarg = (i2 *)*av;
	    av++;
	}
	else
	    indarg = (i2 *)0;

	if (hostlen < 0)	/* May have been overloaded by IIParProc */
	    hostlen = 0;

	IIputdomio( indarg, TRUE, hosttype, hostlen, arg );
	b_st = e_st;
    }

    IIwritedb(b_st);
    IIlbqcb->ii_lq_curqry |= II_Q_PARAM;
    return TRUE;
}
