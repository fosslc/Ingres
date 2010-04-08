/* Copyright (c) 1995, 2005 Ingres Corporation
**
**
*/

#include	<compat.h>
#include	<gl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<me.h>
#include	<st.h>
#include	<ddb.h>
#include	<ulf.h>
#include	<ulm.h>
#include        <cs.h>
#include	<cv.h>
#include	<tr.h>
#include	<scf.h>
#include	<adf.h>
#include	<qefmain.h>
#include	<qsf.h>
#include	<qefrcb.h>
#include	<qefcb.h>
#include	<qefscb.h>
#include	<dmf.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<ulh.h>
#include	<qefqeu.h>
#include	<rqf.h>
#include	<rdf.h>
#include	<rdfddb.h>
#include	<ex.h>
#include	<rdfint.h>
#include	<cm.h>
#include	<rdftrace.h>

/*
** Definition of all global variables referenced by this file.
*/

GLOBALREF char                Iird_yes[];           /* string "YES" */
GLOBALREF char                Iird_no[];            /* String "NO" */
GLOBALREF char                Iird_quoted[];        /* String "QUOTED" */
GLOBALREF char                Iird_heap[];          /* Storage "HEAP" */
GLOBALREF char                Iird_isam[];          /* Storage "ISAM" */
GLOBALREF char                Iird_hash[];          /* Storage "HASH" */
GLOBALREF char                Iird_btree[];         /* Storage "BTREE"*/
GLOBALREF char		      Iird_unknown[];	    /* Unknown storage type */
GLOBALREF char		      Iird_lower[];	    /* ldb name case "LOWER" */
GLOBALREF char		      Iird_mixed[];	    /* ldb name case "MIXED" */
GLOBALREF char		      Iird_upper[];	    /* ldb name case "UPPER" */
GLOBALREF char		      *Iird_caps[];	    /* ldb capabilities */
GLOBALREF DD_CAPS	      Iird_dd_caps;	    /* initial value for DD_CAPS */

/*{
** Name: RDFGETOBJ.C - Request distributed information from CDB or LDB.
**		       External call format: rdd_getobjinfo(global)	
**
** Description:
**      This file contains routines for requesting distributed relation,
**	attribute, index, and statistics information from cdb or ldb.
**
**	To request relation information, 
**	   set global->rdf_ddrequests.rdd_types_mask to RDD_RELATION
**	   internal module: rdd_gobjinfo
**	To request attribute information,
**	   set global->rdf_ddrequests.rdd_types_mask to RDD_ATTRIBUTE
**	   internal module: rdd_gattr
**	To request mapped attribute information
**	   set global->rdf_ddrequests.rdd_types_mask to RDD_MAPPEDATTR
**	   internal module: rdd_gmapattr 
**	To request index information
**	   set global->rdf_ddrequests.rdd_types_mask to RDD_INDEX
**	   internal module: rdd_gindex 
**	To request statistics information,
**	   set global->rdf_ddrequests.rdd_types_mask to RDD_STATISTICS
**	   internal module: rdd_gstat
**	To request histogram information,
**	   set global->rdf_ddrequests.rdd_types_mask to RDD_HSTOGRAM
**	   internal module: rdd_ghistogram
**	
**	
** History:
**      10-aug-88 (mings)
**         initial creation
**      26-jun-89 (mings)
**         don't check schema if RDR_USERTAB is set
**      07-sep-89 (carl)
**	    added rdd_commit, rdd_gobjbase and modified rdd_getobjinfo to
**	    implement deadlock avoidance
**	08-oct-89 (carl)
**	    removed from rdd_getobjinfo call to rdd_gobjbase since QEF is
**	    committing every CDB query from RDF to avoid deadlocking CDB 
**	    catalog updaters
**      17-jun-91 (fpang)
**          Initialized qef_rcb->qef_modifier, causes qef to get confused
**          about the tranxasction state if it is not initialized.
**          Fixes B37913.
**	20-dec-91 (teresa)
**	    removed include of qefddb.h since that is now part of qefrcb.h
**	16-jul-92 (teresa)
**	    prototypes
**	13-jan-1993 (fpang)
**	    In rdd_gldbdesc(), set temp table capability if ldb's ingsql level
**	    is 65 or higher. Qef and rqf will use temp table syntax when the flag
**	    is set.
**	13-mar-93 (fpang)
**	    In rdd_gattr(), if mapped, copy each column name to the names array.
**	    Fixes B49737, Schema change error if register w/refresh
**	    and columns are mapped.
**	22-mar-93 (fpang)
**	    In rdd_gattr(), correctly casted arguements to MEcopy(). Also 
**	    specifed copy amount in terms of the actual copy argument, rather 
**	    than it's type. This requires less maintenance should the type of 
**	    the arguments change.
**	23-jun-1993 (shailaja)
**	    Cast key_name to (char *) for assignment type compatibility.
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.  Also added include of cv.h and tr.h.
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**	14-oct-94	(ramra01)
**		Fixed lint errors 533, 828, 857 -- b 65263
**	06-mar-96 (nanpr01)
**	    Standard catalogue interface change for variable page size project.
**	    Also right after establising the connection, find out the buffer
**	    manager capability of the remote/CDB installation.
**      15-jul-96 (ramra01)
**          Standard catalogue interface change for Alter table project.
**	18-dec-96 (chech02)
**	   fixed vms compiler error: changed a local variable 'len' 
**	   of rdd_buffcap() to u_i2.
**	16-mar-1998 (rigka01)
**	   Cross integrate change 434645 for bug 89105
**         Bug 89105 - clear up various access violations which occur
**	   when rdf memory is low.  In rdd_gattr(), add missing 
**	   DB_FAILURE_MACRO(status) after rdu_malloc call.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    RDI_SVCB *svcb is now a GLOBALREF, added macros 
**	    GET_RDF_SESSCB macro to extract *RDF_SESSCB directly
**	    from SCF's SCB, added (ADF_CB*)rds_adf_cb,
**	    deprecating rdu_gcb(), rdu_adfcb(), rdu_gsvcb()
**	    functions.
**      22-Mar-2002 (hanal04) Bug 107330 INGSRV 1729.
**         rdd_buffcap() may cause QEF to initiate an internal MST.
**         If we were not already in an MST we should commit the
**         internal MST. Failure to do so will cause the user's
**         first statement to execute whilst we are already in
**         an MST. This would cause "set lockmode" and other similar
**         operations to fail with E_US172C.
**      07-Jun-2005 (inifa01) b111211 INGSRV2580
**          Verifydb reports error; S_DU1619_NO_VIEW iirelation indicates that
**          there is a view defined for table a (owner ingres), but none exists,
**          when view(s) on a table are dropped.
**          Were making DMT_ALTER call on DMT_C_VIEW, to turn on iirelation.relstat
**          TCB_VBASE bit, indicating view is defined over table(s) and setting
**	    DMT_BASE_VIEW here.  TCB_VBASE and DMT_BASE_VIEW 
**          not tested anywhere in 2.6 and not turned off when view is dropped.
**          qeu_cview() call to dmt_alter() to set TCB_VBASE has been removed ,
**          removing references to TCB_VBASE & DMT_C_VIEW & DMT_BASE_VIEW.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass pointer to facilities DB_ERROR instead of just its err_code
**	    in rdu_ferror().
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
*/

/*{
** Name: rdd_caps   - setup ldb capabilities.
**
** Description:
**      This routine setup ldb capabilities in DD_CAPS structure.
**
** Inputs:
**	global				    ptr to RDF global state variable
**	capability			    ldb capability
**	cap_value			    value of service
**
** Outputs:
**      cap_p				    ptr to DD_CAPS 
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	14-jun-91 (teresa)
**	    fix bug 34864 by checking to see if LDB supports tids.
**	18-jun-91 (teresa)
**	    fixed several bugs where if () statements had "=" instead of "=="
**	16-jul-92 (teresa)
**	    prototypes
**	01-mar-93 (barbara)
**	    support for delimited identifiers in Star: new capabilities
**	    are OPEN SQL and DELIMITED NAME CASE.
**	02-sep-93 (barbara)
**	    1. Process another new capability: DB_REAL_USER_CASE.
**	    2. The presence of the DB_DELIMITED_CASE capability implies
**	       that the LDB supports delimited ids.  Record this in
**	       the dd_c1_ldb_caps field as bit DD_8CAP_DELIMITED_IDS. 
*/
DB_STATUS
rdd_caps(   RDF_GLOBAL	    *global,
	    DD_CAPS	    *cap_p,
	    RDC_DBCAPS	    *caps)
{
    DB_STATUS	    status = E_DB_OK;
    i4		    cnt;
    i4 		    temp_value;
    
    for (cnt = 0; cnt < RDD_MAXCAPS; cnt++)
    {
	/* determine what kind of capability */

	if (STcompare((PTR)caps->cap_capability, (PTR)Iird_caps[cnt]) == 0)
	{
	    switch(cnt)
	    {
	    case RDD_DISTRIBUTED:

                    /* whether dbms service is distributed */	
		    if (caps->cap_value[0] == 'Y')
			cap_p->dd_c1_ldb_caps |= DD_1CAP_DISTRIBUTED;
		    break;

	    case RDD_DB_NAME_CASE:

		    /* what type of case sensitivity */
		    if (STcompare((PTR)caps->cap_value, (PTR)Iird_lower) == 0)
			cap_p->dd_c6_name_case = (i4)DD_0CASE_LOWER;
		    else if (STcompare((PTR)caps->cap_value, (PTR)Iird_mixed) == 0)
			cap_p->dd_c6_name_case = (i4)DD_1CASE_MIXED;
		    else if (STcompare((PTR)caps->cap_value, (PTR)Iird_upper) == 0)
			cap_p->dd_c6_name_case = (i4)DD_2CASE_UPPER;
		    else
		    {
			status = E_DB_ERROR;
			rdu_ierror(global, status, E_RD0275_UNKNOWN_LDBCAPABILITY);
		    }
		    break;

	    case RDD_UNIQUE_KEY_REQ:

		    /* whether unique key required */

		    if (caps->cap_value[0] == 'Y')
			cap_p->dd_c1_ldb_caps |= DD_6CAP_UNIQUE_KEY_REQ;
		    break;

	    case RDD_CAP_INGRES:

		    /* whether 100% ingres supports */

		    if (caps->cap_value[0] == 'Y')
		    {
			/* if 100% ingres supports, setup savepoints and tids */

			cap_p->dd_c1_ldb_caps |= DD_2CAP_INGRES;
			cap_p->dd_c1_ldb_caps |= DD_3CAP_SAVEPOINTS;
			cap_p->dd_c1_ldb_caps |= DD_5CAP_TIDS;
		    }
		    break;

	    case RDD_QUEL_LEVEL:
                   
		    /* this capability contains supported quel level */

		    caps->cap_value[5] = EOS;
		    CVal((char *)caps->cap_value, (i4 *)&cap_p->dd_c5_ingquel_lvl);
		    break;

	    case RDD_SQL_LEVEL:
 
		    /* this capability contains supported sql level */

		    caps->cap_value[5] = EOS;
		    CVal((char *)caps->cap_value, (i4 *)&cap_p->dd_c4_ingsql_lvl);
		    break;

	    case RDD_COMMON_SQL_LEVEL:

		    /* this capability contains supported common sql level */

		    caps->cap_value[5] = EOS;
		    CVal((char *)caps->cap_value, (i4 *)&cap_p->dd_c3_comsql_lvl);
		    break;

	    case RDD_SAVEPOINTS:

		    /* whether savepoints is supported */

		    if (caps->cap_value[0] == 'Y')
			cap_p->dd_c1_ldb_caps |= DD_3CAP_SAVEPOINTS;
		    break;

	    case RDD_DBMS_TYPE:

		    /* this capability contains dbms type */

		    MEcopy((PTR)caps->cap_value, sizeof(DD_NAME),
			    (PTR)cap_p->dd_c7_dbms_type);
		    break;

	    case RDD_OWNER_NAME:
		    
		    /* this capability shows whether owner can be prefixed or quoted */

	            if (STcompare((PTR)caps->cap_value, (PTR)Iird_no) == 0)
			cap_p->dd_c8_owner_name = DD_0OWN_NO;
		    else if (STcompare((PTR)caps->cap_value, (PTR)Iird_yes) == 0)
			    cap_p->dd_c8_owner_name = DD_1OWN_YES;
	            else if (STcompare((PTR)caps->cap_value, (PTR)Iird_quoted) == 0)
			    cap_p->dd_c8_owner_name = DD_2OWN_QUOTED;
		    else
			rdu_ierror(global, E_DB_ERROR, E_RD0275_UNKNOWN_LDBCAPABILITY);
		    break;
		    
	    case RDD_PHYSICAL_SOURCE:
		
		    /* this capability shows where the physical table description is:
		    ** iitables or iiphysical_tables */


		    if (caps->cap_value[0] == 'P')
			cap_p->dd_c1_ldb_caps |= DD_7CAP_USE_PHYSICAL_SOURCE;
		    break;

	    case RDD_TID_LEVEL:
	    
		    /* this capability indicates if TIDS are supported.
		    ** It is a level code, but for our purposes, if all 5
		    ** characters are zero, then no tid support.  Else tid
		    ** support -- fix for bug 34684. */

		    caps->cap_value[5] = EOS;
		    CVal((char *)caps->cap_value, (i4 *)&temp_value);
		    if (temp_value)
			cap_p->dd_c1_ldb_caps |= DD_5CAP_TIDS;
		    break;

	    case RDD_OPENSQL_LEVEL:

		    /* this capability contains supported open sql level */

		    caps->cap_value[5] = EOS;
		    CVal((char *)caps->cap_value, 
				(i4 *)&cap_p->dd_c9_opensql_level);
		    break;

	    case RDD_DELIMITED_NAME_CASE:

		    /* what type of case sensitivity */
		    if (STcompare((PTR)caps->cap_value, (PTR)Iird_lower) == 0)
			cap_p->dd_c10_delim_name_case = (i4)DD_0CASE_LOWER;
		    else if (STcompare((PTR)caps->cap_value, (PTR)Iird_mixed) == 0)
			cap_p->dd_c10_delim_name_case = (i4)DD_1CASE_MIXED;
		    else if (STcompare((PTR)caps->cap_value, (PTR)Iird_upper) == 0)
			cap_p->dd_c10_delim_name_case = (i4)DD_2CASE_UPPER;
		    else
		    {
			status = E_DB_ERROR;
			rdu_ierror(global, status, E_RD0275_UNKNOWN_LDBCAPABILITY);
		    }
		    cap_p->dd_c1_ldb_caps |= DD_8CAP_DELIMITED_IDS;
		    break;

	    case RDD_REAL_USER_CASE:

		    /* what type of case sensitivity */
		    if (STcompare((PTR)caps->cap_value, (PTR)Iird_lower) == 0)
			cap_p->dd_c11_real_user_case = (i4)DD_0CASE_LOWER;
		    else if (STcompare((PTR)caps->cap_value, (PTR)Iird_mixed) == 0)
			cap_p->dd_c11_real_user_case = (i4)DD_1CASE_MIXED;
		    else if (STcompare((PTR)caps->cap_value, (PTR)Iird_upper) == 0)
			cap_p->dd_c11_real_user_case = (i4)DD_2CASE_UPPER;
		    else
		    {
			status = E_DB_ERROR;
			rdu_ierror(global, status, E_RD0275_UNKNOWN_LDBCAPABILITY);
		    }
		    break;

	    default:
		    rdu_ierror(global, E_DB_ERROR, E_RD0275_UNKNOWN_LDBCAPABILITY);
		    break;
	    }

	    /* exit here */

	    return(status);
	}
    }

    /* no matching capability */

    rdu_ierror(global, E_DB_ERROR, E_RD0275_UNKNOWN_LDBCAPABILITY);
    return(status);
}

/*{
** Name: rdd_buffcap - retrieve buffer manager capabilities.
**
** Description:
**      This routine queries the buffer manager capabilities of an 
**	installation.
**
** Inputs:
**      global				    ptr to RDF state descriptor
**	ldb_p				    ldb pointer
**
** Outputs:
**	ldb_p
**	  dd_i2_ldb_plus
**        	dd_p7_maxtup 		- max tuple size of installation.
**      	dd_p8_maxpage 		- max page size of installation.
**      	dd_p9_pagesize[6]	- diff. page sizes available  
**      	dd_p10_tupsize[6] 	- diff. tup sizes corresponding to page
**					  size.
**		
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-mar-96 (nanpr01)
**         initial creation
**	    Right after establising the connection, find out the buffer
**	    manager capability of the remote/CDB installation.
**	18-dec-96 (chech02)
**	   fixed vms compiler error: changed local variable 'len' to u_i2.
**      22-Mar-2002 (hanal04) Bug 107330 INGSRV 1729.
**         Flag rdd_query() calls with QEF_RDF_INTERNAL to indicate that
**         this is an internal query.
**         If QEF initiates an MST on our behalf QEF_RDF_MSTRAN will be
**         set on the return from rdd_query(). This will be used
**         by QEF to commit the internal MST via rdd_fetch() or
**         rdd_flush().
**         Ensure all related QEF_RDF... flags are unset before exiting
**         rdd_buffcap().
*/
DB_STATUS
rdd_buffcap(	RDF_GLOBAL         *global,
		DD_1LDB_INFO	   *ldb_p)
{
    DB_STATUS		status;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    DD_0LDB_PLUS        *ldbplus_p = &ldb_p->dd_i2_ldb_plus;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    RQB_BIND            rq_bind[RDD_01_COL],	/* for 1 column */
			*bind_p = rq_bind;
    DD_PACKET		ddpkt;
    char		qrytxt[RDD_QRY_LENGTH];
    char		temp[34];
    char		temp1[34];
    i4 		pagesize, i, j;
    u_i2		len;

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)ldb_p;
    for (i = 0, pagesize = 2; i < DB_MAX_CACHE; i++, pagesize *= 2)
    {
      /* set up query 
      **
      **	select dbmsinfo(page_size_%dK);
      */
      ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
    	"%s%d%s", "select dbmsinfo('PAGE_SIZE_", pagesize,"k');");

      ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = 
			(i4) STlength((char *)qrytxt);
      ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

      global->rdf_qefrcb.qef_stmt_type |= QEF_RDF_INTERNAL;
      
      status = rdd_query(global);	    /* send query to qef */
      if (DB_FAILURE_MACRO(status))
      {
	global->rdf_qefrcb.qef_stmt_type &= (~(ALL_QEF_RDF));
        return (status); 
      }

      /* Is a particular page size available in that installation ? */
      rq_bind[0].rqb_addr = (PTR)temp;
      rq_bind[0].rqb_length = 34;
      rq_bind[0].rqb_dt_id = DB_VCH_TYPE;

      qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
      qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_01_COL;

      status = rdd_fetch(global);
      if (DB_FAILURE_MACRO(status))
      {
	global->rdf_qefrcb.qef_stmt_type &= (~(ALL_QEF_RDF));
        return (status); 
      }
      if (ddr_p->qer_d5_out_info.qed_o3_output_b)
      {
        /* flush */
        status = rdd_flush(global);
        if (DB_FAILURE_MACRO(status))
        {
	    global->rdf_qefrcb.qef_stmt_type &= (~(ALL_QEF_RDF));
            return (status); 
        }
      }

      global->rdf_qefrcb.qef_stmt_type &= (~(ALL_QEF_RDF));

      if (temp[2] == 'Y')
      {
        ldbplus_p->dd_p9_pagesize[i] = pagesize * 1024; 
        ldbplus_p->dd_p8_maxpage = pagesize * 1024; 
      }
      else
        if (temp[2] == 'N')
          ldbplus_p->dd_p9_pagesize[i] = 0; 
        else
        {
          /*
          ** Probably version previous to OpenIngres with Variable Page
          ** size.
          */
          ldbplus_p->dd_p9_pagesize[0] = DB_MIN_PAGESIZE; 
          ldbplus_p->dd_p8_maxpage = DB_MIN_PAGESIZE; 
          ldbplus_p->dd_p10_tupsize[0] = DB_MAXTUP;
          ldbplus_p->dd_p7_maxtup = DB_MAXTUP; 
          /* Initialize rest */
          for (j = 1; j < DB_MAX_CACHE; j++)
          {
            ldbplus_p->dd_p9_pagesize[j] = 0; 
            ldbplus_p->dd_p10_tupsize[j] = 0;
          }
          return(E_DB_OK);
        }
    }
    for (i = 0, pagesize = 2; i < DB_MAX_CACHE; i++, pagesize *= 2)
    {
      if (ldbplus_p->dd_p9_pagesize[i] == 0)
      {
        ldbplus_p->dd_p10_tupsize[i] = 0;
        continue;
      }
      /* set up query 
      **
      **	select dbmsinfo(tup_len_%dK);
      */
      ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
    	    "%s%d%s", "select dbmsinfo('TUP_LEN_", pagesize,"k');");

      ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = 
			(i4) STlength((char *)qrytxt);
      ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

      global->rdf_qefrcb.qef_stmt_type |= QEF_RDF_INTERNAL;
      
      status = rdd_query(global);	    /* send query to qef */
      if (DB_FAILURE_MACRO(status))
      {
	global->rdf_qefrcb.qef_stmt_type &= (~(ALL_QEF_RDF));
        return (status); 
      }

      /* tuple length */
      rq_bind[0].rqb_addr = (PTR) temp; 
      rq_bind[0].rqb_length = 34;
      rq_bind[0].rqb_dt_id = DB_VCH_TYPE;

      qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
      qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_01_COL;

      status = rdd_fetch(global);
      if (DB_FAILURE_MACRO(status))
      {
	global->rdf_qefrcb.qef_stmt_type &= (~(ALL_QEF_RDF));
        return (status); 
      }
      if (ddr_p->qer_d5_out_info.qed_o3_output_b)
      {
        /* flush */
        status = rdd_flush(global);
        if (DB_FAILURE_MACRO(status))
        {
 	    global->rdf_qefrcb.qef_stmt_type &= (~(ALL_QEF_RDF));
            return (status); 
        }
      }
      global->rdf_qefrcb.qef_stmt_type &= (~(ALL_QEF_RDF));

      MEcopy(&temp[0], 2, &len); 
      MEcopy(&temp[2], len, &temp1[0]); 
      temp1[len] = EOS;
      status = CVal(temp1, &ldbplus_p->dd_p10_tupsize[i]); 
      if (DB_FAILURE_MACRO(status))
        return(status);
      ldbplus_p->dd_p7_maxtup = ldbplus_p->dd_p10_tupsize[i];
    }
    return(status);
}

