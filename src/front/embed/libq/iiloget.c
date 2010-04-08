/*
** Copyright (c) 2004 Ingres Corporation
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
# include <iisqlca.h>
# include <iilibq.h>
# include <erlq.h>
# include <generr.h>
# include <er.h>
# include <eqrun.h>
# include <fe.h>
# include <afe.h>

/* {
** Name: iiloget.c - Retrieve and Process Large Objects.
**
** Description:
**    This module contains the routines to handle the retrieval of large 
**    objects. 
**
** Defines:
**	IILQlvg_LoValueGet	- Get and entire large object.
**	IILQlgd_LoGetData	- Process the GET DATA statement.
**	IILQled_LoEndData	- Process the ENDDATA statement.
**	IILQlrs_LoReadSegment   - Read a segment of specified length.
**	IILQlag_LoAdpGet	- Read an ADP_PERIPHERAL from the dbms.
**	IILQlmg_LoMoreGet	- Read segment indicator from dbms.
**    
** History:
**	05-oct-1992 (kathryn) 
**	    Written.
**      10-feb-1993 (kathryn)
**          Use new macro ADP_HDR_SIZE rather than sizeof for adp peripheral.
**	11-feb-1993 (kathryn)
**	    Cast ADP_HDR_SIZE macro to (i4) so that result is signed,
**	    and comparisons performed against DB_DATA_VALUE lengths which may
**	    be "-1" will evaluate correctly.
**	28-feb-1994 (mgw) Bug #56880
**	    Add flag to adh_losegcvt() to tell whether to null terminate.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-jun-2001 (stial01)
**          IILQlrs_LoReadSegment UCS2 conversion for long nvarchar
**      25-jun-2001 (stial01)
**          More changes for DB_LNVCHR_TYPE
**      10-jul-2001 (stial01)
**          More changes for DB_LNVCHR_TYPE
**      12-sep-2001 (stial01)
**          Text array in nvarchar may not have DB_CNTSIZE alignment
**      24-jan-2005 (stial01)
**          Fixes for binary copy LONG NVARCHAR (DB_LNVCHR_TYPE) (b113792)
*/


/*{
** Name: IILQlvg_LoValueGet - Get an entire large object.
**
** Description:
**	This module is the standard sql method of retrieving a large object.
**      It reads an puts together the data segments of a large object and 
**	deposits the entire large object into the users variable.
**	If the program variable is not large enough the data will be truncated.
**
** Input:
**	thr_cb		Thread-local-storage control block.
**	indaddr:	Null Indicator variable.
**	isvar:		Variable.
**	hosttype:	Host variable type.
**	hostlen:	Host variable length.
**	addr:		pointer to host language variable.
**	dbv:		DB_DATA_VALUE of row descriptor.
**
** Outputs:
**      None.
**      Returns:
**          VOID.
**      Errors:
**
** Side Effects:
**
** History:
**      05-oct-1992 (kathryn)
**          Written.
**	01-nov-1992 (kathryn)
**	    Change definition of colnum from (i4 *) to just nat.
**	14-dec-1992 (kathryn)
**	    Blank pad DB_CHA_TYPE variables. This should be moved to
**	    ADH later.
**	15-jan-1993 (kathryn)
**	    Put in functionality to handle DB_DBV_TYPE and DB_VCH_TYPE.
**	    host variables.
**	10-dec-1993 (kathryn)
**	    Ensure that ADP_HDR will fit into DB_DATA_VALUE length.
**	01-mar-1993 (kathryn)
**	    Use new ii_lo_msgtype field to specify GCA message type.
**	26-apr-1993 (kathryn)
**	    Move code into new routine adh_losegcvt. Now blank-pad or
**	    EOS terminate char type segments.
**	    Add calls to IILQlcb_LoCbClear to clear the large object info
**	    structure before returning.
**	20-jun-1993 (kathryn) Bug 52769
**	    When truncating character string data, ensure that the indicator
**	    contains the original length of data, rather than the length 
**	    of what was truncated.
**	01-oct-1993 (kathryn)
**	    Subtract DB_CNTSIZE from max_len for varying datatypes.
**	28-feb-1994 (mgw) Bug #56880
**	    Add flag to adh_losegcvt() to tell whether to null terminate.
*/

