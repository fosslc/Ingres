/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <sl.h>
# include <cv.h>
# include <iicommon.h>
# include <cm.h>
# include <st.h>
# include <gca.h>
# include <adf.h>
# include <adp.h>
# include <iicgca.h>
# include <iirowdsc.h>
# include <iisqlca.h>
# include <iilibq.h>
# include <erlq.h>
# include <er.h>
# include <generr.h>

/* {
** Name: iilodata.c - Process the DATAHANDLER clause for Large Objects (BLOBS).
**
** Description:
**	This module contains the routines to invoke user defined datahandlers.
**
** Defines:
**	IILQldh_LoDataHandler - Call user defined data handler to process 
**				Large Object.
**	IILQlcd_LoColDesc - 	Get column descriptor of large object.
**	IILQlcc_LoCbClear - 	Clear LIBQ control block large object struct.
**
** History:
**	05-oct-1992 	(kathryn)
**	    Written.
**	21-jan-1993	(sandyd)
**	    Latest version of <adp.h> now requires <adf.h> to be included first.
**	25-Aug-1993 (fredv)
**		Needed to include <st.h>.
**	15-feb-1994 (mgw) Bug #58294
**		prevent reference through null pointer in IILQldh_LoDataHandler
**	06-may-1994 (teresal)
**		Make IILQldh_LoDataHandler define the handler function as
**		returning an i4  - this is the way we document it to 
**		users and this will make it consistent with the other
**		user-defined handlers. Bug 63202.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	aug-2009 (stephenb)
**		Prototyping front-ends
*/

# define IIGETHDLR	1
# define IIPUTHDLR  	2

/* {
** Name: IILQldh_LoDataHandler -- Call user defined data handler to process 
**				  Large Object.
**
** Description:
**  	This routine is called when an application uses the DATAHANDLER
**   	clause for retrieving or inserting a large object into the database.
**	EX:
**		EXEC SQL SELECT  column_name
**		INTO DATAHANDLER(user_func,arg):null_indicator
**		FROM table_name;
**			OR
**		EXEC SQL INSERT INTO table_name (column_name)
**		VALUES (DATAHANDLER(user_func,arg):null_indicator);
**		
**	    The DATAHANDLER clause will generate:
**		IILQldh_LoDataHandler(type,indvar,datahdlr,hdlr_arg)
**	    rather than the normal IIgetdomio or IIputdomio.
**	
**	If the large object data is not NULL, this routine sets up the local 
**	LIBQ control block large object structure and invokes the datahandler.
**	It is the repsonsiblity of the datahandler to loop on GET/PUT DATA
**	statements until the entire large object is transmitted.
**
** Input:
**	type	  -  Datahandler type 
**		      1: GET handler - generated from retrieve query.
**		      2: PUT handler - generated from insert query. 
**   	indvar	  -  Null Indicator Variable.
**	datahdlr  -  Pointer to datahandler - user defined function to invoke.
**	hdlr_arg  -  User specified pointer argument to pass to handler when
**		     it is invoked.
**
** Outputs:
**	None.
**	Returns:
**	    void.
**	Errors:
**	    E_LQ00E0_NODATAHDLR  - Datahandler function pointer is null.
**	    E_LQ00E1_DATHDLRTYPE - Invalid datatype for datahandler.
**	    E_LQ00E4_NOENDDATA   - Get handler returned before getting all data.
**	    E_LQ00E7_NODATEND	 - Put handler did not issued DATAEND param.
**
**
** Side Effects:
**
** History:
**      05-oct-1992     (kathryn) Written.
**	01-mar-1993 	(kathryn)
**	    Added changes for COPY to call datahandlers for BLOBS.
**	01-apr-1993	(kathryn)
**	    Added changes to use lodata->db_datatype set by COPY, and
**	    ensure that null indicator variable gets set correctly.
**	25-jun-1993 	(kathryn)
**	    Set II_LO_ISNULL flag when value is NULL.
**	20-jul-1993 (kathryn) Bug 53472
**	    Send DB_NUL_TYPE to IIputdomio for NULL value BLOB with datahandler.
**	01-oct-1993 (kathryn)
**	    Issue IIbreak() when error occurrs within PUT datahandler.
**	01-oct-1993 (kathryn) 55775
**	    Issue an error message when get handler receives a null value
**	    but no null indicator has been supplied.
**	15-feb-1994 (mgw) Bug #58294
**	    prevent referencing through a null pointer in case of IIGETHDLR
**	    with no indicator variable and non-null value.
**	06-may-1994 (teresal) Bug #63202
**	    Make the handler return a i4  not a long.
*/

