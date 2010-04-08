/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <gl.h>
# include <sl.h>
# include <me.h>
# include <iicommon.h>
# include <gca.h>
# include <adf.h>
# include <adp.h>
# include <adh.h>
# include <iicgca.h>
# include	<iisqlca.h>
# include <iilibq.h>
# include <erlq.h>
# include <generr.h>
# include <er.h>
# include <eqrun.h>
# include <fe.h>
# include <afe.h>


/* {
** Name: iiloput.c - Transmit Large Objects to the DBMS.
**
** Description:
**    This module contains the routines to transmit large objects to the
**    database.  Applications may insert/transmit a large object using a
**    program variable to hold the entire object (STD SQL Method) or
**    they may transmit the object a segment at time via the user defined
**    DATAHANDLER Method. When the STD SQL method is used, IILQlpd_LoPutData
**    is called from IIgetdomio with a segment size equal to the entire
**    large object.
**
** Defines:
**      IILQlpd_LoPutData		- Process PUT DATA statement, one
**					  user specified segment.
**	IILQlws_LoWriteSegment		- Write specified segment to DBMS
**					  will be broken into 2K segments.
**	IILQlap_LoAdpPut		- Write ADP_PERIPHERAL header
**	IILQlmp_LoMorePut		- Write more indicator (0 or 1)
**
**
** History:
**      05-oct-1992     (kathryn)
**          Written.
**      10-feb-1993 (kathryn)
**          Use new macro ADP_HDR_SIZE rather than sizeof for adp peripheral.
**	20-jul-1993 	(kathryn)
**	    Issue an error if not in datahandler when this routine is called.
**	01-oct-1993	(kathryn)
**	    No longer trace first segment of large object for Printqry.
**	    Large Object values will not be traced.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**    12-jun-2001 (stial01)
**          IILQlws_LoWriteSegment UCS2 conversion for long nvarchar
**    21-jun-2001 (stial01)
**          IILQlws_LoWriteSegment send #chars not #bytes for long nvarchar
**    27-jul-2001 (stial01)
**          If long nvarchar, hostlen is #bytes, SEGMENTLENGTH is #chars.
**    12-sep-2001 (stial01)
**          Text array in nvarchar may not have DB_CNTSIZE alignment
**    24-jan-2005 (stial01)
**          Fixes for binary copy LONG NVARCHAR (DB_LNVCHR_TYPE) (b113792)
**    15-Mar-2005 (schka24)
**	    Treat II_BCOPY to mean UCS2 format for unicode throughout.
**	    (It was working before, but rather mysteriously.)
[@history_template@]...
*/

