/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <st.h>
#include    <cm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <adf.h>
#include    <ade.h>
#include    <cui.h>
#include    <dudbms.h>
#include    <dmf.h> 
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <ex.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefgbl.h>
#include    <tm.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: QELAUX.C - Auxiliary routines for CREATE/DESTROY LINK
**
**  Description:
**      Utility routines for CREATE/DESTROY LINK:
**
**	    qel_a0_map_col	    - map local column name to link column name
**	    qel_a1_min_cap_lvls	    - compute existing minimum levels
**				      of specific LDB capabilities
**	    qel_a2_case_col	    - case translate LDB col name to DDB col
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**	01-jun-93 (barbara)
**	    Added qel_a2_case_col for support of delimited identifiers.
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**      20-jan-94 (heleny)
**          rewrite qel_a2_case_col to always check for delimit support.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**/


/*{
** Name: qel_a0_map_col - Map and return the link column name
**
** Description:
**	This function compares a column name against the local name list
**  and returns the link's corresponding column name if there is a match.
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	i_name_p		    ptr to local column name
**
** Outputs:
**	boolean			    TRUE if mapped column found, FALSE otherwise
**	o_coldesc_pp		    DDB column name in array
**	i_qer_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
**	25-jan-89 (carl)
**	    modified qel_a1_min_cap_lvls to process UNIQUE_KEY_REQ
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


bool
qel_a0_map_col(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p,
char		*i_name_p,
DD_COLUMN_DESC	**o_coldesc_pp )
{
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_2LDB_TAB_INFO
		    *dd_info_p = ddl_p->qed_d6_tab_info_p;
    DD_COLUMN_DESC  **ldbcols_pp = dd_info_p->dd_t8_cols_pp,
					    /* ptr to LDB list */
		    **ddbcols_pp = ddl_p->qed_d4_ddb_cols_pp,
					    /* ptr to DDB list */
		    *ldbdesc_p;		    /* use to pt at LDB column name
					    ** array */
    i4		    i;			    /* loop control variable */
    bool	    found_b = FALSE;	    /* assume */


    ldbcols_pp++;			    /* must skip first slot */ 
    ddbcols_pp++;			    /* must skip first slot */ 

    for (i = 0; ! found_b && i < v_lnk_p->qec_4_col_cnt; 
	 i++, ldbcols_pp++, ddbcols_pp++)
    {
	ldbdesc_p = *ldbcols_pp;	    /* ptr to column descriptor */

	if (MEcmp(i_name_p, ldbdesc_p->dd_c1_col_name, DB_MAXNAME)
	    == 0)
	{
	    found_b = TRUE;
	    *o_coldesc_pp = *ddbcols_pp;
	}
    }
    return(found_b);
}    


/*{
** Name: qel_a1_min_cap_lvls - Derive from IIDD_DDB_LDB_DBCAPS minimum
**			       levels of known LDB capabililities
**
** Description:
**	This routine computes the MIN of each known LDB capability
**  in IIDD_DDB_LDB_DBCAPS.
**
** Inputs:
**	i_use_pre_b			TRUE if values to be returned in
**					qec_25_pre_mins_p, FALSE if returned
**					in qec_26_aft_mins_p
**      i_qer_p				QEF_RCB
**	v_lnk_p				ptr to QEC_LINK
**
** Outputs:
**	v_lnk_p
**	    .qec_25_pre_mins_p | .qec_26_aft_mins_p
**		.qec_m1_com_sql		i2
**		.qec_m2_ing_quel	i2
**		.qec_m3_ing_sql		i2
**		.qec_m4_savepts		i2
**		.qec_m5_uniqkey		i2
**		.qec_m6_opn_sql		i2
**	i_qer_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
**	25-jan-89 (carl)
**	    modified to process UNIQUE_KEY_REQ
**	01-mar-93 (barbara)
**	    Star delimited id support.  Adjust iidd_dbcapabilities based
**	    on minimum OPEN/SQL_LEVEL.
**	02-sep-93 (barbara)
**	    Initialize qec_m6_opn_sql before for querying minimum
**	    OPEN/SQL_LEVEL.  Changed #define regarding querying for minimum
**	    iidd_ddb_ldb_dbcaps capability from SEL_210_OPN_SQL_LVL to
**	    SEL_210_MIN_OPN_SQL_LVL.
*/