void
IILQldh_LoDataHandler(type, indvar, datahdlr, hdlr_arg)
i4	type;
i2	*indvar;
i4  	(*datahdlr)();
PTR	hdlr_arg;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    i4                  resval;
    i4			*col_num;
    char		ebuf1[20], ebuf2[20];
    bool		isnull;
    i4		locerr;
    STATUS              stat = OK;
    IICGC_MSG           *msg;
    ADP_PERIPHERAL	tmp_adp;
    ADP_PERIPHERAL      *adp_per;
    DB_DATA_VALUE	*dbv;
    IILQ_LODATA		*lodata;

    if (IIlbqcb->ii_lq_curqry & II_Q_QERR)
	return;    

    if (datahdlr == NULL)
    {
        IIlocerr(GE_SYNTAX_ERROR, E_LQ00E0_NODATAHDLR, II_ERR, 0, (char *)0);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
        return;
    }

    adp_per = &tmp_adp;
    isnull = FALSE;
    lodata = &IIlbqcb->ii_lq_lodata;

    if (IIlbqcb->ii_lq_flags & II_L_COPY)
	lodata->ii_lo_msgtype = GCA_CDATA;
    else
	lodata->ii_lo_msgtype = GCA_TUPLES;

    /*
    ** The datahandler specified is for retrieving data from the database.
    ** Read the ADP_PERIPHERAL header and the first segment indicator.  If the
    ** segment indicator is zero, then the large object value is null, so
    ** just set the null indicator variable and return.
    */

    if (type == IIGETHDLR)
    {
	msg = &IIlbqcb->ii_lq_gca->cgc_result_msg;

	/* Cursor or Select Loop - Check number of columns - Set DBV */

	if ((stat = IILQlcd_LoColDesc( thr_cb, &col_num, &dbv )) != OK)
	{
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return;
        }

        lodata->ii_lo_datatype = dbv->db_datatype;

        /* We are not retreiving a large object - may not use datahandler */
	if (!IIDB_LONGTYPE_MACRO(dbv))
	{
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ00E1_DATHDLRTYPE, II_ERR,0,(char *)0);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
            stat = FAIL;
	}
        else if ((stat = IILQlag_LoAdpGet( IIlbqcb, (PTR)adp_per )) == OK)
        {
	    if ((stat = IILQlmg_LoMoreGet( IIlbqcb )) == OK)
	    {
		isnull = (lodata->ii_lo_flags & II_LO_ISNULL);
		if (isnull)
		{
		    if (indvar != (i2 *)0)
			*indvar = DB_EMB_NULL;
		    else if (isnull)
		    {
			CVna(*col_num + 1, ebuf1);
		        locerr = E_LQ000E_EMBIND;
			IIlocerr(GE_DATA_EXCEPTION + GESC_NEED_IND, locerr, 
				II_ERR, 1, ebuf1);
		        IIlbqcb->ii_lq_curqry |= II_Q_QERR;
		        stat = FAIL;
		    }
		}
		else
		{
		    if (indvar != (i2 *)0) /* for bug 58294 */
			*indvar = 0;
		    lodata->ii_lo_flags |= II_LO_GETHDLR;
	 	} 

	    } /* MoreGet OK */
	} /* AdpGet OK */
    } /* GetHandler */

    /*
    ** The datahandler specified is for sending data to the dbms.
    ** If the data is not null, then set the II_LO_START flag and invoke the
    ** datahandler.  We won't know the datatype (cannot send anything) until
    ** the datahandler is called and we have the segment type (char/byte).
    ** If the data is null then just send a null-valued DB_LTXT_TYPE, and 
    ** return. A null DB_LTXT_TYPE is coercible to any  datatype.
    ** For COPY, the datatypes have already been negotiated between FE and
    ** DBMS, so for a null value we must send a null large object. The
    ** the ii_lq_lodata field of the local control block has been set correctly
    ** by the internal COPY code.
    */
    else if (type == IIPUTHDLR)
    {
	if (indvar != (i2 *)0)
	{
	    if (*indvar == DB_EMB_NULL)
	    {
		isnull = TRUE;	
		if (lodata->ii_lo_msgtype == GCA_CDATA)
		{
		    lodata->ii_lo_flags |= II_LO_ISNULL;
		    IILQlap_LoAdpPut( IIlbqcb );
		    IILQlmp_LoMorePut( IIlbqcb, 0 );
		}
		else
		{
		    IIputdomio(indvar, 1, DB_NUL_TYPE, 0, (char *)0);
		}
	    }
	}
	if (!isnull)
	{
	    lodata->ii_lo_flags |= II_LO_PUTHDLR;
	    lodata->ii_lo_flags |= II_LO_START;
        }
    }	

    /* No errors, and the large object is not null - Invoke the Datahandler.*/

    if (!isnull && (stat == OK))
    {
	IIlbqcb->ii_lq_flags |=  II_L_DATAHDLR;

    
	if (hdlr_arg != NULL)
	    (void)(*datahdlr)(hdlr_arg);
	else
	    (void)(*datahdlr)();

	if (!(lodata->ii_lo_flags & II_LO_END))
        {
	    /* The caller returned from the datahandler prematurely. Either
	    ** from a GET handler without issuing and ENDDATA statement,
	    ** OR, from a PUT handler without setting the DATAEND parameter on
	    ** the PUT DATA statement.  Issue the Error, set the error flag.
	    */

	    if (type == IIGETHDLR)
	    {
		locerr = E_LQ00E4_NOENDDATA;
	    }
	    else
	    {
		locerr = E_LQ00E7_NODATAEND;
		IIbreak();
	    }
	    IIlocerr(GE_SYNTAX_ERROR, locerr, II_ERR, 0, (char *)0);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	}
    }

    /* We have completed the tranmission of this large object. 
    ** Clear the large object state information from the LIBQ control block.
    */

    IILQlcc_LoCbClear( IIlbqcb );
    if (type == IIGETHDLR)
	*col_num += 1;
    IIlbqcb->ii_lq_flags &=  ~II_L_DATAHDLR;
}

