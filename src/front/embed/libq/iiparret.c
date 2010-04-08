/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<me.h>
# include	<cm.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<eqrun.h> 		/* Preprocessor/Runtime common */
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<erlq.h>

/**
+*  Name: iiparret.c - Execute the variable saving for PARAM RETRIEVE.
**
**  Defines:
**	IIparret - PARAM RETRIEVE execution.
**
-*
**  History:
**	16-sep-1984 	- Added support for OSL (%cXXX) (jrc)
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
+*  Name: IIparret - Writes a PARAM RETRIEVE and saves host variables for 
**		     later retrieval.
**
**  Description:
**	This routine walks through the PARAM target list.  It sends the
**	query text to the DBMS (via IIwritedb), and saves variables in the
**	PARAM fields of the global "ii_lq_retdesc" structure.
**
**	See iigettup.c for an example of how this is generated.
**	The actual arguments may look something like the following:
**	
**	argv[0] = (int *)&numvar;
**	argv[1] = (int *)&salvar;		/ May have null value /
**	argv[2] = (int *)&indicatorvar;
**	argv[3] = (int *)&namevar;
**	tlist = "%i4 = e.num, %f8:i2 = e.sal, %c = e.name";
**	IIparret(tlist, argv, NULL);
**
**	See iiparproc.c for accepted format specifications.
**
**  Inputs:
**	tlist	- char * - Target list containing everything inside the
**			   equivalent RETRIEVE statement, but instead of result
**			   domain names, the string should have '%<type>', where
**			   <type> is the INGRES type of the resulting column.
**			   To escape a '%' use 2 ("%%").
**	argv	- i4  ** - Argument array.
** 	xfunc	- i4  *()- Translation function for non-C data.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    MAXDOM      -- More than MAXDOM parameters.
-*
**  Side Effects:
**	1. Sets the PARAM fields in IIlbqcb->ii_lq_retdesc to flag that a PARAM
**	   RETRIEVE is being executed.
**	2. May allocate an array of DB_EMBEDDED_DATA values.
**	
**  History:
**	10-oct-1986	- Rewrote (mrw)
**	23-oct-1986	- Extracted code to parse elements in target list (bjb)
**	26-feb-1987	- Added new null indicator support (bjb)
**	12-dec-1988	- Generic error processing. (ncg)
*/