DB_STATUS
qel_a1_min_cap_lvls(
bool		i_use_pre_b,
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_MIN_CAP_LVLS  
		    *mins_p;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;



    if (i_use_pre_b)
	mins_p = v_lnk_p->qec_25_pre_mins_p;
    else
	mins_p = v_lnk_p->qec_26_aft_mins_p;

    mins_p->qec_m1_com_sql = 0;
    mins_p->qec_m2_ing_quel = 0;
    mins_p->qec_m3_ing_sql = 0;
    mins_p->qec_m4_savepts = 0;
    mins_p->qec_m5_uniqkey = 0;
    mins_p->qec_m6_opn_sql = 0;

    /* 1.  retrieve MIN(cap_level) from IIDD_DDB_LDB_DBCAPS where
    **	   cap_capability = 'COMMON/SQL_LEVEL' */

    sel_p->qeq_c1_can_id = SEL_200_MIN_COM_SQL;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.min_cap_lvls_p = mins_p;
    sel_p->qeq_c4_ldb_p = cdb_p;

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {
	status = qel_s3_flush(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    /* 2.  retrieve MIN(cap_level) from IIDD_DDB_LDB_DBCAPS where
    **	   cap_capability = 'INGRES/QUEL_LEVEL' */

    sel_p->qeq_c1_can_id = SEL_201_MIN_ING_QUEL;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.min_cap_lvls_p = mins_p;
    sel_p->qeq_c4_ldb_p = cdb_p;

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {
	status = qel_s3_flush(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    /* 3.  retrieve MIN(cap_level) from IIDD_DDB_LDB_DBCAPS where
    **	   cap_capability = 'INGRES/SQL_LEVEL' */

    sel_p->qeq_c1_can_id = SEL_202_MIN_ING_SQL;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.min_cap_lvls_p = mins_p;
    sel_p->qeq_c4_ldb_p = cdb_p;

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {
	status = qel_s3_flush(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    /* 4.  retrieve MIN(cap_level) from IIDD_DDB_LDB_DBCAPS where 
    **	   cap_capability = 'SAVEPOINTS' */

    sel_p->qeq_c1_can_id = SEL_203_MIN_SAVEPTS;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.min_cap_lvls_p = mins_p;
    sel_p->qeq_c4_ldb_p = cdb_p;

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {
	status = qel_s3_flush(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }


    /* 5.  retrieve MIN(cap_level) from IIDD_DDB_LDB_DBCAPS where 
    **	   cap_capability = 'UNIQUE_KEY_REQ' */

    sel_p->qeq_c1_can_id = SEL_204_MIN_UNIQUE_KEY_REQ;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.min_cap_lvls_p = mins_p;
    sel_p->qeq_c4_ldb_p = cdb_p;

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {
	status = qel_s3_flush(i_qer_p, v_lnk_p);
        return(status);
    }

    /* 6.  retrieve MIN(cap_level) from IIDD_DDB_LDB_DBCAPS where
    **	   cap_capability = 'OPEN/SQL_LEVEL' */

    sel_p->qeq_c1_can_id = SEL_210_MIN_OPN_SQL_LVL;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.min_cap_lvls_p = mins_p;
    sel_p->qeq_c4_ldb_p = cdb_p;

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {
	status = qel_s3_flush(i_qer_p, v_lnk_p);
    }

    return (status);
}    


/*{
** Name: qel_a2_case_col - Translate case of LDB column name in place
**
** Description:
**	This function performs case mapping of an LDB column name for
**  insertion into the DDB's cataglogs.	
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	i_colptr	    	    ptr to local column name
**	i_xlate			    case translation semantics of ddb
**					these bits may be set:
**			 		CUI_ID_REG_U
**					CUI_ID_DLM_U or CUI_ID_DLM_M
**	i_ldb_caps		    ptr to capabilities of LDB
**
** Outputs:
**	i_colptr		    ptr to translated column name
**
**      Returns:
**          VOID                 
**
** Side Effects:
**          none
**
** History:
**	01-jun-93 (barbara)
**          written
**	02-sep-93 (barbara)
**	    Wasn't handling the case of mapping a name from a
**	    lower/lower LDB to an upper/mixed DDB.
**      20-Jan-94 (heleny)
**          fix b58094. rewritten. handle pre-65 & gateway ldb 
**          which does not support delimiter.
*/


VOID
qel_a2_case_col(
QEF_RCB		*i_qer_p,
char		*i_colptr,
u_i4	i_xlate,
DD_CAPS		*i_ldb_caps)
{
    char	*last = i_colptr + sizeof(DD_NAME) -1,
		*colptr = i_colptr;

    if (i_xlate & CUI_ID_DLM_U)             /* upper delimiter */
    {
	if (i_ldb_caps->dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)	
					/* ldb support delimiter */
	{
		if (i_ldb_caps->dd_c10_delim_name_case != DD_2CASE_UPPER)
		{
	    		while (colptr <= last)
	    		{
				CMtoupper(colptr, colptr);
				CMnext(colptr);
	    		}
		}
    	}
	else 		/* ldb does not support delimiter */
	{
		if (i_ldb_caps->dd_c6_name_case == DD_0CASE_LOWER)
		{
	    		while (colptr <= last)
	    		{
				CMtoupper(colptr, colptr);
				CMnext(colptr);
	    		}
		}
   	}
   } 
   
   else if (i_xlate & CUI_ID_DLM_M) 	/* mixed case DDB */
   {
	if (i_xlate & CUI_ID_REG_U)
	{
		if (i_ldb_caps->dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)	
					/* ldb support delimiter */
		{ 
			if (i_ldb_caps->dd_c10_delim_name_case ==DD_0CASE_LOWER)
			{
	    			while (colptr <= last)
	    			{
					CMtoupper(colptr, colptr);
					CMnext(colptr);
	    			}
			}
		}
		else    /* ldb does not support delimiter */
	        {
			if (i_ldb_caps->dd_c6_name_case == DD_0CASE_LOWER)
			{
	    			while (colptr <= last)
	    			{
					CMtoupper(colptr, colptr);
					CMnext(colptr);
	    			}
			}
   		}
	}     
	else   /* CUI_ID_REG_L */
	{
		if (i_ldb_caps->dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)	
					/* ldb support delimiter */
		{
			if (i_ldb_caps->dd_c10_delim_name_case== DD_2CASE_UPPER)
	    		{
				while (colptr <= last)
				{
		    			CMtolower(colptr, colptr);
		    			CMnext(colptr);
				}
	    		}
                }
		else  /* ldb does not support delimiter */
		{
	   		if (i_ldb_caps->dd_c6_name_case == DD_2CASE_UPPER)
	    		{
				while (colptr <= last)
				{
		    			CMtolower(colptr, colptr);
		    			CMnext(colptr);
				}
	    		}
		}
        }	
   }
   else         /* default - lower */
   {
	if (i_ldb_caps->dd_c1_ldb_caps & DD_8CAP_DELIMITED_IDS)	
					/* ldb support delimiter */
    	{
		if (i_ldb_caps->dd_c10_delim_name_case != DD_0CASE_LOWER)
		{
	    		while (colptr <=last)
	    		{
				CMtolower(colptr, colptr);
				CMnext(colptr);
	    		}
		}
    	}
	else  /* ldb does not support delimiter */
	{
   		if (i_ldb_caps->dd_c6_name_case == DD_2CASE_UPPER)
    		{
			while (colptr <= last)
			{
    				CMtolower(colptr, colptr);
    				CMnext(colptr);
			}
    		}
	}
   }
}