/*{
** Name: rdd_gldbdesc	- retrieve LDB description from CDB.
**
** Description:
**      This routine generates query to retrieve LDB description from CDB.
**
** Inputs:
**      global				    ptr to RDF state descriptor
**
** Outputs:
**      global				    ptr to RDF state descriptor
**	    .rdf_tobj			    temporary object descriptor
**		.dd_o9_tab_info		    ldb table info
**		    .dd_t9_ldb_p	    pointer to ldb desc
**			.dd_l1_ingres_b	    FALSE
**			.dd_l2_node_name    node name
**			.dd_l3_ldb_name	    ldb name
**			.dd_l4_dbms_name    dbms name
**			.dd_l5_ldb_id	    ldb id
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**      06-jun-89 (mings)
**         let PSF take care the error if E_QE0506_CANNOT_GET_ASSOCIATION 
**	20-jun-90 (teresa)
**	   added new parameter to rdu_ferror() for bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
**	13-jan-1993 (fpang)
**	    If ldb's ingsql level is 65 or higher set temp table capability
**	    so qef and rqf will know to use temp table syntax.
**	14-oct-1994	(ramra01)
**		fixed lint error b 65263
**	06-mar-96 (nanpr01)
**	    Right after establising the connection, find out the buffer
**	    manager capability of the remote/CDB installation.
**	20-jun-1997 (nanpr01)
**	    Bug 81889 : Commit all the open sql statements.
**	28-Jan-2009 (kibro01) b121521
**	    Add flag for ingresdate support to pass down into tmp table gen.
*/
DB_STATUS
rdd_gldbdesc(	RDF_GLOBAL         *global,
		DD_1LDB_INFO	   *ldb_p)
{
    DB_STATUS		status, x_status;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    DD_LDB_DESC         *ldbdesc_p = &ldb_p->dd_i1_ldb_desc;
    DD_0LDB_PLUS        *ldbplus_p = &ldb_p->dd_i2_ldb_plus;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    bool		desc_found = FALSE,	
			longname = FALSE;
    RQB_BIND            rq_bind[RDD_08_COL],	/* for 8 columns */
			*bind_p = rq_bind;
    RDC_LDBID		ldbid;
    RDC_DBCAPS		ldbcaps;
    DD_PACKET		ddpkt;
    char		qrytxt[RDD_QRY_LENGTH];

    qefrcb->qef_modifier = 0;

    /* initialize ldb capability */
    STRUCT_ASSIGN_MACRO(Iird_dd_caps, ldbplus_p->dd_p3_ldb_caps);
    
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select a.ldb_node, a.ldb_dbms, a.ldb_database, a.ldb_longname, 
    **	       b.cap_capability, b.cap_value from
    **	       iidd_ddb_ldbids a, iidd_ddb_ldb_dbcaps b 
    **         where a.ldb_id = ldb_id and b.ldb_id = ldb_id;
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	    "%s %d and b.ldb_id = %d;", 	    
	    "select a.ldb_node, a.ldb_dbms, a.ldb_database, a.ldb_longname, \
a.ldb_dba, a.ldb_dbaname, b.cap_capability, b.cap_value from \
iidd_ddb_ldbids a, iidd_ddb_ldb_dbcaps b where a.ldb_id =",
	    ldbdesc_p->dd_l5_ldb_id,
	    ldbdesc_p->dd_l5_ldb_id);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = (i4) STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 


    /* node name */
    rq_bind[0].rqb_addr = (PTR)ldbid.ldb_node;
    rq_bind[0].rqb_length = sizeof(DD_NAME);
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
    /* dbms */
    rq_bind[1].rqb_addr = (PTR)ldbid.ldb_dbms;
    rq_bind[1].rqb_length = sizeof(DD_NAME);
    rq_bind[1].rqb_dt_id = DB_CHA_TYPE;
    /* ldb name */
    rq_bind[2].rqb_addr = (PTR)ldbid.ldb_database;
    rq_bind[2].rqb_length = sizeof(DD_NAME);
    rq_bind[2].rqb_dt_id = DB_CHA_TYPE;
    /* long name */
    rq_bind[3].rqb_addr = (PTR)ldbid.ldb_longname;
    rq_bind[3].rqb_length = sizeof(RDD_8CHAR);
    rq_bind[3].rqb_dt_id = DB_CHA_TYPE;
    /* ldb_dba */
    rq_bind[4].rqb_addr = (PTR)ldbid.ldb_dba;
    rq_bind[4].rqb_length = sizeof(RDD_8CHAR);
    rq_bind[4].rqb_dt_id = DB_CHA_TYPE;
    /* ldb_dbaname */
    rq_bind[5].rqb_addr = (PTR)ldbid.ldb_dbaname;
    rq_bind[5].rqb_length = sizeof(DD_NAME);
    rq_bind[5].rqb_dt_id = DB_CHA_TYPE;

    /* cap_capability */
    ldbcaps.cap_capability[DB_MAXNAME] = EOS;
    rq_bind[6].rqb_addr = (PTR)ldbcaps.cap_capability;
    rq_bind[6].rqb_length = sizeof(DD_NAME);
    rq_bind[6].rqb_dt_id = DB_CHA_TYPE;
    /* cap_value */
    ldbcaps.cap_value[DB_MAXNAME] = EOS;
    rq_bind[7].rqb_addr = (PTR)ldbcaps.cap_value;
    rq_bind[7].rqb_length = sizeof(DD_NAME);
    rq_bind[7].rqb_dt_id = DB_CHA_TYPE;
        


    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_08_COL;

    ldbplus_p->dd_p3_ldb_caps.dd_c8_owner_name = DD_2OWN_QUOTED;

    for (;;)
    {
	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (!ddr_p->qer_d5_out_info.qed_o3_output_b)
	    break;

	/* because it's a joined query, only need to pick up 
	** the first ldb descriptor (desc_found == FALSE) and
	** discard the rest */

	if (!desc_found)
	{
	    desc_found = TRUE;
	    ldbdesc_p->dd_l1_ingres_b = FALSE;
	 
	    /* if ldb name is longname, set longname to TRUE and
	    ** retrieve the longname at following query */

	    if (ldbid.ldb_longname[0] == 'Y')
		longname = TRUE;
	    else
	    {
		MEfill(sizeof(DD_256C), (u_char)' ',
			    (PTR)ldbdesc_p->dd_l3_ldb_name);
		MEcopy((PTR)ldbid.ldb_database, sizeof(DD_NAME),
			    (PTR)ldbdesc_p->dd_l3_ldb_name);
		MEcopy((PTR)ldbid.ldb_database, sizeof(DD_NAME),
			    (PTR)ldbplus_p->dd_p4_ldb_alias);
	    }
	    MEcopy((PTR)ldbid.ldb_node, sizeof(DD_NAME),
			(PTR)ldbdesc_p->dd_l2_node_name);
	    MEcopy((PTR)ldbid.ldb_dbms, sizeof(DD_NAME),
			(PTR)ldbdesc_p->dd_l4_dbms_name);
		/* b65263 - This is a lint error */
	    /* if (ldbid.ldb_dba[0] = 'Y') */
	    if (ldbid.ldb_dba[0] == 'Y') 
	    {
		ldbplus_p->dd_p1_character = DD_1CHR_DBA_NAME;			
		MEcopy((PTR)ldbid.ldb_dbaname, sizeof(DD_NAME),
			(PTR)ldbplus_p->dd_p2_dba_name);
	    }
	    else
	    {
		ldbplus_p->dd_p1_character = 0;
		MEfill(sizeof(DD_NAME), (u_char)' ',
			     (PTR)ldbplus_p->dd_p2_dba_name);
	    }
	}
	
	/* trim trailing blanks and null terminated ldb capability and cap value */

	(VOID)STtrmwhite(ldbcaps.cap_capability);
	(VOID)STtrmwhite(ldbcaps.cap_value);

	/* setup ldb capability in DD_CAPS structure */

	status = rdd_caps(global, &ldbplus_p->dd_p3_ldb_caps, &ldbcaps);

	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    if (!desc_found)
    {  /* no ldb descriptor was found in iidd_ddb_ldbids */
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0274_LDBDESC_NOTFOUND); 
        return(status);
    }

    

    /* If owner name can be prefixed and quoted and SQL level
    ** is before release 602, then set dd_c2_ldb_caps t0
    ** DD_0CAP_PRE_602_ING_SQL */

    if ((ldbplus_p->dd_p3_ldb_caps.dd_c8_owner_name & DD_2OWN_QUOTED)
	&&
	(
	    (ldbplus_p->dd_p3_ldb_caps.dd_c4_ingsql_lvl == 600)
	    ||
	    (ldbplus_p->dd_p3_ldb_caps.dd_c4_ingsql_lvl == 601)
	))
	ldbplus_p->dd_p3_ldb_caps.dd_c2_ldb_caps |= DD_0CAP_PRE_602_ING_SQL;

    /* if ldb has a longname, generate query to retrieve longname and alias 
    ** form iidd_ddb_long_ldbnames */
 
    if (longname)
    {
	ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

	/* set up query 
	**
	**	select long_ldbname, long_ldbalias from iidd_ddb_long_ldbnames where
	**           long_ldbid = ldb_id;
	*/

	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
		"%s %d;", 	    
		"select long_ldbname, long_ldbalias from iidd_ddb_long_ldbnames where long_ldbid =",
		ldbdesc_p->dd_l5_ldb_id);

	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = (i4) STlength((char *)qrytxt);
	ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
	ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

	status = rdd_query(global);	    /* send query to qef */
	if (DB_FAILURE_MACRO(status))
	    return (status); 

	/* long_ldbname */
	rq_bind[0].rqb_addr = (PTR)ldbdesc_p->dd_l3_ldb_name;
	rq_bind[0].rqb_length = sizeof(DD_256C);
	rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
	/* long_ldbalias */
	rq_bind[1].rqb_addr = (PTR)ldbplus_p->dd_p4_ldb_alias;
	rq_bind[1].rqb_length = sizeof(DD_NAME);
	rq_bind[1].rqb_dt_id = DB_CHA_TYPE;
        
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_02_COL;

	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

        if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	{ 
	    /* flush */
	    status = rdd_flush(global);
	}
	else
        {  /* ldb longname was not found in iidd_ddb_long_ldbname */
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0273_LDBLONGNAME_NOTFOUND); 
	}
    }

    /* call qef for architecture infomation */

    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)ldb_p;

    status = qef_call(QED_7RDF_DIFF_ARCH, ( PTR ) qefrcb);

    if (DB_FAILURE_MACRO(status))
	if (qefrcb->error.err_code == E_QE0506_CANNOT_GET_ASSOCIATION)
	    SETDBERR(&global->rdfcb->rdf_error, 0, E_RD0277_CANNOT_GET_ASSOCIATION);
	else 
	    rdu_ferror(global, status, &qefrcb->error,
		       E_RD027A_GET_ARCHINFO_ERROR,0); 
    else if (ddr_p->qer_d15_diff_arch_b)
	    ldbplus_p->dd_p3_ldb_caps.dd_c2_ldb_caps |= DD_201CAP_DIFF_ARCH;

    /* If LDB is 65 or greater, set temp table capability */
    if (ldbplus_p->dd_p3_ldb_caps.dd_c4_ingsql_lvl >= DD_605CAP_LEVEL)
	ldbdesc_p->dd_l6_flags |= DD_04FLAG_TMPTBL;

    /* If LDB is 910 or greater, set ingresdate capability */
    if (ldbplus_p->dd_p3_ldb_caps.dd_c4_ingsql_lvl >= DD_910CAP_LEVEL)
	ldbdesc_p->dd_l6_flags |= DD_05FLAG_INGRESDATE;

    /* Now a connection to LDB has been set up. Now we need to inquire 
    ** about buffer manager capability. We have to check to couple of
    ** capabilities before we do that :
    ** if ldbplus_p->dd_p3_ldb_caps.dd_c4_ingsql_lvl > DD_605CAP_LEVEL 
    */
    if ((ldbplus_p->dd_p3_ldb_caps.dd_c4_ingsql_lvl > DD_605CAP_LEVEL) &&
	(MEcmp(ldbplus_p->dd_p3_ldb_caps.dd_c7_dbms_type, "INGRES", 6) == 0))
    {
	status = rdd_buffcap(global, ldb_p);
	x_status = rdd_commit(global);
	if ((status == E_DB_OK) && (x_status != E_DB_OK))
	    status = x_status;
    }
    else {
      i4 j;

      ldbplus_p->dd_p9_pagesize[0] = DB_MIN_PAGESIZE; 
      ldbplus_p->dd_p8_maxpage = DB_MIN_PAGESIZE; 
      ldbplus_p->dd_p10_tupsize[0] = DB_MAXTUP;

      for (j = 1; j < DB_MAX_CACHE; j++)
      {
        ldbplus_p->dd_p9_pagesize[j] = 0; 
        ldbplus_p->dd_p10_tupsize[j] = 0;
      }
      ldbplus_p->dd_p7_maxtup = DB_MAXTUP; 
    }
    return(status);
}

/*{
** Name: rdd_ldbsearch	- search ldb description by ldb id.
**
** Description:
**      This routine search ldb list for ldb description. If no
**  ldb descriptor was found in the list, a ldb description will
**  be allocated in the global area. The ldb information is retrieved
**  from cdb by calling rdd_ldbdesc.  
**
** Inputs:
**      global                          ptr to RDF state descriptor
**	ldb_id				ldb id
**	ddb_id			        DDB unique (infodb) id
**
** Outputs:
**	
**	desc_p				address of pointer to ldb descriptor
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	25-jun-89 (mings)
**	   clear ldb_id when error
**	20-may-92 (teresa)
**	    Rewrote to use ULH to manage the cache.  Also, 
**	    change rdd_setsem and rdd_resetsem interface to stop passing SCF_CB
**	    and SCF_SEMAPHORE parameters.
**	16-jul-92 (teresa)
**	    prototypes
**	17-feb-93 (teresa)
**	    add sem_type (RDF_BLD_LDBDESC) to rdd_setsem/rdd_resetsem interface.
**	21-may-93 (teresa)
**	    changed key_name from char to unsigned char to make prototype of
**	    ulh_create() happier.
**	14-oct-94	(ramra01)
**		Fixed an lint error b65263
**	10-jan-1996 (toumi01; from 1.1 axp_osf port) (kinpa04)
**	    Corrected typo for calculation of key_len which prevented the
**	    functioning of Star for axp_osf.
**	26-jul-1997 (nanpr01)
**	    Roger's MEfill code yet revealed another problem for star.
**	    rdf_ulhobject->rdd_ldbdesc->dd_i1_ldb_desc.dd_l6_flags is not 
**	    initialized and consequently, rqf was trying to openup another
**	    connection and causing lock wait on multiple session.
**	6-Jul-2006 (kschendel)
**	    unique dbid's aren't pointers, fix here.
*/
DB_STATUS
rdd_ldbsearch(	RDF_GLOBAL         *global,
		DD_1LDB_INFO	   **desc_p,
		i4		   ddb_id,
		i4		   ldb_id)
{
    DB_STATUS	    status,t_status;
    unsigned char   key_name[ULH_MAXNAME];
    PTR		    key_ptr= (char *)&key_name[0];
    i4		    key_len;

    /* 
    ** set up key into cache for this object:  (ddb_id,ldb_id) 
    */
    MEcopy ( (PTR) &ddb_id, sizeof(ddb_id), key_ptr);
    key_ptr += sizeof(ddb_id);
    MEcopy ( (PTR) &ldb_id, sizeof(ldb_id), key_ptr);
    key_len = sizeof(ddb_id) + sizeof(ldb_id);

    /* 
    ** initialize star ulhcb 
    */
    if (!(global->rdf_init & RDF_D_IULH))
    {	/* init the dist ULH control block if necessary, only need the server
        ** level hashid obtained at server startup 
	*/
	global->rdf_dist_ulhcb.ulh_hashid = Rdi_svcb->rdv_dist_hashid;
	global->rdf_init |= RDF_D_IULH;
    }

    /* 
    ** now get item from cache, if it exists 
    */
    status = ulh_create(&global->rdf_dist_ulhcb, &key_name[0],  key_len, 
			ULH_DESTROYABLE, 0);
    if (DB_SUCCESS_MACRO (status) )
    {
	RDF_DULHOBJECT	    *rdf_ulhobject;

	global->rdf_resources |= RDF_DULH;	/* mark ulh resource as fixed */

	/*
	** now see if this is a newly created distributed cache object
	*/
	rdf_ulhobject = (RDF_DULHOBJECT *) global->rdf_dist_ulhcb.
						    ulh_object->ulh_uptr;

	/*  We can be here for two cases:
	**	    - we have successfully created an empty 
	**	      ULH object, or
	**	    - we have successfully gained access to an object.
	*/
	if (!rdf_ulhobject)		
	{
	    do		/* control loop */
	    {   
		/* object newly created or it has no info block */
		/* get a semaphore on the ulh object in order to update
		** the object status */
		status = rdd_setsem(global,RDF_BLD_LDBDESC);
		if (DB_FAILURE_MACRO(status))
		    return(status);
		/* re-fetch the ULH object under semaphore protection in order 
		** to check if another thread is trying to create this object
		*/
		rdf_ulhobject = (RDF_DULHOBJECT *) global->rdf_dist_ulhcb.
				ulh_object->ulh_uptr;

		global->rdf_d_ulhobject = rdf_ulhobject; /* save ptr
				    ** to the resource which is to be updated */
		if (!rdf_ulhobject)
		{   
		    /* another thread is not attempting to create this object
		    ** so allocate some memory for the object descriptor.
		    ** Also, set RDF_D_RUPDATE to indicate that the ULH object
		    ** has been successfully obtained and will be updated by
		    ** this thread.  Finally, set RDF_BLD_LDBDESC to let
		    ** rdu_malloc know it is allocating memory for the LDBcache.
		    ** (remember to unset RDF_BLD_LDBDESC after all allocation
		    ** successfully completes, or futher processing will try
		    ** to do rdu_malloc calls to build relation cache objects
		    ** and will fail.
		    */
		    global->rdf_resources |= (RDF_D_RUPDATE | RDF_BLD_LDBDESC); 
		    status = rdu_malloc(global, (i4)sizeof(*rdf_ulhobject), 
			(PTR *)&rdf_ulhobject);
		    if (DB_FAILURE_MACRO(status))
			break;
		    /* save ptr to ULH object for unfix commands */
		    rdf_ulhobject->rdf_ulhptr = 
					    global->rdf_dist_ulhcb.ulh_object; 
		    /* indicate that this object is being updated */
		    rdf_ulhobject->rdf_state = RDF_SUPDATE; 
		    global->rdf_d_ulhobject = rdf_ulhobject; /* save ptr
				    ** to the resource which is to be updated */
		    rdf_ulhobject->rdf_ddbid = ddb_id;
		    rdf_ulhobject->rdf_ldbid = ldb_id;
		    rdf_ulhobject->rdf_pstream_id = 
				global->rdf_dist_ulhcb.ulh_object->ulh_streamid;
		    status = rdu_malloc(global, 
			(i4)sizeof(*rdf_ulhobject->rdd_ldbdesc), 
			(PTR *)&rdf_ulhobject->rdd_ldbdesc);
		    if (DB_FAILURE_MACRO(status))
			break;

		    /*
		    ** We've allocated everything we need for the LDBdesc
		    ** cache.  So, clear RDF_BLD_LDBDESC so future allocation 
		    ** calls (which will try to build relation or qtree cache
		    ** objects) do not use LDBdesc cache memory by accident.
		    */
		    global->rdf_resources &= (~RDF_BLD_LDBDESC); 

		    /*
		    ** Now populate the LDB descriptor
		    */
		    rdf_ulhobject->rdd_ldbdesc->dd_i1_ldb_desc.dd_l5_ldb_id =
									 ldb_id;
		    rdf_ulhobject->rdd_ldbdesc->dd_i1_ldb_desc.dd_l6_flags = 0;
		    status = rdd_gldbdesc(global, rdf_ulhobject->rdd_ldbdesc);
		    if (DB_SUCCESS_MACRO(status))
			*desc_p = rdf_ulhobject->rdd_ldbdesc;
		    else
		    {
			/* if having problem in ldb descriptor, clear the 
			** ldb_id so that it won't get reused. */
			rdf_ulhobject->rdd_ldbdesc->
					dd_i1_ldb_desc.dd_l5_ldb_id = 0;
		 	rdf_ulhobject->rdf_state = RDF_SBAD;
			global->rdf_resources |= RDF_BAD_LDBDESC;
		    }

		    /* save this object after marking its' status as being 
		    ** updated */
		    global->rdf_dist_ulhcb.ulh_object->ulh_uptr = 
							    (PTR)rdf_ulhobject;
		}
		else
		{
		    /* some other thread created it while we were waiting,
		    ** so use it unless its bad.
		    */
		    if (rdf_ulhobject->rdf_state == RDF_SBAD)
		    {
			/* attempt to populate it again, under semaphore */
			rdf_ulhobject->rdf_state = RDF_SUPDATE;
			rdf_ulhobject->rdd_ldbdesc->dd_i1_ldb_desc.
							  dd_l5_ldb_id = ldb_id;
			status =rdd_gldbdesc(global,rdf_ulhobject->rdd_ldbdesc);
			if (DB_SUCCESS_MACRO(status))
			    *desc_p = rdf_ulhobject->rdd_ldbdesc;
			else
			{
			    /* if having problem in ldb descriptor, clear the 
			    ** ldb_id so that it won't get reused. */
			    rdf_ulhobject->rdd_ldbdesc->
					    dd_i1_ldb_desc.dd_l5_ldb_id = 0;
			    rdf_ulhobject->rdf_state = RDF_SBAD;
			    global->rdf_resources |= RDF_BAD_LDBDESC;
			}
		    }
		    else
			*desc_p = rdf_ulhobject->rdd_ldbdesc;
		}
	    } while (0);	/* end control loop */
	    /* release semaphore if held */
	    if (global->rdf_ddrequests.rdd_ddstatus | RDD_SEMAPHORE)
	    {
		t_status = rdd_resetsem(global,RDF_BLD_LDBDESC);
		/* keep the most severe error status */
		status = (t_status > status) ? t_status : status;
	    }
	}
	else
	{
	    /* the object was already on the cache, so return ptr to it. */
	    *desc_p = rdf_ulhobject->rdd_ldbdesc;
	    /* save ptr for future ref */
	    global->rdf_d_ulhobject = rdf_ulhobject; 
	}
    }
    else    /* ulh_create failed */
	rdu_ferror(global, status, &global->rdf_dist_ulhcb.ulh_error,
		   E_RD0045_ULH_ACCESS,0);

    return (status);
}

/*
** {
** Name: rdd_cvtype	- convert data type name to data type id.
**
** Description:
**      This routine calls adf to convert data type name to 
**	data type id. Note that datatype must be converted to
**	lower case and null terminated before calling adf. 
**
** Inputs:
**      global                          ptr to RDF global state variable
**	type_name			ptr to data type name
**	tp				ptr to retrun data type id
**
** Outputs:
**	tp				ptr to data type id
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-sep-88 (mings)
**          initial creation
**	20-jun-90 (teresa)
**	   added new parameter to rdu_ferror() for bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
**      13-Oct-1993 (fred)
**          Internationalized and allowed blanks inside type names
**          (for example, "long varchar" or "long byte").
*/
DB_STATUS
rdd_cvtype( RDF_GLOBAL	*global,
	    char	*type_name,
	    DB_DT_ID	*tp,
	    ADF_CB	*adfcb)
{
    DB_STATUS	    status;
    DB_DT_ID	    dt_id;
    ADI_DT_NAME	    dt_name;
    char            buffer[DB_MAXNAME + 1];
    char            *string = buffer;
    char	    *dst = dt_name.adi_dtname;
    i4		    cnt;
    i4              saw_space = 0;

    /* convert datatype to lower case */

    STncpy(string, type_name, DB_MAXNAME);
    string[ DB_MAXNAME ] = '\0';
    STtrmwhite(string);

    for (cnt = 0;
	 cnt < DB_MAXNAME;
	 CMbyteinc(cnt, string), CMnext(string), CMnext(dst))
    {
	CMtolower (string, dst);
	if (string[0] == EOS)
	    break;
    }

    /* call adf for data type id */

    status = adi_tyid(adfcb, &dt_name, &dt_id);

    if (DB_FAILURE_MACRO(status))
    {
        DB_ERROR	adferror;

	SETDBERR(&adferror, 0, adfcb->adf_errcb.ad_errcode);
	rdu_ferror(global, status, &adferror,
		   E_RD025D_INVALID_DATATYPE,0);
    }
    else
	*tp = dt_id;

    return(status);
}    

/*{
** Name: rdd_cvstorage	- convert storage type to storage id.
**
** Description:
**      This routine converts storage type to it's corresponding
**	storage id.
**
** Inputs:
**      storage                         ptr to storage name
**	tp	         		ptr to return storage id
**
** Outputs:
**	tp				ptr to storage id
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-sep-88 (mings)
**          initial creation
**      23-may-89 (mings)
**          return 0 for unknow storage type
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_cvstorage(	RDD_16CHAR	storage,
		i4		*tp)
{

	if (MEcmp((PTR)storage, (PTR)Iird_heap,
		 STlength(Iird_heap)) == 0)
	    *tp = DMT_HEAP_TYPE;
	else if (MEcmp((PTR)storage, (PTR)Iird_isam,
		 STlength(Iird_isam)) == 0)
	        *tp = DMT_ISAM_TYPE;
	else if (MEcmp((PTR)storage, (PTR)Iird_hash,
		 STlength(Iird_hash)) == 0)
	        *tp = DMT_HASH_TYPE;
	else if (MEcmp((PTR)storage, (PTR)Iird_btree,
		 STlength(Iird_btree)) == 0)
	        *tp = DMT_BTREE_TYPE;
	else if (MEcmp((PTR)storage, (PTR)Iird_unknown,
		 STlength(Iird_unknown)) == 0)
		*tp = 0;	/* unknown storage type */
	     else 
		return(E_DB_ERROR);

	return(E_DB_OK);
}

/*{
** Name: rdd_cvobjtype - convert object/table type.
**
** Description:
**      This routine converts object/table to it's corresponding
**	id. Followings are converting rules:
**
**	"L" to DD_1OBJ_LINK,
**	"T" to DD_2OBJ_TABLE,
**      "V" to DD_3OBJ_VIEW,
**	"I" to DD_4OBJ_INDEX,
**	and DD_0OBJ_NONE for unknown.
**
** Inputs:
**      storage                         ptr to storage name
**	tp	         		ptr to return storage id
**
** Outputs:
**	tp				ptr to storage id
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-sep-88 (mings)
**          initial creation
**	16-jul-92 (teresa)
**	    prototypes
**	16-nov-92 (teresa)
**	    Add support for REGPROCS.  
*/
VOID
rdd_cvobjtype(	char		*object_type,
		DD_OBJ_TYPE	*tp)
{
    switch (object_type[0])
    {
	case 'L':
		*tp = DD_1OBJ_LINK;
		break;
        case 'T':
		*tp = DD_2OBJ_TABLE;
		break; 
        case 'V':
		*tp = DD_3OBJ_VIEW; 
		break;
	case 'I':
		*tp = DD_4OBJ_INDEX; 
		break;
	case 'P':
		*tp = DD_5OBJ_REG_PROC; 
		break;
	default:
		*tp = DD_0OBJ_NONE; 
    }
    
}

/*{
** Name: rdd_tab_init - initialize DMT_TBL_ENTRY.
**
** Description:
**      This routine initialize fields in DMT_TBL_ENTRY.
**
** Inputs:
**      tbl_p				ptr to DMT_TBL_ENTRY
**
** Outputs:
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-sep-88 (mings)
**          initial creation
**	16-jul-92 (teresa)
**	    prototypes
**	06-mar-96 (nanpr01)
**	    Variable page size project. Initialize the pgsize field. 
*/
VOID
rdd_tab_init(DMT_TBL_ENTRY  *tbl_p)
{

    tbl_p->tbl_id.db_tab_base = 0;
    tbl_p->tbl_id.db_tab_index = 0;
    tbl_p->tbl_loc_count = 0;
    tbl_p->tbl_attr_count = 0;
    tbl_p->tbl_index_count = 0;
    tbl_p->tbl_width = 0;
    tbl_p->tbl_storage_type = 0;
    tbl_p->tbl_status_mask = 0;
    tbl_p->tbl_record_count = 0;
    tbl_p->tbl_page_count = 0;
    tbl_p->tbl_dpage_count = 0;
    tbl_p->tbl_ipage_count = 0;
    tbl_p->tbl_expiration = 0;
    tbl_p->tbl_date_modified.db_tab_high_time = 0;
    tbl_p->tbl_date_modified.db_tab_low_time = 0;
    tbl_p->tbl_create_time = 0;
    tbl_p->tbl_qryid.db_qry_high_time = 0;
    tbl_p->tbl_qryid.db_qry_low_time = 0;
    tbl_p->tbl_struct_mod_date = 0;
    tbl_p->tbl_i_fill_factor = 0;
    tbl_p->tbl_d_fill_factor = 0;
    tbl_p->tbl_l_fill_factor = 0;
    tbl_p->tbl_min_page = 0;
    tbl_p->tbl_max_page = 0;
    tbl_p->tbl_pgsize = 0;
		
}

/*{
** Name: rdd_rtrim - trim the blanks on the right and null terminate.
**
** Description:
**      This routine trim the blanks for object name and object owner.
**
** Inputs:
**	tab_name		pointer to name
**	tab_owner		pointer to owner
**	id_p			pointer to returned name and owner
**
** Outputs:
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-sep-88 (mings)
**          initial creation
**	16-jul-92 (teresa)
**	    prototypes
*/
VOID
rdd_rtrim(  PTR		    tab_name,
	    PTR		    tab_owner,
	    RDD_OBJ_ID	    *id_p)
{

    /* Copy name to id_p->tab_name and trim the right blanks. */

    MEcopy((PTR)tab_name, sizeof(DD_NAME), (PTR)id_p->tab_name);
    id_p->tab_name[DB_MAXNAME] = EOS;
    id_p->name_length = STtrmwhite(id_p->tab_name);

    /* Copy owner to id_p->tab_owner and trim the right blanks. */
    MEcopy((PTR)tab_owner, sizeof(DD_NAME), (PTR)id_p->tab_owner);
    id_p->tab_owner[DB_MAXNAME] = EOS;
    id_p->owner_length = STtrmwhite(id_p->tab_owner);
    
}

/*{
** Name: rdd_query - send a query to qef.
**
** Description:
**      This routine sends query to QEF.
**
** Inputs:
**      global				    ptr to RDF state descriptor 
**	    .rdf_qefrcb			    qef request control block
**		.qef_r3_ddb_req		    QEF_DDB_REQ
**		    .qer_d4_qry_info	
**			.qed_q4_packet      
**			    .dd_p1_len	    length of query text
**			    .dd_p2_pkt_p    pointer to query text
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-sep-88 (mings)
**	    initial creation
**      06-jun-89 (mings)
**          let PSF take care the error if E_QE0506_CANNOT_GET_ASSOCIATION 
**      07-jun-89 (mings)
**	    assign zero to timeout
**	20-jun-90 (teresa)
**	   added new parameter to rdu_ferror() for bug 4390.
**	27-jun-90 (teresa)
**	    added trace point RDU_QRYTXT (96) to print query text RDF will send.
**	16-jul-92 (teresa)
**	    prototypes
**	17-sep-92 (teresa)
**	    changed trace poitn rdu_qrytxt (96) to RD0023 as per RDF sybil spec.
*/
DB_STATUS
rdd_query(RDF_GLOBAL *global)
{
    DB_STATUS		status;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    i4		firstval, secondval;

    /* consistency check */

    if (global->rdf_ddrequests.rdd_ddstatus & RDD_FETCH)
    {
	rdu_ierror(global, E_DB_ERROR, E_RD026E_RDFQUERY); 
	return(E_DB_ERROR);
    }

    qefrcb->qef_cb = (QEF_CB *)NULL;
    qefrcb->qef_r1_distrib |= DB_3_DDB_SESS;
    qefrcb->qef_modifier = 0;
    ddr_p->qer_d4_qry_info.qed_q1_timeout = 0;
    ddr_p->qer_d4_qry_info.qed_q2_lang = DB_SQL;
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p3_nxt_p = (DD_PACKET *)NULL;
    ddr_p->qer_d4_qry_info.qed_q5_dv_cnt = 0;
    ddr_p->qer_d4_qry_info.qed_q6_dv_p = (DB_DATA_VALUE *)NULL;

    if ( global->rdf_sess_cb
       &&
	 (ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0023, &firstval, 
			  &secondval))
       )
    {
	/* trace point to display query text -- set trace point rd23 */
	TRdisplay("\n\nRDF query text:%s\n",
	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p);
	if ( ddr_p->qer_d2_ldb_info_p)
	{
	    TRdisplay("LDB_DESC.dd_l1_ingres_b : %d\n",
		ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc.dd_l1_ingres_b);
	    TRdisplay("LDB_DESC.dd_l2_node_name : %32s\n",
		ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc.dd_l2_node_name );
	    TRdisplay("LDB_DESC.dd_l3_ldb_name: %32s\n",
		ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc.dd_l3_ldb_name);
	    TRdisplay("LDB_DESC.dd_l4_dbms_name: %32s\n",
		ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc.dd_l4_dbms_name );
	    TRdisplay("LDB_DESC.dd_l5_ldb_id : %d\n",
		ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc.dd_l5_ldb_id );
	    TRdisplay("LDB_DESC.dd_l6_flags: %d\n",
		ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc.dd_l6_flags );
	}
	TRdisplay(
	"----------------------------------------------------------------\n");
    }

    status = qef_call(QED_1RDF_QUERY, ( PTR ) qefrcb);	    /* send query to qef */

    if (DB_FAILURE_MACRO(status))
	if (qefrcb->error.err_code == E_QE0506_CANNOT_GET_ASSOCIATION)
	    SETDBERR(&global->rdfcb->rdf_error, 0, E_RD0277_CANNOT_GET_ASSOCIATION);
	else
	    rdu_ferror(global, status, &qefrcb->error,
			E_RD0250_DD_QEFQUERY,0); 
    else    
        global->rdf_ddrequests.rdd_ddstatus |= RDD_FETCH;

    return(status);
}