/* {
** Name:  IILQlpd_LoPutData -- Transmit a large object segment to the DBMS.
**
** Description:
**	Tansmit a segment of a large object from the callers variable to
**	the DBMS. This routine is generated from the following sql stmt:
**	EXEC SQL PUT DATA
**	    (SEGMENT = :var|val, SEGMENTLENGTH = :var|val, DATAEND = :var|val)
**	This routine may be called with a segment, or just a dataend
**	parameter.  With the STD SQL method this routine will be called from
**	IIgetdomio with a segment length equal to the entire large object and 
**	the DATAEND parameter set to true.
**
**	This routine sets up a DB_DATA_VALUE (evdv) with the db_data field 
**	pointing directly into the callers buffer. It calls 
**      IILQlws_LoWriteSegment with the DB_DATA_VALUE to send the data to 
**	the server. 
**
**   Assumptions:
**	1) Only DB_DATA_VALUEs will ever have a large object datatype.
**	   (DB_LVCH_TYPE).  User Applications have no way of specifying a
**	   large object host variable.
**	2) A variable with a large object datatype (DB_LVCH_TYPE) is an
**	   ADP_PERIPHERAL (db_data points to an ADP_PERIPHERAL).
**
** Input:
**	isvar:  	Is there a segment variable.  - (SEGMENT = :host_var)
**	hosttype:	Datatype of segment variable.
**	hostlen:	Length of segment variable.
**	addr:		Address of segment variable.
**	seglen:		Length of segment to be transmitted.
**	dataend:	This is the last segment of the large object.
**
** Outputs:
**    None.
**    Returns: 
**	  VOID
**    Errors:
**	  E_LQ00E6_PUTNOLEN - No segmentlength and no hostlen (ptr).
**
** Hitory:
**    05-oct-1992 (kathryn)
**	    Written.
**    10-dec-1992 (kathryn)
**	    Modified to allow for no segment variable (just dataend). 
**    15-jan-1993 (kathryn)
**	    Put in functionality for handling DB_DBV_TYPE.
**	    Write large object value for printqry and savequery here.
**          The value traced for a large object will be the first segment
**          only.
**    01-apr-1993 (kathryn)
**	    Set the lodata->ii_lo_datatype field according to the variable type
**	    if it has not already been set. Copy may set nullable type.
**    25-jun-1993 (kathryn) 	Bug 52937 & 52938
**	    Set default ii_lo_datatype to DB_LVCH_TYPE.  
**    26-jul-1993 (kathryn)
**	    Add support for byte datatypes.
**    12-oct-1993 (kathryn)
**	    Add braces around II_DB_SCAL_MACRO in else stmt.
**    11-nov-1993 (smc)
**	    Bug #58882
**          Fixed up truncating cast in call to IIlocerr.
**    16-apr-99 (stephenb)
**	    init evdv, this won't happen if we are passed a zero length
**	    segment up front.
**    30-jul-99 (stial01)
**          IILQlpd_LoPutData() If not COPY, flush out the query text (to the
**          back end) before we start sending the large object data.
**    05-oct-2000 (stial01)
**          IILQlpd_LoPutData() Only flush querytext for FIRST blob column
**    07-nov-2000 (stial01)
**          Backout 30-jul-99 and 05-oct-2000 changes to flush querytext
**    20-apr-2001 (gupsh01)
**	    Added support for long nvarchar data type.
*/

