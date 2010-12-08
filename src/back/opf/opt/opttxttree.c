/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <ade.h>
#include    <adfops.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <tr.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefqeu.h>
#include    <qefmain.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>
#include    <opckey.h>
#include    <opcd.h>
#define        OPT_TXTTREE		TRUE
#include    <cm.h>
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: optdtext.c - Display distributed query text from co trees
**
**  Description:
**      Routines to dump trace output of the action headers and query
**	text for distributed compiled queries. 
**
**          opt_qtdump - dump query text for an action
**
**
**  History:
**      27-sep-88 (robin)
**          Created.
**      26-mar-91 (fpang)
**          Renamed opdistributed.h to opdstrib.h
**      3-jun-91 (seputis)
**          Renamed header files to match local
**      11-jun-91 (seputis)
**          added cm.h header file
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	30-jun-93 (rickh)
**	    Added TR.H.
**	15-sep-93 (swm)
**	    Moved cs.h include above other header files which need its
**	    definition of CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opt_seg_dump(
	OPS_STATE *global,
	QEQ_TXT_SEG *qseg);
void opt_ldb_dump(
	OPS_STATE *global,
	QEQ_LDB_DESC *ldbdesc,
	bool justone);
void opt_qrydump(
	OPS_STATE *global,
	QEQ_D1_QRY *qryptr);
void opt_qtdump(
	OPS_STATE *global,
	QEF_AHD *dda);
void opt_qpdump(
	OPS_STATE *global,
	QEF_QP_CB *qplan);
bool opt_chkprint(
	char *buf_to_check,
	i4 buflen);
void opt_seg_chk(
	OPS_STATE *global,
	QEQ_D1_QRY *qseg);
void opt_qtchk(
	OPS_STATE *global,
	QEF_QP_CB *qplan);

/*{
** Name: opt_seg_dump	- Dump a list of text segments
**
** Description:
**      Display a list of text segments as trace output.
**	Text will be used as literally specified in
**	the segment.  Temp table names will be displayed
**	as 't' followed by the temp number. 
**
** Inputs:
**	global				Ptr to the global state variable
**      qseg                            addr of first QEQ_TXT_SEG
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
**      27-sep-88 (robin)
**          Created.
[@history_template@]...
*/
VOID
opt_seg_dump(
	OPS_STATE	   *global,
	QEQ_TXT_SEG        *qseg)
{
    DD_PACKET	*pktptr;
    QEQ_TXT_SEG	*segptr;
    i4		lensofar;    
        
    if ( qseg == NULL )
    {	
	return;
    }

    /*  Print initial newline */

    TRformat( opt_scc, (i4 *) global,
	(char *) &global->ops_trace.opt_trformat[0],
	    (i4) sizeof(global->ops_trace.opt_trformat),
	    "%s", "\n");

    for ( segptr = qseg; segptr != NULL; segptr = segptr->qeq_s3_nxt_p )
    {
	for (lensofar = 0,pktptr = segptr->qeq_s2_pkt_p; pktptr != NULL; pktptr = pktptr->dd_p3_nxt_p )
	{
	    if (lensofar >= 55)
	    {
	    	TRformat( opt_scc, (i4 *) global,
		    (char *) &global->ops_trace.opt_trformat[0],
		    (i4) sizeof(global->ops_trace.opt_trformat),
		    "%s", "\n");
		lensofar = 0;
	    }

	    if (pktptr->dd_p4_slot != DD_NIL_SLOT )
	    {
		/* this segment specifies a temp table or column */
	    	TRformat( opt_scc, (i4 *) global,
		    (char *) &global->ops_trace.opt_trformat[0],
		    (i4) sizeof(global->ops_trace.opt_trformat),
		    "**{ %d }**", pktptr->dd_p4_slot);
		lensofar += 10;
	    }
	    else
	    {
		/* there is text in this segment */
		TRformat( opt_scc, (i4 *) global,
		    (char *) &global->ops_trace.opt_trformat[0],
		    (i4) sizeof(global->ops_trace.opt_trformat),
		    "%t", pktptr->dd_p1_len, pktptr->dd_p2_pkt_p );
		lensofar += pktptr->dd_p1_len;

	    }
	}
    }
}