/*{
** Name: rdd_fetch  - call qef to fetch a tuple.
**
** Description:
**      This routine calls QEF to fetch a tuple.
**
** Inputs:
**      global				    ptr to RDF state descriptor 
**	    .rdf_qefrcb			    qef request control block
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-sep-88 (mings)
**          initial creation
**      07-jun-89 (mings)
**	    assign zero to timeout
**	20-jun-90 (teresa)
**	   added new parameter to rdu_ferror() for bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_fetch( RDF_GLOBAL *global)
{
    DB_STATUS		status;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;

    /* consistency check */

    if (!(global->rdf_ddrequests.rdd_ddstatus & RDD_FETCH))
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD026F_RDFFETCH); 
	return(status);
    }

    qefrcb->qef_cb = (QEF_CB *)NULL;
    qefrcb->qef_r1_distrib |= DB_3_DDB_SESS;
    qefrcb->qef_modifier = 0;
    ddr_p->qer_d4_qry_info.qed_q1_timeout = 0;
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_0MODE_NONE;

    status = qef_call(QED_2RDF_FETCH, ( PTR ) qefrcb);	/* fetch tuple */

    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &qefrcb->error,
	           E_RD0251_DD_QEFFETCH,0); 
	global->rdf_ddrequests.rdd_ddstatus &= (~RDD_FETCH); /* reset the status */

	return(status);
    }

    if (!(ddr_p->qer_d5_out_info.qed_o3_output_b))
	global->rdf_ddrequests.rdd_ddstatus &= (~RDD_FETCH); /* reset the status */

    return(status);
}

/*{
** Name: rdd_flush - call qef to flush.
**
** Description:
**      This routine calls QEF to flush the tuples return from
**	the sending query.
**
** Inputs:
**      global				    ptr to RDF state descriptor 
**	    .rdf_qefrcb			    qef request control block
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-sep-88 (mings)
**          initial creation
**	20-jun-90 (teresa)
**	   added new parameter to rdu_ferror() for bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_flush( RDF_GLOBAL *global)
{
    DB_STATUS		status;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;

    /* consistency check */

    if (!(global->rdf_ddrequests.rdd_ddstatus & RDD_FETCH))
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0270_RDFFLUSH); 
	return(status);
    }

    qefrcb->qef_cb = (QEF_CB *)NULL;
    qefrcb->qef_r1_distrib |= DB_3_DDB_SESS;
    qefrcb->qef_modifier = 0;
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_0MODE_NONE;

    status = qef_call( QED_3RDF_FLUSH, ( PTR ) qefrcb);

    global->rdf_ddrequests.rdd_ddstatus &= (~RDD_FETCH); /* reset the status */

    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &qefrcb->error,
	           E_RD0252_DD_QEFFLUSH,0); 

    }
    return(status);
}

/*{
** Name: rdd_vdada - send a query with db data values to qef.
**
** Description:
**      This routine sends query to QEF.
**
** Inputs:
**      global				    ptr to RDF state descriptor 
**	    .rdf_qefrcb			    qef request control block
**		.qef_r3_ddb_req		    QEF_DDB_REQ
**		    .qer_d4_qry_info	
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-sep-88 (mings)
**          initial creation
**	20-jun-90 (teresa)
**	   added new parameter to rdu_ferror() for bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_vdata( RDF_GLOBAL *global)
{
    DB_STATUS		status;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;

    qefrcb->qef_cb = (QEF_CB *)NULL;
    qefrcb->qef_r1_distrib |= DB_3_DDB_SESS;
    qefrcb->qef_modifier = 0;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;
    ddr_p->qer_d4_qry_info.qed_q2_lang = DB_SQL;
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p3_nxt_p = (DD_PACKET *)NULL;

    status = qef_call(QED_4RDF_VDATA, ( PTR ) qefrcb);	    /* send query to qef */

    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &qefrcb->error,
	    E_RD0250_DD_QEFQUERY,0); 
    }

    return(status);
}

/*{
** Name: rdd_begin - send a notice to qef for a series of operations.
**
** Description:
**      This routine sends notice to QEF for a series of operations.
**
** Inputs:
**      global				    ptr to RDF state descriptor 
**	    .rdf_qefrcb			    qef request control block
**		.qef_r3_ddb_req		    QEF_DDB_REQ
**		    .qer_d4_qry_info	
**			.qed_q4_packet      
**			    .dd_p1_len	    length of query text
**			    .dd_p2_pkt_p    pointer to query text
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-sep-88 (mings)
**          initial creation
**	20-jun-90 (teresa)
**	   added new parameter to rdu_ferror() for bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_begin( RDF_GLOBAL *global)
{
    DB_STATUS		status = E_DB_OK;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;

    qefrcb->qef_cb = (QEF_CB *)NULL;
    qefrcb->qef_r1_distrib |= DB_3_DDB_SESS;
    qefrcb->qef_modifier = 0;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;
    ddr_p->qer_d4_qry_info.qed_q2_lang = DB_SQL;
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p3_nxt_p = (DD_PACKET *)NULL;
    ddr_p->qer_d4_qry_info.qed_q5_dv_cnt = 0;
    ddr_p->qer_d4_qry_info.qed_q6_dv_p = (DB_DATA_VALUE *)NULL;

    status = qef_call(QED_5RDF_BEGIN, ( PTR ) qefrcb);	    /* send operation notice to qef */

    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &qefrcb->error,
	    E_RD0250_DD_QEFQUERY,0);
    }

    return(status);
}

/*{
** Name: rdd_username - retrieve local username and compare with local object name.
**		    
**
** Description:
**      This routine retrieve local user name and compare with local object name in
**	CDB.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**	    .rdf_tobj			temporary object descriptor
**		.dd_o9_tab_info		ldb table info
**		    .dd_t2_tab_owner	local table ownere for an object
**
** Outputs:
**	None
**					
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_username(RDF_GLOBAL *global)
{
    DB_STATUS		status = E_DB_OK;
    DD_2LDB_TAB_INFO	*ldbtab_p = &global->rdf_tobj.dd_o9_tab_info;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_1LDB_INFO	ldb_info;
    DD_NAME		user_name;

    /* Assign ldb info */

    STRUCT_ASSIGN_MACRO(*ldbtab_p->dd_t9_ldb_p, ldb_info);
    ldb_info.dd_i1_ldb_desc.dd_l1_ingres_b = FALSE;
    ddr_p->qer_d2_ldb_info_p = &ldb_info;
    
    /* get local user name */

    status = rdf_userlocal(global, user_name);	    

    if (DB_SUCCESS_MACRO(status)
	&&
	MEcmp((PTR)ldbtab_p->dd_t2_tab_owner, (PTR)user_name, sizeof(DD_NAME))
       )
    {
	status = E_DB_ERROR;
	SETDBERR(&global->rdfcb->rdf_error, 0, E_RD026D_USER_NOT_OWNER);
    }	 
    
    return(status);
}

/*{
** Name: rdd_tableupd - update num of rows and pages in iidd_tables.
**
** Description:
**      This routine generates query to update the number of rows and number of
**	pages columns in iidd_tables.
**	This is the case when rows or pages is out side the 10 percent range.
**
**	We will use ingres association to update the catalog. 
**
** Inputs:
**      global                          ptr to RDF state descriptor
**	    .rdf_tobj			temporary object descriptor
**		.dd_o9_tab_info		ldb table info
**		    .dd_t1_tab_name	local table name for an object
**		    .dd_t2_tab_owner	local table ownere for an object
**		    .dd_t5_alt_date	local table alter date stored in CDB
**		    .dd_t9_ldb_p	ldb info
**
** Outputs:
**	None
**					
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	20-jun-90 (teresa)
**	   added new parameter to rdu_ferror() for bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_tableupd(	RDF_GLOBAL         *global,
		RDD_OBJ_ID         *id_p,
		i4		   numrows,
		i4		   numpages,
		i4		   overpages)
{
    DB_STATUS		status;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET		ddpkt;
    char		qrytxt[RDD_QRY_LENGTH];


    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	update iidd_tables set num_rows = numrows, number_pages = numpages, 
    **			       overflow_pages = overpages
    **   where table_name = object_name and table_owner = object_owner;
    **
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	    "update iidd_tables set num_rows = %d, number_pages = %d, \
overflow_pages = %d where table_name = '%s' and table_owner = '%s';",
	    numrows,
	    numpages,
	    overpages,
	    id_p->tab_name,
	    id_p->tab_owner);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = (i4) STlength((char *)qrytxt);
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;
    qefrcb->qef_cb = (QEF_CB *)NULL;
    qefrcb->qef_r1_distrib |= DB_3_DDB_SESS;
    qefrcb->qef_modifier = 0;
    ddr_p->qer_d4_qry_info.qed_q2_lang = DB_SQL;
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p3_nxt_p = (DD_PACKET *)NULL;

    /* although sending update query out, we have to pretend that this is
    ** a fetch query.  */
 
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

    /* Send query to update iidd_tables */

    status = qef_call(QED_1RDF_QUERY, ( PTR ) qefrcb);	    

    if (DB_FAILURE_MACRO(status))
	rdu_ferror(global, status, &qefrcb->error,
		   E_RD0250_DD_QEFQUERY,0); 
    else
    {
	global->rdf_ddrequests.rdd_ddstatus |= RDD_FETCH; /* set status */ 

	/* flush */

	status = rdd_flush(global);
    }

    return(status);
}

/*{
** Name: rdd_stampupd - update time stamp in iidd_ddb_tableinfo.
**
** Description:
**      This routine generates query to update the time stamps in iidd_ddb_tableinfo.
**	This is the case when local table time stamp changed but not schema.
**
**	We will use ingres association to update the catalog. 
**
** Inputs:
**      global                          ptr to RDF state descriptor
**	    .rdf_tobj			temporary object descriptor
**		.dd_o9_tab_info		ldb table info
**		    .dd_t1_tab_name	local table name for an object
**		    .dd_t2_tab_owner	local table ownere for an object
**		    .dd_t5_alt_date	local table alter date stored in CDB
**		    .dd_t9_ldb_p	ldb info
**
** Outputs:
**	None
**					
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	20-jun-90 (teresa)
**	   added new parameter to rdu_ferror() for bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_stampupd(	RDF_GLOBAL         *global,
		u_i4		   stamp1,
		u_i4		   stamp2)
{
    DB_STATUS		status;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET		ddpkt;
    char		qrytxt[RDD_QRY_LENGTH];

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	update iidd_ddb_tableinfo set table_relstamp1 = stamp1, table_relstamp1 = stamp2 
    **	    where object_base = object_base and object_index = object_index;
    **
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	    "update iidd_ddb_tableinfo set table_relstamp1 = %d, table_relstamp2 = %d \
where object_base = %d and object_index = %d;",
	    stamp1,
	    stamp2,
	    global->rdf_tobj.dd_o3_objid.db_tab_base,
	    global->rdf_tobj.dd_o3_objid.db_tab_index);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = (i4) STlength((char *)qrytxt);
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;
    qefrcb->qef_cb = (QEF_CB *)NULL;
    qefrcb->qef_r1_distrib |= DB_3_DDB_SESS;
    qefrcb->qef_modifier = 0;
    ddr_p->qer_d4_qry_info.qed_q2_lang = DB_SQL;
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p3_nxt_p = (DD_PACKET *)NULL;
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

    /* Send query to update iidd_ddb_tableinfo time stamp */

    status = qef_call(QED_1RDF_QUERY, ( PTR ) qefrcb);	    

    if (DB_FAILURE_MACRO(status))
	rdu_ferror(global, status, &qefrcb->error,
		    E_RD0250_DD_QEFQUERY,0); 
    else
    {
	global->rdf_ddrequests.rdd_ddstatus |= RDD_FETCH; /* set status */ 

	/* flush */

	status = rdd_flush(global);
    }

    return(status);
}

/*{
** Name: rdd_baseviewchk - check views on an object 
**
** Description:
**      This routine generates query to retrieve dependent objects and the setup
**	view info to DMT_VIEWBASE in tbl_status_mask of DMT_TBL_ENTRY.
**
** Inputs:
**      global                          ptr to RDF state descriptor
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-apr-89 (mings)
**         created for Titan
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_baseviewchk( RDF_GLOBAL *global)
{
    DB_STATUS		status = E_DB_OK;
    DD_OBJ_DESC         *obj_p = &global->rdf_tobj;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DMT_TBL_ENTRY	*tbl_p = &global->rdf_trel;
    DD_PACKET		ddpkt;
    RDC_DBDEPENDS	dbdepends;
    RQB_BIND            rq_bind[RDD_01_COL],	/* 1 column */
			*bind_p = rq_bind;
    char		qrytxt[RDD_QRY_LENGTH];

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select dtype from iidd_ddb_dbdepends where 
    **	  inid1 = object_base and inid2 = object_index
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	    "%s %d and inid2 = %d;",
	    "select dtype from iidd_ddb_dbdepends where inid1 = ",
	    obj_p->dd_o3_objid.db_tab_base, 
	    obj_p->dd_o3_objid.db_tab_index);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    /* dependency type */
    rq_bind[0].rqb_addr = (PTR)&dbdepends.dtype;
    rq_bind[0].rqb_length = sizeof(dbdepends.dtype);
    rq_bind[0].rqb_dt_id = DB_INT_TYPE;
    
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_01_COL;

    do
    {
	/* fetch the next dependency */

	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	/* more non protection dependency tuples */

        if ((ddr_p->qer_d5_out_info.qed_o3_output_b)
	    &&
	    (dbdepends.dtype == DB_VIEW)
	   )
        {  
	    /* set DMT_VIEWBASE, flush buffer, and return */
	    tbl_p->tbl_status_mask |= DMT_BASE_VIEW;
	    return(rdd_flush(global));
	}

    } while(ddr_p->qer_d5_out_info.qed_o3_output_b);

    return(E_DB_OK);
}

/*{
** Name: rdd_schemachk - schema consistency check.
**
** Description:
**	This routine checks the schema of the table by comparing the iicolumns
**	in the LDB with iidd_columns from the CDB.  It tolerates new attributes
**	at the end of the table, but does not tollerate any changes to the
**	attributes already known to Star.
**
**	The schema is considered to change if:
**	    fields:		    critieria:		    
**	    -------------------	    -------------------------------------------
**	    att_name/name	    column name changes
**	    att_number/sequence	    column moves in table
**	    att_prec/scale	    column's precision changes
**	    att_flags/nulls &	    column changes from/to not null, no default
**		      default
**	    att_type/datatype	    data type changes
**	    att_width/length	    attribute size changes
**
**	The comparison of DDB to LDB column names requires case translation
**	if there is no column mapping (i.e., no entry in iiddb_ldb_columns).
**	The entry in iidd_columns follows the case translation semantics of
**	the DDB.  In order to make the comparison between the column name
**	in iidd_columns with that in the LDB's iicolumns, the following
**	rules apply.
**
**	Case translation semantics  Comparison action of Star for
**	of delimited ids	        iidd_columns vs. iicolumns
**	--------------------------  ----------------------------
**	DDB                LDB
**      -----              -----
**	Upper/Lower/Mixed  Upper    Upper DDB name
**	Upper/Lower/Mixed  Lower    Lower DDB name
**	Upper/Lower        Mixed    This case should never happen because Star
**				    will create a mapping when table is first
**				    registered.
**	Mixed              Mixed    No case changes
**      
**	This routine generates and sends a query to the LDB to query iicolumns.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**	    .rdfcb			RDF Control Block
**		.rdf_rb			RDF Request Block
**		    .rdf_info_blk	Ptr to cache object
**			.rdr_obj_desc	Distributed object descriptor
**			    .ldbtab_p	Ptr to Local Table Descriptor
**				.dd_t7_col_cnt  num. attributes in iidd_columns
**	    .rdr_types_mask		Bitfield of instructions:
**					    RDR_REFRESH - register w/ refresh
**	    .rdf_ddrequests		Distributed Request communication block
**		.rdd_attr		Ptr to array of attribute infomation
**					    that will eventually go on the cache
**		    .att_name		column name
**		    .att_number		column number
**		    .att_prec		column's precision
**		    .att_flags		bitmap of info about column: DMT_N_NDEFAULT
**		    .att_type		column's datatype
**		    .att_width		number of bytes to store this column
**	obj_id_p			Ptr to object id
**	map_p				Ptr to array of mapped names
**	adfcb				Ptr to initialized control block for ADF
**
** Outputs:
**      global                          ptr to RDF state descriptor
**	    rdfcb			RDF Request Control Block
**		rdf_error		RDF's error control block
**		    err_code		Error code if status != E_DB_OK:
**						E_RD0269_OBJ_DATEMISMATCH
**		
**	    rdf_local_info		contains description of local's schema
**					change:
**					    RDR_L0_NO_SCHEMA_CHANGE
**					    RDR_L1_SUPERSET_SCHEMA
**					    RDR_L2_SUBSET_SCHEMA
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	24-sep-90 (teresa)
**	    order by column_sequence since some gateways naturally return
**	    tuples in decending order instead of ascending
**	08-may-92 (teresa)
**	    rewrote for Sybil to eliminate use of temporary memory.
**	16-jul-92 (teresa)
**	    prototypes
**	19-aug-92 (teresa)
**	    Initialize local_names.
**	1-mar-93 (barbara)
**	    Support for delimited ids -- convert case before comparing
**	    column names in CDB with column names from LDB.
**      26-apr-93 (vijay)
**          Using array name to access the first element not allowed.
**	08-sep-93 (teresa)
**	    Fix bug 54383 - calculate percision for Decimal Data types.
**	01-nov-93 (teresa)
**	    Fix bug 56185.
*/
DB_STATUS
rdd_schemachk(	RDF_GLOBAL         *global,
		RDD_OBJ_ID	   *obj_id_p,
		DD_NAME		   *map_p,
		ADF_CB		   *adfcb)
{
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		t_status;
    DD_OBJ_DESC		*obj_p = global->rdfcb->rdf_info_blk->rdr_obj_desc;
    DD_2LDB_TAB_INFO    *ldbtab_p = &obj_p->dd_o9_tab_info;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DMT_ATT_ENTRY	*star_attr;
    DD_PACKET		ddpkt;
    RDD_OBJ_ID		table_id,
			*id_p = &table_id;
    DD_1LDB_INFO	ldb_info;
    RDC_COLUMNS		column;
    i4			cnt;
    RQB_BIND            rq_bind[RDD_07_COL],	/* for 7 columns */
			*bind_p = rq_bind;
    char		qrytxt[RDD_QRY_LENGTH];
    DD_NAME		*local_names;
    DB_DATA_VALUE       dv,col_width;


    /* set local table name */
    rdd_rtrim((PTR)ldbtab_p->dd_t1_tab_name,
	      (PTR)ldbtab_p->dd_t2_tab_owner,
	       id_p);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select column_name, column_datatype, column_length, column_scale, column_nulls,
    **	column_defaults, column_sequence from IICOLUMNS where
    **           table_name = 'table_name' and
    **           table_owner = 'table_owner' order by column_sequence
    **
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	     "%s '%s' and table_owner = '%s' order by column_sequence;", 	    
	    "select column_name, column_datatype, column_length, column_scale, column_nulls, \
column_defaults, column_sequence from iicolumns where table_name =",
	    id_p->tab_name, 
	    id_p->tab_owner);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = (i4) STlength((char *)qrytxt);

    /* assign ldb info for ldb access */

    STRUCT_ASSIGN_MACRO(*ldbtab_p->dd_t9_ldb_p, ldb_info);
    ldb_info.dd_i1_ldb_desc.dd_l1_ingres_b = FALSE;
    ddr_p->qer_d2_ldb_info_p = &ldb_info;
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

    /* Send query to ldb for local table column info */

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    rq_bind[0].rqb_addr = (PTR)column.name; 
    rq_bind[0].rqb_length = sizeof(DD_NAME);
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
    rq_bind[1].rqb_addr = (PTR)column.datatype; 
    rq_bind[1].rqb_length = sizeof(DD_NAME);
    rq_bind[1].rqb_dt_id = DB_CHA_TYPE;
    rq_bind[2].rqb_addr = (PTR)&column.length;
    rq_bind[2].rqb_length = sizeof(column.length);
    rq_bind[2].rqb_dt_id = DB_INT_TYPE;
    rq_bind[3].rqb_addr = (PTR)&column.scale;
    rq_bind[3].rqb_length = sizeof(column.scale);
    rq_bind[3].rqb_dt_id = DB_INT_TYPE;
    rq_bind[4].rqb_addr = (PTR)column.nulls; 
    rq_bind[4].rqb_length = sizeof(RDD_8CHAR);
    rq_bind[4].rqb_dt_id = DB_CHA_TYPE;
    rq_bind[5].rqb_addr = (PTR)column.defaults;
    rq_bind[5].rqb_length = sizeof(RDD_8CHAR);
    rq_bind[5].rqb_dt_id = DB_CHA_TYPE;
    rq_bind[6].rqb_addr = (PTR)&column.sequence;
    rq_bind[6].rqb_length = sizeof(column.sequence);
    rq_bind[6].rqb_dt_id = DB_INT_TYPE;

    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_07_COL;

    cnt = 0;
    do
    {
	/* Fetch next tuple */
	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	{
	    cnt += 1;
	    if (cnt <= ldbtab_p->dd_t7_col_cnt)
	    {
		star_attr = (DMT_ATT_ENTRY *)
		             global->rdf_ddrequests.rdd_attr[cnt];

		/* check for schema change.  Are all of these really necessary,
		** or should some of them (like scale and some datatype changes)
		** be relaxed??
		**
		** A schema change if:
		**  fields:		    critieria:		    
		**  -------------------	    ------------------------------------
		**  att_name/name	    column name changes
		**  att_number/sequence	    column moves in table
		**  att_prec/scale	    ???
		**  att_flags/nulls &	    column changes from/to not null, no
		**	     default	    default
		**  att_type/datatype	    data type changes
		**  att_width/length	    attribute size has changed
		*/
		/* Compare attribute names.  See rules in header comments. */
		if (map_p != NULL)
		{
		    if ( MEcmp( (PTR)column.name, 
				(PTR)map_p[cnt],
				sizeof(DD_NAME)
			      )
		       )
		    {
			status = E_DB_ERROR;
			break;
		    }
		}
		else
		{
		    char attname_buf[DB_MAXNAME];
		    char *buf_p = attname_buf;
		    char *attname_p = star_attr->att_name.db_att_name;
		    char *last = attname_p + sizeof(DD_NAME) -1;

		    if (ldb_info.dd_i2_ldb_plus.dd_p3_ldb_caps.
			dd_c10_delim_name_case == DD_0CASE_LOWER)
		    {
			while (attname_p <= last)
		    	{
			    CMtolower(attname_p, buf_p);
			    CMnext(attname_p);
			    CMnext(buf_p);
			}
			attname_p = attname_buf;
		    }
		    else if (ldb_info.dd_i2_ldb_plus.dd_p3_ldb_caps.
			dd_c10_delim_name_case == DD_2CASE_UPPER)
		    {
			while (attname_p <= last)
		    	{
			    CMtoupper(attname_p, buf_p);
			    CMnext(attname_p);
			    CMnext(buf_p);
			}
			attname_p = attname_buf;
		    }
		    else
		    {
			attname_p = star_attr->att_name.db_att_name;
		    }

 		    if ( MEcmp((PTR)column.name, (PTR)attname_p,
			        sizeof(DD_NAME))
			)
		    {
		    	status = E_DB_ERROR;
		    	break;
		    }
		}

		/* compare sequence and null/default */
		if ( column.sequence != star_attr->att_number ||
		     (
		       (
		        (column.nulls[0] == 'N' && column.defaults[0] == 'N') ||
		        (column.nulls[0] == 'n' && column.defaults[0] == 'n')
		       ) 
		       &&
		       !(star_attr->att_flags & DMU_F_NDEFAULT)
		     )
		   )
		{
		    status = E_DB_ERROR;
		    break;
		}

		/* set up datavalue for datatype conversion and compare */
		t_status = rdd_cvtype(global, column.datatype,
                                &dv.db_datatype, adfcb);
		if (DB_FAILURE_MACRO(t_status))
		    return (t_status);

		/* Setup db data value for column width conversion and compare--
		** (to set up precision, use zero for all but decimal data types
		** and call macros to set up precision based on length and scale
		** for decimal data types.)
		*/
		if( dv.db_datatype == DB_DEC_TYPE)
		{
		    dv.db_prec = DB_PS_ENCODE_MACRO(column.length,
						    column.scale);
		    dv.db_length= DB_PREC_TO_LEN_MACRO( (i4)column.length);
		    if ( (column.nulls[0] == 'Y') || (column.nulls[0] == 'y') )
		    {
			/* for nullable Decimal Datatype: 1. negate datatype
			** and 2. increase length by one for default
			*/
			dv.db_datatype *= -1;
			col_width.db_length = dv.db_length + 1;
		    }
		    /* assure scaling has not changed */
		    if (dv.db_prec != star_attr->att_prec)
		    {
			status = E_DB_ERROR;
			break;
		    }
		}
		else
		{
		    /* attribute is NOT decimal, so force precision to 0 and
		    ** call ADF to calculate actual physical length from the
		    ** catalog length
		    */
		    dv.db_prec = 0;
		    dv.db_length = column.length;
		    /* if the column is nullable, make the datatype negative */
		    if ( (column.nulls[0] == 'Y') || (column.nulls[0] == 'y') )
			dv.db_datatype *= -1;
		    t_status = adc_lenchk(adfcb, TRUE, &dv, &col_width);
		    if (DB_FAILURE_MACRO(t_status))
			return (t_status);
		}
		/* if datatypes do not match, complain violently */
		if (dv.db_datatype != star_attr->att_type)
		{
		    status = E_DB_ERROR;
		    break;
		}
		/* if the size of the attribute has changed, complain violently
		*/
		if (col_width.db_length != star_attr->att_width)
		{
		    status = E_DB_ERROR;
		    break;
		}
	    }
	}

    } while (ddr_p->qer_d5_out_info.qed_o3_output_b);

    /* If we found a schema mismatch, we may have stopped reading early.
    ** So assure that the read buffer is cleared out.
    */
    if (ddr_p->qer_d5_out_info.qed_o3_output_b)
    {
	t_status = rdd_flush(global);
	if (DB_FAILURE_MACRO(t_status))
	    return(t_status);
    }
    
    if (global->rdfcb->rdf_rb.rdr_types_mask & RDR_REFRESH)
    {
	if (DB_FAILURE_MACRO(status))
	   /* schema mismatched */
	   global->rdf_local_info |= RDR_L3_SCHEMA_MISMATCH;
	else if (cnt == ldbtab_p->dd_t7_col_cnt)
		/* no schema change */
		global->rdf_local_info |= RDR_L0_NO_SCHEMA_CHANGE;
	     else if (cnt < ldbtab_p->dd_t7_col_cnt)    
		     /* local schema is proper subset */
		     global->rdf_local_info |= RDR_L2_SUBSET_SCHEMA;
		  else
		     /* local schema is proper superset */
		     global->rdf_local_info |= RDR_L1_SUPERSET_SCHEMA;
	status = E_DB_OK;		    
    }
    else
    {
	if (DB_FAILURE_MACRO(status) || cnt < ldbtab_p->dd_t7_col_cnt)
	{
	    status = E_DB_ERROR;
	    SETDBERR(&global->rdfcb->rdf_error, 0, E_RD0269_OBJ_DATEMISMATCH);
	}
	else
	    status = E_DB_OK;
    }

    return(status);
}