VOID
IILQlpd_LoPutData(flags,hosttype,hostlen,addr,seglen,dataend)
i4              flags;
i4              hosttype;
i4		hostlen;
char            *addr;
i4         seglen;
i4          	dataend;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    DB_DATA_VALUE       *dv;		/* Set to addr when hosttype is DBV */
    DB_DATA_VALUE       evdv;		/* Temp DB_DATA_VALUE to send blob */
    IILQ_LODATA         *lodata;	/* LIBQ large object info structure */
    ADP_PERIPHERAL	*adp_per;	/* Ptr to adp_peripheral of blob */
    DB_DATA_VALUE       tmp_dbv;
    i4                  i;
    i4			wchar_size = sizeof(wchar_t);
    IICGC_DESC          *cgc = IIlbqcb->ii_lq_gca;

    lodata = &IIlbqcb->ii_lq_lodata;

    /* init evdv */
    MEfill(sizeof(DB_DATA_VALUE), 0, &evdv);

    /* If not in datahandler then this stmt not allowed. */
    if ( ((IIlbqcb->ii_lq_flags & II_L_DATAHDLR) == 0)
         || ((lodata->ii_lo_flags & II_LO_PUTHDLR) == 0)
       )
    {
        IIlocerr(GE_SYNTAX_ERROR, E_LQ00E5_PUTHDLRONLY,
            	 II_ERR, 1, ERx("PUT DATA"));
        IIlbqcb->ii_lq_curqry |= II_Q_QERR;
    }

    if (IIlbqcb->ii_lq_curqry & II_Q_QERR)
	return;

    /* If caller said input is UCS2, use that size */
    if (flags & II_BCOPY)
	wchar_size = sizeof(UCS2);

    /* There is a segment variable - not just dataend specifier 
    ** Set up a DBV (evdv) with db_data pointing to callers variable.
    */
    if (flags & II_DATSEG)
    {
	if (hosttype == DB_DBV_TYPE)
	{
	    dv = (DB_DATA_VALUE *)addr;
	    II_DB_SCAL_MACRO(evdv, dv->db_datatype, dv->db_length, dv->db_data);
	    if (ADH_NULLABLE_MACRO(dv))
	    {
		evdv.db_length -= 1;
		evdv.db_datatype = abs(evdv.db_datatype);
	    }
	}
	else
	{
	    II_DB_SCAL_MACRO(evdv,hosttype,hostlen,addr);
	}

	/* 3GL applications have no way of specifying a large object
	** variable.  Only internal DB_DATA_VALUES of type DB_DBV_TYPE
	** should ever have large object datatype.
	** ASSUMES NO COUPONS YET 
	*/
	if (IIDB_LONGTYPE_MACRO((DB_DATA_VALUE *)&evdv))
	{
	    adp_per = (ADP_PERIPHERAL *)evdv.db_data;
	    evdv.db_length = adp_per->per_length0;
	    evdv.db_data += (i4)ADP_HDR_SIZE;
	}
	/* Use the length specified in the cnt field of the varchar */
	else if (IIDB_VARTYPE_MACRO((DB_DATA_VALUE *)&evdv))
	{
	    evdv.db_length = (i2)((DB_TEXT_STRING *)evdv.db_data)->db_t_count;
	    if (hosttype == DB_NVCHR_TYPE)
	    {
		evdv.db_data = (PTR)((AFE_NTEXT_STRING *)evdv.db_data)->afe_t_text;
		evdv.db_length *= wchar_size;
	    }
	    else
		evdv.db_data += DB_CNTSIZE;
	}
	/* Use the length specified by the segmentlength attribute */
	else if (flags & II_DATLEN)
	{
	    /* this assumes SEGMENTLENGTH (seglen) for wchar is # wchars */
	    if (evdv.db_datatype == DB_NCHR_TYPE 
		    || evdv.db_datatype == DB_NVCHR_TYPE)
		seglen *= wchar_size;
	    if ((seglen < 0) || (seglen > hostlen && hostlen > 0))
	    {
		IIlocerr(GE_SYNTAX_ERROR, E_LQ00E9_BADSEGLEN, II_ERR,
				1, (PTR)&seglen);
		IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    }
	    else
		evdv.db_length = seglen;
	}
	/* segmentlength not specified - host variable is pointer */
	else if (hostlen == 0)
	{
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ00E6_PUTNOLEN, II_ERR, 0, (char *)0);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
        }

	if (IIlbqcb->ii_lq_curqry == II_Q_QERR)
		return;
    }

    /* Start Transmission of Large object 
    ** This is the first segment - Send ADP_PERIPHERAL header 
    */

    if (lodata->ii_lo_flags & II_LO_START)
    {
	    /* In dynamic SQL queries, (with the USING clause) a list of 
            ** variables may be sent without separating commas.
	    ** initial flag set in IIsqExStmt and IIcsOpen.
	    */
    	    if (IIlbqcb->ii_lq_curqry & II_Q_2COMMA) /* Not first one */
    	    {
        	DB_DATA_VALUE   dbcomma;

        	II_DB_SCAL_MACRO(dbcomma, DB_QTXT_TYPE, 1, ERx(","));
        	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbcomma);
    	    }
	    	/* First item - need commas on next */
    	    else if (IIlbqcb->ii_lq_curqry & II_Q_1COMMA) 
        	IIlbqcb->ii_lq_curqry |= II_Q_2COMMA;
            
	    /* Set datatype based on hosttype */
	    if (lodata->ii_lo_datatype == 0)
	    {
		if (IIDB_LONGTYPE_MACRO((DB_DATA_VALUE *)&evdv))
		    lodata->ii_lo_datatype = evdv.db_datatype;
		else if (IIDB_BYTETYPE_MACRO((DB_DATA_VALUE *)&evdv))
		    lodata->ii_lo_datatype = DB_LBYTE_TYPE;
		else if (IIDB_UNICODETYPE_MACRO((DB_DATA_VALUE *)&evdv))
		    lodata->ii_lo_datatype = DB_LNVCHR_TYPE;
		else /* This is default needed for empty strings */
		    lodata->ii_lo_datatype = DB_LVCH_TYPE;
	    }

            /* Put ADP_PERIPHERAL header */
	    IILQlap_LoAdpPut( IIlbqcb );
	    lodata->ii_lo_flags &= ~II_LO_START;
    }

    /* Send the actual data */
    if (addr != (PTR)0) 
    {
	IILQlws_LoWriteSegment( flags, IIlbqcb, &evdv );
    }

    /* send the no-more indicator and set end-flag for IILQldh_LoDataHandler */
    if (dataend == 1 && (!(lodata->ii_lo_flags & II_LO_END)))
    {
    	    IILQlmp_LoMorePut( IIlbqcb, 0 );
	    lodata->ii_lo_flags |= II_LO_END;
    }
}