STATUS
IILQlvg_LoValueGet
(
    II_THR_CB		*thr_cb,
    i2			*indaddr, 
    i4			isvar, 
    i4			hosttype, 
    i4		hostlen, 
    PTR			addr, 
    DB_DATA_VALUE	*dbv, 
    i4			colnum
)
{
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    i4			resval;
    i4			dataend= 0;	/* have we read all the segments */
    i4		seg_len;	/* actual segment length retrieved */
    i4		trunc;		/* length of truncated data */
    i4		max_len;	/* maximum length ot retrieve */
    bool		isnull;	 	/* is the large object a null */
    char                ebuf[20];	/* temp buf for error message */
    STATUS		stat;		/* Current status of reading data */
    IILQ_LODATA		*lodata;	/* LIBQ large object info structure */
    DB_DATA_VALUE	evdv;		/* temp DB_DATA_VALUE to get blob */
    DB_DATA_VALUE       *dbvptr;	/* set to addr when hosttype is DBV */
    ADP_PERIPHERAL      *adp_ptr;	/* ptr to adp_peripheral of blob */
    ADP_PERIPHERAL	adp_tmp;	/* adp_peripheral header info */

    if (indaddr != (i2 *)0)  		/* Reset null indicator just in case */
	*indaddr = 0;

    /* Check for null result addresses */
    if (!(PTR)addr)
    {
	CVna(colnum, ebuf);
	IIlocerr((i4)(GE_DATA_EXCEPTION + GESC_ASSGN),
		E_LQ0011_EMBNULLOUT, II_ERR, 1, ebuf );
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return FAIL;
    }

    seg_len = 0;
    lodata = &IIlbqcb->ii_lq_lodata;
    lodata->ii_lo_datatype = dbv->db_datatype;
    if (IIlbqcb->ii_lq_flags & II_L_COPY)
	lodata->ii_lo_msgtype = GCA_CDATA;
    else
    	lodata->ii_lo_msgtype = GCA_TUPLES;


    /* Set up the DBV (evdv) with db_data pointing directly into callers
    ** buffer. 
    */

    if (hosttype == DB_DBV_TYPE )
    {
	dbvptr = (DB_DATA_VALUE *)addr;
	II_DB_SCAL_MACRO(evdv, dbvptr->db_datatype, dbvptr->db_length,
			 dbvptr->db_data);

	/* evdv will not include the adp_peripheral header */

	if ( (IIDB_LONGTYPE_MACRO((DB_DATA_VALUE *)addr)) &&
	     (evdv.db_length >= (i4)ADP_HDR_SIZE) )
	{
	    adp_ptr = (ADP_PERIPHERAL *)evdv.db_data;
	    evdv.db_data = (PTR)&adp_ptr->per_value;
	    evdv.db_length -= (i4)ADP_HDR_SIZE;
	}
	else
	    adp_ptr = &adp_tmp;

	max_len = evdv.db_length;
 	if (ADH_NULLABLE_MACRO((DB_DATA_VALUE *)&evdv))
	    max_len -= 1;
    }
    else
    {
	II_DB_SCAL_MACRO(evdv, hosttype, hostlen, addr);
        adp_ptr = &adp_tmp;
	dbvptr = &evdv;
        if (evdv.db_length == 0)
            evdv.db_length = MAXI4;
	max_len = evdv.db_length;
    }
    if (IIDB_VARTYPE_MACRO(&evdv))
	max_len -= DB_CNTSIZE;

    /* Read the ADP_PERIPHERAL header */
    if (IILQlag_LoAdpGet( IIlbqcb, (PTR)adp_ptr ) != OK)
	return FAIL;
    adp_ptr->per_tag = ADP_P_DATA;
    adp_ptr->per_length0 = 0;

    /* Read the more segments indicator */
    if (IILQlmg_LoMoreGet( IIlbqcb ) != OK)
	return FAIL;

    /* The data is null - Set by IILQlmg_LoMoreGet */
    if (isnull = (lodata->ii_lo_flags & II_LO_ISNULL))
    {
	if ((hosttype == DB_DBV_TYPE) && 
	    ADH_NULLABLE_MACRO((DB_DATA_VALUE *)dbvptr))
	{
	    if (dbvptr->db_length > 0)
	        ADH_SETNULL_MACRO((DB_DATA_VALUE *)dbvptr);
	}
        else if (indaddr != (i2 *)0)
	{
    	    *indaddr = DB_EMB_NULL;
	}
	else
	{
	    CVna(colnum, ebuf);
	    IIlocerr(GE_DATA_EXCEPTION + GESC_NEED_IND, E_LQ000E_EMBIND,
			II_ERR, 1, ebuf);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    IILQlcc_LoCbClear( IIlbqcb );
	    return FAIL;
	}
    }

    /* Read the large object into the callers variable.
    ** Invoke IILQlrs_LoReadSegment, specifying a segment length that is the 
    ** size of the callers variable. IILQlrs_LoReadSegment will put the
    ** GCA segments together into the callers variable.
    */
    if (!isnull)
    {
	IIlbqcb->ii_lq_flags |= II_L_DATAHDLR;
	lodata->ii_lo_flags |= II_LO_GETHDLR;

        stat = IILQlrs_LoReadSegment( 0, IIlbqcb, dbv, &evdv, 
				      max_len, &seg_len, &dataend );
	if (stat != OK)
	{
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    IILQlcc_LoCbClear( IIlbqcb );
	    return FAIL;
	}
	adp_ptr->per_length1 = (u_i4)seg_len;
	adh_losegcvt(hosttype, (hostlen == 0 ? evdv.db_length : hostlen), 
			&evdv, seg_len, ADH_STD_SQL_MTHD);
        
	/* Variable is not large enough - truncate data */
        if (dataend == 0)
        {
	    trunc = IILQled_LoEndData();
	    if (indaddr != (i2 *)0)
	    {
		if ((trunc + seg_len) <= MAXI2)
		    *indaddr = trunc + seg_len;
                else
		    *indaddr = 0;
	    }
	    if (IISQ_SQLCA_MACRO(IIlbqcb))
		IIsqWarn( thr_cb, 1 );
        }
	
	IIlbqcb->ii_lq_flags &= ~II_L_DATAHDLR;
    }
    IILQlcc_LoCbClear( IIlbqcb );
    return OK;
}
/*{
** Name: IILQlgd_LoGetData - Process the GET DATA statment.
**
** Description:
**	Read a segment of a large object and deposit it into the host
**	variable.  This routine is generated from the following:
**       EXEC SQL GET DATA 
**		(:var = SEGMENT, :var = SEGMENTLENGTH, :var = DATAEND)
**	        WITH MAXLENGTH = :var | val;
**
** Input:
**    isvar: 	  Is there a segment variable (:host_var = SEGMENT)
**    hosttype:   Datatype of segment variable.
**    hostlen:	  Length of segment variable.
**    addr:	  Address of segment variable.
**    maxlen:	  Maximum size of segment to return.
**
** Outputs:
**    seglen:	  Actual segment size retrieved.
**    dataend:	  Is this the last segment.
**
** Returns:
**    VOID.
** Errors:
**
** Side Effects:
**
** History:
**      05-oct-1992     (kathryn)
**          Written.
**	21-jan-1993 	(kathryn)
**	    For DB_DATA_VALUES set evdv.db_data to dbvptr->db_data.
**	01-oct-1993 	(kathryn)
**	    Changes flags field to a bit mask.
**	28-feb-1994 (mgw) Bug #56880
**	    Add flag to adh_losegcvt() to tell whether to null terminate.
*/