/* {
** Name: IILQlcd_LoColDesc - Get column descriptor.
**
** Description:
**	Determine the correct row desciptor from the current query in
**	process (SELECT/FETCH) and return the address of the row descriptor 
**	DB_DATA_VALUE and the address of the current column number.
**	This routine is called from IILQldh_LoDataHandler.
**	
** Input:
**	thr_cb		Thread-local-storage control block.
**
** Outputs:
**
** Side Effects:
**
** History:
**	01-nov-1992 (kathryn)
**	    Written.
**	10-dec-1992 (kathryn)
**	    Corrected check for columname.
**	01-mar-1993 (kathryn)
**	    Added check for COPY row descriptor (II_L_COPY).
**	26-Aug-2009 (kschendel) b121804
**	    Fix column number CVna call in error path.
*/

STATUS
IILQlcd_LoColDesc( II_THR_CB *thr_cb, i4  **col_num, DB_DATA_VALUE **dbv )
{
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    u_i4		col_nmlen;
    II_RET_DESC         *rd;
    IICS_STATE          *csr_cur;
    ROW_DESC		*row_desc;		
    ROW_COLNAME		*col_name;
    IILQ_LODATA		*lodata;
    char                csrname[ GCA_MAXNAME +1];
    char                ebuf1[20], ebuf2[20];

    lodata = &IIlbqcb->ii_lq_lodata;

    if (IIlbqcb->ii_lq_flags & 
       (II_L_DYNSEL | II_L_SINGLE | II_L_RETRIEVE | II_L_COPY))
    {
	rd = &IIlbqcb->ii_lq_retdesc;
	*col_num = &rd->ii_rd_colnum;
	row_desc = &rd->ii_rd_desc;
        if (**col_num >= row_desc->rd_numcols)
        {
            CVna(row_desc->rd_numcols, ebuf1);         /* i4  */
            CVna(**col_num+1, ebuf2);                /* i4  */
            IIlocerr(GE_CARDINALITY, E_LQ003C_RETCOLS, II_ERR, 2, ebuf1, ebuf2);
            IIlbqcb->ii_lq_curqry |= II_Q_QERR;
            if (IISQ_SQLCA_MACRO(IIlbqcb))
                IIsqWarn( thr_cb, 3 );    /* Status information for SQLCA */
            return FAIL;
        }
        *dbv = &rd->ii_rd_desc.RD_DBVS_MACRO(rd->ii_rd_colnum);
    }
    else
    {
        csr_cur = IIlbqcb->ii_lq_csr.css_cur;
	row_desc = &csr_cur->css_cdesc;
        *col_num = &csr_cur->css_colcount;
        if (**col_num >= csr_cur->css_cdesc.rd_numcols)
        {
            STlcopy( csr_cur->css_name.gca_name, csrname, GCA_MAXNAME );
            STtrmwhite( csrname );
            IIlocerr(GE_CARDINALITY, E_LQ005B_CSXCOLS, II_ERR, 2,
                         ERx("fetch"), csrname);
            IIlbqcb->ii_lq_curqry |= II_Q_QERR;
            if (IISQ_SQLCA_MACRO(IIlbqcb))
                IIsqWarn( thr_cb, 3 );    /* Status information for SQLCA */
            return FAIL;
        }
        *dbv = &csr_cur->css_cdesc.RD_DBVS_MACRO(csr_cur->css_colcount);
    }

    /* Save the column name for INQUIRE_SQL */

    if ((row_desc->rd_flags & RD_NAMES) && row_desc->rd_names != NULL
       && (**col_num < row_desc->rd_numcols))
    {
	col_name = &row_desc->rd_names[**col_num];
	col_nmlen = col_name->rd_nmlen;
	CMcopy(col_name->rd_nmbuf, col_nmlen, lodata->ii_lo_name);
    }

    return OK;
}
/* {
** Name: IILQlcc_LoCbClear - Clear the local control block large object struct.
**
** Description:
**	This routine clears the LIBQ local control block large object 
**	structure.  It should be called prior to sending/retrieving a large
**	object from the DBMS.  This structure maintains the information
**	about a large object between invocations of GET/PUT DATA statements. 
**
**	This routine also clears the LIBQ control block datahandler flag.
**
** Input:
**	IIlbqcb		Current session control block.
**
** Output:
**     None.
** 
** History:
**	20-apr-1993 (kathryn) Written.
**
*/
void
IILQlcc_LoCbClear( II_LBQ_CB *IIlbqcb )
{
	IILQ_LODATA         *lodata;

	IIlbqcb->ii_lq_flags &=  ~II_L_DATAHDLR;
	lodata = &IIlbqcb->ii_lq_lodata;
	lodata->ii_lo_maxseglen = (i4)0;
	lodata->ii_lo_name[0] = EOS;
	lodata->ii_lo_flags = (i4)0;
	lodata->ii_lo_msgtype = (i4)0;
	lodata->ii_lo_datatype = (DB_DT_ID)0;
}