/*{
** Name: rdd_alterdate - retrieve table alternation date LDB.
**
** Description:
**      This routine generates query to retrieve local table alter date.
**
**	Note that in order to sync star objects with ldb objects, two updates
**	are implemented in this routine. One is to update time stamps in
**	iidd_ddb_tableinfo when ldb object time stamps were changed.  The other 
**	is to update row counts, pages,... in iidd_tables.  Also note, these
**      two updates are send through ingres association, meaning, deadlock may
**      occure when other user associations tring to query these two catalogs.
**	A trace option RD0021 is supplied, for trace purpose, to turn off these 
**	two updates.  
**
** Inputs:
**      global                          ptr to RDF state descriptor
**	    .rdf_tobj			temporary object descriptor
**		.dd_o9_tab_info		ldb table info
**		    .dd_t1_tab_name	local table name for an object
**		    .dd_t2_tab_owner	local table ownere for an object
**		    .dd_t5_alt_date	local table alter date stored in CDB
**		    .dd_t9_ldb_p	ldb info
**	    .rdfcb			RDF Control Block
**		.rdf_rb			RDF Request Block
**		    .rdr_types_mask	Bitmap of requests:
**					    RDR_REFRESH, RDR_USERTAB
**		.rdf_info_blk		ptr to cache object
**		    rdr_obj_desc	distributed object descriptor
**	mapped_names			ptr to array of mapped names for columns
**	adfcb				ptr to adf control block
**
** Outputs:
**      global                          ptr to RDF state descriptor
**	    rdfcb			RDF Request Control Block
**		rdf_error		RDF's error control block
**		    err_code		Error code if status != E_DB_OK:
**					    E_RD0268_LOCALTABLE_NOTFOUND
**					    E_RD0002_UNKNOWN_TBL
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**      25-apr-89 (mings)
**         add RDU_NO_UPDATE trace checking
**      26-jun-89 (mings)
**         don't check schema if RDR_USERTAB is set
**	12-may-92 (teresa)
**	    Sybil: stop using temporary memory for call to RDD_SCHEMACHK, pass
**	    along new parameter of mapped attribute name array.
**	16-jul-92 (teresa)
**	    prototypes
**	17-sep-92 (teresa)
**	    change RDU_NO_UPDATE to rd0021
*/
DB_STATUS
rdd_alterdate(	RDF_GLOBAL         *global,
		RDD_OBJ_ID	   *obj_id_p,
		DD_NAME		   *mapped_names,
		ADF_CB              *adfcb)
{
    DB_STATUS		status;
    DD_OBJ_DESC		*obj_p = global->rdfcb->rdf_info_blk->rdr_obj_desc;
    DD_2LDB_TAB_INFO    *ldbtab_p = &obj_p->dd_o9_tab_info;
    DD_CAPS		*cap_p = &ldbtab_p->dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps;
    DMT_TBL_ENTRY	*tbl_p = &global->rdf_trel;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    u_i4		stamp1;
    u_i4		stamp2;
    i4			numrows;
    i4			numpages;
    i4			overpages;
    i4		firstval, secondval;
    DD_PACKET		ddpkt;
    RDD_OBJ_ID		table_id,
			*id_p = &table_id;
    DD_1LDB_INFO	ldb_info;
    RQB_BIND            rq_bind[RDD_05_COL],	/* for 5 columns */
			*bind_p = rq_bind;
    char		qrytxt[RDD_QRY_LENGTH];

    /* if this is a RDR_REFRESH request from PSF for register with refresh */

    if (!(global->rdfcb->rdf_rb.rdr_types_mask & RDR_REFRESH))
    {

	rdd_rtrim((PTR)ldbtab_p->dd_t1_tab_name,
		(PTR)ldbtab_p->dd_t2_tab_owner,
		id_p);

	ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

	/* set up query 
	**
	**	select table_relstamp1, table_relstamp2, num_rows, number_pages
	**		 overflow_pages from iitables where
	**           table_name = 'table_name' and table_owner = 'table_owner'
	**
	*/

	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	    "%s '%s' and table_owner = '%s';", 	    
	    "select table_relstamp1, table_relstamp2, num_rows, number_pages, \
overflow_pages from iitables where table_name =",
	    id_p->tab_name, 
	    id_p->tab_owner);

	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = (i4) STlength((char *)qrytxt);

	/* Assign ldb info */
	STRUCT_ASSIGN_MACRO(*ldbtab_p->dd_t9_ldb_p, ldb_info);
	ldb_info.dd_i1_ldb_desc.dd_l1_ingres_b = FALSE;
	ddr_p->qer_d2_ldb_info_p = &ldb_info;
	ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

	status = rdd_query(global);	    /* send query to qef */
	if (DB_FAILURE_MACRO(status))
	    return (status); 

	/* table_relstamp1 */
	rq_bind[0].rqb_addr = (PTR)&stamp1;
	rq_bind[0].rqb_length = sizeof(stamp1);
	rq_bind[0].rqb_dt_id = DB_INT_TYPE;

	/* table_relstamp2 */
	rq_bind[1].rqb_addr = (PTR)&stamp2;
	rq_bind[1].rqb_length = sizeof(stamp2);
	rq_bind[1].rqb_dt_id = DB_INT_TYPE;

	/* number of rows */
	rq_bind[2].rqb_addr = (PTR)&numrows;
	rq_bind[2].rqb_length = sizeof(numrows);
	rq_bind[2].rqb_dt_id = DB_INT_TYPE;
    
	/* number of pages */
	rq_bind[3].rqb_addr = (PTR)&numpages;
	rq_bind[3].rqb_length = sizeof(numpages);
	rq_bind[3].rqb_dt_id = DB_INT_TYPE;

	/* number of overflow pages */
	rq_bind[4].rqb_addr = (PTR)&overpages;
	rq_bind[4].rqb_length = sizeof(overpages);
	rq_bind[4].rqb_dt_id = DB_INT_TYPE;
    
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_05_COL;

	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (!ddr_p->qer_d5_out_info.qed_o3_output_b)
	{  /* no object was found */
	    status = E_DB_ERROR;
	    SETDBERR(&global->rdfcb->rdf_error, 0, E_RD0268_LOCALTABLE_NOTFOUND);
	    return(status);
	}

	/* flush */

	status = rdd_flush(global);

	if (DB_FAILURE_MACRO(status))
	    return(status);

	/* if there is PHYSICAL_SOURCE capability and cap_value is 'P',
	** retrieve physical info from iiphysical_tables */

	if ((cap_p->dd_c1_ldb_caps & DD_7CAP_USE_PHYSICAL_SOURCE)
	    &&
	    (ldbtab_p->dd_t3_tab_type != DD_3OBJ_VIEW)
	   )
	{
	    i4			pnumrows;
	    i4			pnumpages;
	    i4			poverpages;

	    /* set up query to retrieve iiphysical_tables info 
	    **
	    **	select num_rows, number_pages, overflow_pages from iitables where
	    **           table_name = 'table_name' and table_owner = 'table_owner'
	    **
	    */

	    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	    "%s '%s' and table_owner = '%s';", 	    
	    "select num_rows, number_pages, overflow_pages from iiphysical_tables \
where table_name =",
	    id_p->tab_name, 
	    id_p->tab_owner);

	    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
						 (i4)STlength((char *)qrytxt);
	    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

	    status = rdd_query(global);		/* send query to qef */
	    if (DB_FAILURE_MACRO(status))
		return (status); 

	    global->rdf_ddrequests.rdd_ddstatus |= RDD_FETCH; /* set status */ 

	    /* number of rows */
	    rq_bind[0].rqb_addr = (PTR)&pnumrows;
	    rq_bind[0].rqb_length = sizeof(pnumrows);
	    rq_bind[0].rqb_dt_id = DB_INT_TYPE;
    
	    /* number of pages */
	    rq_bind[1].rqb_addr = (PTR)&pnumpages;
	    rq_bind[1].rqb_length = sizeof(pnumpages);
	    rq_bind[1].rqb_dt_id = DB_INT_TYPE;

	    /* number of overflow pages */
	    rq_bind[2].rqb_addr = (PTR)&poverpages;
	    rq_bind[2].rqb_length = sizeof(poverpages);
	    rq_bind[2].rqb_dt_id = DB_INT_TYPE;
    
	    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
	    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_03_COL;

	    status = rdd_fetch(global);
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	    {
		/* flush */
    
		status = rdd_flush(global);

		if (DB_FAILURE_MACRO(status))
		    return(status);
	    
		/* use num_rows, number_pages, and overflow_pages from iiphysical_tables */
		numrows = pnumrows;
		numpages = pnumpages;
		overpages = poverpages;
	    }
	} /* end of iiphysical_tables checking */
    } /* end if for RDR_REFRESH checking */
    
    /* we have everything on hand now. let's start the comparison */

    {

	/* Following counts are retrieved from iidd_tables in rdd_relinfo routine. */

	i4		record_count = tbl_p->tbl_record_count;
	i4         page_count = tbl_p->tbl_page_count;
	i4		over_count = page_count - (tbl_p->tbl_dpage_count);

	/* check schema change when time stamp mismatch or 
        ** it is a RDR_REFRESH request from PSF.
	** Also note that RDR_USERTAB means DROP query, we don't have
	** to check schema at all. */

	if (	
		(
		    !(global->rdfcb->rdf_rb.rdr_types_mask & RDR_USERTAB)
		)
		&&
		(
		    (global->rdfcb->rdf_rb.rdr_types_mask & RDR_REFRESH)
		    ||
		    (tbl_p->tbl_date_modified.db_tab_high_time != stamp1)
		    || 
		    (tbl_p->tbl_date_modified.db_tab_low_time != stamp2) 
	        )
	   )
        {
	    /* this is the case that local table has been modified.
	    ** however, flag error only when schema changed */
	    DB_STATUS	    ulm_status;
	    ULM_RCB	    ulmrcb;

	    /* compare star object schema with local's */
	    status = rdd_schemachk(global, obj_id_p, mapped_names,
				   adfcb);
    	    if (DB_FAILURE_MACRO(status))
		return(status);			     /* return previous error status */

	    /* return here if this is a RDR_REFRESH request from PSF for
	    ** register with refresh */

	    if (global->rdfcb->rdf_rb.rdr_types_mask & RDR_REFRESH)
		return(status);

	    /* Since local table schema didn't get changed, we might just go ahead
	    ** change the time stamp in iidd_ddb_tableinfo as well.  However, if
	    ** RD0021 trace is set, then skip the updating */	    
	    
	    if (!ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0021,
				 &firstval, &secondval)
	       )
		status = rdd_stampupd(global, stamp1, stamp2);

	    if (DB_FAILURE_MACRO(status))
		return(status);

	    tbl_p->tbl_date_modified.db_tab_high_time = stamp1;
	    tbl_p->tbl_date_modified.db_tab_low_time = stamp2; 
	}
	
	/* The following check is an attempt to fix star object inconsistence with
	** local table.  Whenever number of rows or number of pages out side the 10 percent
	** range, we update the iidd_tables.  However, if RD0021 trace is set, then
        ** skip the update here. */

        if ((!ult_check_macro(&global->rdf_sess_cb->rds_strace,RD0021,&firstval, &secondval))
	    &&
	    (((numrows > record_count) && ((numrows - record_count) > (0.1 * record_count)))
	     ||
             ((record_count > numrows) && ((record_count - numrows) > (0.1 * record_count)))
	     ||
             ((numpages > page_count) && ((numpages - page_count) > (0.1 * page_count)))
	     ||
             ((page_count > numpages) && ((page_count - numpages) > (0.1 * page_count)))
	     ||
             ((overpages > over_count) && ((overpages - over_count) > (0.1 * over_count)))
	     ||
             ((over_count > overpages) && ((over_count - overpages) > (0.1 * over_count)))
	    )
	   )
	{
	    status = rdd_tableupd(global, obj_id_p, numrows, numpages, overpages);
	    
	    if (DB_FAILURE_MACRO(status))
		return(status);
	}

	/* Use the row and page counts from ldb's iitable anyway. */

	tbl_p->tbl_record_count = (i4)numrows;
	tbl_p->tbl_page_count = (i4)numpages;

	/* Note that we compute dpage by subtract overflow from number of pages */

	if (numpages > overpages) 
	   tbl_p->tbl_dpage_count = numpages - overpages;
	else
	   tbl_p->tbl_dpage_count = 0;

    }

    return(status);
}

/*{
** Name: rdd_getcolno - retrieve attribute count of an objects from CDB.
**
** Description:
**      This routine generates query to retrieve attribute count of an object.
**
**	Note that if attributes are mapped then a consistency check is perform 
**	to ensure number of attributes in the iidd_columns is equal to number
**	of attributes in the iidd_ddb_ldb_columns.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**
** Outputs:
**	global				ptr to RDF state descriptor
**	    .rdf_trel
**		.tbl_attr_count		number of attributes
**	    .rdf_tobj			temporary object descriptor
**		.dd_o9_tab_info		ldb table info
**		    .dd_t7_col_cnt	number of attributes
**					
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_getcolno(	RDF_GLOBAL         *global,
		RDD_OBJ_ID	   *id_p)
{
    DB_STATUS		status;
    DD_OBJ_DESC         *obj_p = &global->rdf_tobj;
    DD_2LDB_TAB_INFO	*ldbtab_p = &global->rdf_tobj.dd_o9_tab_info;
    DMT_TBL_ENTRY       *tbl_p = &global->rdf_trel;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET		ddpkt;
    RQB_BIND            rq_bind[RDD_01_COL],	/* for 1 columns */
			*bind_p = rq_bind;
    i4			cnt;
    char		qrytxt[RDD_QRY_LENGTH];

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select count(*) from iidd_columns where
    **           table_name = 'obj_name' and
    **           table_owner = 'obj_own'
    **
    **	Note that size of count(*) is i2.
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 	STprintf(qrytxt,
	    "%s '%s' and table_owner = '%s';", 	    
	    "select count(*) from iidd_columns where table_name =",
	    id_p->tab_name, 
	    id_p->tab_owner);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = (i4) STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    /* local type */
    rq_bind[0].rqb_addr = (PTR)&cnt;
    rq_bind[0].rqb_length = sizeof(cnt);
    rq_bind[0].rqb_dt_id = DB_INT_TYPE;
    
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_01_COL;

    status = rdd_fetch(global);
    if (DB_FAILURE_MACRO(status))
	return(status);

    if (ddr_p->qer_d5_out_info.qed_o3_output_b)
    {
	ldbtab_p->dd_t7_col_cnt = (i4)cnt;

	/* flush */
	status = rdd_flush(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }
    else
	ldbtab_p->dd_t7_col_cnt = (i4)0;
	
    if (ldbtab_p->dd_t6_mapped_b)
    {   /* consistancy check number of attributes */

	ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

	/* set up query 
	**
	**	select count(*) from iiddb_ldb_columns where
	**           object_base = 'object_base' and
	**           object_index = 'object_index'
	*/
    
	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
		"%s %d and object_index = %d;", 	    
	        "select count(*) from iidd_ddb_ldb_columns where object_base =",
		obj_p->dd_o3_objid.db_tab_base, 
		obj_p->dd_o3_objid.db_tab_index);
    
	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = (i4)STlength((char *)qrytxt);
        ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
	ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

	status = rdd_query(global);	    /* send query to qef */
	if (DB_FAILURE_MACRO(status))
	    return (status); 

	/* local type */
	rq_bind[0].rqb_addr = (PTR)&cnt;
	rq_bind[0].rqb_length = sizeof(cnt);
	rq_bind[0].rqb_dt_id = DB_INT_TYPE;
    
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_01_COL;

	status = rdd_fetch(global);

	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	{
	    tbl_p->tbl_attr_count = (i4)cnt;

	    /* flush */
	    status = rdd_flush(global);
	    if (DB_FAILURE_MACRO(status)) 
		return(status);
	}
	else
	    tbl_p->tbl_attr_count = (i4)0;

    }
    else
    {
	tbl_p->tbl_attr_count = ldbtab_p->dd_t7_col_cnt;
    }

    if ((ldbtab_p->dd_t6_mapped_b)
	 && 
        (tbl_p->tbl_attr_count != ldbtab_p->dd_t7_col_cnt)
       )
    {/* attribute number inconsistant */
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0256_DD_COLMISMATCH); 
    }
    
    return(status);
}

/*{
** Name: rdd_localstats	- check whether statistics is available.
**
** Description:
**      This routine send query to ldb to check whether local table has
**      statistics or not.
**
**	Mostly, this is for RDR_REFRESH request from PSF for register
**	with refresh query.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_localstats(	RDF_GLOBAL *global )
{
    DB_STATUS		status;
    DD_2LDB_TAB_INFO	*ldbtab_p = &global->rdf_tobj.dd_o9_tab_info;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    RDD_OBJ_ID		table_id,
			*id_p = &table_id;
    DD_1LDB_INFO	ldb_info;
    DD_PACKET		ddpkt;
    RQB_BIND            rq_bind[RDD_01_COL],	/* for 1 columns */
			*bind_p = rq_bind;
    char		qrytxt[RDD_QRY_LENGTH];

    rdd_rtrim((PTR)ldbtab_p->dd_t1_tab_name,
	      (PTR)ldbtab_p->dd_t2_tab_owner,
	       id_p);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select count(*) from iistats where
    **           table_name = 'table_name' and
    **           table_owner = 'table_owner'
    **
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	     "%s '%s' and table_owner = '%s';", 	    
             "select count(*) from iistats where table_name =",
	     id_p->tab_name, 
	     id_p->tab_owner);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = (i4)STlength((char *)qrytxt);

    /* Assign ldb info */
    STRUCT_ASSIGN_MACRO(*ldbtab_p->dd_t9_ldb_p, ldb_info);
    ldb_info.dd_i1_ldb_desc.dd_l1_ingres_b = FALSE;
    ddr_p->qer_d2_ldb_info_p = &ldb_info;
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

    /* Send query to ldb for statistics */

    status = rdd_query(global);
    if (DB_FAILURE_MACRO(status))
	return (status); 

    {
	i4	    cnt;

	/* iistats tuple count */
	rq_bind[0].rqb_addr = (PTR)&cnt;
	rq_bind[0].rqb_length = sizeof(cnt);
	rq_bind[0].rqb_dt_id = DB_INT_TYPE;
    
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_01_COL;

	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	{
	    if (cnt > 0)
		global->rdf_local_info |= RDR_L4_STATS_EXIST;

	    status = rdd_flush(global);
	}
    }

    return(status);
}

/*{
** Name: rdd_idxcount	- retrieve index count.
**
** Description:
**      This routine generates query to retrieve index count of an object.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_idxcount(	RDF_GLOBAL         *global,
		RDD_OBJ_ID	   *id_p)
{
    DB_STATUS		status;
    DMT_TBL_ENTRY       *tbl_p = &global->rdf_trel;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET		ddpkt;
    RQB_BIND            rq_bind[RDD_01_COL],	/* for 1 columns */
			*bind_p = rq_bind;
    char		qrytxt[RDD_QRY_LENGTH];

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select count(*) from iidd_indexes where
    **           base_name = 'obj_name' and
    **           base_owner = 'obj_owner'
    **
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	     "%s '%s' and base_owner = '%s';", 	    
             "select count(*) from iidd_indexes where base_name =",
	     id_p->tab_name, 
	     id_p->tab_owner);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = (i4) STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    {
	i4	    cnt;

	/* local type */
	rq_bind[0].rqb_addr = (PTR)&cnt;
	rq_bind[0].rqb_length = sizeof(cnt);
	rq_bind[0].rqb_dt_id = DB_INT_TYPE;
    
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_01_COL;

	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	{
	    tbl_p->tbl_index_count = (i4)cnt;
	
	    status = rdd_flush(global);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	
	    if (cnt > 0)
		global->rdf_trel.tbl_status_mask |= DMT_IDXD;
	    else if (global->rdf_trel.tbl_status_mask & DMT_IDXD)
		 {  /* This is the case when iidd_physical_tables's table_indexes
		    ** shows index exist but no tuple was found in iidd_indexes */
		    status = E_DB_ERROR;
		    rdu_ierror(global, status, E_RD0266_NO_INDEXTUPLE);
		    return(status);
		 }
	}
	else
	    tbl_p->tbl_index_count = (i4)0;
	    
    }

    return(status);
}

/*{
** Name: rdd_ldbtab - retrieve ldb table info from CDB.
**
** Description:
**      This routine generates query to retrieve ldb table description
**	from iidd_ddb_tableinfo in CDB.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**
** Outputs:
**      global				    ptr to RDF state descriptor
**	    .rdf_trel			    temporary relation desc storage 
**	    .rdf_tobj			    temporary object descriptor
**		.dd_o9_tab_info		    ldb table info
**		    .dd_t1_tab_name	    local table name
**		    .dd_t2_tab_owner	    local table owner
**		    .dd_t3_tab_type	    local type
**		    .dd_t4_cre_date	    create date
**		    .dd_t5_alt_date	    alter date
**		    .dd_t6_mapped_b	    TRUE if columns were mappped,
**					    FALSE otherwise
**		    .dd_t7_col_cnt	    number of columns
**		    .dd_t9_ldb_p	    pointer to ldb desc
**			.dd_l1_ingres_b	    FALSE
**			.dd_l2_node_name    node name
**			.dd_l3_ldb_name	    ldb name
**			.dd_l4_dbms_name    dbms name
**			.dd_l5_ldb_id	    ldb id
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	13-oct-89 (teresa)
**	    add logic to skip getting info from LDB if this is a remove cmd.
**	    this fixes a SYSCOM bug where they cannot remove the link to an
**	    object if the LDB that the object exists in is destroyed.
**	09-jul-90 (teresa)
**	    changed "select *" to specify the name of each desired column.
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_ldbtab( RDF_GLOBAL     *global,
	    RDD_OBJ_ID	   *id_p)
{
    DB_STATUS		status;
    DD_2LDB_TAB_INFO	*ldbtab_p = &global->rdf_tobj.dd_o9_tab_info;
    DMT_TBL_ENTRY       *tbl_p = &global->rdf_trel;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET		ddpkt;
    RDC_LOCTABLE	tabinfo;            /* use as tmep area to
					    ** store retrieve tuple */
    RQB_BIND            rq_bind[RDD_11_COL],	/* for 11 columns */
			*bind_p = rq_bind;
    char		qrytxt[RDD_QRY_LENGTH];

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select * from iidd_ddb_tableinfo
    **	         where object_base  = 'object_base' and
    **	               object_index = 'object_index'
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	    "%s %d and object_index = %d;", 	    
	    "select object_base, object_index, local_type, table_name,\
	    table_owner, table_date, table_alter, table_relstamp1,\
	    table_relstamp2, columns_mapped, ldb_id \
	    from iidd_ddb_tableinfo where object_base =",
	    global->rdf_tobj.dd_o3_objid.db_tab_base,
	    global->rdf_tobj.dd_o3_objid.db_tab_index);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4) STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    /* object base */
    rq_bind[0].rqb_addr = (PTR)&tabinfo.object_base;
    rq_bind[0].rqb_length = sizeof(i4);
    rq_bind[0].rqb_dt_id = DB_INT_TYPE;
    /* object index */
    rq_bind[1].rqb_addr = (PTR)&tabinfo.object_index;
    rq_bind[1].rqb_length = sizeof(i4);
    rq_bind[1].rqb_dt_id = DB_INT_TYPE;
    /* local type */
    rq_bind[2].rqb_addr = (PTR)tabinfo.local_type;
    rq_bind[2].rqb_length = sizeof(RDD_8CHAR);
    rq_bind[2].rqb_dt_id = DB_CHA_TYPE;
    /* table name */
    rq_bind[3].rqb_addr = (PTR)tabinfo.table_name;
    rq_bind[3].rqb_length = sizeof(DD_NAME);
    rq_bind[3].rqb_dt_id = DB_CHA_TYPE;
    /* table owner */
    rq_bind[4].rqb_addr = (PTR)tabinfo.table_owner;
    rq_bind[4].rqb_length = sizeof(DD_NAME);
    rq_bind[4].rqb_dt_id = DB_CHA_TYPE;
    /* create date */
    rq_bind[5].rqb_addr = (PTR)tabinfo.table_date;
    rq_bind[5].rqb_length = sizeof(DD_DATE);
    rq_bind[5].rqb_dt_id = DB_CHA_TYPE;
    /* alter date */
    rq_bind[6].rqb_addr = (PTR)tabinfo.table_alter;
    rq_bind[6].rqb_length = sizeof(DD_DATE);
    rq_bind[6].rqb_dt_id = DB_CHA_TYPE;
    /* stamp1 */
    rq_bind[7].rqb_addr = (PTR)&tabinfo.stamp1;
    rq_bind[7].rqb_length = sizeof(tabinfo.stamp1);
    rq_bind[7].rqb_dt_id = DB_INT_TYPE;
    /* stamp2 */
    rq_bind[8].rqb_addr = (PTR)&tabinfo.stamp2;
    rq_bind[8].rqb_length = sizeof(tabinfo.stamp2);
    rq_bind[8].rqb_dt_id = DB_INT_TYPE;
    /* columns_mapped */
    rq_bind[9].rqb_addr = (PTR)tabinfo.columns_mapped;
    rq_bind[9].rqb_length = sizeof(RDD_8CHAR);
    rq_bind[9].rqb_dt_id = DB_CHA_TYPE;
    /* ldb_id */
    rq_bind[10].rqb_addr = (PTR)&tabinfo.ldb_id;
    rq_bind[10].rqb_length = sizeof(tabinfo.ldb_id);
    rq_bind[10].rqb_dt_id = DB_INT_TYPE;

    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_11_COL;

    status = rdd_fetch(global);
    if (DB_FAILURE_MACRO(status))
	return(status);

    if (ddr_p->qer_d5_out_info.qed_o3_output_b)
    {
	MEcopy((PTR)tabinfo.table_name, sizeof(DD_NAME),
	       (PTR)ldbtab_p->dd_t1_tab_name);
	MEcopy((PTR)tabinfo.table_owner, sizeof(DD_NAME),
	       (PTR)ldbtab_p->dd_t2_tab_owner);

	rdd_cvobjtype(tabinfo.local_type, &ldbtab_p->dd_t3_tab_type);

	MEcopy((PTR)tabinfo.table_date, sizeof(DD_DATE),
	       (PTR)ldbtab_p->dd_t4_cre_date);
	MEcopy((PTR)tabinfo.table_alter, sizeof(DD_DATE),
	       (PTR)ldbtab_p->dd_t5_alt_date);

	(tabinfo.columns_mapped[0] == 'Y') ? (ldbtab_p->dd_t6_mapped_b = TRUE)
					   : (ldbtab_p->dd_t6_mapped_b = FALSE);			   
	    
	/* table modification time stamp */
	tbl_p->tbl_date_modified.db_tab_high_time = tabinfo.stamp1; 
	tbl_p->tbl_date_modified.db_tab_low_time = tabinfo.stamp2; 

	/* flush */
	status = rdd_flush(global);
    }
    else
    { 
	/* This is the case when local table information was not found in
	** iidd_ddb_tableinfo catalog.  However, if RDR_CHECK_EXIST was set,
	** we need not to flag an error here. */

	if (global->rdfcb->rdf_rb.rdr_types_mask & RDR_CHECK_EXIST)
	{
	    ldbtab_p->dd_t3_tab_type = DD_0OBJ_NONE;
	    return(E_DB_OK);
	}
	else
	{	    
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0254_DD_NOTABLE);
        } 
    }

    if (DB_FAILURE_MACRO(status))
	return(status);

    /* Use ldb_id to search for LDB desc in the ldb array,
    ** if not found retrieve from CDB and cache it */

    /* if we are doing a "drop link" or "remove" command, then we do not want
    ** want to go to the LDB for info
    */
    if  (global->rdfcb->rdf_rb.rdr_types_mask & RDR_REMOVE)
	return (status);

    return(rdd_ldbsearch(global, &ldbtab_p->dd_t9_ldb_p,
                           global->rdfcb->rdf_rb.rdr_unique_dbid, tabinfo.ldb_id));

}