void
IILQlgd_LoGetData(flags, hosttype, hostlen, addr, maxlen, seglen, dataend)
i4          	flags;
i4          	hosttype;
i4		hostlen;
char            *addr;
i4		maxlen;
i4		*seglen;
i4		*dataend;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    STATUS             	stat;		/* Result of getting data */
    i4            	max_len = 0;	/* max length to retrieve */
    i4		cur_seglen = 0; /* current segment length */
    i4              	data_end = 0;	/* all segments have been retrieved */
    i4                  *col_num;	/* col number in row descriptor */ 
    IILQ_LODATA		*lodata;	/* IIlbqcb large object info struct */
    DB_DATA_VALUE	evdv;		/* tmp DBV to retrieve blob */
    DB_DATA_VALUE	*dbv;		/* DBV of column from row descriptor */
    DB_DATA_VALUE	*dbvptr;	/* set to addr when hosttype is DBV */
    
    lodata = &IIlbqcb->ii_lq_lodata;
    
     
    if (IIlbqcb->ii_lq_curqry & II_Q_QERR)
        return;

    if (((IIlbqcb->ii_lq_flags & II_L_DATAHDLR) == 0) ||
	((lodata->ii_lo_flags & II_LO_GETHDLR) == 0)
       )
    {
	IIlocerr(GE_SYNTAX_ERROR, E_LQ00E2_GETHDLRONLY, 
		 II_ERR, 1, ERx("GET DATA"));
        IIlbqcb->ii_lq_curqry |= II_Q_QERR;
    }

    /* pointer variable with no max length specified */ 
    if ((hostlen <= 0) && (!(flags & II_DATLEN)) && (flags & II_DATSEG))
    {
	IIlocerr(GE_SYNTAX_ERROR, E_LQ00E3_GETNOLEN, II_ERR, 0, (char *)0);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
    }		
    if ((IIlbqcb->ii_lq_curqry & II_Q_QERR) || 
	(lodata->ii_lo_flags & II_LO_END))
    {
	cur_seglen = 0;
	data_end = 1;
    }
    else
    {
    
        /* Get Column Descriptor */
        if ((stat = IILQlcd_LoColDesc( thr_cb, &col_num, &dbv )) != OK)
        {
            IIlbqcb->ii_lq_curqry |= II_Q_QERR;
            return;
        }
	if (hosttype == DB_DBV_TYPE)
	{
	    dbvptr = (DB_DATA_VALUE *)addr;
	    max_len = dbvptr->db_length;
	    II_DB_SCAL_MACRO(evdv,dbvptr->db_datatype,dbvptr->db_length,
	    			dbvptr->db_data);
	    if (ADH_NULLABLE_MACRO(dbvptr))
	    	max_len -= 1;
	}
	else
	{
	   II_DB_SCAL_MACRO(evdv, hosttype, hostlen, addr);
	   if (IIDB_VARTYPE_MACRO(&evdv))
		max_len = hostlen - DB_CNTSIZE;
	   else 
		max_len = hostlen;
        }
        /* Determine the maximum length to read. max_len now contains
	** host variable buffer size, change  to user specified maxlen.
        ** It will be the maximum length specified by the caller or the
        ** length of the host variable whichever is smaller
        */

        if (flags & II_DATLEN)
        {
	    if ((maxlen < max_len) || (max_len == 0))
	   	max_len = maxlen; 
        }

	if (evdv.db_data != (PTR)0)
	{
	    stat = IILQlrs_LoReadSegment( flags, IIlbqcb, dbv, &evdv, 
					  max_len, &cur_seglen, &data_end );
	    if (stat == OK)
	        adh_losegcvt(hosttype, hostlen, &evdv, cur_seglen, 0);
	}
    }
    if (seglen != NULL)
	*seglen = cur_seglen;
    if (dataend != NULL)
	*dataend = data_end;
}