/*{
** Name: opt_ldb_dump	- Dump an ldb descriptor
**
** Description:
**      Print out an ldb descriptor for tracing. 
**
** Inputs:
**      global                          Global state variable
**      ldbdesc                         Addr of ldb desc. to display
**	justone				True if only 1 should be dumped
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**      27-sep-88 (robin)
**          Created.
[@history_template@]...
*/
VOID
opt_ldb_dump(
	OPS_STATE	*global,
	QEQ_LDB_DESC	*ldbdesc,
	bool		justone)
{
    DD_LDB_DESC         *ddldbptr;
    char		*tfptr;
    bool		stop;

    
    TRformat( opt_scc, (i4 *) global,
        (char *) &global->ops_trace.opt_trformat[0],
        (i4) sizeof(global->ops_trace.opt_trformat),
        "%s","\nLDB DESCRIPTORS:\n");

    for(stop = FALSE ; ldbdesc != NULL && stop == FALSE; ldbdesc = ldbdesc->qeq_l2_nxt_ldb_p )
    {
	if (justone)
	    stop = TRUE;

	ddldbptr = &ldbdesc->qeq_l1_ldb_desc;

	TRformat( opt_scc, (i4 *) global,
	    (char *) &global->ops_trace.opt_trformat[0],
	    (i4) sizeof(global->ops_trace.opt_trformat),
	    "\nNODE NAME: %24t", sizeof(ddldbptr->dd_l2_node_name), 
	    ddldbptr->dd_l2_node_name );

	TRformat( opt_scc, (i4 *) global,
	    (char *) &global->ops_trace.opt_trformat[0],
	    (i4) sizeof(global->ops_trace.opt_trformat),
	    "\nLDB NAME: %24t", DD_256_MAXDBNAME, ddldbptr->dd_l3_ldb_name );

	TRformat( opt_scc, (i4 *) global,
	    (char *) &global->ops_trace.opt_trformat[0],
	    (i4) sizeof(global->ops_trace.opt_trformat),
	    "\nDBMS NAME: %24t", sizeof(ddldbptr->dd_l4_dbms_name), 
	    ddldbptr->dd_l4_dbms_name );

	if (ddldbptr->dd_l1_ingres_b)
	    tfptr = " ";
	else
	    tfptr = "not";

	TRformat( opt_scc, (i4 *) global,
	    (char *) &global->ops_trace.opt_trformat[0],
	    (i4) sizeof(global->ops_trace.opt_trformat),
	    "\nLDB ID: %5d   $INGRES status is %s required for access", ddldbptr->dd_l5_ldb_id, tfptr );

    }

    return;
}


/*{
** Name: opt_qrydump	-   Dump a QEQ_D1_QRY structure
**
** Description:
**
** Inputs:
**	global		global stat variable
**	qryptr		Ptr to the QEQ_D1_QRY to dump
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
**      08-dec-88 (robin)
**          Created.
[@history_template@]...
*/
VOID
opt_qrydump(
	OPS_STATE   *global,
	QEQ_D1_QRY  *qryptr)
{
	    char    *varstring;
	    i4	    n;

	    if (qryptr->qeq_q1_lang == DB_SQL)
		varstring = "DB_SQL";
	    else if (qryptr->qeq_q1_lang == DB_QUEL)
		varstring = "DB_QUEL";
	    else
		varstring = "***UNKNOWN - ERROR****";

	    TRformat( opt_scc, (i4 *) global,
		(char *) &global->ops_trace.opt_trformat[0],
		(i4) sizeof(global->ops_trace.opt_trformat),
		"\n Query Language: %s", varstring);	

	    TRformat( opt_scc, (i4 *) global,
		(char *) &global->ops_trace.opt_trformat[0],
		(i4) sizeof(global->ops_trace.opt_trformat),
		"\n Quantum: %d", qryptr->qeq_q2_quantum);	

	    if (qryptr->qeq_q3_read_b == TRUE)
		varstring = "TRUE";
	    else
		varstring = "FALSE";
    	    TRformat( opt_scc, (i4 *) global,
		(char *) &global->ops_trace.opt_trformat[0],
		(i4) sizeof(global->ops_trace.opt_trformat),
		"\n Retrieval (qeq_q3_read_b): %s", varstring);

	    opt_seg_dump( global, qryptr->qeq_q4_seg_p);
	    opt_ldb_dump( global, (QEQ_LDB_DESC *)qryptr->qeq_q5_ldb_p, (bool) TRUE );

    	    TRformat( opt_scc, (i4 *) global,
		(char *) &global->ops_trace.opt_trformat[0],
		(i4) sizeof(global->ops_trace.opt_trformat),
		"\n Column count: %d", qryptr->qeq_q6_col_cnt);

	    if (qryptr->qeq_q6_col_cnt > 0  && qryptr->qeq_q7_col_pp != NULL )
	    {
		/* dump out column names */
		for (n = 0; n < qryptr->qeq_q6_col_cnt; n++)
		{
		    varstring = (char *) qryptr->qeq_q7_col_pp[n];

		    TRformat( opt_scc, (i4 *) global,
			(char *) &global->ops_trace.opt_trformat[0],
			(i4) sizeof(global->ops_trace.opt_trformat),
			"\n Column %d: %s", n, varstring );
		}
		TRformat( opt_scc, (i4 *) global,
		    (char *) &global->ops_trace.opt_trformat[0],
		    (i4) sizeof(global->ops_trace.opt_trformat),
		    "%s","\n\n");

	    }

	return;

}