/*{
** Name: rdd_objinfo	- retrieve object information from CDB.
**
** Description:
**      This routine generates query to retrieve object description
**	from iidd_ddb_objects catalog in CDB.
**
**	Note that this routine is a simplified version of rdd_relinfo.
**	In rdd_relinfo, a join query is generated for object and physical
**	information which may not desire at create/drop link time.  
**	At create/drop link time, a join query sometimes won't be able
**	to detect the exsitence of a dangling link.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**	    .rdf_tobj			temp object descriptor
**
** Outputs:
**      global                          ptr to RDF state descriptor
**	    .rdf_trel			temp DMT_TBL_ENTRY
**		.tbl_name		object name
**		.tbl_owner		object owner
**		.tbl_id			object id
**		.tbl_qryid		query id if view
**	    .rdf_tobj			temp object descriptor
**		.dd_o1_objname		object name
**		.dd_o2_objowner		object owner
**		.dd_o3_objid		object id
**		.dd_o4_qryid		query id if view
**		.dd_o5_cre_date		create date
**		.dd_o6_objtype		object type
**		.dd_o7_alt_date		alter date
**		.dd_o8_sysobj_b		TRUE if system object
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	27-feb-89 (mings)
**	   reinstall for RDR_CHECK_EXIST
**	09-jul-90 (teresa)
**	   changed "select *" to specify the name of each desired column.
**	16-jul-92 (teresa)
**	    prototypes
**	16-nov-92 (teresa)
**	    Fix system_object test from 'S' to 'Y' since qef populates catalog
**	    with 'Y' or 'N' for this column.
*/
DB_STATUS
rdd_objinfo(	RDF_GLOBAL         *global,
		RDD_OBJ_ID	   *id_p)
{
    DB_STATUS		status = E_DB_OK;
    DD_OBJ_DESC		*obj_p = &global->rdf_tobj;
    DMT_TBL_ENTRY	*tbl_p = &global->rdf_trel;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET           ddpkt;
    RQB_BIND            rq_bind[RDD_12_COL],	/* for 12 columns */
			*bind_p = rq_bind;
    char		qrytxt[RDD_QRY_LENGTH];

    /* Initialization */
    rdd_tab_init(tbl_p);
    obj_p->dd_o9_tab_info.dd_t6_mapped_b = FALSE;
    obj_p->dd_o9_tab_info.dd_t7_col_cnt = 0;
    obj_p->dd_o9_tab_info.dd_t8_cols_pp = (DD_COLUMN_DESC **)NULL;
    obj_p->dd_o9_tab_info.dd_t9_ldb_p = (DD_1LDB_INFO *)NULL;
    
    if (global->rdfcb->rdf_rb.rdr_types_mask & RDR_BY_NAME)
    {
	/* Setup name and owner */
    
	 rdd_rtrim((PTR)global->rdfcb->rdf_rb.rdr_name.rdr_tabname.db_tab_name,
		   (PTR)global->rdfcb->rdf_rb.rdr_owner.db_own_name,
		   id_p);

	/* set up query 
	**
	**	select * from iidd_ddb_objects where
	**	         object_name  = 'obj_name' and
	**	         object_owner = 'obj_own'
	*/
	STprintf(qrytxt,
	    "%s '%s' and object_owner = '%s';", 	    
	    "select object_name, object_owner, object_base, object_index,\
	    qid1, qid2, create_date, object_type, alter_date, system_object,\
	    to_expire, expire_date from iidd_ddb_objects where object_name =",
            id_p->tab_name,
	    id_p->tab_owner);
    }
    else
    {
	/* set up query 
	**
	**	select * from iidd_ddb_objects where
	**	         object_base  = object_base and
	**	         object_index = object_index
	*/

	STprintf(qrytxt,
	    "%s %d and object_index = %d;", 	    
	    "select object_name, object_owner, object_base, object_index,\
	    qid1, qid2, create_date, object_type, alter_date, system_object,\
	    to_expire, expire_date from iidd_ddb_objects where object_base =",
            global->rdfcb->rdf_rb.rdr_tabid.db_tab_base,
	    global->rdfcb->rdf_rb.rdr_tabid.db_tab_index);
    }

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = qrytxt;
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

    status = rdd_query(global);	    /* send query to qef */
    if (status)
	return (status); 

    {
	RDC_OBJECTS		iiddb_objects;

	/* object name */
	rq_bind[0].rqb_addr = (PTR)iiddb_objects.object_name;
	rq_bind[0].rqb_length = sizeof(DD_NAME);
	rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
	/* object owner */
	rq_bind[1].rqb_addr = (PTR)iiddb_objects.object_owner;
	rq_bind[1].rqb_length = sizeof(DD_NAME);
	rq_bind[1].rqb_dt_id = DB_CHA_TYPE;
	/* table id base */
	rq_bind[2].rqb_addr = (PTR)&iiddb_objects.object_base;
	rq_bind[2].rqb_length = sizeof(i4);
	rq_bind[2].rqb_dt_id = DB_INT_TYPE;
        /* table id index */
	rq_bind[3].rqb_addr = (PTR)&iiddb_objects.object_index;
	rq_bind[3].rqb_length = sizeof(i4);
	rq_bind[3].rqb_dt_id = DB_INT_TYPE;
	/* query id high time */
	rq_bind[4].rqb_addr = (PTR)&iiddb_objects.qid1;
	rq_bind[4].rqb_length = sizeof(i4);
	rq_bind[4].rqb_dt_id = DB_INT_TYPE;
        /* query id low time */
	rq_bind[5].rqb_addr = (PTR)&iiddb_objects.qid2;
	rq_bind[5].rqb_length = sizeof(i4);
	rq_bind[5].rqb_dt_id = DB_INT_TYPE;
        /* create date */
	rq_bind[6].rqb_addr = (PTR)iiddb_objects.create_date;
	rq_bind[6].rqb_length = sizeof(DD_DATE);
	rq_bind[6].rqb_dt_id = DB_CHA_TYPE;
        /* object type */
	rq_bind[7].rqb_addr = (PTR)iiddb_objects.object_type;
	rq_bind[7].rqb_length = sizeof(RDD_8CHAR);
	rq_bind[7].rqb_dt_id = DB_CHA_TYPE;
        /* alter date */
	rq_bind[8].rqb_addr = (PTR)iiddb_objects.alter_date;
	rq_bind[8].rqb_length = sizeof(DD_DATE);
	rq_bind[8].rqb_dt_id = DB_CHA_TYPE;
        /* system object */
	rq_bind[9].rqb_addr = (PTR)iiddb_objects.system_object;
	rq_bind[9].rqb_length = sizeof(RDD_8CHAR);
	rq_bind[9].rqb_dt_id = DB_CHA_TYPE;
        /* to_expire */
	rq_bind[10].rqb_addr = (PTR)iiddb_objects.to_expire;
	rq_bind[10].rqb_length = sizeof(RDD_8CHAR);
	rq_bind[10].rqb_dt_id = DB_CHA_TYPE;
        /* expire_date */
	rq_bind[11].rqb_addr = (PTR)iiddb_objects.expire_date;
	rq_bind[11].rqb_length = sizeof(DD_DATE);
	rq_bind[11].rqb_dt_id = DB_CHA_TYPE;
	
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_12_COL;

	status = rdd_fetch(global);
	if (status)
	    return(status);

        if (!ddr_p->qer_d5_out_info.qed_o3_output_b)
        {  /* No object tuple was found in system catalog. 
	   ** At create link time parser use this error code
	   ** to check the link is not exist.*/
	    SETDBERR(&global->rdfcb->rdf_error, 0, E_RD0002_UNKNOWN_TBL);
	    return (E_DB_ERROR); 
	}
	
	/* Move tuple into object descriptor */
	if (!(global->rdfcb->rdf_rb.rdr_types_mask & RDR_BY_NAME))
	{
	    /* Setup name and owner */
    
	    rdd_rtrim((PTR)iiddb_objects.object_name,
		      (PTR)iiddb_objects.object_owner,
		       id_p);
	}

	MEcopy((PTR)iiddb_objects.object_name,
	       sizeof(DD_NAME),
	       (PTR)obj_p->dd_o1_objname);
	MEcopy((PTR)iiddb_objects.object_owner,
	       sizeof(DD_NAME),
	       (PTR)obj_p->dd_o2_objowner);
	obj_p->dd_o3_objid.db_tab_base = iiddb_objects.object_base;
	obj_p->dd_o3_objid.db_tab_index = iiddb_objects.object_index;
	obj_p->dd_o4_qryid.db_qry_high_time = iiddb_objects.qid1;
	obj_p->dd_o4_qryid.db_qry_low_time = iiddb_objects.qid2;
	MEcopy((PTR)iiddb_objects.create_date,
	       sizeof(DD_DATE),
	       (PTR)obj_p->dd_o5_cre_date);

        /* convert object type */
        rdd_cvobjtype(iiddb_objects.object_type, &obj_p->dd_o6_objtype);

	MEcopy((PTR)iiddb_objects.alter_date,
	       sizeof(DD_DATE),
	       (PTR)obj_p->dd_o7_alt_date);

        (iiddb_objects.system_object[0] == 'Y') ? (obj_p->dd_o8_sysobj_b = TRUE)
					        : (obj_p->dd_o8_sysobj_b = FALSE);			   

	obj_p->dd_o9_tab_info.dd_t6_mapped_b = FALSE;  /* init column mapped 
						       ** indicator to FALSE */

	/* Fill object name, owner, and id in temp DMT_TBL_ENTRY */	    
	MEcopy((PTR)iiddb_objects.object_name,
	       sizeof(DD_NAME), tbl_p->tbl_name.db_tab_name);
	MEcopy((PTR)iiddb_objects.object_owner,
	       sizeof(DD_NAME), tbl_p->tbl_owner.db_own_name);
	STRUCT_ASSIGN_MACRO(obj_p->dd_o3_objid, tbl_p->tbl_id);
	STRUCT_ASSIGN_MACRO(obj_p->dd_o4_qryid, tbl_p->tbl_qryid);
	if (obj_p->dd_o8_sysobj_b)
	    tbl_p->tbl_status_mask |= DMT_CATALOG;
	if (obj_p->dd_o6_objtype == DD_3OBJ_VIEW)
	    tbl_p->tbl_status_mask |= DMT_VIEW;

    }

    /* flush */
    status = rdd_flush(global);
    return(status);
}

/*{
** Name: rdd_relinfo	- retrieve object information from CDB.
**
** Description:
**      This routine generates query to retrieve object description
**	from iidd_ddb_objects in CDB.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**	    .rdf_tobj			temp object descriptor
**
** Outputs:
**      global                          ptr to RDF state descriptor
**	    .rdf_trel			temp DMT_TBL_ENTRY
**		.tbl_name		object name
**		.tbl_owner		object owner
**		.tbl_id			object id
**		.tbl_qryid		query id if view
**		.tbl_location		first location name
**		.tbl_width		table record width
**		.tbl_tbl_status_mask    table status
**		.tbl_date_modified	table last modified date
**		.tbl_i_fill_factor	fill factor for index pages
**		.tbl_d_fill_factor	fill factor for data pages
**		.tbl_l_fill_factor	fill factor for BETREE leaf pages
**		.tbl_min_page		minimum pages
**		.tbl_max_page		maximum pages
**		.tbl_storage_type	table storage structure
**		.tbl_tbl_status_mask    table status
**		.tbl_record_count	table estimated record count
**		.tbl_page_count		estimated page count in table
**		.tbl_dpage_count	estimated data page count in table
**	    .rdf_tobj			temp object descriptor
**		.dd_o1_objname		object name
**		.dd_o2_objowner		object owner
**		.dd_o3_objid		object id
**		.dd_o4_qryid		query id if view
**		.dd_o5_cre_date		create date
**		.dd_o6_objtype		object type
**		.dd_o7_alt_date		alter date
**		.dd_o8_sysobj_b		TRUE if system object
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	05-jul-90 (teresa)
**	    ifdeffed out the section that sets DMT_INTEGRITIES and DMT_PERMITS
**	    since star cannot handle these.  The ifdef should be removed when
**	    star stars supporting integrities or permits.
**	09-jul-90 (teresa)
**	    changed "select * " to specify each desired column name.
**	14-jun-91 (teresa)
**	    fix bug 34864 by checking to see if LDB supports tids.
**	16-jul-92 (teresa)
**	    prototypes
**	16-nov-92 (teresa)
**	    Fix system_object test from 'S' to 'Y' since qef populates catalog
**	    with 'Y' or 'N' for this column.
**	15-mar-94 (teresa)
**	    Fix bug 60609 -- assure that the local type is 'P' if we are doing
**	    a regproc.
**	06-mar-96 (nanpr01)
**	    Added pagesize column in standard catalogues of the star
**	    for variable page size project.
**	16-jul-96 (ramra01)
**	    Added relversion and reltotwidth in standard catalog for 
**	    distributed (star) as part of Alter table project.
**	24-feb-04 (hayke02)
**	    Add hidata (iitables.is_compressed == 'H', iirelation.relcomptype
**	    == 7), as a valid compression type. This prevents the internal
**	    error E_RD0276 and the external errors E_QE001F and E_QE0519.
**	    This change fixes problem INGSTR 65, bug 111813.
**	23-nov-2006 (dougi)
**	    Reinstate code to set DMT_BASE_VIEW.
*/
DB_STATUS
rdd_relinfo(	RDF_GLOBAL         *global,
		RDD_OBJ_ID	   *id_p)
{
    DB_STATUS		status = E_DB_OK;
    DD_OBJ_DESC		*obj_p = &global->rdf_tobj;
    DMT_TBL_ENTRY	*tbl_p = &global->rdf_trel;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET           ddpkt;
    RQB_BIND            rq_bind[RDD_53_COL],	/* for 53 columns */
			*bind_p = rq_bind;
    char		qrytxt[RDD_QRY_LENGTH];
    i4			ldb_id;

    /* Initialization */
    rdd_tab_init (tbl_p);
    obj_p->dd_o9_tab_info.dd_t8_cols_pp = (DD_COLUMN_DESC **)NULL;
    obj_p->dd_o9_tab_info.dd_t9_ldb_p = (DD_1LDB_INFO *)NULL;

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    if (global->rdfcb->rdf_rb.rdr_types_mask & RDR_BY_NAME)
    {
	/* Setup name and owner */

	rdd_rtrim((PTR)global->rdfcb->rdf_rb.rdr_name.rdr_tabname.db_tab_name,
		  (PTR)global->rdfcb->rdf_rb.rdr_owner.db_own_name,
		   id_p);

	if (global->rdfcb->rdf_rb.rdr_instr & RDF_REGPROC)
	{
	    
	    /* set up query:
	    **
	    **	select o.object_name, o.object_owner, o.object_base, 
	    **	       o.object_index, o.qid1, o.qid2, o.create_date, 
	    **         o.object_type, o.alter_date, o.system_object, 
	    **	       o.to_expire, o.expire_date
	    **	from iidd_ddb_objects o, iidd_ddb_tableinfo t
	    **	where
	    **	         o.object_name  = 'obj_name'  and
	    **	         o.object_owner = 'obj_own'  and
	    **		 t.object_base = o.object_base and t.local_type = 'P'
	    **	(fixes bug 60609 where we could execute a procedure if there 
	    **   was a table/view registered by the same name)
	    */
	    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
		"%s%s%s%s%s '%s' and object_owner = '%s';",
		"select o.object_name, o.object_owner, o.object_base, ",
		"o.object_index, o.qid1, o.qid2, o.create_date,o.object_type, ",
		"o.alter_date, o.system_object, o.to_expire, o.expire_date ",
		"from iidd_ddb_objects o, iidd_ddb_tableinfo t where ",
	    "t.object_base=o.object_base and t.local_type='P' and object_name=",
		id_p->tab_name,
		id_p->tab_owner);
	    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_12_COL;
	}
	else
	{
	    /* set up query 
	    **
	    **	select a.*, b.* from iidd_ddb_objects a, iidd_tables where
	    **	         a.object_name  = 'obj_name'  and
	    **	         a.object_owner = 'obj_own'   and
	    **		 b.table_name = a.object_name and
	    **		 b.table_owner = a.table_owner
	    */
	    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
		"%s '%s' and a.object_owner = '%s' and b.table_name = a.object_name and b.table_owner = a.object_owner;", 	    
		"select a.*, b.* from iidd_ddb_objects a,\
		 iidd_tables b where a.object_name =",
		id_p->tab_name,
		id_p->tab_owner);
	}
#if 0
  This is what the text should be, but it overflows the 900 char buffer, so for
  now leave as select a.*, b.*
	    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	    "%s '%s' and a.object_owner = '%s' and b.table_name = a.object_name and b.table_owner = a.object_owner;", 	    
	    "select a.object_name, a.object_owner,a.object_base,a.object_index,\
	     a.qid1, a.qid2, a.create_date, a.object_type, a.alter_date, a.system_object,\
	     a.to_expire, a.expire_date, b.table_name, b.table_owner,\
	     b.create_date,b.alter_date, b.table_type, b.table_subtype, \
	     b.table_version, b.system_use, b.table_stats, b.table_indexes, \
	     b.is_readonly, b.num_rows, b.storage_structure, b.is_compressed,\
	     b.duplicate_rows, b.unique_rule, b.number_pages, b.overflow_pages,\
	     b.row_width, b.expire_date, b.modify_date, b.location_name, \
	     b.table_integrities, b.table_permits, b.all_to_all, b.ret_to_all,\
	     b.is_journalled, b.view_base, b.multi_locations, b.table_ifillpct,\
	     b.table_dfillpct, b.table_lfillpct, b.table_minpages, \
	     b.table_maxpages, b.table_relstamp1, b.table_relstamp2,\
	     b.table_reltid, b.table_reltidx, b.table_pagesize from \
	     iidd_ddb_objects a, iidd_tables b where a.object_name =",
            id_p->tab_name,
	    id_p->tab_owner);
#endif
    }
    else
    {
	/* set up query 
	**
	**	select a.*, b.* from iidd_ddb_objects a, iidd_tables b where
	**	         a.object_base  = object_base and
	**	         a.object_index = object_index and
	**		 b.table_name = a.object_name and
	**               b.table_owner = a.object_owner
	*/

	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	    "%s %d and a.object_index = %d and b.table_name = a.object_name and b.table_owner = a.object_owner;", 	    
	    "select a.*, b.* from iidd_ddb_objects a, iidd_tables b where a.object_base =",
            global->rdfcb->rdf_rb.rdr_tabid.db_tab_base,
	    global->rdfcb->rdf_rb.rdr_tabid.db_tab_index);
	    
#if 0
  This is what the text should be, but it overflows the 900 char buffer, so for
  now leave as select a.*, b.*
	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	    "%s %d and a.object_index = %d and b.table_name = a.object_name and b.table_owner = a.object_owner;", 	    
	    "select a.object_name, a.object_owner,a.object_base,a.object_index,\
	     a.qid1, a.qid2, a.create_date, a.object_type, a.alter_date, a.system_object,\
	     a.to_expire, a.expire_date, b.table_name, b.table_owner,\
	     b.create_date,b.alter_date, b.table_type, b.table_subtype, \
	     b.table_version, b.system_use, b.table_stats, b.table_indexes, \
	     b.is_readonly, b.num_rows, b.storage_structure, b.is_compressed,\
	     b.duplicate_rows, b.unique_rule, b.number_pages, b.overflow_pages,\
	     b.row_width, b.expire_date, b.modify_date, b.location_name, \
	     b.table_integrities, b.table_permits, b.all_to_all, b.ret_to_all,\
	     b.is_journalled, b.view_base, b.multi_locations, b.table_ifillpct,\
	     b.table_dfillpct, b.table_lfillpct, b.table_minpages, \
	     b.table_maxpages, b.table_relstamp1, b.table_relstamp2,\
	     b.table_reltid, b.table_reltidx, b.table_pagesize from \
	     iidd_ddb_objects a, iidd_tables b where a.object_base =",
            global->rdfcb->rdf_rb.rdr_tabid.db_tab_base,
	    global->rdfcb->rdf_rb.rdr_tabid.db_tab_index);