/* {
** Name: IILQled_LoEndData - Process EXEC SQL ENDDATA statement.
** 
** Description:
**	Skip remaining data segments of large object. This routine 
**	invokes IILQlmg_LoMoreGet to get the length of the next segment
**	and then calls IIcgc_get_value with a null pointer to skip the
**	segment, until the entire large object has been retrieved.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**	Returns: 
**	    Number of bytes skipped.
**	Errors:
**	    E_LQ00E2_GETHDLRONLY - ENDDATA statement has been issued outside
**				   of a datahandler.
**
** Side effects:
**	None.
**
** History:
**	23-oct-1992 (kathryn) Written.
**	01-nov-1992 (kathryn)
**	    Set lodata II_LO_END flag on completion.
**	01-mar-1993 (kathryn)
**	    Use new ii_lo_msgtype field to specify GCA message type.
**	20-jun-1993 (kathryn) Bug 52769
**	    Only set tot_len when bytes have been retrieved.
*/

i4
IILQled_LoEndData()
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    i4			tot_len = 0;
    i2			msg_seglen;
    STATUS		stat;
    IILQ_LODATA		*lodata;
    IICGC_MSG		*msg;
    DB_DATA_VALUE	dbv;
    

    lodata = &IIlbqcb->ii_lq_lodata;

    if (((IIlbqcb->ii_lq_flags & II_L_DATAHDLR) == 0) ||
        ((lodata->ii_lo_flags & II_LO_GETHDLR) == 0))
    {
        IIlocerr(GE_SYNTAX_ERROR, E_LQ00E2_GETHDLRONLY, II_ERR, 1,
		 ERx("ENDDATA"));
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return 0;
    }

    stat = OK;
    tot_len = 0;
    msg = &IIlbqcb->ii_lq_gca->cgc_result_msg;
    msg_seglen = msg->cgc_seg_len - msg->cgc_seg_used;

    if (msg_seglen == 0) 
    {
	if (msg->cgc_more_segs > 0)
	{
	    stat = IILQlmg_LoMoreGet( IIlbqcb );
	    msg_seglen = msg->cgc_seg_len - msg->cgc_seg_used;
	}
    }

    /* Read the segments from the DBMS and concatenate where necessary
    ** to copy get the length requested. 
    */
    while ((msg->cgc_more_segs == 1 || msg_seglen != 0) && (stat == OK))
    {

	dbv.db_datatype = DB_CHR_TYPE;
	dbv.db_data = NULL;			/* will not copy data */
	dbv.db_length = msg_seglen;
        tot_len += IIcgc_get_value(IIlbqcb->ii_lq_gca, lodata->ii_lo_msgtype, 
				   IICGC_VVAL, &dbv);
	stat = IILQlmg_LoMoreGet( IIlbqcb );
	msg_seglen = msg->cgc_seg_len - msg->cgc_seg_used;
    }
    lodata->ii_lo_flags |= II_LO_END;
    return (tot_len);
}