/*{
** Name: opt_qtdump	- Dump list of query text for an action
**
** Description:
**
** Inputs:
**	global		    Global state variable
**      dda		    Ptr to first distributed action header
**
** Outputs:
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      27-sep-88 (robin)
**          created.
**	11-nov-88 (robin)
**	    Added code for xfr actions.
[@history_template@]...
*/
VOID
opt_qtdump  (
	OPS_STATE   *global,
	QEF_AHD	    *dda)
{
    i4		    acttype;
    QEF_AHD	    *ddaptr;
    QEQ_D1_QRY	    *qryptr;
    char	    *actstr;

    for ( ddaptr = dda; ddaptr != NULL; 
	ddaptr = ddaptr->ahd_next)	    
    {
        acttype = ddaptr->ahd_atype;

        switch (acttype)
        {

	  case QEA_D2_GET:
	    /* this is a get - follows when a query returns tuples */
	    TRformat( opt_scc, (i4 *) global,
		(char *) &global->ops_trace.opt_trformat[0],
		(i4) sizeof(global->ops_trace.opt_trformat),
		"%s","\nGET:");	
	    opt_ldb_dump( global, (QEQ_LDB_DESC *)ddaptr->qhd_obj.qhd_d2_get.qeq_g1_ldb_p, (bool) TRUE );
	    break;

	  case QEA_D3_XFR:
	    /* this is a transfer action */

	    TRformat( opt_scc, (i4 *) global,
		(char *) &global->ops_trace.opt_trformat[0],
		(i4) sizeof(global->ops_trace.opt_trformat),
		"%s","\nXFR:");	

	    TRformat( opt_scc, (i4 *) global,
		(char *) &global->ops_trace.opt_trformat[0],
		(i4) sizeof(global->ops_trace.opt_trformat),
		"%s","\nXFR srce:");	
	    qryptr = &ddaptr->qhd_obj.qhd_d3_xfr.qeq_x1_srce;
	    opt_qrydump( global, qryptr );

	    TRformat( opt_scc, (i4 *) global,
		(char *) &global->ops_trace.opt_trformat[0],
		(i4) sizeof(global->ops_trace.opt_trformat),
		"%s","\nXFR temp:");	
	    qryptr = &ddaptr->qhd_obj.qhd_d3_xfr.qeq_x2_temp;
	    opt_qrydump( global, qryptr );

	    TRformat( opt_scc, (i4 *) global,
		(char *) &global->ops_trace.opt_trformat[0],
		(i4) sizeof(global->ops_trace.opt_trformat),
		"%s","\nXFR sink:");	
	    qryptr = &ddaptr->qhd_obj.qhd_d3_xfr.qeq_x3_sink;
	    opt_qrydump( global, qryptr );

	    break;

	  case QEA_D4_LNK:
	    /* Not yet implemented */

	    break;

          case QEA_D5_DEF:
	    /*  this is a define cursor */
	    TRformat( opt_scc, (i4 *) global,
		(char *) &global->ops_trace.opt_trformat[0],
		(i4) sizeof(global->ops_trace.opt_trformat),
		"%s","\nDEFINE CURSOR:");	
	    opt_qrydump( global, &ddaptr->qhd_obj.qhd_d1_qry);

	    break;

	  default:
	    if ( acttype == QEA_D1_QRY )
	    {
		actstr = "\nQUERY: ";
	    }
	    else if ( acttype == QEA_D8_CRE )
	    {
		actstr = "\nINTERNAL CREATE_AS_SELECT: ";
	    }
	    else if ( acttype == QEA_D6_AGG )
	    {
		actstr = "\nAGGREGATE: ";
	    }
	    else if ( acttype == QEA_D7_OPN )
	    {
		actstr = "\nDEFINE/OPEN CURSOR: ";
	    }
	    else if ( acttype == QEA_D9_UPD )
	    {
		actstr = "\nCURSOR UPDATE: ";
	    }
	    else
	    {
		actstr = "\n**UNKNOWN ACTION TYPE** ";
	    }

	    /*  this is a query */
	    TRformat( opt_scc, (i4 *) global,
		(char *) &global->ops_trace.opt_trformat[0],
		(i4) sizeof(global->ops_trace.opt_trformat),
		"%s", actstr);	
	    opt_qrydump( global, &ddaptr->qhd_obj.qhd_d1_qry);

	}
    }

    return;
}