/* {
** Name: IILQlws_LoWriteSegment -- Write a data segment.
**
** Description:
**   	Takes a segment from a program variable, and writes it to the 
**	GCA buffer. If the segment length is too large then split it into
**	2K segments.  
**
** Input:
**	IIlbqcb		Current session control block.
**	evdv:		DB_DATA_VALUE containing segment to be transmitted.
**
** Outputs:
**    none.
**    Returns:
**	VOID.
**
** History:
**	05-oct-1992 (kathryn)
**		Written.
**	01-oct-1993 (kathryn)
**	    Move handling of varying datatypes from this routine.
**	    IILQlpd_LoPutData will set evdv pointers correctly before calling.
*/

VOID
IILQlws_LoWriteSegment( i4 flags, II_LBQ_CB *IIlbqcb, DB_DATA_VALUE *evdv )
{
	
    i2			cur_seg_chars;  /* # chars in current segment */
    i4			cur_seglen;	/* length of current segment */
    i4			tot_seglen;	/* total segment length */
    i4			wchar_size;	/* UCS2 or native wchar_t size */
    PTR			evptr;		/* ptr to actual data */
    DB_DATA_VALUE	dbv_len;	/* DBV to send segment length */
    DB_DATA_VALUE	dbv_seg;	/* DBV to send segment */
    DB_DATA_VALUE	dbv_tmp;
    f8			setbuf[(DB_GW4_MAXSTRING + DB_CNTSIZE)/sizeof(f8) +1];
    DB_STATUS		stat;

    tot_seglen = evdv->db_length;
    if (evdv->db_datatype == DB_NCHR_TYPE 
	    || evdv->db_datatype == DB_NVCHR_TYPE)
    {
	/*
	** We always send UCS2 format to the dbms, however input wchar_t
	** may be UCS2 or UCS4 
	*/
	wchar_size = sizeof(wchar_t);
	if (flags & II_BCOPY)
	    wchar_size = sizeof(UCS2);
	else
	    tot_seglen = (evdv->db_length / sizeof(wchar_t)) * sizeof(UCS2);
    }

    evptr = evdv->db_data;
  
    II_DB_SCAL_MACRO(dbv_len, DB_INT_TYPE, sizeof(cur_seg_chars), &cur_seg_chars);

    while (tot_seglen > 0)
    {
	if (tot_seglen > DB_MAXSTRING)
	    cur_seglen = DB_MAXSTRING;
        else
	    cur_seglen = tot_seglen;

	IILQlmp_LoMorePut( IIlbqcb, 1 );

	if (evdv->db_datatype == DB_NCHR_TYPE 
		|| evdv->db_datatype == DB_NVCHR_TYPE)
	    cur_seg_chars = cur_seglen /2;
	else
	    cur_seg_chars = cur_seglen;

        IIcgc_put_msg(IIlbqcb->ii_lq_gca, TRUE, IICGC_VVAL, &dbv_len);    

	if (evdv->db_datatype == DB_NCHR_TYPE 
		|| evdv->db_datatype == DB_NVCHR_TYPE)
	{
	    if (wchar_size == 2)
	    {
		II_DB_SCAL_MACRO(dbv_seg, evdv->db_datatype, cur_seglen, evptr);
	    }
	    else
	    {
		/* ucs4 - ucs2 conversion required */
		II_DB_SCAL_MACRO(dbv_tmp, DB_NCHR_TYPE, cur_seglen*2, evptr);
		II_DB_SCAL_MACRO(dbv_seg, DB_NCHR_TYPE, cur_seglen, (PTR)setbuf);
		stat = afe_cvinto( IIlbqcb->ii_lq_adf, &dbv_tmp, &dbv_seg);

		if (stat != E_DB_OK)
		{
		    IIlocerr(GE_DATA_EXCEPTION + GESC_TYPEINV, E_LQ0014_DBVAR, 
				II_ERR, 0, (char *)0);
		    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
		    return;
		}
	    }
	    dbv_seg.db_datatype = DB_LNVCHR_TYPE;
	}
	else
	{
	    II_DB_SCAL_MACRO(dbv_seg, DB_LVCH_TYPE, cur_seglen, evptr);
	}

	IIcgc_put_msg(IIlbqcb->ii_lq_gca, TRUE, IICGC_VVAL, &dbv_seg);
	if ((evdv->db_datatype == DB_NCHR_TYPE
		    || evdv->db_datatype == DB_NVCHR_TYPE)
		&& wchar_size != 2)
	    evptr += cur_seglen * 2;
	else
	    evptr += cur_seglen;
	tot_seglen -= cur_seglen;
    }
}