#endif
    }

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    {
	RDC_OBJECTS		iiddb_objects;
	RDC_TABLES		iidd_tables;

	/* object name */
	rq_bind[0].rqb_addr = (PTR)iiddb_objects.object_name;
	rq_bind[0].rqb_length = sizeof(DD_NAME);
	rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
	/* object owner */
	rq_bind[1].rqb_addr = (PTR)iiddb_objects.object_owner;
	rq_bind[1].rqb_length = sizeof(DD_NAME);
	rq_bind[1].rqb_dt_id = DB_CHA_TYPE;
	/* table id base */
	rq_bind[2].rqb_addr = (PTR)&iiddb_objects.object_base;
	rq_bind[2].rqb_length = sizeof(i4);
	rq_bind[2].rqb_dt_id = DB_INT_TYPE;
        /* table id index */
	rq_bind[3].rqb_addr = (PTR)&iiddb_objects.object_index;
	rq_bind[3].rqb_length = sizeof(i4);
	rq_bind[3].rqb_dt_id = DB_INT_TYPE;
	/* query id high time */
	rq_bind[4].rqb_addr = (PTR)&iiddb_objects.qid1;
	rq_bind[4].rqb_length = sizeof(i4);
	rq_bind[4].rqb_dt_id = DB_INT_TYPE;
        /* query id low time */
	rq_bind[5].rqb_addr = (PTR)&iiddb_objects.qid2;
	rq_bind[5].rqb_length = sizeof(i4);
	rq_bind[5].rqb_dt_id = DB_INT_TYPE;
        /* create date */
	rq_bind[6].rqb_addr = (PTR)iiddb_objects.create_date;
	rq_bind[6].rqb_length = sizeof(DD_DATE);
	rq_bind[6].rqb_dt_id = DB_CHA_TYPE;
        /* object type */
	rq_bind[7].rqb_addr = (PTR)iiddb_objects.object_type;
	rq_bind[7].rqb_length = sizeof(RDD_8CHAR);
	rq_bind[7].rqb_dt_id = DB_CHA_TYPE;
        /* alter date */
	rq_bind[8].rqb_addr = (PTR)iiddb_objects.alter_date;
	rq_bind[8].rqb_length = sizeof(DD_DATE);
	rq_bind[8].rqb_dt_id = DB_CHA_TYPE;
        /* system object */
	rq_bind[9].rqb_addr = (PTR)iiddb_objects.system_object;
	rq_bind[9].rqb_length = sizeof(RDD_8CHAR);
	rq_bind[9].rqb_dt_id = DB_CHA_TYPE;
        /* to_expire */
	rq_bind[10].rqb_addr = (PTR)iiddb_objects.to_expire;
	rq_bind[10].rqb_length = sizeof(RDD_8CHAR);
	rq_bind[10].rqb_dt_id = DB_CHA_TYPE;
        /* expire_date */
	rq_bind[11].rqb_addr = (PTR)iiddb_objects.expire_date;
	rq_bind[11].rqb_length = sizeof(DD_DATE);
	rq_bind[11].rqb_dt_id = DB_CHA_TYPE;
	
	if (! (global->rdfcb->rdf_rb.rdr_instr & RDF_REGPROC))
	{
	    /* iidd_tables info */

	    /* table_name */
	    rq_bind[12].rqb_addr = (PTR)iidd_tables.table_name;
	    rq_bind[12].rqb_length = sizeof(DD_NAME);
	    rq_bind[12].rqb_dt_id = DB_CHA_TYPE;
	    /* table_owner */
	    rq_bind[13].rqb_addr = (PTR)iidd_tables.table_owner;
	    rq_bind[13].rqb_length = sizeof(DD_NAME);
	    rq_bind[13].rqb_dt_id = DB_CHA_TYPE;
	    /* create_date */
	    rq_bind[14].rqb_addr = (PTR)iidd_tables.create_date;
	    rq_bind[14].rqb_length = sizeof(DD_DATE);
	    rq_bind[14].rqb_dt_id = DB_CHA_TYPE;
	    /* alter_date */
	    rq_bind[15].rqb_addr = (PTR)iidd_tables.alter_date;
	    rq_bind[15].rqb_length = sizeof(DD_DATE);
	    rq_bind[15].rqb_dt_id = DB_CHA_TYPE;
	    /* table_type */
	    rq_bind[16].rqb_addr = (PTR)iidd_tables.table_type;
	    rq_bind[16].rqb_length = sizeof(RDD_8CHAR);
	    rq_bind[16].rqb_dt_id = DB_CHA_TYPE;
	    /* table_subtype */
	    rq_bind[17].rqb_addr = (PTR)iidd_tables.table_subtype;
	    rq_bind[17].rqb_length = sizeof(RDD_8CHAR);
	    rq_bind[17].rqb_dt_id = DB_CHA_TYPE;
	    /* table_version */
	    rq_bind[18].rqb_addr = (PTR)iidd_tables.table_version;
	    rq_bind[18].rqb_length = sizeof(RDD_8CHAR);
	    rq_bind[18].rqb_dt_id = DB_CHA_TYPE;
	    /* system_use */
	    rq_bind[19].rqb_addr = (PTR)iidd_tables.system_use;
	    rq_bind[19].rqb_length = sizeof(RDD_8CHAR);
	    rq_bind[19].rqb_dt_id = DB_CHA_TYPE;
	    
	    /* stats */
	    rq_bind[20].rqb_addr = (PTR)iidd_tables.stats;
	    rq_bind[20].rqb_length = sizeof(iidd_tables.stats);
	    rq_bind[20].rqb_dt_id = DB_CHA_TYPE;
	    /* indexes */
	    rq_bind[21].rqb_addr = (PTR)iidd_tables.indexes;
	    rq_bind[21].rqb_length = sizeof(iidd_tables.indexes);
	    rq_bind[21].rqb_dt_id = DB_CHA_TYPE;
	    /* rdonly */
	    rq_bind[22].rqb_addr = (PTR)iidd_tables.rdonly;
	    rq_bind[22].rqb_length = sizeof(iidd_tables.rdonly);
	    rq_bind[22].rqb_dt_id = DB_CHA_TYPE;
	    /* num_rows */
	    rq_bind[23].rqb_addr = (PTR)&iidd_tables.num_rows;
	    rq_bind[23].rqb_length = sizeof(iidd_tables.num_rows);
	    rq_bind[23].rqb_dt_id = DB_INT_TYPE;
	    /* storage */
	    rq_bind[24].rqb_addr = (PTR)iidd_tables.storage;
	    rq_bind[24].rqb_length = sizeof(iidd_tables.storage);
	    rq_bind[24].rqb_dt_id = DB_CHA_TYPE;
	    /* compressed */
	    rq_bind[25].rqb_addr = (PTR)iidd_tables.compressed;
	    rq_bind[25].rqb_length = sizeof(iidd_tables.compressed);
	    rq_bind[25].rqb_dt_id = DB_CHA_TYPE;
	    /* duplicate_rows */
	    rq_bind[26].rqb_addr = (PTR)iidd_tables.duplicate_rows;
	    rq_bind[26].rqb_length = sizeof(iidd_tables.duplicate_rows);
	    rq_bind[26].rqb_dt_id = DB_CHA_TYPE;
	    /* uniquerule */
	    rq_bind[27].rqb_addr = (PTR)iidd_tables.uniquerule;
	    rq_bind[27].rqb_length = sizeof(iidd_tables.uniquerule);
	    rq_bind[27].rqb_dt_id = DB_CHA_TYPE;
	    /* number_pages */
	    rq_bind[28].rqb_addr = (PTR)&iidd_tables.number_pages;
	    rq_bind[28].rqb_length = sizeof(iidd_tables.number_pages);
	    rq_bind[28].rqb_dt_id = DB_INT_TYPE;
	    /* overflow */
	    rq_bind[29].rqb_addr = (PTR)&iidd_tables.overflow;
	    rq_bind[29].rqb_length = sizeof(iidd_tables.overflow);
	    rq_bind[29].rqb_dt_id = DB_INT_TYPE;

	    /* row_width */
	    rq_bind[30].rqb_addr = (PTR)&iidd_tables.row_width;
	    rq_bind[30].rqb_length = sizeof(iidd_tables.row_width);
	    rq_bind[30].rqb_dt_id = DB_INT_TYPE;
	    /* expire_date */
	    rq_bind[31].rqb_addr = (PTR)&iidd_tables.expire_date;
	    rq_bind[31].rqb_length = sizeof(iidd_tables.expire_date);
	    rq_bind[31].rqb_dt_id = DB_CHA_TYPE;
	    /* modify_date */
	    rq_bind[32].rqb_addr = (PTR)iidd_tables.modify_date;
	    rq_bind[32].rqb_length = sizeof(iidd_tables.modify_date);
	    rq_bind[32].rqb_dt_id = DB_CHA_TYPE;
	    /* location_name */
	    rq_bind[33].rqb_addr = (PTR)iidd_tables.location_name.db_loc_name;
	    rq_bind[33].rqb_length = sizeof(iidd_tables.location_name.db_loc_name);
	    rq_bind[33].rqb_dt_id = DB_CHA_TYPE;
	    /* integrities */
	    rq_bind[34].rqb_addr = (PTR)iidd_tables.integrities;
	    rq_bind[34].rqb_length = sizeof(iidd_tables.integrities);
	    rq_bind[34].rqb_dt_id = DB_CHA_TYPE;
	    /* permits */
	    rq_bind[35].rqb_addr = (PTR)iidd_tables.permits;
	    rq_bind[35].rqb_length = sizeof(iidd_tables.permits);
	    rq_bind[35].rqb_dt_id = DB_CHA_TYPE;
	    /* alltoall */
	    rq_bind[36].rqb_addr = (PTR)iidd_tables.alltoall;
	    rq_bind[36].rqb_length = sizeof(iidd_tables.alltoall);
	    rq_bind[36].rqb_dt_id = DB_CHA_TYPE;
	    /* rettoall */
	    rq_bind[37].rqb_addr = (PTR)iidd_tables.rettoall;
	    rq_bind[37].rqb_length = sizeof(iidd_tables.rettoall);
	    rq_bind[37].rqb_dt_id = DB_CHA_TYPE;
	    /* journal */
	    rq_bind[38].rqb_addr = (PTR)iidd_tables.journal;
	    rq_bind[38].rqb_length = sizeof(iidd_tables.journal);
	    rq_bind[38].rqb_dt_id = DB_CHA_TYPE;
	    /* view_base */
	    rq_bind[39].rqb_addr = (PTR)iidd_tables.view_base;
	    rq_bind[39].rqb_length = sizeof(iidd_tables.view_base);
	    rq_bind[39].rqb_dt_id = DB_CHA_TYPE;
	    /* multi_location */
	    rq_bind[40].rqb_addr = (PTR)iidd_tables.multi_location;
	    rq_bind[40].rqb_length = sizeof(iidd_tables.multi_location);
	    rq_bind[40].rqb_dt_id = DB_CHA_TYPE;
	    /* ifillpct */
	    rq_bind[41].rqb_addr = (PTR)&iidd_tables.ifillpct;
	    rq_bind[41].rqb_length = sizeof(iidd_tables.ifillpct);
	    rq_bind[41].rqb_dt_id = DB_INT_TYPE;
	    /* dfillpct */
	    rq_bind[42].rqb_addr = (PTR)&iidd_tables.dfillpct;
	    rq_bind[42].rqb_length = sizeof(iidd_tables.dfillpct);
	    rq_bind[42].rqb_dt_id = DB_INT_TYPE;
	    /* lfillpct */
	    rq_bind[43].rqb_addr = (PTR)&iidd_tables.lfillpct;
	    rq_bind[43].rqb_length = sizeof(iidd_tables.lfillpct);
	    rq_bind[43].rqb_dt_id = DB_INT_TYPE;
	    /* minpages */
	    rq_bind[44].rqb_addr = (PTR)&iidd_tables.minpages;
	    rq_bind[44].rqb_length = sizeof(iidd_tables.minpages);
	    rq_bind[44].rqb_dt_id = DB_INT_TYPE;
	    /* maxpages */
	    rq_bind[45].rqb_addr = (PTR)&iidd_tables.maxpages;
	    rq_bind[45].rqb_length = sizeof(iidd_tables.maxpages);
	    rq_bind[45].rqb_dt_id = DB_INT_TYPE;
	    /* table_relstamp1: high part of last create or modify time stamp */
	    rq_bind[46].rqb_addr = (PTR)&iidd_tables.stamp1;
	    rq_bind[46].rqb_length = sizeof(iidd_tables.stamp1);
	    rq_bind[46].rqb_dt_id = DB_CHA_TYPE;
	    /* table_relstamp2: low part of last create or modify time stamp */
	    rq_bind[47].rqb_addr = (PTR)&iidd_tables.stamp2;
	    rq_bind[47].rqb_length = sizeof(iidd_tables.stamp2);
	    rq_bind[47].rqb_dt_id = DB_CHA_TYPE;
	    /* reltid */
	    rq_bind[48].rqb_addr = (PTR)&iidd_tables.reltid;
	    rq_bind[48].rqb_length = sizeof(iidd_tables.reltid);
	    rq_bind[48].rqb_dt_id = DB_INT_TYPE;
	    /* reltidx */
	    rq_bind[49].rqb_addr = (PTR)&iidd_tables.reltidx;
	    rq_bind[49].rqb_length = sizeof(iidd_tables.reltidx);
	    rq_bind[49].rqb_dt_id = DB_INT_TYPE;
	    /* relpagesize */
	    rq_bind[50].rqb_addr = (PTR)&iidd_tables.table_pagesize;
	    rq_bind[50].rqb_length = sizeof(iidd_tables.table_pagesize);
	    rq_bind[50].rqb_dt_id = DB_INT_TYPE;
            /* relversion */
            rq_bind[51].rqb_addr = (PTR)&iidd_tables.table_relversion;
            rq_bind[51].rqb_length = sizeof(iidd_tables.table_relversion);
            rq_bind[51].rqb_dt_id = DB_INT_TYPE;
            /* reltotwid */
            rq_bind[52].rqb_addr = (PTR)&iidd_tables.table_reltotwid;
            rq_bind[52].rqb_length = sizeof(iidd_tables.table_reltotwid);
            rq_bind[52].rqb_dt_id = DB_INT_TYPE;

	    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_53_COL;
	}    
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;

	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

        if (!ddr_p->qer_d5_out_info.qed_o3_output_b)
        {  /* No object tuple was found in system catalog. 
	   ** At create link time parser use this error code
	   ** to check the link is not exist.*/
	    SETDBERR(&global->rdfcb->rdf_error, 0, E_RD0002_UNKNOWN_TBL);
	    return (E_DB_ERROR); 
	}

	/* generate iidd_tables info from iiddb_objects info for registered
	** procedures (star only) 
	*/
	if (global->rdfcb->rdf_rb.rdr_instr & RDF_REGPROC)
	{
	    MEfill(sizeof(RDC_TABLES), (u_char)' ', (PTR)&iidd_tables);

	    /* table_name */
	    MEcopy( (PTR)iiddb_objects.object_name,
		    sizeof(iiddb_objects.object_name),
		    (PTR)iidd_tables.table_name);
	    /* table_owner */
	    MEcopy( (PTR)iiddb_objects.object_owner,
		    sizeof(iiddb_objects.object_owner),
		    (PTR)iidd_tables.table_owner);
	    /* create_date */
	    MEcopy( (PTR)iiddb_objects.create_date,
		    sizeof(iiddb_objects.create_date),
		    (PTR)iidd_tables.create_date);
	    /* alter_date */
	    MEcopy( (PTR)iiddb_objects.alter_date,
		    sizeof(iiddb_objects.alter_date),
		    (PTR)iidd_tables.alter_date);
	    /* table_type */
	    iidd_tables.table_type[0] = 'P';
	    /* table_subtype */
	    iidd_tables.table_subtype[0] = 'N';
	    /* storage type */
	    iidd_tables.stats[0] = 'N';
	    /* statistics info */
	    STcopy(Iird_heap, iidd_tables.storage);
	}
	
	/* Do some consistency checks here. We are going check field
	** by field so lint won't complain. */

	if (iidd_tables.table_type[0] != 'T'
	    &&
	    iidd_tables.table_type[0] != 'V'
	    &&
	    iidd_tables.table_type[0] != 'P'
	    &&
	    iidd_tables.table_type[0] != 'I')
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0276_INCONSISTENT_CATINFO);
	    return(status);	
	}
	    
	if (iidd_tables.system_use[0] != 'S'
	    &&
	    iidd_tables.system_use[0] != 'U'
	    &&
	    iidd_tables.system_use[0] != ' ')
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0276_INCONSISTENT_CATINFO);
	    return(status);	
	}

	if (iidd_tables.compressed[0] != 'Y'
	    &&
	    iidd_tables.compressed[0] != 'H'
	    &&
	    iidd_tables.compressed[0] != 'N'
	    &&
	    iidd_tables.compressed[0] != ' ')
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0276_INCONSISTENT_CATINFO);
	    return(status);	
	}

	if (iidd_tables.duplicate_rows[0] != 'D'
	    &&
	    iidd_tables.duplicate_rows[0] != 'U'
	    &&
	    iidd_tables.duplicate_rows[0] != ' ')
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0276_INCONSISTENT_CATINFO);
	    return(status);	
	}

	if (iidd_tables.uniquerule[0] != 'U'
	    &&
	    iidd_tables.uniquerule[0] != 'D'
	    &&
	    iidd_tables.uniquerule[0] != ' ')
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0276_INCONSISTENT_CATINFO);
	    return(status);	
	}

	if (iidd_tables.alltoall[0] != 'Y'
	    &&
	    iidd_tables.alltoall[0] != 'N'
	    &&
	    iidd_tables.alltoall[0] != ' ')
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0276_INCONSISTENT_CATINFO);
	    return(status);	
	}

	if (iidd_tables.rettoall[0] != 'Y'
	    &&
	    iidd_tables.rettoall[0] != 'N'
	    &&
	    iidd_tables.rettoall[0] != ' ')
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0276_INCONSISTENT_CATINFO);
	    return(status);	
	}

	if (iidd_tables.journal[0] != 'Y'
	    &&
	    iidd_tables.journal[0] != 'N'
	    &&
	    iidd_tables.journal[0] != 'C'
	    &&
	    iidd_tables.journal[0] != ' ')
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0276_INCONSISTENT_CATINFO);
	    return(status);	
	}

	if (iidd_tables.view_base[0] != 'Y'
	    &&
	    iidd_tables.view_base[0] != 'N'
	    &&
	    iidd_tables.view_base[0] != ' ')
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0276_INCONSISTENT_CATINFO);
	    return(status);	
	}

	if (iidd_tables.multi_location[0] != 'Y'
	    &&
	    iidd_tables.multi_location[0] != 'N'
	    &&
	    iidd_tables.multi_location[0] != ' ')
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0276_INCONSISTENT_CATINFO);
	    return(status);	
	}

	/* Move tuple into object descriptor */
	if (!(global->rdfcb->rdf_rb.rdr_types_mask & RDR_BY_NAME))
	{
	    /* Setup name and owner */
    
	    rdd_rtrim((PTR)iiddb_objects.object_name,
		      (PTR)iiddb_objects.object_owner,
		      id_p);
	}

	MEcopy((PTR)iiddb_objects.object_name,
	       sizeof(DD_NAME),
	       (PTR)obj_p->dd_o1_objname);
	MEcopy((PTR)iiddb_objects.object_owner,
	       sizeof(DD_NAME),
	       (PTR)obj_p->dd_o2_objowner);
	obj_p->dd_o3_objid.db_tab_base = iiddb_objects.object_base;
	obj_p->dd_o3_objid.db_tab_index = iiddb_objects.object_index;
	obj_p->dd_o4_qryid.db_qry_high_time = iiddb_objects.qid1;
	obj_p->dd_o4_qryid.db_qry_low_time = iiddb_objects.qid2;

	MEcopy((PTR)iiddb_objects.create_date, sizeof(DD_DATE),
	       (PTR)obj_p->dd_o5_cre_date);

        /* convert object type */
        rdd_cvobjtype(iiddb_objects.object_type, &obj_p->dd_o6_objtype);

	MEcopy((PTR)iiddb_objects.alter_date,
	       sizeof(DD_DATE),
	       (PTR)obj_p->dd_o7_alt_date);

        (iiddb_objects.system_object[0] == 'Y') ? (obj_p->dd_o8_sysobj_b = TRUE)
					        : (obj_p->dd_o8_sysobj_b = FALSE);			   

	obj_p->dd_o9_tab_info.dd_t6_mapped_b = FALSE;  /* init column mapped 
						       ** indicator to FALSE */

	/* Fill object name, owner, and id in temp DMT_TBL_ENTRY */	    
	MEcopy((PTR)iiddb_objects.object_name,
	       sizeof(DD_NAME), tbl_p->tbl_name.db_tab_name);
	MEcopy((PTR)iiddb_objects.object_owner,
	       sizeof(DD_NAME), tbl_p->tbl_owner.db_own_name);
	STRUCT_ASSIGN_MACRO(iidd_tables.location_name, tbl_p->tbl_location);
	MEfill(sizeof(DD_NAME), (u_char)' ', 
		(PTR)tbl_p->tbl_filename.db_loc_name);
	STRUCT_ASSIGN_MACRO(obj_p->dd_o3_objid, tbl_p->tbl_id);
	STRUCT_ASSIGN_MACRO(obj_p->dd_o4_qryid, tbl_p->tbl_qryid);
	if (obj_p->dd_o8_sysobj_b)
	{
	    tbl_p->tbl_status_mask |= DMT_CATALOG;
	    tbl_p->tbl_status_mask |= DMT_EXTENDED_CAT;
	}
	if (obj_p->dd_o6_objtype == DD_3OBJ_VIEW)
	    tbl_p->tbl_status_mask |= DMT_VIEW;

	/* Information in the return tuple  not necessary correct for index and
	** statistics. We have to generate two extra query for index and
	** statistics information */

	if (iidd_tables.stats[0] == 'Y' || iidd_tables.stats[0] == ' ')
	    tbl_p->tbl_status_mask |= DMT_ZOPTSTATS;
    	if (iidd_tables.indexes[0] == 'Y')
	    tbl_p->tbl_status_mask |= DMT_IDXD;
	if (iidd_tables.rdonly[0] == 'Y')
	    tbl_p->tbl_status_mask |= DMT_RD_ONLY;	    
	if (iidd_tables.compressed[0] == 'Y' || iidd_tables.compressed[0]== 'H')
	    tbl_p->tbl_status_mask |= DMT_COMPRESSED;
	if (iidd_tables.duplicate_rows[0] == 'D')
	    tbl_p->tbl_status_mask |= DMT_DUPLICATES;	    
	if (iidd_tables.uniquerule[0] == 'U')
	    tbl_p->tbl_status_mask |= DMT_UNIQUEKEYS;

	/* convert storage description to storage id */
 
	if (rdd_cvstorage(iidd_tables.storage, &tbl_p->tbl_storage_type))
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0271_UNKNOWN_STORAGETYPE);
	    return(status);	
	}

	tbl_p->tbl_record_count = (i4)iidd_tables.num_rows;
	tbl_p->tbl_page_count = (i4)iidd_tables.number_pages;

	(iidd_tables.number_pages > iidd_tables.overflow)
	? (tbl_p->tbl_dpage_count = iidd_tables.number_pages - iidd_tables.overflow)
	: (tbl_p->tbl_dpage_count = 0);

#if 0
/* right now star is NOT set up to handle integrity or permits.  If a user
** was to change iidd_columns in LDB to have 'Y' for indexes, protects or rules,
** STAR server will AV.  So, cut them off at the pass.  For NOT do not allow
** DMT_INTEGRITIES or DMT_PROTECTION to be specified.  This section that is
** ifdeffed out will be re-added when STAR supports integrities/permits
*/
	if (iidd_tables.integrities[0] == 'Y')
	    tbl_p->tbl_status_mask |= DMT_INTEGRITIES;	    
    	if (iidd_tables.permits[0] == 'Y')
	    tbl_p->tbl_status_mask |= DMT_PROTECTION;
#endif
	if (iidd_tables.alltoall[0] == 'N')
	    tbl_p->tbl_status_mask |= DMT_ALL_PROT;	    
	if (iidd_tables.rettoall[0] == 'N')
	    tbl_p->tbl_status_mask |= DMT_RETRIEVE_PRO;	    
	if (iidd_tables.journal[0] == 'Y')
	    tbl_p->tbl_status_mask |= DMT_JNL;
	if (iidd_tables.view_base[0] == 'Y')
	    tbl_p->tbl_status_mask |= DMT_BASE_VIEW;
	if (iidd_tables.multi_location[0] == 'Y')
	    tbl_p->tbl_status_mask |= DMT_MULTIPLE_LOC;
	
	tbl_p->tbl_date_modified.db_tab_high_time = iidd_tables.stamp1; 
	tbl_p->tbl_date_modified.db_tab_low_time = iidd_tables.stamp2; 

	tbl_p->tbl_width = (i4)iidd_tables.row_width;
	tbl_p->tbl_i_fill_factor = (i4)iidd_tables.ifillpct;
	tbl_p->tbl_d_fill_factor = (i4)iidd_tables.dfillpct;
	tbl_p->tbl_l_fill_factor = (i4)iidd_tables.lfillpct;
	tbl_p->tbl_min_page = (i4)iidd_tables.minpages;
	tbl_p->tbl_max_page = (i4)iidd_tables.maxpages;
	tbl_p->tbl_pgsize = (i4)iidd_tables.table_pagesize;

    }

    /* flush */
    return(rdd_flush(global));
}

/*{
** Name: rdd_gobjinfo - inquire object info from CDB.
**
** Description:
**      This routine acts as an main module to get object information.
**	
**
** Inputs:
**      global				    ptr to RDF state descriptor
**		
** Outputs:
**      global				    ptr to RDF state descriptor
**	    .rdf_trel			    temporary relation desc storage 
**	    .rdf_tobj			    temporary object desc storeage
**		.dd_o1_objname		    object name
**		.dd_o2_objowner		    object owner
**		.dd_o3_objid		    object id
**		.dd_o4_qryid		    query id if view
**		.dd_o5_cre_date		    create date
**		.dd_o6_objtype		    object type
**		.dd_o7_alt_date		    alter date
**		.dd_o8_sysobj_b		    TRUE if a system object, FALSE
**                                          if user created object
**		.dd_o9_tab_info		    ldb table info
**		    .dd_t1_tab_name	    local table name
**		    .dd_t2_tab_owner	    local table owner
**		    .dd_t3_tab_type	    local object type
**		    .dd_t4_cre_date	    create date
**		    .dd_t5_alt_date	    alter date
**		    .dd_t6_mapped_b	    TRUE if column mappping
**		    .dd_t7_col_cnt	    column count
**		    .dd_t9_ldb_p	    pointer to ldb desc
**			.dd_l1_ingres_b	    FALSE
**			.dd_l2_node_name    node name
**			.dd_l3_ldb_name	    ldb name
**			.dd_l4_dbms_name    dbms name
**			.dd_l5_ldb_id	    ldb id
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**      29-apr-89 (mings)
**         check RDR_SET_LOCKMODE 
**	09-jun-89 (mings)
**	   clear local table type for view
**	08-aug-90 (teresa)
**	    add logic to skip checking user name if user has system catalog
**	    update priv and tablename starts with "ii"
**	18-jun-91 (teresa)
**	    fix bug 34684 by setting dmt_gateway bit if the LDB does not support
**	    tids (ie, iidbcapabilities' INGRES=Y or TID_LEVEL is nonzero)
**	12-may-92 (teresa)
**	    move schema check and timestamp change logic to attributes 
**	    processing for sybil.
**	16-jul-92 (teresa)
**	    prototypes
**	20-nov-92 (teresa)
**	    fix bug where RDF was no longer detecting if table exists in
**	    LDB for a "drop" command.
**      07-Jun-2005 (inifa01) b111211 INGSRV2580
**          Verifydb reports error; S_DU1619_NO_VIEW iirelation indicates that
**          there is a view defined for table a (owner ingres), but none exists,
**          when view(s) on a table are dropped.
**          Were making DMT_ALTER call on DMT_C_VIEW, to turn on iirelation.relstat
**          TCB_VBASE bit, indicating view is defined over table(s).  TCB_VBASE
**          not tested anywhere in 2.6 and not turned off when view is dropped.
**          qeu_cview() call to dmt_alter() to set bit has been removed ,
**          removing all references to TCB_VBASE and removing call to 
**	    rdd_baseviewchk() to set DMT_BASE_VIEW.
**	23-nov-2006 (dougi)
**	    TCB_VBASE/DMT_BASE_VIEW reinstated.
*/
DB_STATUS
rdd_gobjinfo( RDF_GLOBAL *global)
{
    DB_STATUS		status = E_DB_OK;
    DD_OBJ_DESC		*obj_p = &global->rdf_tobj;
    DD_2LDB_TAB_INFO    *ldbtab_p = &obj_p->dd_o9_tab_info;
    DMT_TBL_ENTRY	*tbl_p = &global->rdf_trel;

    DD_1LDB_INFO	*info_ptr = ldbtab_p->dd_t9_ldb_p;
    RDD_OBJ_ID		obj_id;

    /* Retrieve object information and store in temp DD_OBJ_DESC. */

    if (global->rdfcb->rdf_rb.rdr_types_mask & RDR_CHECK_EXIST)
	status = rdd_objinfo(global, &obj_id);
    else
	status = rdd_relinfo(global, &obj_id);
                                 
    if (DB_FAILURE_MACRO(status))
	return(status);

    /* Validate the object type. Currently only Link, Table, View, and
    ** Index are supported. Partitioned and Replicated objects will be
    ** supported at later time */	

    if (
	    (obj_p->dd_o6_objtype != DD_1OBJ_LINK)
	    &&
	    (obj_p->dd_o6_objtype != DD_2OBJ_TABLE)
	    &&
	    (obj_p->dd_o6_objtype != DD_3OBJ_VIEW)
	    &&
	    (obj_p->dd_o6_objtype != DD_4OBJ_INDEX)
       )
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0264_UNKOWN_OBJECT);
	return(status);	
    }

    /* Retrieve local table information and store in DD_2LDB_TAB_INFO
    ** and DMT_TBL_ENTRY structures. */

    if (obj_p->dd_o6_objtype == DD_1OBJ_LINK
	||
	obj_p->dd_o6_objtype == DD_2OBJ_TABLE
       )
    {
	status = rdd_ldbtab(global, &obj_id);
				
	if (DB_FAILURE_MACRO(status))
	    return(status);

	/* fix bug 34684 -- if the ldb does not support tids, then we need to
	** set the DMT_GATEWAY bit so OPF does not try to use tids with this
	** object.  NOTE: if we are doing an RDR_REMOVE type of call, then
	** info_ptr will NOT be initialized by rdd_ldbtab, and we do not
	** care whether or not the DB supports TIDs because  */
	info_ptr = ldbtab_p->dd_t9_ldb_p;
	if (info_ptr)
	    if ( ! (info_ptr->dd_i2_ldb_plus.dd_p3_ldb_caps.dd_c1_ldb_caps
		    & DD_5CAP_TIDS)
	       )
	    {
		tbl_p->tbl_status_mask |= DMT_GATEWAY;
	    }

	status = rdd_baseviewchk(global);

	if (DB_FAILURE_MACRO(status))
	    return(status);
    }
    else
    {
	/* no local table info */

	ldbtab_p->dd_t3_tab_type = DD_0OBJ_NONE;
    }

    /* If RDR_USERTAB was set, this means caller requests
    ** the local user name. mostly happen at drop table time.
    **
    ** However, if the user is dropping a catalog (and, hence, has the catalog
    ** update privilege), then skip checking the owner
    ** name against the user name, as the system catalog will be owned by the
    ** system catalog owner, who is probably NOT the user.
    */
    if (
	    (global->rdfcb->rdf_rb.rdr_types_mask & RDR_USERTAB)
	    &&
	    (obj_p->dd_o6_objtype != DD_3OBJ_VIEW)
	    &&
	    (~global->rdfcb->rdf_rb.rdr_2types_mask & RDR2_SYSCAT)
       )	 
    {
	status = rdd_username(global);
	
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    /* If RDR_CHECK_EXIST, exit here. Mostly, this is the case 
    ** when PSF processes DDL query, like, create/drop link. 
    ** Also exit here if we're doing a registered procedure.
    */

    if ( (global->rdfcb->rdf_rb.rdr_types_mask & RDR_CHECK_EXIST) 
	  ||
	  (global->rdfcb->rdf_rb.rdr_instr & RDF_REGPROC)	  
        )
	return(status);

    /* Retrieve attribute count */

    status = rdd_getcolno(global, &obj_id); 
    
    /* If error or view, exit here. */

    if ((DB_FAILURE_MACRO(status))
	||
        (obj_p->dd_o6_objtype == DD_3OBJ_VIEW)
       )
	return(status);

    /* If RDR_REFRESH was set, this means PSF needs local statistics 
    ** info for register with refresh. */

    if ((global->rdfcb->rdf_rb.rdr_types_mask & RDR_REFRESH)
	&&
	!(global->rdfcb->rdf_rb.rdr_types_mask & RDR_SET_LOCKMODE)
       )
    {
	status = rdd_localstats(global);
	
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    /* If object type is Link, Table, or Index, check to see if more 
    ** iformation is available in iidd_physical_tables */ 

    if (
	    (obj_p->dd_o6_objtype == DD_1OBJ_LINK)
	    ||
	    (obj_p->dd_o6_objtype == DD_2OBJ_TABLE)
	    ||
	    (obj_p->dd_o6_objtype == DD_4OBJ_INDEX)
       )
    {
	if (
		(ldbtab_p->dd_t3_tab_type == DD_2OBJ_TABLE)
		||
		(ldbtab_p->dd_t3_tab_type == DD_4OBJ_INDEX)
	    )
	{

	    /* Check if this object has been indexed */
            status = rdd_idxcount(global, &obj_id);
	}
    }

    if (global->rdfcb->rdf_rb.rdr_types_mask & RDR_USERTAB)
    {
	status = rdd_lcltab_exists(global);
    }
    return(status);
}

/*{
** Name: rdd_gmapatt - build mapped column descriptor for object.
**
** Description:
**      This routine builds mapped column descriptor for an object.
**
** Inputs:
**      global                          ptr to RDF state variable
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**          initial creation
**      16-may-89 (mings)
**          added number of columns consistency check
**	09-jul-90 (teresa)
**	   changed "select *" to specify each desired column name.
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdd_gmapattr( RDF_GLOBAL *global)
{
    DB_STATUS		status = E_DB_OK;
    QEF_RCB		*qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_COLUMN_DESC	**attrmap = global->rdf_ddrequests.rdd_mapattr;
    DD_COLUMN_DESC	*col_ptr;
    i4			col_cnt = 0;
    DD_PACKET		ddpkt;
    RDC_MAP_COLUMNS	ldb_col;
    RQB_BIND    	rq_bind[RDD_02_COL],	/* 2 columns */
			*bind_p = rq_bind;
    char		qrytxt[RDD_QRY_LENGTH];

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select * from iidd_ddb_ldb_columns where
    **	        object_base = 'obj_base' and object_index = 'obj_index'
    **	
    */
    
    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	    "%s %d and object_index = %d order by column_sequence;",
            "select local_column, column_sequence from iidd_ddb_ldb_columns \
where object_base =",
	    global->rdfcb->rdf_rb.rdr_tabid.db_tab_base,
	    global->rdfcb->rdf_rb.rdr_tabid.db_tab_index);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = (i4) STlength(qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    rq_bind[0].rqb_addr = (PTR)ldb_col.local_column; 
    rq_bind[0].rqb_length = sizeof(DD_NAME);
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
    rq_bind[1].rqb_addr = (PTR)&ldb_col.column_sequence;
    rq_bind[1].rqb_length = sizeof(ldb_col.column_sequence);
    rq_bind[1].rqb_dt_id = DB_INT_TYPE;

    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_02_COL;

    do
    {
	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	{
	    col_cnt++;
	    
	    /* check number of columns */

	    if ((col_cnt > global->rdfcb->rdf_info_blk->rdr_no_attr)
	        ||  
	        (ldb_col.column_sequence > global->rdfcb->rdf_info_blk->rdr_no_attr)
	       )
	    {
		status = E_DB_ERROR;
		rdu_ierror(global, E_DB_ERROR, E_RD0256_DD_COLMISMATCH);
		status = rdd_flush(global);
		return(E_DB_ERROR);
	    }

            col_ptr = (DD_COLUMN_DESC *)attrmap[ldb_col.column_sequence];
	    MEcopy((PTR)ldb_col.local_column, sizeof(DD_NAME),
			     (PTR)col_ptr->dd_c1_col_name);
	}
    } while (ddr_p->qer_d5_out_info.qed_o3_output_b);

    /* check number of columns */

    if (col_cnt != global->rdfcb->rdf_info_blk->rdr_no_attr)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, E_DB_ERROR, E_RD0256_DD_COLMISMATCH);
    }
    return(status);
}