/*{
** Name: opt_qpdump	- Dump query plan tracing info
**
** Description:
**  
**	Dumps data structures in a distributed query plan
**
** Inputs:
**	global		    Global state variable
**      qplan		    Ptr to a distributed query plan
**
** Outputs:
**	Returns:
**
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**      27-sep-88 (robin)
**          created.
[@history_template@]...
*/
VOID
opt_qpdump  (
	OPS_STATE   *global,
	QEF_QP_CB   *qplan)
{
	if ( qplan == NULL )
	    return;

	TRformat( opt_scc, (i4 *) global,
	    (char *) &global->ops_trace.opt_trformat[0],
	    (i4) sizeof(global->ops_trace.opt_trformat),
	    "\nDISTRIBUTED QUERY PLAN\nQP_NEXT:%6x QP_PREV:%6x",
	    qplan->qp_next, qplan->qp_prev);
	
	TRformat( opt_scc, (i4 *) global,
	    (char *) &global->ops_trace.opt_trformat[0],
	    (i4) sizeof(global->ops_trace.opt_trformat),
	    "\nQP_MODE:%6d QP_RES_ROW_SZ:%6d",
	    qplan->qp_qmode, qplan->qp_res_row_sz);

	TRformat( opt_scc, (i4 *) global,
	    (char *) &global->ops_trace.opt_trformat[0],
	    (i4) sizeof(global->ops_trace.opt_trformat),
	    "\nQP_AHD_CNT:%6d" ,qplan->qp_ahd_cnt);

	TRformat( opt_scc, (i4 *) global,
	    (char *) &global->ops_trace.opt_trformat[0],
	    (i4) sizeof(global->ops_trace.opt_trformat),
	    "\nThe query actions are:");

	opt_qtdump( global, qplan->qp_ahd);

	if (qplan->qp_ddq_cb.qeq_d1_end_p != NULL)
	{
	    TRformat( opt_scc, (i4 *) global,
		(char *) &global->ops_trace.opt_trformat[0],
		(i4) sizeof(global->ops_trace.opt_trformat),
		"\nThe drop list is:");

	    opt_qtdump( global, qplan->qp_ddq_cb.qeq_d1_end_p);
	}
	else
	{
	    TRformat( opt_scc, (i4 *) global,
		(char *) &global->ops_trace.opt_trformat[0],
		(i4) sizeof(global->ops_trace.opt_trformat),
		"\nThere is no drop list");
	}


	TRformat( opt_scc, (i4 *) global,
	    (char *) &global->ops_trace.opt_trformat[0],
	    (i4) sizeof(global->ops_trace.opt_trformat),
	    "\nThe ldbs are:");

	opt_ldb_dump( global, qplan->qp_ddq_cb.qeq_d2_ldb_p, (bool) FALSE);
}




/*{
** Name: opt_chkprint	- Check that text string contains printable ascii.
**
** Description:
**	Check that all characters in the text string provided are ascii
**	for the length provided, or up to the first '\0' encountered,
**	whichever comes first.  Return TRUE if all
**	characters are ascii, FALSE otherwise.
**
** Inputs:
**	buf_to_check			Ptr to char string to check
**	buflen				Maximum length to be checked
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
**      03jul-89 (robin)
**          Created.
[@history_template@]...
*/
bool
opt_chkprint(
	char	*buf_to_check,
	i4	buflen)
{
    char    wbuf[5];
    i4	    bytecount;
    char    *bufptr;

    for ( bytecount = 0;
	  bytecount <= buflen && *buf_to_check != '\0';
	  CMbyteinc( bytecount, buf_to_check))
    {
	bufptr = &wbuf[0];
	CMcpyinc(buf_to_check,bufptr);
	*bufptr = '\0';
	
	if ( !CMprint(&wbuf[0] ))
	{
	    return( FALSE );
	}
    }
    return( TRUE );
}