/*{
** Name: IILQlrs_LoReadSegment - Get the next segment of the Large Object.
**
** Description:
**    Get the next segment of a large object as specified by the caller.
**    This routine reads the large object value and constructs a segment
**    of the size that that the caller requested.  User applications
**    may specify the segment size that they wish to read.  
**
**    The GCA segments may be larger or smaller than the length specified
**    by the caller.  To avoid extra copying of the data, the fields
**    cgc_seg_len, cgc_seg_used, and cgc_more_segs are used to determine
**    how to read the GCA segment. 
**	cgc_seg_len:   number of bytes in current GCA segment
**	cgc_seg_used:  number of bytes of segment already read.
**      cgc_more_segs: when "0" there are no more segments.
**    The number of bytes to read before calling IILQlmg_LoMoreGet to get
**    the next segment is cgc_seg_len = cgc_seg_used.
**    
**
** Input:
**    IIlbqcb	Current Session control block.
**    dbv:	Column descriptor DB_DATA_VALUE.
**    evdv:	DB_DATA_VALUE of users segment variable.
**    max_len:  Maximum segment length to read. 
** 
** Outputs:
**    seglen:   Actual segment length. This will be less than max_len for
**	        the last segment.
**    dataend:  Set to "1" when the last segment has been read.
**
** Returns:
**	    Length of the segment retrieved.
**	Errors:
**
** Side Effects:
**
** History:
**	05-oct-1992	(kathryn)
**	    Written.
**	01-mar-1993 (kathryn)
** 	    Use new ii_lo_msgtype field to specify GCA message type.
**	01-oct-1993 (kathryn)
**	    Don't include the count field of varying types in seglen.
*/
STATUS
IILQlrs_LoReadSegment
(
    i4			flags,
    II_LBQ_CB		*IIlbqcb,
    DB_DATA_VALUE	*dbv, 
    DB_DATA_VALUE	*evdv, 
    i4		max_len,
    i4		*seglen, 
    i4			*data_end
)
{
	
    i2			msg_seglen = 0;	/* length of GCA segment */
    i4			res;		/* number of bytes read */
    PTR			evptr;		/* where to put next segment */
    i4		bytes_left = 0;	/* no of bytes left in gca segment */
    STATUS		stat;		/* current status */
    IICGC_MSG           *msg;		/* GCA message */
    IICGC_DESC          *cgc;		/* GCA Descriptor */
    IILQ_LODATA		*lodata;	/* Large object info structure */
    i4			loc_seglen;
    i4			n_chars;
    

    *seglen = loc_seglen = 0;
    evptr = evdv->db_data;
    msg = &IIlbqcb->ii_lq_gca->cgc_result_msg;
    lodata = &IIlbqcb->ii_lq_lodata;

    if (evdv->db_datatype == DB_NCHR_TYPE 
			    || evdv->db_datatype == DB_NVCHR_TYPE)
    {
	if (flags & II_BCOPY)
	    n_chars = max_len / sizeof(UCS2);
	else
	    n_chars = max_len / sizeof(wchar_t);
    }
    else
	n_chars = max_len;

    /* Set the db_data pointer correctly */

    if (evdv->db_datatype == DB_DBV_TYPE)
    {
        ((DB_DATA_VALUE *)evdv->db_data)->db_length = n_chars;
	evptr = ((DB_DATA_VALUE *)evdv->db_data)->db_data;
    }
    if (IIDB_VARTYPE_MACRO(evdv) ||
        (evdv->db_datatype == DB_DBV_TYPE &&  
	  IIDB_VARTYPE_MACRO((DB_DATA_VALUE *)(evdv->db_data)))
       )
    {
	/* evptr += DB_CNTSIZE; not true for host type DB_NVCHR_TYPE */
	if (evdv->db_datatype == DB_NVCHR_TYPE)
	    evptr = (PTR)((AFE_NTEXT_STRING *)evptr)->afe_t_text;
	else
	    evptr = (PTR)((DB_TEXT_STRING *)evptr)->db_t_text;
    }

    /* If long nchar data, We always get UCS2 format from the dbms */
    if (evdv->db_datatype == DB_NCHR_TYPE 
			    || evdv->db_datatype == DB_NVCHR_TYPE)
	max_len = n_chars * 2;
    else
	max_len = n_chars;

    while (((msg_seglen = (msg->cgc_seg_len - msg->cgc_seg_used)) > 0)
	   &&  (loc_seglen < max_len))
    {
	/* Determine how much of the segment to read */
	if (msg_seglen > (bytes_left = (max_len - loc_seglen)))
	    dbv->db_length = bytes_left;
        else
	    dbv->db_length = msg_seglen;

        /* point at the callers variable */

	dbv->db_data = evptr;

       /* read the data segment */

        res = IIcgc_get_value(IIlbqcb->ii_lq_gca, lodata->ii_lo_msgtype, 
		IICGC_VVAL,dbv);
        if (res != dbv->db_length)
        {
	    if (res != IICGC_GET_ERR)   /* Error should cause end of qry */
	        IIlocerr(GE_COMM_ERROR, E_LQ0040_SHORTREAD,II_ERR,0,(char *)0);
            IIlbqcb->ii_lq_curqry |= II_Q_QERR;
            return FAIL;
        }

	/* convert UCS2 to UCS4 if necessary */
	if ((evdv->db_datatype == DB_NCHR_TYPE 
		|| evdv->db_datatype == DB_NVCHR_TYPE)
		&& sizeof(wchar_t) != 2
		&& (flags & II_BCOPY) == 0) /* bulk copy, return UCS2 */
	{
	    i4		i;
	    i4		n_wchars = dbv->db_length / 2;
	    u_i2	*pi = ((u_i2 *)dbv->db_data) + (n_wchars - 1);
	    u_i4	*po = ((u_i4 *)dbv->db_data) + (n_wchars - 1);
	    u_i2	first;

	    first = *((u_i2 *)dbv->db_data);
	    for (i = 0; i < n_wchars - 1; ++i, --pi, --po)
		*po = (u_i4)*pi;
	    *((u_i4 *)dbv->db_data) = first;
	    res *= 2;
	}

	loc_seglen += dbv->db_length;

	/* This assumes seglen to be returned for wchar is # wchars */
	if (evdv->db_datatype == DB_NCHR_TYPE 
		|| evdv->db_datatype == DB_NVCHR_TYPE)
	    *seglen += dbv->db_length/2;
	else
	    *seglen += dbv->db_length;
	msg->cgc_seg_used += dbv->db_length;
	evptr += res;

	/* Finished the last segment are there more segments? */
	if (msg->cgc_seg_len == msg->cgc_seg_used && msg->cgc_more_segs != 0)
	    if (IILQlmg_LoMoreGet( IIlbqcb ) != OK)
	    {
		IIlbqcb->ii_lq_curqry |= II_Q_QERR;
		return FAIL;
	    }
    }

    if (msg->cgc_more_segs == 0 && (msg->cgc_seg_len == msg->cgc_seg_used))
    {
        *data_end = 1;
	lodata->ii_lo_flags |= II_LO_END;
    }

  return OK;
}