/* {
** Name:  IILQlap_LoAdpPut -- Write the ADP_PERIPHERAL header.
**
** Description:
**	This routine writes the ADP_PERIPHERAL header to the GCA message
**	buffer.  An adp_peripheral header consists of the first three
**	fields of an adp_peripheral:
**		i4               per_tag;	
**		u_i4            per_length0;
**		u_i4            per_length1;
**	
**	Currently, all large objects are sent with:
**		per_tag = ADP_P_DATA	- Specifies Actual Data follows header
**		per_length0 = 0		- per_lengths of 0 indicate that the
**		per_length1 = 0		  actual length of the data is unknown.
** Input:
**	IIlbqcb		Current session control block.
**
** Output:
**	None.
**
** Side Effects:
**	An ADP_PERIPHERAL header is written to the message buffer.
**
** History:
**	05-oct-1992 (kathryn)
**		Written.
**	01-mar-1993 (kathryn)
**	    For COPY statement send VVAL rather than VDBV.
**	01-apr-1993 (kathryn)
**	    Use global IIlbqcb->ii_lq_lodata.ii_lo_datatype for setting type.
*/

VOID
IILQlap_LoAdpPut( II_LBQ_CB *IIlbqcb )
{
    ADP_PERIPHERAL	adp_per;
    ADP_PERIPHERAL	*aptr;
    DB_DATA_VALUE	db_tmp;

    aptr = &adp_per;
    aptr->per_tag = ADP_P_GCA_L_UNK;
    aptr->per_length0 = 0;
    aptr->per_length1 = 0;

    db_tmp.db_datatype = IIlbqcb->ii_lq_lodata.ii_lo_datatype;  
    db_tmp.db_length = (i4)ADP_HDR_SIZE;
    db_tmp.db_data = (PTR)aptr;
    db_tmp.db_prec = 0;
    if (IIlbqcb->ii_lq_flags & II_L_COPY)
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, TRUE, IICGC_VVAL, &db_tmp);
    else
        IIcgc_put_msg(IIlbqcb->ii_lq_gca, TRUE, IICGC_VDBV, &db_tmp);
}

/* {
** Name:  IILQlmp_LoMorePut -- Send the more indicator 
**
** Description:
**	This routine writes the "more" indicator to the GCA message buffer.
**	If the input is "0" then there are no more segments, the entire
**	large object has been transmitted.  If the large object was nullable
**	(datatype < 0) and there are no more segments (more = 0) then the 
**	null byte is written and the transmission is complete.
**
** Input:
**	IIlbqcb		Current Session control block.
**	more:     values 0 or 1   0 indicates that there are no more segments.
**
** Output:
**	None.
** History:
**	05-oct-1992 (kathryn)
**	    Written.
**	25-jun-1993 (kathryn)
**	    Set null byte when II_LO_ISNULL flag is set.
*/

VOID
IILQlmp_LoMorePut( II_LBQ_CB *IIlbqcb, i4  more)
{
    char		nullbyte;
    DB_DATA_VALUE       db_tmp;

    II_DB_SCAL_MACRO(db_tmp, DB_INT_TYPE, sizeof(more), &more);
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VVAL, &db_tmp);

    /* Write the NULL byte */
    if (!more && IIlbqcb->ii_lq_lodata.ii_lo_datatype < 0)  
    {
        II_DB_SCAL_MACRO(db_tmp, DB_CHA_TYPE, sizeof(nullbyte), &nullbyte);
	nullbyte = 0;
	if (IIlbqcb->ii_lq_lodata.ii_lo_flags & II_LO_ISNULL)
	    nullbyte |= ADH_NVL_BIT;
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VVAL, &db_tmp);
    }
}