/*{
** Name: opt_seg_chk	- Check that all text segs are ascii.
**
** Description:
**	Check that all characters in text segments are ascii. If
**	any are not, print the text segment up to the bad spot
**	into the log, and give an error.  This is used as a
**	check to try and isolate memory trashing problems.
**
** Inputs:
**	global				Ptr to the global state variable
**      qseg                            addr of first QEQ_TXT_SEG
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
**      30-jun-89 (robin)
**          Created.
[@history_template@]...
*/
VOID
opt_seg_chk(
	OPS_STATE   *global,
	QEQ_D1_QRY  *qseg)
{
    DD_PACKET	*pktptr;
    QEQ_TXT_SEG	*segptr;
    DD_PACKET	*stop_pkt;
    i4		pkt_counter;

    if ( qseg == NULL )
    {	
	return;
    }


    for ( segptr = qseg->qeq_q4_seg_p; segptr != NULL; segptr = segptr->qeq_s3_nxt_p )
    {
	for ( pktptr = segptr->qeq_s2_pkt_p; pktptr != NULL; pktptr = pktptr->dd_p3_nxt_p )
	{

	    if (pktptr->dd_p4_slot == DD_NIL_SLOT )
	    {
		/*
		**  there is query text in the segment, so check that
		**  every character in it is printable.  If any are
		**  not, ERROR!!
		*/
	    

		if (!opt_chkprint( pktptr->dd_p2_pkt_p, pktptr->dd_p1_len))
		{

		    TRdisplay("\n\t\t**ERROR! ERROR! ERROR! ERROR! ERROR! ERROR!");
		    TRdisplay("\n\t\t**OPC CONSISTENCY CHECK FAILED - NONPRINTING CHARACTER IN TEXT SEGMENT\n");


		    /*
		    **  Print packets up to and including the bad one
		    */

		    

		    stop_pkt = pktptr->dd_p3_nxt_p;

		    for (pktptr = segptr->qeq_s2_pkt_p, pkt_counter = 1; pktptr!= NULL && pktptr != stop_pkt; 
			 pktptr = pktptr->dd_p3_nxt_p)
		    {
			TRdisplay("\n\t\t Packet %d:", pkt_counter);
			TRdisplay("\n\t\t\t%t", pktptr->dd_p1_len, pktptr->dd_p2_pkt_p );
			pkt_counter++;
		    }
		    TRdisplay("\n\t\t*********************************************\n");
		    return;
		}
	    }

	}
    }

    return;

}



/*{
** Name: opt_qtchk	- Check action list query text for printable chars
**
** Description:
**
** Inputs:
**	global		    Global state variable
**      qplan		    Query plan
**
** Outputs:
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      03-jul-89 (robin)
**          created.
[@history_template@]...
*/
VOID
opt_qtchk  (
	OPS_STATE   *global,
	QEF_QP_CB   *qplan)
{
    i4		    acttype;
    QEF_AHD	    *actionptr;
    QEQ_D1_QRY	    *qryptr;

    for ( actionptr = qplan->qp_ahd; actionptr != NULL; 
	actionptr = actionptr->ahd_next)	    
    {
        acttype = actionptr->ahd_atype;

        switch (acttype)
        {

	  case QEA_D2_GET:
	    /* this is a get - follows when a query returns tuples */
	    break;

	  case QEA_D3_XFR:
	    /* this is a transfer action */

	    qryptr = &actionptr->qhd_obj.qhd_d3_xfr.qeq_x1_srce;
	    opt_seg_chk( global, qryptr );

	    qryptr = &actionptr->qhd_obj.qhd_d3_xfr.qeq_x2_temp;
	    opt_seg_chk( global, qryptr );

	    qryptr = &actionptr->qhd_obj.qhd_d3_xfr.qeq_x3_sink;
	    opt_seg_chk( global, qryptr );

	    break;

	  case QEA_D4_LNK:
	    /* Not yet implemented */

	    break;

          case QEA_D5_DEF:
	    /*  this is a define cursor */
	    opt_seg_chk( global, &actionptr->qhd_obj.qhd_d1_qry);

	    break;

	  case QEA_D1_QRY:
	  case QEA_D6_AGG:
	  case QEA_D7_OPN:
	  case QEA_D8_CRE:
	  case QEA_D9_UPD:
	  default:

	    opt_seg_chk( global, &actionptr->qhd_obj.qhd_d1_qry);

	}
    }

    return;
}