/*
** {
** Name: rdd_gattr - build attributes descriptor for object.
**
** Description:
**      This routine builds attributes descriptor for an object.
**
** Inputs:
**      global                          ptr to RDF state variable
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**          initial creation
**      18-may-89 (mings)
**          added column number consistency check
**	12-may-92 (teresa)
**	    move timestamp and schema checking logic here for sybil.
**	16-jul-92 (teresa)
**	    prototypes
**	13-mar-93 (fpang)
**	    If mapped, copy each column name to the names array.
**	    Fixes B49737, Schema change error if register w/refresh
**	    and columns are mapped.
**	22-mar-93 (fpang)
**	    Correctly casted arguements to MEcopy(). Also specifed copy amount
**	    in terms of the actual copy argument, rather than it's type. This
**	    requires less maintenance should the type of the arguments change.
**	08-sep-93 (teresa)
**	    Fix bug 54383 - scale calculate percision for Decimal Data types.
**      12-Oct-1993 (fred)
**          Fix declaration of rq_bind.  It was declared as
**          [RDD_08_COL] even though it is explicitly set to 9 columns
**          sometimes.  Lots of merry mixups...
**	16-mar-1998 (rigka01)
**	   Cross integrate change 434645 for bug 89105
**         Bug 89105 - clear up various access violations which occur
**	   when rdf memory is low.  In rdd_gattr(), add missing 
**	   DB_FAILURE_MACRO(status) after rdu_malloc call.
**	20-Jan-2005 (schka24)
**	    Clear out collation ID, not yet supported by Star catalogs.
*/
DB_STATUS
rdd_gattr( RDF_GLOBAL  *global)
{
    DB_STATUS		status = E_DB_OK;
    QEF_RCB		*qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_OBJ_DESC		*obj_p = global->rdfcb->rdf_info_blk->rdr_obj_desc;
    DMT_ATT_ENTRY       **attr = global->rdf_ddrequests.rdd_attr;
    DMT_ATT_ENTRY	*col_ptr;
    ADF_CB		*adfcb;
    i4			col_cnt = 0;
    DD_PACKET		ddpkt;
    RDC_COLUMNS		iicolumns;
    RQB_BIND    	rq_bind[RDD_09_COL], /* 9 columns */
			*bind_p = rq_bind;
    RDD_OBJ_ID		object_id,
			*id_p = &object_id;
    char		qrytxt[RDD_QRY_LENGTH];
    DD_2LDB_TAB_INFO    *ldbtab_p;
    bool                mapped;
    DD_NAME             *local_names=NULL;
    DD_NAME             local_name;


    if (!obj_p)
    	obj_p = global->rdf_ulhobject->rdf_sysinfoblk->rdr_obj_desc;
  
    if (!obj_p)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }

    ldbtab_p = &obj_p->dd_o9_tab_info;
    if (!ldbtab_p)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }

    mapped = ldbtab_p->dd_t6_mapped_b;
    /* allocate memory to hold array of iidd_ddb_ldb_column.local_column */
    if (mapped)
    {
	status = rdu_malloc(global, 
		    (i4) ((ldbtab_p->dd_t7_col_cnt + 1) * sizeof(DD_NAME)),
		    (PTR *) &local_names);
        if (DB_FAILURE_MACRO(status))
   	    return (status); 
	MEfill ( (ldbtab_p->dd_t7_col_cnt + 1) * sizeof(DD_NAME),
		 (u_char)'\0',
		 (PTR)local_names);
	/* FIXME ~~~ Need to put this ptr in the infoblk somehow. */
	
    }
    else
	ldbtab_p = NULL;

    /* Setup name and owner */
    
    rdd_rtrim((PTR)obj_p->dd_o1_objname,
	      (PTR)obj_p->dd_o2_objowner,
	      id_p);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    if (mapped)
    {
	/* set up query to retrieve from iidd_ddb_ldb_columns and iidd_columns
	**
	**  select a.local_column, b.column_name, b.column_datatype, 
	**	   b.column_length, b.column_scale, b.column_nulls, 
	**	   b.column_defaults, b.column_sequence, b.key_sequence
	**  from iidd_ddb_ldb_columns a, iidd_columns b 
	**  where 
        **      a.object_base = object_base and 
	**	a.object_index = object_index and
	**      b.table_name = 'table_name' and 
	**	b.table_owner = 'table_owner' and
	**	a.column_sequence = b.column_sequence 
	**  order by b.column_sequence;
	*/
    
	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	"%s %d and a.object_index = %d and b.table_name = '%s' and \
b.table_owner = '%s' and a.column_sequence = b.column_sequence order by \
b.column_sequence;", 	    
	    "select b.column_name, b.column_datatype, b.column_length, \
b.column_scale, b.column_nulls, b.column_defaults, b.column_sequence, \
b.key_sequence, a.local_column from iidd_ddb_ldb_columns a, iidd_columns b \
where a.object_base =", 
	    obj_p->dd_o3_objid.db_tab_base, 
	    obj_p->dd_o3_objid.db_tab_index,
	    id_p->tab_name, 
	    id_p->tab_owner);
    }
    else
    {
	/* set up query 
	**
	**	select * from iidd_columns where table_name = 'obj_name' and
	**	            table_owner = 'obj_owner'
	**	
	*/
	ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	    "%s '%s' and table_owner = '%s' order by column_sequence;",
	    "select column_name, column_datatype, column_length, column_scale, \
column_nulls, column_defaults, column_sequence, key_sequence from \
iidd_columns where table_name =",
	    id_p->tab_name,
            id_p->tab_owner);
    }

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = (i4)STlength(qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 
    
    /* Get adf session control block for column length conversion.
    ** Note that the physical length of nullable column is different
    ** from user defined length */
    adfcb = (ADF_CB*)global->rdf_sess_cb->rds_adf_cb;

    /* column_name */
    rq_bind[0].rqb_addr = (PTR)iicolumns.name; 
    rq_bind[0].rqb_length = sizeof(DD_NAME);
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
    /* column_datatype */
    rq_bind[1].rqb_addr = (PTR)iicolumns.datatype;
    rq_bind[1].rqb_length = sizeof(DD_NAME);
    rq_bind[1].rqb_dt_id = DB_CHA_TYPE;
    /* column_length */
    rq_bind[2].rqb_addr = (PTR)&iicolumns.length;
    rq_bind[2].rqb_length = sizeof(iicolumns.length);
    rq_bind[2].rqb_dt_id = DB_INT_TYPE;
    /* column_scale */
    rq_bind[3].rqb_addr = (PTR)&iicolumns.scale;
    rq_bind[3].rqb_length = sizeof(iicolumns.scale);
    rq_bind[3].rqb_dt_id = DB_INT_TYPE;
    /* column_nulls */
    rq_bind[4].rqb_addr = (PTR)iicolumns.nulls; 
    rq_bind[4].rqb_length = sizeof(RDD_8CHAR);
    rq_bind[4].rqb_dt_id = DB_CHA_TYPE;
    /* column_defaults */
    rq_bind[5].rqb_addr = (PTR)iicolumns.defaults;
    rq_bind[5].rqb_length = sizeof(RDD_8CHAR);
    rq_bind[5].rqb_dt_id = DB_CHA_TYPE;
    /* column_sequence */
    rq_bind[6].rqb_addr = (PTR)&iicolumns.sequence;
    rq_bind[6].rqb_length = sizeof(iicolumns.sequence);
    rq_bind[6].rqb_dt_id = DB_INT_TYPE;
    /* key_sequence */
    rq_bind[7].rqb_addr = (PTR)&iicolumns.key_sequence;
    rq_bind[7].rqb_length = sizeof(iicolumns.key_sequence);
    rq_bind[7].rqb_dt_id = DB_INT_TYPE;
    if (mapped)
    {
	/* local_column */
	rq_bind[8].rqb_addr = (PTR)&local_name; 
	rq_bind[8].rqb_length = sizeof(DD_NAME);
	rq_bind[8].rqb_dt_id = DB_CHA_TYPE;
    }
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    if (mapped)
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_09_COL;
    else
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_08_COL;

    do
    {
	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	{   /* attribute starts from 1 */

	    DB_DATA_VALUE	dv;
	    DB_DATA_VALUE	col_width;

	    col_cnt++;
	    if ((col_cnt > global->rdfcb->rdf_info_blk->rdr_no_attr)
	        ||  
	        (iicolumns.sequence > global->rdfcb->rdf_info_blk->rdr_no_attr)
	       )
	    {
		status = E_DB_ERROR;
		rdu_ierror(global, E_DB_ERROR, E_RD0256_DD_COLMISMATCH);
		break;
	    }
		
            col_ptr = (DMT_ATT_ENTRY *)attr[iicolumns.sequence];
	    MEcopy((PTR)iicolumns.name, sizeof(DD_NAME),
                      (PTR)&col_ptr->att_name);
	    col_ptr->att_number = iicolumns.sequence;

	    /* Call adf to convert data type name to data type id */

	    status = rdd_cvtype(global, iicolumns.datatype,
				&dv.db_datatype, adfcb);
	    if (DB_FAILURE_MACRO(status))
		break;
	    
	    col_ptr->att_offset = 0;	/* do we need column offset? */
	    if( dv.db_datatype == DB_DEC_TYPE)
	    {
		/* decimal datatypes get run through a macro to be scaled
		** for length and precision.  Also, if the decimal data type
		** is nullable, then it must be negated.
		*/
		col_ptr->att_prec = DB_PS_ENCODE_MACRO(	iicolumns.length,
							iicolumns.scale);
		col_ptr->att_width = DB_PREC_TO_LEN_MACRO( 
							 (i4)iicolumns.length);
		if (iicolumns.nulls[0] == 'Y')
		{
		    /* nullable data types are indicated by negating the
		    ** datatype.  Also, if the datatype is nullable, then the
		    ** length must be increased by 1 to hold the null character,
		    ** since the DB_PREC_TO_LEN_MACRO does not take data type
		    ** into consideration.
		    */
		    col_ptr->att_type = (i4)dv.db_datatype * -1;
		    col_ptr->att_width++;
		}
		else
		    col_ptr->att_type = (i4)dv.db_datatype;
	    }
	    else
	    {
		/* Setup db data value for column width conversion from what is
		** in the catalogs to a physical length that also takes into
		** consideration defaults and count fields (for varchar).
		** Force precision to zero for non-decimal data types.  Also, 
		** if the data type is nullable, then it must be negated.
		*/
		dv.db_prec = 0;
		dv.db_length = iicolumns.length;
		/* negative the id if nullable datatype */
		if (iicolumns.nulls[0] == 'Y')
		    dv.db_datatype *= -1;
		status = adc_lenchk(adfcb, TRUE, &dv, &col_width);
		if (DB_FAILURE_MACRO(status))
		    break;
		col_ptr->att_width = col_width.db_length;  
		col_ptr->att_prec = 0;
		col_ptr->att_type = (i4)dv.db_datatype;
	    }

	    /* If not nullable and not default set, set att_flags
	    ** to DMT_F_NDEFAULTS */ 

	    (iicolumns.nulls[0] == 'N' && iicolumns.defaults[0] == 'N')
	    ? (col_ptr->att_flags = DMU_F_NDEFAULT)
	    : (col_ptr->att_flags = 0); 

	    col_ptr->att_key_seq_number = iicolumns.key_sequence;	    
	    col_ptr->att_collID = 0;
	    col_ptr->att_geomtype = -1;
	    col_ptr->att_srid = -1;
	    
	    if (mapped)
		MEcopy( (PTR)&local_name, sizeof(*local_names), 
		        (PTR)&local_names[col_cnt] );
	}
    } while (ddr_p->qer_d5_out_info.qed_o3_output_b);

    /* is this the case that error occurs before finishing tuple fetching */

    if ((DB_FAILURE_MACRO(status))
	&&
	(global->rdf_ddrequests.rdd_ddstatus & RDD_FETCH)
       )
    {
	/* flush the buffer before exit */

	status = rdd_flush(global);
	return(E_DB_ERROR);
    }

    /* check the column number */

    if (col_cnt != global->rdfcb->rdf_info_blk->rdr_no_attr)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0256_DD_COLMISMATCH);
    }

    /* If object is a link, get alter date from ldb and then
    ** compare to the alter date stored in iidd_ddb_tableinfo.
    ** Note that if RDR_SET_LOCKMODE is set by PSF, then no
    ** schema checking will be performed to prevent transaction
    ** problem.  This is because ldb schema is send out through
    ** user association. */

    if (  DB_SUCCESS_MACRO(status)
	  &&
	  !(global->rdfcb->rdf_rb.rdr_types_mask & RDR_SET_LOCKMODE)
	  &&
	  (  obj_p->dd_o6_objtype == DD_1OBJ_LINK
	     ||
	     obj_p->dd_o6_objtype == DD_2OBJ_TABLE
	  )
       )
	    status = rdd_alterdate(global, id_p, local_names, adfcb);

    return(status);
}

/*{
** Name: rdd_gindex	- build index descriptor for an object.
**
** Description:
**      This routine will build index descriptor for an object.
**
** Inputs:
**      global                          ptr to RDF state variable
**      indx				ptr to array of pointers to index descriptors
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**          initial creation
**	16-jul-92 (teresa)
**	    prototypes
**	30-May-2006 (jenjo02)
**	    DMT_IDX_ENTRY idx_attr_id array now has DB_MAXIXATTS instead
**	    of DB_MAXKEYS elements.
*/
DB_STATUS
rdd_gindex(RDF_GLOBAL *global)
{
    DB_STATUS		status = E_DB_OK;
    QEF_RCB		*qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DMT_IDX_ENTRY       **indx = global->rdf_ddrequests.rdd_indx;
    DD_OBJ_DESC		*obj_p = global->rdfcb->rdf_info_blk->rdr_obj_desc;
    DMT_IDX_ENTRY	*col_ptr = NULL;
    DD_PACKET		ddpkt;
    i4			idx_cnt, att_cnt, key_cnt;
    i4			keyseq;
    i4			colseq;                 /* column sequence in base table */
    i4			icolseq;		/* column sequence in secondary index */
    u_i4		save_idx;
    RDC_INDEXES		idxdesc;
    RDC_OBJECTS		objects;
    RDC_TABLES		tables;
    RQB_BIND    	rq_bind[RDD_10_COL],	/* for 10 columns */
			*bind_p = rq_bind;
    RDD_OBJ_ID		object_id,
			*id_p = &object_id;
    char		qrytxt[RDD_QRY_LENGTH];

    if (!obj_p)
    	obj_p = global->rdf_ulhobject->rdf_sysinfoblk->rdr_obj_desc;
  
    if (!obj_p)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }

    /* set up object name and owner */

    rdd_rtrim((PTR)obj_p->dd_o1_objname,
	      (PTR)obj_p->dd_o2_objowner,
	      id_p);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select a.index_name, a.index_owner, a.storeage_structure,
    **	       b.object_base, b.object_index,
    **	       c.number_pages, c.overflow,
    **	       d.column_sequence, d.keysequence,
    **	       e.column_sequence from
    **	       IIDD_INDEXES a,
    **	       IIDD_DDB_OBJECTS b,
    **	       IIDD_TABLES c, 
    **	       IIDD_COLUMNS d, 
    **	       IIDD_COLUMNS e            		    
    **	       where a.base_name = 'object_name'      and
    **	             a.base_owner = 'object_owner'    and
    **		     b.object_name = a.index_name     and
    **		     b.object_owner = a.index_owner   and
    **		     c.number_pages = a.index_name    and
    **		     c.overflow = a.index_owner	      and
    **		     d.table_name = a.index_name      and
    **               d.table_owner = a.index_owner    and
    **		     e.table_name = a.base_name	      and
    **		     e.table_owner = a.base_owner     and
    **		     e.column_name = d.column_name
    **	       order by a.index_name, d.column_sequence;
    **
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	     "%s '%s' and %s = '%s' and %s and %s and %s and %s and %s \
order by a.index_name, d.column_sequence;",
	     "select a.index_name, a.index_owner, a.storage_structure, \
b.object_base, b.object_index, c.number_pages, c.overflow_pages, d.column_sequence, \
d.key_sequence, e.column_sequence from iidd_indexes a, iidd_ddb_objects b, \
iidd_tables c, iidd_columns d, iidd_columns e where a.base_name =",
             id_p->tab_name,
 	     "a.base_owner",
             id_p->tab_owner,
	     "b.object_name = a.index_name and b.object_owner = a.index_owner",
	     "c.table_name = a.index_name and c.table_owner = a.index_owner",
	     "d.table_name = a.index_name and d.table_owner = a.index_owner",
	     "e.table_name = a.base_name and e.table_owner = a.base_owner",
	     "e.column_name = d.column_name");

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = (i4)STlength(qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    /* 0. index_name from iidd_indexes */
    rq_bind[0].rqb_addr = (PTR)idxdesc.index_name;
    rq_bind[0].rqb_length = sizeof(DD_NAME);
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
    /* 1. index_owner from iidd_indexes */
    rq_bind[1].rqb_addr = (PTR)idxdesc.index_owner;
    rq_bind[1].rqb_length = sizeof(DD_NAME);
    rq_bind[1].rqb_dt_id = DB_CHA_TYPE;
    /* 2. storage_structure from iidd_indexes */
    rq_bind[2].rqb_addr = (PTR)idxdesc.storage;
    rq_bind[2].rqb_length = sizeof(RDD_16CHAR);
    rq_bind[2].rqb_dt_id = DB_CHA_TYPE;
    /* fields in iidd_ddb_objects
    ** 3. object_base from iidd_ddb_objects */
    rq_bind[3].rqb_addr = (PTR)&objects.object_base; 
    rq_bind[3].rqb_length = sizeof(objects.object_base);
    rq_bind[3].rqb_dt_id = DB_INT_TYPE;
    /* 4. object_index from iidd_ddb_objects */
    rq_bind[4].rqb_addr = (PTR)&objects.object_index;
    rq_bind[4].rqb_length = sizeof(objects.object_index);
    rq_bind[4].rqb_dt_id = DB_INT_TYPE;
    /* 5. number_pages from iidd_tables */
    rq_bind[5].rqb_addr = (PTR)&tables.number_pages;
    rq_bind[5].rqb_length = sizeof(tables.number_pages);
    rq_bind[5].rqb_dt_id = DB_INT_TYPE;
    /* 6. overflow_pages from iidd_tables */
    rq_bind[6].rqb_addr = (PTR)&tables.overflow;
    rq_bind[6].rqb_length = sizeof(tables.overflow);
    rq_bind[6].rqb_dt_id = DB_INT_TYPE;
    /* 7. column sequence for secondary index */
    rq_bind[7].rqb_addr = (PTR)&icolseq;
    rq_bind[7].rqb_length = sizeof(icolseq);
    rq_bind[7].rqb_dt_id = DB_INT_TYPE;
    /* 7. key sequence from iidd_columns */
    rq_bind[8].rqb_addr = (PTR)&keyseq;
    rq_bind[8].rqb_length = sizeof(keyseq);
    rq_bind[8].rqb_dt_id = DB_INT_TYPE;
    /* 8. column sequence from iidd_columns */
    rq_bind[9].rqb_addr = (PTR)&colseq;
    rq_bind[9].rqb_length = sizeof(colseq);
    rq_bind[9].rqb_dt_id = DB_INT_TYPE;

    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_10_COL;

    /* initialize variables */

    idx_cnt = 0;    /* index count */
    att_cnt = 0;    /* index column count */
    key_cnt = 0;    /* index key count */
    save_idx = 0;   /* use to break retrieved tuplue by index */

    do
    {
	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	{
	    /* Setup index info from zeor and up. */

	    if (save_idx != objects.object_index)
	    {
		if (save_idx)
		{
		    
		    /* Complete the current index info and go to next one. */

		    col_ptr->idx_array_count = att_cnt;
		    col_ptr->idx_key_count = key_cnt;

		    idx_cnt++;

		    /* Consistency check number of indexes */

		    if (idx_cnt >= global->rdfcb->rdf_info_blk->rdr_no_index)
		    {

			/* index count error. let opf take care the error, don't
			** display error in rdf */

			SETDBERR(&global->rdfcb->rdf_error, 0, E_RD026A_OBJ_INDEXCOUNT);
			status = rdd_flush(global);
			return(E_DB_ERROR);
		    }
		}

		/* pointer to the index descriptor */

		col_ptr = (DMT_IDX_ENTRY *)indx[idx_cnt];	    

		/* save object_index */

		save_idx = objects.object_index;

		/* init fields */

		key_cnt = 0;
		att_cnt = 0;

		/* fields from iidd_physical_tables */

		if (tables.number_pages > 0 && tables.number_pages >= tables.overflow)
		    col_ptr->idx_dpage_count = tables.number_pages - tables.overflow;
		else
		    col_ptr->idx_dpage_count = 0;		

		/* fields from iidd_indexes */

		MEcopy((PTR)idxdesc.index_name, sizeof(DD_NAME),
			  (PTR)col_ptr->idx_name.db_tab_name);

		status = rdd_cvstorage(idxdesc.storage,
				  (i4 *)&col_ptr->idx_storage_type);

		if (DB_FAILURE_MACRO(status))
		{
		    rdu_ierror(global, status, E_RD0265_STORAGE_TYPE_ERROR);
		    status = rdd_flush(global);
		    return(E_DB_ERROR);
		}

		/* fields from iidd_ddb_objects */

		col_ptr->idx_id.db_tab_base = objects.object_base;
		col_ptr->idx_id.db_tab_index = objects.object_index; 

	    }

	    att_cnt++;

	    if (keyseq > 0)
		key_cnt++;

	    /* move base table column no to the array based on 
	    ** column sequence on secondary index */

	    if ((icolseq > 0) && (icolseq < DB_MAXIXATTS))
		col_ptr->idx_attr_id[icolseq - 1] = colseq; 

	}
	else if (att_cnt > 0)
	     {
		/* complete the last index info */
		col_ptr->idx_array_count = att_cnt;
		col_ptr->idx_key_count = key_cnt;
		++idx_cnt;
	     }   
    } while (ddr_p->qer_d5_out_info.qed_o3_output_b);

    if (idx_cnt != global->rdfcb->rdf_info_blk->rdr_no_index)
    {
	/* index count error. This problem may cause by inconsistent
	** secondary index object. For example, user drops a link by accident,
        ** say ddx_1001_1002, and which is a local index promoted to star at
        ** create link time. In this case, index info was left in star without
        ** intact but all other info was long gone. */

	status = E_DB_ERROR;
	SETDBERR(&global->rdfcb->rdf_error, 0, E_RD026A_OBJ_INDEXCOUNT);
    }

    return(status);
}

/*{
** Name: rdd_gstats - retrieve object information from CDB.
**
** Description:
**      This routine generates query to retrieve object description
**	from iidd_ddb_objects in CDB.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**	    .rdf_tobj			temp object descriptor
**
** Outputs:
**      global                          ptr to RDF state descriptor
**	    .rdf_tobj			temp object descriptor
**		.dd_o1_objname		    object name
**		.dd_o2_objowner		    object owner
**		.dd_o3_objid		    object id
**		.dd_o4_qryid		    query id if view
**		.dd_o5_cre_date		    create date
**		.dd_o6_objtype		    object type
**		.dd_o7_alt_date		    alter date
**		.dd_o8_sysobj_b		    TRUE if system object
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	9-jul-90 (teresa)
**	    changed "select *" to specify each desired column name.
**	16-jul-92 (teresa)
**	    prototypes
**	09-nov-92 (rganski)
**	    Added initialization of new RDD_HISTO fields charnunique and
**	    chardensity.
**	18-jan-93 (rganski)
**	    Character Histogram Enhancements project, continued:
**	    Added initialization of new rdd_statp fields shistlength and
**	    sversion. If there is a version in sversion, cellsize gets set to
**	    shislength. Added consistency check for shistlength.
**	    Also added initialization of scomplete and sdomain, which are now
**	    in iidd_stats.is_complete and iidd_stats.column_domain.
**	23-jul-93 (rganski)
**	    Added new iidd_stats columns to select statement and to bindings.
*/
DB_STATUS
rdd_gstats(RDF_GLOBAL *global)
{
    DB_STATUS		status = E_DB_OK;
    RDD_HISTO	        *rdd_statp;
    DD_OBJ_DESC		*obj_p = global->rdfcb->rdf_info_blk->rdr_obj_desc; 
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET		ddpkt;
    RQB_BIND            rq_bind[RDD_09_COL],	/* 9 columns */
			*bind_p = rq_bind;
    RDD_OBJ_ID	        object_id,
			*id_p = &object_id;
    char		qrytxt[RDD_QRY_LENGTH];

    if (!obj_p)
    	obj_p = global->rdf_ulhobject->rdf_sysinfoblk->rdr_obj_desc;
  
    if (!obj_p)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }
	
    /* Setup object name */

    rdd_rtrim((PTR)obj_p->dd_o1_objname,
	      (PTR)obj_p->dd_o2_objowner,
	      id_p);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select a.* from iidd_stats a, iidd_columns b where 
    **	  a.table_name  = 'obj_name' and a.table_owner = 'obj_owner' and
    **    a.column_name = b.column_name and b.column_sequence = rdr_hattr_no
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	    "%s %s %s %s '%s' and a.table_owner = '%s' and %s and %s %d;",
	    "select a.num_unique, a.rept_factor, a.has_unique,",
            "a.pct_nulls, a.num_cells, a.column_domain, a.is_complete,",
	    "a.stat_version, a.hist_data_length",
	    "from iidd_stats a, iidd_columns b where a.table_name =",
	    id_p->tab_name,
	    id_p->tab_owner,
	    "b.table_name = a.table_name and b.table_owner = a.table_owner",
	    "b.column_name = a.column_name and b.column_sequence =",
	    global->rdfcb->rdf_rb.rdr_hattr_no);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    {
	RDC_STATS		iidd_stats;

	/* num_unique */
	rq_bind[0].rqb_addr = (PTR)&iidd_stats.num_unique;
	rq_bind[0].rqb_length = sizeof(iidd_stats.num_unique);
	rq_bind[0].rqb_dt_id = DB_FLT_TYPE;
	/* repition factor */
	rq_bind[1].rqb_addr = (PTR)&iidd_stats.rept_factor;
	rq_bind[1].rqb_length = sizeof(iidd_stats.rept_factor);
	rq_bind[1].rqb_dt_id = DB_FLT_TYPE;
        /* has unique value */
	rq_bind[2].rqb_addr = (PTR)iidd_stats.has_unique;
	rq_bind[2].rqb_length = sizeof(RDD_8CHAR);
	rq_bind[2].rqb_dt_id = DB_CHA_TYPE;
        /* pct_nulls */
	rq_bind[3].rqb_addr = (PTR)&iidd_stats.pct_nulls;
	rq_bind[3].rqb_length = sizeof(iidd_stats.pct_nulls);
	rq_bind[3].rqb_dt_id = DB_FLT_TYPE;
        /* num_cells */
	rq_bind[4].rqb_addr = (PTR)&iidd_stats.num_cells;
	rq_bind[4].rqb_length = sizeof(iidd_stats.num_cells); 
	rq_bind[4].rqb_dt_id = DB_INT_TYPE;
        /* column_domain */
	rq_bind[5].rqb_addr = (PTR)&iidd_stats.column_domain;
	rq_bind[5].rqb_length = sizeof(iidd_stats.column_domain); 
	rq_bind[5].rqb_dt_id = DB_INT_TYPE;
        /* is_complete */
	rq_bind[6].rqb_addr = (PTR)iidd_stats.is_complete;
	rq_bind[6].rqb_length = sizeof(RDD_8CHAR);
	rq_bind[6].rqb_dt_id = DB_CHA_TYPE;
        /* stat_version */
	rq_bind[7].rqb_addr = (PTR)iidd_stats.stat_version;
	rq_bind[7].rqb_length = sizeof(RDD_8CHAR);
	rq_bind[7].rqb_dt_id = DB_CHA_TYPE;
        /* hist_data_length */
	rq_bind[8].rqb_addr = (PTR)&iidd_stats.hist_data_length;
	rq_bind[8].rqb_length = sizeof(iidd_stats.hist_data_length); 
	rq_bind[8].rqb_dt_id = DB_INT_TYPE;
    
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
	qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_09_COL;

	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

        if (ddr_p->qer_d5_out_info.qed_o3_output_b)
	{/* Process return stats tuple */ 

	    DB_STATUS	    flush_status = E_DB_OK;

	    /* Allocate spaces for stats info */
            status = rdu_malloc(global, (i4)sizeof(RDD_HISTO),
			         (PTR *)&rdd_statp);

	    if (DB_SUCCESS_MACRO(status))
	    {		    
                global->rdf_ddrequests.rdd_statp = rdd_statp;

		/* 
		** Assign stats info to the histogram info block.
		*/
	  	rdd_statp->f4count 	= NULL;
	  	rdd_statp->f4repf  	= NULL;
		rdd_statp->datavalue 	= NULL;
		rdd_statp->charnunique 	= NULL;
		rdd_statp->chardensity 	= NULL;
		rdd_statp->snunique 	= iidd_stats.num_unique;
		rdd_statp->sreptfact 	= iidd_stats.rept_factor;
		rdd_statp->snumcells 	= iidd_stats.num_cells;
		if (iidd_stats.has_unique[0] == 'Y')
		    rdd_statp->sunique 	= TRUE;
		else
		    rdd_statp->sunique 	= FALSE;
		if (iidd_stats.is_complete[0] == 'Y')
		    rdd_statp->scomplete = TRUE;
		else
		    rdd_statp->scomplete = FALSE;
		rdd_statp->sdomain 	= iidd_stats.column_domain;
		rdd_statp->snull 	= iidd_stats.pct_nulls;
		rdd_statp->shistlength	= iidd_stats.hist_data_length;

		MEcopy(iidd_stats.stat_version,
		       sizeof(DB_STAT_VERSION),rdd_statp->sversion);
		rdd_statp->sversion[sizeof(DB_STAT_VERSION)] = '\0';

		/* Determine length of histogram boundary values. If there
		** is a version in the sversion field of iistatistics,use
		** shistlength; otherwise use the one the caller provided.
		*/
		if (STcompare(rdd_statp->sversion, DB_NO_STAT_VERSION))
		{
		    /* There is a version */
		    if (rdd_statp->shistlength <= 0)
		    {
			status = E_DB_ERROR;
			rdu_ierror(global, status, E_RD010C_HIST_LENGTH);
			return(status);
		    }
		}
		else
		{
		    /* There is no version; use length caller provided */
		    rdd_statp->shistlength =
			global->rdfcb->rdf_rb.rdr_cellsize;
		}

                flush_status = rdd_flush(global);
	        if (flush_status)
		    return(flush_status);
	    }
	    else	    
	    {
                flush_status = rdd_flush(global);
	        return(status);
	    }
	}
	else
	{
	    /* No stats tuple was found */

	    global->rdf_ddrequests.rdd_ddstatus |= RDD_NOTFOUND;
	}
    }
    return(status);
}