/* {
** Name: IILQlmg_LoMoreGet - Check for more segments of large object.
**
** Description:
**    This routine reads the GCA message buffer to deterimine if there are
**    more segments of the large object to be read.  If there are no more
**    segments (more = 0), then it will check the NULL byte, and set the
**    isnull flag appropriately.
**
**    If there are more segments, then this routine will read the segment
**    length and store it in the cgc_seg_len of the cgc result message,
**    for subsequent calls to IILQlrs_LoReadSegment which will read the actual
**    segment data.  
**
** Input:
**	IIlbqcb		Current session control block.
**
** Outputs:
**	None.
**	Returns:
**	    VOID.
**
** Side Effects:
**
** History:
**      05-oct-1992     (kathryn)
**          Written.
**	01-mar-1993	(kathryn)
**	    Use new ii_lo_msgtype field to specify GCA message type.
*/

STATUS
IILQlmg_LoMoreGet( II_LBQ_CB *IIlbqcb )
{
    i4  		more_segmts;
    i2			seg_len;
    i4                  resval;
    char		nullbyte;
    IICGC_MSG           *msg;
    IILQ_LODATA		*lodata;
    DB_DATA_VALUE	db_tmp;
    
    lodata = &IIlbqcb->ii_lq_lodata;
    msg = &IIlbqcb->ii_lq_gca->cgc_result_msg;
    msg->cgc_more_segs = 0;
    msg->cgc_seg_len = 0;
    msg->cgc_seg_used = 0;

    lodata->ii_lo_flags &= ~II_LO_ISNULL; 

    II_DB_SCAL_MACRO(db_tmp, DB_INT_TYPE, sizeof(more_segmts), &more_segmts);
    resval = IIcgc_get_value(IIlbqcb->ii_lq_gca, lodata->ii_lo_msgtype, 
	IICGC_VVAL, &db_tmp);
    if (resval != sizeof(more_segmts))
    {
        if (resval != IICGC_GET_ERR)   /* Error should cause end of qry */
            IIlocerr(GE_COMM_ERROR, E_LQ0040_SHORTREAD, II_ERR,0,(char *)0);
        IIlbqcb->ii_lq_curqry |= II_Q_QERR;
        return FAIL;
    }
    else
	msg->cgc_more_segs = more_segmts;

	
    if (more_segmts == 1)
    {
        II_DB_SCAL_MACRO(db_tmp, DB_INT_TYPE, sizeof(seg_len), &seg_len);
        resval = IIcgc_get_value(IIlbqcb->ii_lq_gca, lodata->ii_lo_msgtype,
                                 IICGC_VVAL,&db_tmp);
        if (resval != sizeof(seg_len))
        {
            if (resval != IICGC_GET_ERR)   /* Error should cause end of qry */
                IIlocerr(GE_COMM_ERROR, E_LQ0040_SHORTREAD, II_ERR,0,(char *)0);
            IIlbqcb->ii_lq_curqry |= II_Q_QERR;
            return FAIL;
        }
	if (abs(lodata->ii_lo_datatype) == DB_LNVCHR_TYPE) 
	    seg_len *= 2;
	msg->cgc_seg_len = seg_len;
    }
    else	
    {
	/* If there are no more segments to be read. Read the NULL byte */

	if (lodata->ii_lo_datatype < 0)
	{
            II_DB_SCAL_MACRO(db_tmp, DB_CHA_TYPE, 1, &nullbyte);
            resval = IIcgc_get_value(IIlbqcb->ii_lq_gca, lodata->ii_lo_msgtype,
                                 IICGC_VVAL, &db_tmp);
            if (resval != 1)
            {
                if (resval != IICGC_GET_ERR)   /* Error causes end of qry */
                    IIlocerr(GE_COMM_ERROR, E_LQ0040_SHORTREAD, II_ERR,
                                0, (char *)0);
                IIlbqcb->ii_lq_curqry |= II_Q_QERR;
                return FAIL;
            }
	    if (nullbyte & ADF_NVL_BIT)
	        lodata->ii_lo_flags |= II_LO_ISNULL;
	}
    }
    return OK; 
}