i4
IIparret(tlist, argv, xfunc)
char	*tlist;
char	**argv;
i4	(*xfunc)();
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    register char	*b_st, *e_st;
    register char	**av;
    DB_DATA_VALUE	dbv;
    II_RET_DESC		*ret;
    i4			is_ind;
    i4			hosttype;
    i4			hostlen;
    i4			resnum;	    	    /* Number of result parameter */
    char		buf[20];	    /* Buffer for param name/err msg */
    DB_EMBEDDED_DATA	edvarr[DB_GW2_MAX_COLS]; /* Place to put user vars */
    DB_EMBEDDED_DATA	*ed;		    /* Pointer into above array */
    i4			ednum;		    /* Number of EDV's used */
    i4			stat;		    /* Result of parsing param str */

  /* If there was already an error on this query, just give up now. */
    if (IIlbqcb->ii_lq_curqry & II_Q_QERR)
	return FALSE;

    ret = &IIlbqcb->ii_lq_retdesc;
    av = argv;				/* User's variables */
    resnum = 1;				/* Result column number */
    ed = &edvarr[0];			/* Where to put the user's EDV */
    ednum = 0;				/* ... and none used yet */
    ret->ii_rd_patts = 0;		/* No. of result attributes requested */

    for (b_st = e_st = tlist; *e_st; )
    {
	is_ind = FALSE;		/* No indicator var specification seen yet */
	if (*e_st != '%')
	{
	    CMnext(e_st);
	    continue;
	}

	/* Provide '%%' escape mechanism */
	if (e_st[1] == '%')
	{
	    II_DB_SCAL_MACRO(dbv, DB_QTXT_TYPE, e_st-b_st+1, b_st);
	    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv);
	    b_st = e_st = &e_st[2];	/* Skip them both */
	    continue;
	}

        /* Send what we've seen so far. */
	II_DB_SCAL_MACRO( dbv, DB_QTXT_TYPE, e_st-b_st, b_st );
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv);
	CMnext(e_st);		/* Skip over the '%' */

	/* Send dummy result columns, must be documented for SORT clause */
	STprintf(buf, ERx("RET_VAR%d"), resnum);
	II_DB_SCAL_MACRO(dbv, DB_QTXT_TYPE, STlength(buf), buf);
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv);

	if (resnum++ > DB_GW2_MAX_COLS)
	{
	    CVna((i4)DB_GW2_MAX_COLS, buf);
	    IIlocerr(GE_CARDINALITY, E_LQ0020_PARMMAX, II_ERR, 1, buf);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return FALSE;
	}

	/* Parse next target in param string */
	stat = IIParProc(e_st,&hosttype,&hostlen,&is_ind,buf,II_PARRET);

	if (stat < 0)			/* Error encountered? */
	{
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ001F_BADPARAM, II_ERR, 1, buf);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;	/* Stop more error proc'sing */
	    /*
	    ** Make sure that any query text already written down will
	    ** cause an error
	    */
	    II_DB_SCAL_MACRO(dbv, DB_QTXT_TYPE, 12, ERx("PARAM ERROR!"));
	    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv);
	    return FALSE;
	}
	e_st += stat;			/* Move along in param string */

	ed->ed_type = hosttype;
	ed->ed_length = hostlen;
	ed->ed_data = (PTR)*av++;
	if (is_ind == TRUE)
	    ed->ed_null = (i2 *)*av++;
	else
	    ed->ed_null = (i2 *)0;
	ret->ii_rd_patts++; 	/* Number of result attributes */
	ednum++;		/* Number of EDV's used */
	ed++;
	b_st = e_st;
    } /* for */

    IIwritedb( b_st );
    IIlbqcb->ii_lq_curqry |= II_Q_PARAM;

    ret->ii_rd_ptrans = xfunc;
    if (ednum == 0)
	return TRUE;		/* Error later in IIretinit - mismatch cols */

    ed = ret->ii_rd_param;

    /* Can we reuse the RETRIEVE's PARAM array of EDV's ? */
    if (ed != (DB_EMBEDDED_DATA *)0 && ret->ii_rd_pnum < ednum)
    {
	MEfree((PTR)ed);
	ed = (DB_EMBEDDED_DATA *)0;	/* Will be reallocated */
    }
    /* Is PARAM array null - allocate correct size */
    if (ed == (DB_EMBEDDED_DATA *)0)
    {
	if ((ed = (DB_EMBEDDED_DATA *)MEreqmem((u_i4)0,
	    (u_i4)(ednum * sizeof(*ed)), TRUE, (STATUS *)NULL)) == NULL)
	{
	    IIlocerr(GE_NO_RESOURCE, E_LQ0021_RETPARAM, II_ERR, 0, (char *)0);
	    ret->ii_rd_param = (DB_EMBEDDED_DATA *)0;
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return FALSE;
	}
	ret->ii_rd_param = ed;
	ret->ii_rd_pnum = ednum;
    }
    /* 
    ** ii_rd_param - Non-null RETRIEVE PARAMA array.
    ** ii_rd_pnum  - Number of EDV's in PARAM array.
    ** ii_rd_patts - Number of result columns retrieved.
    ** ii_rd_ptrans- Translation function for host variables.
    ** ed          - Points at RETRIEVE PARAM array.
    ** ednum  	   - Number of EDV's used for this PARAM instance <= ii_rd_pnum.
    ** edvarr 	   - EDV's used this time round.
    */
    MEcopy((PTR)edvarr, ednum*sizeof(*ed), (PTR)ret->ii_rd_param);
    return TRUE;
}