/*{
** Name: rdd_ghistogram - retrieve histogram information from iidd_histogram in CDB.
**
** Description:
**      This routine generates query to retrieve object description
**	from iidd_ddb_objects in CDB.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**	    .rdf_tobj			temp object descriptor
**
** Outputs:
**      global                          ptr to RDF state descriptor
**	    .rdf_tobj			temp object descriptor
**		.dd_o1_objname		    object name
**		.dd_o2_objowner		    object owner
**		.dd_o3_objid		    object id
**		.dd_o4_qryid		    query id if view
**		.dd_o5_cre_date		    create date
**		.dd_o6_objtype		    object type
**		.dd_o7_alt_date		    alter date
**		.dd_o8_sysobj_b		    TRUE if system object
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	16-jul-92 (teresa)
**	    prototypes
**	09-nov-92 (rganski)
**	    Added allocation and initialization of new RDD_HISTO fields
**	    charnunique and chardensity. The memory is only allocated if the
**	    attribute is of a character type.
**	    For now, these fields are being initialized to constants that will
**	    retain OPF's current behavior. 
**	18-jan-93 (rganski)
**	    Character Histogram Enhancements project, continued:
**	    Changed initialization of cellsize to rdd_statp->shistlength, which
**	    gets read from iidd_stats, or is set to value provided by user.
**	    Removed initialization of character set statistics arrays when
**	    there is a version ID. Instead, read the statistics from the
**	    catalog and copy them into the arrays. This involves extending the
**	    algorithm for copying the histogram tuples into the appropriate
**	    buffers.
**	25-apr-94 (rganski)
**	    b62184: determine the histogram type of the attribute by checking
**	    new rdr_histo_type field of rdf_cb->rdf_rb.
**	30-sept-97 (inkdo01)
**	    Add code for new "f4repf" array in RDD_HISTO for per-cell 
**	    repetition factors (now that I know that Star duplicates the
**	    rdfgetinfo functionality in rdfgetobj).
**	26-Nov-2003 (bolke01, hanal04) Bug 108310 INGSTR44
**	    Revoked change for 108310 submittd as 461461 due to incorrect usage
**	    of a global variable (non re-entrant)
**	    Re-Added the code for correcting statistics which do not have the 
**          per cell repition factors.
**	03-Oct-2008 (kiria01) b120966
**	    Added DB_BYTE_TYPE support to allow for Unicode CE entries
**	    to be treated as raw collation data alongside DB_CHA_TYPE.
*/
DB_STATUS
rdd_ghistogram( RDF_GLOBAL *global)
{
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		flush_status;
    DD_OBJ_DESC		*obj_p = global->rdfcb->rdf_info_blk->rdr_obj_desc;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    RDD_HISTO	        *rdd_statp = global->rdf_ddrequests.rdd_statp;
    DD_PACKET		ddpkt;
    u_i4		onebit = 0x00000001;
    u_i4                bit_mask = 0;
    i4	        tuplecount, tuple1count;
    i4			cellsize;
    i4		seq = 0;
    i4			i;
    f4			*f4count;
    f4			*f4repf; 
    i4			*i4repf;
    i4			f4countlength;
    i4			f4repflength;
    PTR			datavalue;
    i4			histolength;
    i4			*charnunique;
    f4		    	*chardensity;
    i4			charnlength = 0;
    i4		    	chardlength = 0;
    i4			f4countmax, f4repfmax, histomax;
    i4		    	charnmax, chardmax;
    RQB_BIND            rq_bind[RDD_02_COL],	/* for 2 columns */
			*bind_p = rq_bind;
    RDC_HISTOGRAMS	iidd_hist;
    RDD_OBJ_ID		object_id,
			*id_p = &object_id;
    char		qrytxt[RDD_QRY_LENGTH];
    bool		pcrepfs = TRUE;
    i4	        	beginhis;
    i4	        	maxhis;
    i4			copy_amount, total_copied;

    if (!obj_p)
    	obj_p = global->rdf_ulhobject->rdf_sysinfoblk->rdr_obj_desc;
  
    if (!obj_p)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }
	
    /* 
    ** Allocate a histogram information block for the 
    ** requested attribute number if doesn't exist.
    */

    f4countlength = f4repflength = rdd_statp->snumcells * sizeof(f4);
    status = rdu_malloc(global, (i4)(f4countlength+f4repflength), 
			(PTR *)&f4count);
    if (DB_FAILURE_MACRO(status))
	return(status);
    rdd_statp->f4count = f4count;
    f4repf  = &f4count[rdd_statp->snumcells]; /* f4repf follows f4count */
    i4repf = (i4 *)f4repf;		/* for later content check */
    rdd_statp->f4repf = f4repf;

    cellsize = rdd_statp->shistlength;

    histolength = rdd_statp->snumcells * cellsize;
    status = rdu_malloc(global, (i4)histolength, (PTR *)&datavalue);
    if (DB_FAILURE_MACRO(status))
	return(status);
    rdd_statp->datavalue = datavalue;

    /* If histogram is character type, allocate character
    ** statistics arrays
    */
    if (global->rdfcb->rdf_rb.rdr_histo_type == DB_CHA_TYPE ||
	global->rdfcb->rdf_rb.rdr_histo_type == DB_BYTE_TYPE)
    {
	charnlength = cellsize * sizeof(i4);
	status = rdu_malloc(global, (i4)charnlength, (PTR *)&charnunique);
	if (DB_FAILURE_MACRO(status))
	    return(status);
	rdd_statp->charnunique = charnunique;
	
	chardlength = cellsize * sizeof(f4);
	status = rdu_malloc(global, (i4)chardlength, (PTR *)&chardensity);
	if (DB_FAILURE_MACRO(status))
	    return(status);
	rdd_statp->chardensity = chardensity;
	
	if (!STcompare(rdd_statp->sversion, DB_NO_STAT_VERSION))
	{
	    /* There is no version id. Initialize with default
	    ** character set statistics, and set charnlength and
	    ** chardlength to 0 to indicate that there are no character
	    ** set statistics in the catalog.
	    */
	    i4		i;		/* Loop counter */

            pcrepfs = FALSE;
	    
	    for (i = 0; i < cellsize; i++)
	    {
		charnunique[i] = (i4) DB_DEFAULT_CHARNUNIQUE;
		chardensity[i] = (f4) DB_DEFAULT_CHARDENSITY;
                /* Initialise f4repf[i] to ensure pcrepfs is reset correctly */
		f4repf[i]      = (f4)0.0;
	    }
	    f4repflength = charnlength = chardlength = 0;
	}
	else if ( (!STcompare(rdd_statp->sversion, DB_STATVERS_0DBV60)) ||
		  (!STcompare(rdd_statp->sversion, DB_STATVERS_1DBV61A)) ||
		  (!STcompare(rdd_statp->sversion, DB_STATVERS_2DBV62)) ||
		  (!STcompare(rdd_statp->sversion, DB_STATVERS_3DBV63)) ||
		  (!STcompare(rdd_statp->sversion, DB_STATVERS_4DBV70)) ||
		  (!STcompare(rdd_statp->sversion, DB_STATVERS_5DBV634)) )
	{
	    /* There is a version id. Initialize with default of 0.0.
	    ** f4repf[i] will be set to 1.0 to indicate that there are cell 
	    ** repition factor statistics in the catalog later.
	    */
	    i4		i;		/* Loop counter */

            pcrepfs = FALSE;
 
	    for (i = 0; i < cellsize; i++)
	    {
                /* Initialise f4repf[i] to ensure pcrepfs is reset correctly */
		f4repf[i]      = (f4)0.0;
	    }

	    f4repflength = 0;
	}
	else if ( (!STcompare(rdd_statp->sversion, DB_STATVERS_6DBV80)) ||
		  (!STcompare(rdd_statp->sversion, DB_STATVERS_6DBV85)) )
        {
            pcrepfs = TRUE;
        }
	else 
	{
            pcrepfs = TRUE;
	    TRdisplay ("%@ RDF (STAR) STATISTICS WARNING: Unknown Database compatibility version (%s).\n", rdd_statp->sversion);
	}
    }
    
    /* tuplecount is the number of histogram tuples.  The formula
    ** will compute the required histogram tuples to store entire
    ** histogram.  Note that DB_OPTLENGTH is currently set to 228
    ** which is the size of text_segment in iidd_histgrams. 
    **
    ** tuple1count is the count of tuples for a 2.0 or later 
    ** histogram which includes the computed per-cell repetition 
    ** factors. Both counts are computed to allow us to build default
    ** per-cell rep factors from pre-2.0 histograms. */

    tuplecount = (histolength + f4countlength + charnlength + chardlength +
		  DB_OPTLENGTH - 1) / DB_OPTLENGTH;
    tuple1count = (histolength + f4countlength + charnlength + chardlength +
		  f4repflength + DB_OPTLENGTH - 1) / DB_OPTLENGTH;

    /* Setup object name */

    rdd_rtrim((PTR)obj_p->dd_o1_objname,
	      (PTR)obj_p->dd_o2_objowner,
	      id_p);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select a.* from iidd_histograms a, iidd_columns b where 
    **	  a.table_name  = 'obj_name' and a.table_owner = 'obj_owner' and
    **    a.column_name = b.column_name and b.column_sequence = rdr_hattr_no
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	    "%s '%s' and a.table_owner = '%s' and %s and %s %d order by a.text_sequence;", 	    
 	    "select a.text_sequence, a.text_segment from iidd_histograms a, \
iidd_columns b where a.table_name =",
	    id_p->tab_name,
	    id_p->tab_owner,
	    "b.table_name = a.table_name and b.table_owner = a.table_owner",
	    "b.column_name = a.column_name and b.column_sequence =",
	    global->rdfcb->rdf_rb.rdr_hattr_no);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    /* text_sequence */
    rq_bind[0].rqb_addr = (PTR)&iidd_hist.sequence;
    rq_bind[0].rqb_length = sizeof(iidd_hist.sequence);
    rq_bind[0].rqb_dt_id = DB_INT_TYPE;
    /* text segment */
    rq_bind[1].rqb_addr = (PTR)iidd_hist.optdata;
    rq_bind[1].rqb_length = sizeof(iidd_hist.optdata);
    rq_bind[1].rqb_dt_id = DB_CHA_TYPE;
    
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_02_COL;

    /* The following are boundaries in an imaginary buffer containing
    ** all the data for this attribute in iihistogram.
    ** They are used when copying the data from tuples to the destination
    ** structures.
    */
    f4countmax 	= f4countlength;
    histomax	= f4countmax + histolength;
    charnmax	= histomax + charnlength;
    chardmax	= charnmax + chardlength;
    f4repfmax	= chardmax + f4repflength;
    copy_amount = 0;
    total_copied = 0;

    do
    {
	status = rdd_fetch(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

        if (ddr_p->qer_d5_out_info.qed_o3_output_b)
        {  
	    u_i4        tmp_mask;

	    /* 
	    ** Start the conversion of histogram data into a form
	    ** which can be directly used by OPF.  Note that in standard
	    ** catalog the sequence number is one based (starts from 1) 
	    ** which is different form zero based dbms catalog.
	    */
		    
	    if (
		((iidd_hist.sequence - 1) < 0)
		||
		((iidd_hist.sequence - 1) >= tuple1count)
	       )
	    {
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD0012_BAD_HISTO_DATA);
		flush_status = rdd_flush(global);
		return(status);
	    }

	    if (iidd_hist.sequence < 33)
	    {/* Only check up to 32 histo tuples */
		tmp_mask = onebit << (iidd_hist.sequence - 1);

		/* is this sequence number already processed */

		if (bit_mask & tmp_mask)
		{
		    /* we encounter duplicate sequence */

		    status = E_DB_ERROR;
		    rdu_ierror(global, status, E_RD0012_BAD_HISTO_DATA);
		    flush_status = rdd_flush(global);
		    return(status);
		}
		
		bit_mask |= tmp_mask;
	    }
		
	    seq++;	 /* Keep a count of retrieve tuples */		    

	    /* is the sequence greater computed tuple count */

	    if (seq > tuple1count)
	    {
	    	/* Overflow error, flush buffer and return error */
		status = E_DB_ERROR;

		/* We have a problem: Display information to the STAR DBMS log
		*/
	        TRdisplay("rdfgetobj.c:rdd_ghistogram:%d:,seq=%d,tup=%d,tup1=%d,cp=%d,ttl=%d\n",
		   __LINE__,seq,tuplecount,tuple1count,copy_amount,total_copied);

		rdu_ierror(global, status, E_RD0012_BAD_HISTO_DATA);
		flush_status = rdd_flush(global);
		return(status);
	    }

	    beginhis = (iidd_hist.sequence - 1) * DB_OPTLENGTH;
	    maxhis = beginhis + DB_OPTLENGTH;

	    /* The histogram was stored in system catalog with 
	    ** f4count + datavalue and in this order, and possibly charnunique
	    ** andchardensity after that, in that order.  So we need to fill
	    ** f4count first, then, fill datavalue, then charnunique, then
	    ** chardensity.
	    **
	    ** And for 2.0 or later histograms, the per-cell repetition factor
	    ** array is stored at the end (whether or not charnunique and/or
	    ** chardensity are present). So after everything else, the rep
	    ** factors are also copied. */

	    copy_amount = 0;
	    total_copied =0;

	    /* Get the f4count values */
	    if (beginhis < f4countmax)
	    {
		copy_amount = min(maxhis, f4countmax) - beginhis;
		MEcopy((PTR) iidd_hist.optdata,
		       copy_amount,
		       (PTR) ((char *)f4count + beginhis));
		beginhis	+= copy_amount;
		total_copied 	+= copy_amount;
            }
	    /* Get the histogram boundary values */
	    if (maxhis > f4countmax && beginhis < histomax)
	    {
		copy_amount = min(maxhis, histomax) - beginhis;
		MEcopy((PTR) (iidd_hist.optdata + total_copied),
		       copy_amount,
		       (PTR) ((char *)datavalue + (beginhis -
						   f4countmax)));
		beginhis	+= copy_amount;
		total_copied 	+= copy_amount;
            }
	    /* Get the char set nunique values */
	    if (maxhis > histomax && beginhis < charnmax)
	    {
		copy_amount = min(maxhis, charnmax) - beginhis;
		MEcopy((PTR) (iidd_hist.optdata + total_copied),
		       copy_amount,
		       (PTR) ((char *)charnunique + (beginhis -
						     histomax)));
		beginhis	+= copy_amount;
		total_copied 	+= copy_amount;
            }
	    /* Get the char set density values */
	    if (maxhis > charnmax && beginhis < chardmax)
	    {
		copy_amount = min(maxhis, chardmax) - beginhis;
		MEcopy((PTR) (iidd_hist.optdata + total_copied),
		       copy_amount,
		       (PTR) ((char *)chardensity + (beginhis -
						     charnmax)));
		beginhis	+= copy_amount;
		total_copied 	+= copy_amount;
            }
	    /* Get the f4repf values */
	    if (pcrepfs && maxhis > chardmax && beginhis < f4repfmax)
	    {
		copy_amount = min(maxhis, f4repfmax) - beginhis;
		MEcopy((PTR) (iidd_hist.optdata + total_copied),
			copy_amount,
			(PTR) ((char *)f4repf + (beginhis -
						      chardmax)));
		beginhis	+= copy_amount;
		total_copied	+= copy_amount;
            }
	}

    } while(ddr_p->qer_d5_out_info.qed_o3_output_b);

    if (seq < tuplecount)
    {	/* Return underflow error. */
	flush_status = E_DB_ERROR;
        /* We have a problem: Display information to the STAR DBMS log
        */
        TRdisplay("rdfgetobj.c:rdd_ghistogram:%d:,seq=%d,tup=%d,tup1=%d,cp=%d,ttl=%d\n",
                __LINE__,seq,tuplecount,tuple1count,copy_amount,total_copied);
	rdu_ierror(global, flush_status, E_RD0012_BAD_HISTO_DATA);
	return(flush_status);
    }

    /* Finally, check for repetition factor array (f4repf). If all values
    ** are (i4)0, it wasn't on the catalog (a pre-2.0 histogram) and must 
    ** be set to the relation-wide repetition factor value here. */
    if (pcrepfs)
    {
        for (pcrepfs = FALSE, i = 0; i < rdd_statp->snumcells && !pcrepfs; i++)
            if (i4repf[i] != 0) pcrepfs = TRUE;
    }

    if (!pcrepfs)
      for (i = 0; i < rdd_statp->snumcells; i++)
        f4repf[i] = rdd_statp->sreptfact;
                                /* wasn't on catalog - force to sreptfact */
    return(status);
}


/*{
** Name: rdd_commit - commit to release all the locks gotten on the CDB catalogs
**
** Description:
**      This routine requests QEF to commit the CDB association to release
**	all the locks that have been acquired so far to assure that no locks
**	are left behind.
**
**	Note that QEF will so commit only if DDL_CONCURRENCY is ON.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**
** Outputs:
**      none
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-sep-89 (carl)
**         initial creation
**	20-jun-90 (teresa)
**	   added new parameter to rdu_ferror() for bug 4390.
**	16-jul-92 (teresa)
**	    prototypes
*/

DB_STATUS
rdd_commit( RDF_GLOBAL *global)
{
    DB_STATUS		status = E_DB_OK;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
/*
    DD_PACKET		ddpkt;
    char		qrytxt[RDD_QRY_LENGTH];
    DB_STATUS		flush_status;
    DD_OBJ_DESC		*obj_p = global->rdfcb->rdf_info_blk->rdr_obj_desc;
    RDD_HISTO	        *rdd_statp = global->rdf_ddrequests.rdd_statp;
    u_i4		onebit = 0x00000001;
    u_i4                bit_mask = 0;
    i4	        tuplecount;
    i4		seq = 0;
    f4			*f4count;
    i4			f4countlength;
    PTR			datavalue;
    i4			datalength;
    RQB_BIND            rq_bind[RDD_02_COL],
			*bind_p = rq_bind;
    RDC_HISTOGRAMS	iidd_hist;
    RDD_OBJ_ID		object_id,
			*id_p = &object_id;
*/


    qefrcb->qef_modifier = 0;
    status = qef_call(QED_6RDF_END, ( PTR ) qefrcb);
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &qefrcb->error,
	    E_RD0250_DD_QEFQUERY,0); 
	return(status);
    }

    return(E_DB_OK);
}


/*{
** Name: rdd_gobjbase - read object base from iidd_ddb_object_base in CDB
**
** Description:
**      This routine sends a query to read the current object base from the
**	iidd_ddb_object_base catalog in the CDB as a means to synchnronize
**	with DDL statements to avoid deadlocks.
**
**	Since this routine is called by rdd_getobjinfo, the read lock on 
**	iidd_ddb_object_base must be committed before rdd_getobjinfo
**	returns.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**
** Outputs:
**      none
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-sep-89 (carl)
**         initial creation
**	20-jun-90 (teresa)
**	   added new parameter to rdu_ferror() for bug 4390.
**	09-jul-90 (teresa)
**	   changed "select *" to "select object_base"
**	16-jul-92 (teresa)
**	    prototypes
*/

DB_STATUS
rdd_gobjbase( RDF_GLOBAL *global)
{
    DB_STATUS		status = E_DB_OK;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET		ddpkt;
    char		qrytxt[RDD_QRY_LENGTH];
/*
    DB_STATUS		flush_status;
    DD_OBJ_DESC		*obj_p = global->rdfcb->rdf_info_blk->rdr_obj_desc;
    RDD_HISTO	        *rdd_statp = global->rdf_ddrequests.rdd_statp;
    u_i4		onebit = 0x00000001;
    u_i4                bit_mask = 0;
    i4	        tuplecount;
    i4		seq = 0;
    f4			*f4count;
    i4			f4countlength;
    PTR			datavalue;
    i4			datalength;
    RQB_BIND            rq_bind[RDD_02_COL],
			*bind_p = rq_bind;
    RDC_HISTOGRAMS	iidd_hist;
    RDD_OBJ_ID		object_id,
			*id_p = &object_id;
*/


    status = rdd_commit(global);
    if (status)
	return(status);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select * from iidd_ddb_object_base;
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = 
	STprintf(qrytxt, "select object_base from iidd_ddb_object_base;");

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len =
			 (i4)STlength((char *)qrytxt);
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;
    ddr_p->qer_d2_ldb_info_p = (DD_1LDB_INFO *)NULL;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return(status); 

    /* obtained table read lock on iidd_ddb_object_base */

    status = rdd_flush(global);
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &qefrcb->error,
	    E_RD0252_DD_QEFFLUSH,0); 
	return(status);
    }

    return(E_DB_OK);
}


/*{
** Name: rdd_getobjinfo	- retrieve distributed object information.
**
** Description:
**      This routine acts as an entry point to retrieve distributed object info.
**	Currently, only one request is allowed in each call.
**
** Inputs:
**      global				    ptr to RDF state descriptor
**	    .rdf_ddrequests		    distributed request block
**		.rdd_types_mask		    type of information requested
**		    RDD_OBJECT		    object info
**		    RDD_ATTRIBUTE	    attribute info
**		    RDD_MAPPEDATTR	    mapped attribute info
**		    RDD_INDEX		    index info
**		    RDD_STATISTICS	    statistics info
**		
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	07-sep-89 (carl)
**	    added call to rdd_gobjbase to synchronize with DDL statements
**	    on CDB catalog access and call to qed_r6_commit to remove the
**	    read lock on iidd_ddb_object_base
**	08-oct-89 (carl)
**	    removed call to rdd_gobjbase since QEF is committing every
**	    CDB query from RDF to avoid deadlocking CDB catalog updaters;
**	    however, RDF must observe the rule to commit this call before
**	    returning to th caller
**	16-jul-92 (teresa)
**	    prototypes
*/

DB_STATUS
rdd_getobjinfo( RDF_GLOBAL *global)
{
    DB_STATUS	status,
		x_status;
    QEF_RCB	*qefrcb = &global->rdf_qefrcb;
    
    /* read lock gotten on iidd_ddb_object_base */
    switch(global->rdf_ddrequests.rdd_types_mask)
    {

	/* 1. retrieve object information */
	case RDD_OBJECT:
	    status = rdd_gobjinfo(global);
	    break;

	/* 2. retrieve attribute information */
	case RDD_ATTRIBUTE:
	    status = rdd_gattr(global);
	    break;

	/* 3. retrieve mapped attribute information */
	case RDD_MAPPEDATTR:
	    status = rdd_gmapattr(global);
	    break;

	/* 4. retrieve index information */
	case RDD_INDEX:
	    status = rdd_gindex(global);
	    break;

	/* 5. retrieve statistics information */
	case RDD_STATISTICS:
	    status = rdd_gstats(global);
	    break;

	/* 6. retrieve statistics information */
	case RDD_HSTOGRAM:
	    status = rdd_ghistogram(global);
	    break;
    
	/* unknown requests */
	default:
	{
	    /* must commit to release the read lock on iidd_ddb_object_base */

	    x_status = rdd_commit(global);	/* ignore error if any */

	    rdu_ierror(global, E_DB_ERROR, E_RD0272_UNKNOWN_DDREQUESTS);
	    return(E_DB_ERROR);			
	}
    }

    /* commit to release all CDB catalog locks before returning */

    x_status = rdd_commit(global);
    if (status == E_DB_OK)
	return(x_status);
    else
	return(status);
}		

/*{
** Name: rdd_lcltab_exists - see if a table exists in the LDB
**
** Description:
**      This routine queries the LDB to see if the table still exists in the
**	local database.  It is only called if RDR_USERTAB is set in
**	RDF_RB.rdr_types_mask.  This should only occur for drop statements.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**
** Outputs:
**	global->rdfcb->rdf_error.err_code
**		    E_RD0268_LOCALTABLE_NOTFOUND => table does not exist in LDB.
**      Returns:
**          DB_STATUS
**		E_DB_OK			    LDB table exists in LDB
**		E_DB_ERROR		    Error encountered, see 
**					    rdf_error.err.code
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      20-nov-92 (teresa)
**	    Initial creation to fix a bug.
**	06-mar-96 (nanpr01)
**	    Standard catalogue interface change for variable page size project.
**	    Needs space for only 2 columns.
*/
DB_STATUS rdd_lcltab_exists(RDF_GLOBAL  *global)
{
    DB_STATUS		status = E_DB_OK;
    DD_OBJ_DESC		*obj_p = &global->rdf_tobj;
    QEF_RCB             *qefrcb = &global->rdf_qefrcb;
    QEF_DDB_REQ		*ddr_p = &qefrcb->qef_r3_ddb_req;
    DD_PACKET           ddpkt;
    RQB_BIND            rq_bind[RDD_50_COL],	/* for 50 columns */
			*bind_p = rq_bind;
    char		qrytxt[RDD_QRY_LENGTH];
    DD_2LDB_TAB_INFO    *ldbtab_p = &obj_p->dd_o9_tab_info;
    u_i4		stamp1;
    u_i4		stamp2;
    RDD_OBJ_ID		table_id,
			*id_p = &table_id;
    DD_1LDB_INFO	ldb_info;


    rdd_rtrim((PTR)ldbtab_p->dd_t1_tab_name,
	    (PTR)ldbtab_p->dd_t2_tab_owner,
	    id_p);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p = &ddpkt;

    /* set up query 
    **
    **	select table_relstamp1, table_relstamp2 from iitables where
    **           table_name = 'table_name' and table_owner = 'table_owner'
    **
    */

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p2_pkt_p = STprintf(qrytxt,
	"%s '%s' and table_owner = '%s';", 	    
	"select table_relstamp1, table_relstamp2 from iitables where table_name =",
	id_p->tab_name, 
	id_p->tab_owner);

    ddr_p->qer_d4_qry_info.qed_q4_pkt_p->dd_p1_len = 
	(i4) STlength((char *)qrytxt);

    /* Assign ldb info */
    STRUCT_ASSIGN_MACRO(*ldbtab_p->dd_t9_ldb_p, ldb_info);
    ldb_info.dd_i1_ldb_desc.dd_l1_ingres_b = FALSE;
    ddr_p->qer_d2_ldb_info_p = &ldb_info;
    ddr_p->qer_d4_qry_info.qed_q3_qmode = DD_1MODE_READ;

    status = rdd_query(global);	    /* send query to qef */
    if (DB_FAILURE_MACRO(status))
	return (status); 

    /* table_relstamp1 */
    rq_bind[0].rqb_addr = (PTR)&stamp1;
    rq_bind[0].rqb_length = sizeof(stamp1);
    rq_bind[0].rqb_dt_id = DB_INT_TYPE;

    /* table_relstamp2 */
    rq_bind[1].rqb_addr = (PTR)&stamp2;
    rq_bind[1].rqb_length = sizeof(stamp2);
    rq_bind[1].rqb_dt_id = DB_INT_TYPE;

    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o2_bind_p = (PTR)bind_p;
    qefrcb->qef_r3_ddb_req.qer_d5_out_info.qed_o1_col_cnt = RDD_02_COL;

    status = rdd_fetch(global);
    if (DB_FAILURE_MACRO(status))
	return(status);

    if (!ddr_p->qer_d5_out_info.qed_o3_output_b)
    {  /* no object was found */
	status = E_DB_ERROR;
	SETDBERR(&global->rdfcb->rdf_error, 0, E_RD0268_LOCALTABLE_NOTFOUND);
	return(status);
    }

    /* flush */
    status = rdd_flush(global);
    return(status);
}