/*{
** Name: IILQlag_LoAdpGet - Read an ADP_PERIPHERAL of a large object.
**
** Description:
**	Read the ADP_PERIPHERAL header of the large object.
**	An adp_peripheral header consists of the first three
**      fields of an adp_peripheral:
**              i4              per_tag;
**              u_i4            per_length0;
**              u_i4            per_length1;
**
** Input:
**	IIlbqcb		Current session control block.
**	adp_per:   Pointer to the ADP_PERIPHERAL.
**
** Outputs:
**      None.
**      Returns:
**	    STATUS.
**      Errors:
**
** Side Effects:
**
** History:
**      05-oct-1992     (kathryn)
**          Written.
**	01-mar-1993	(kathryn)
**          Use new ii_lo_msgtype field to specify GCA message type.
*/

STATUS
IILQlag_LoAdpGet( II_LBQ_CB *IIlbqcb, PTR adp_ptr )
{
    ADP_PERIPHERAL	*adp_per = (ADP_PERIPHERAL *)adp_ptr;
    DB_DATA_VALUE       db_tmp;         /* General DBV for segments */
    i4			resval;
    IICGC_MSG           *msg;

    msg = &IIlbqcb->ii_lq_gca->cgc_result_msg;
    msg->cgc_seg_len = 0;
    msg->cgc_seg_used = 0;

    db_tmp.db_datatype = DB_INT_TYPE;
    db_tmp.db_length = (i4)ADP_HDR_SIZE;
    db_tmp.db_data = (char *)adp_per;

    resval= IIcgc_get_value(IIlbqcb->ii_lq_gca, 
		IIlbqcb->ii_lq_lodata.ii_lo_msgtype, IICGC_VVAL,&db_tmp);
    if (resval != db_tmp.db_length)
    {
    	if (resval != IICGC_GET_ERR)   /* Error should cause end of qry */
	    IIlocerr(GE_COMM_ERROR, E_LQ0040_SHORTREAD,II_ERR, 0,(char *)0);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return FAIL;

    }

    return OK;
}
