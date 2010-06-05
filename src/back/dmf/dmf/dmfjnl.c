/*
**Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <id.h>
# include <tm.h>
# include <cv.h>
# include <pc.h>
# include <cm.h>
# include <st.h>
# include <nm.h>
# include <cs.h>
# include <ck.h>
# include <jf.h>
# include <tr.h>
# include <si.h>
# include <lo.h>
# include <bt.h>
# include <er.h>
# include <ex.h>
# include <pm.h>
# include <me.h>
# include <nm.h>
# include <di.h>
# include <iicommon.h>
# include <dbdbms.h>
# include <adf.h>
# include <lk.h>
# include <lg.h>
# include <dudbms.h>
# include <ulf.h>
# include <dmf.h>
# include <dmccb.h>
# include <dmrcb.h>
# include <dmtcb.h>
# include <dmxcb.h>
# include <dm.h>
# include <dml.h>
# include <dmp.h>
# include <dm2d.h>
# include <dm2r.h>
# include <dm2t.h>
# include <dm0c.h>
# include <dm0j.h>
# include <dm0m.h>
# include <dmfjsp.h>
#include  <dm0llctx.h>

/*
**  dmfjnl.c - DMF journal module.
**
**  Description:
**      This file contains the function calls for journaling id case semantics
**      These functions have been moved from dmfjsp.
**  Hitstory:
**      24-Oct-95 (fanra01)
**          Created.
**	15-feb-96 (emmag)
**	    Added open_iidbdb.     
**	20-feb-1996 (canor01)
**	    Made build_loc exportable.
**	23-sep-1996 (canor01)
**	    Updated.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**      27-feb-2003 (stial01)
**          jsp_file_maint() changes to delete checkpoint DIRECTORY (b109738)
**      28-feb-2003 (stial01)
**          jsp_file_maint() Define location bufs big enough for null terminator
**      13-Apr-2004 (nansa02)
**          Added support for -delete_invalid_ckp.
**      02-mar-2005 (stial01)
**          Increase sizeof line_buffer to hold directory and filename (b113999)
**      18-may-2005 (bolke01)
**          Moved all tbllst_* functions from dmfjsp.c as part of VMS Cluster 
**	    port (s114136)
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	13-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0d_?, dm0j_?, dm2d_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**      06-Jan-2009 (stial01)
**          jsp_get_case() use DB_DBCAPABITLITIES instead of local struct
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      14-May-2010 (stial01)
**          More Changes for Long IDs
*/

/*
**      Structure originally defined in dmfjsp.c
*/

typedef struct
{
	STATUS	    error;
	char	    *path_name;	
	int	    path_length;
	int	    pad;
}   DELETE_INFO;

DB_STATUS build_loc(
    DB_DB_NAME      *dbname,
    i4               l_path,
    char             *path,
    char             *loc_type,
    DMP_LOC_ENTRY   *location);

DB_STATUS open_iidbdb(
    DMF_JSX	    *jsx,
    DMP_DCB                **iidcbp,
    i4                access_mode,
    i4                lock_mode,
    i4                iidb_flag);
/*{
** Name: jsp_get_case		- Find databse delimited id case semantics
**
** Description:
**
**	Search iirel_idx to find iidbcapabilities table id.
**	Search iidbcapabilites to find the case semantics of the database,
**	then set results into the (database-specific) jsx context.
**
** Inputs:
**      dcb			   Database
**      jsx                        jsx context
**
** Outputs:
**	err_code
**
** Side Effects:
**	    none
**
** History:
**      20-sep-1993 (jnash)
**          Created in support of delim id case semantics in jsp utilites.
**	23-may-1994 (jnash)
** 	    Caller may want to continue on error, so setup the most
** 	    general case semantics as the default.  Remove xDEBUG code.
**	24-jun-1994 (jnash)
** 	    Return defaults for unopened database.
[@history_template@]...
*/
DB_STATUS
jsp_get_case(
DMP_DCB		*dcb,
DMF_JSX		*jsx)
{
    DB_TAB_ID	    	table_id;
    DB_TRAN_ID		tran_id;
    DB_TAB_TIMESTAMP    timestamp;
    DMP_RCB		*r_idx = NULL;		/* rcb for iirel_idx */
    DMP_RCB		*rel = NULL;		/* rcb for iirelation */
    DMP_RCB		*dbc = NULL;		/* rcb for iidbcapabilities */
    DM2R_KEY_DESC	key_desc[2];		/* search key */
    DM_TID		tid;
    DM_TID		rel_tid;
    DMP_RELATION	relation;
    DMP_RINDEX		rindexrecord;
    DB_STATUS		status;
    i4		loc_status;
    i4		loc_err;
    i4		i;
    DB_TAB_NAME		dbcap_name; /* iidbcapabilities */
    DB_OWN_NAME		dollar_ingres;
    char		*cptr;
    DB_DBCAPABILITIES	iidbc; /* iidbcapabilities structure */

#define			LOWER_DBCAP_NAME "iidbcapabilities"
#define			UPPER_DBCAP_NAME "IIDBCAPABILITIES"

    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Caller may want to continue on error, so setup the most
    ** general case semantics as the default.
    */
    jsx->jsx_real_user_case = JSX_CASE_MIXED;
    jsx->jsx_reg_case       = JSX_CASE_MIXED;
    jsx->jsx_delim_case     = JSX_CASE_MIXED;

    /*
    ** Database may not yet be opened, which is ok from here.
    */
    if (dcb == 0)
  	return (E_DB_OK);

    for (;;)
    {
	/*
	**  get iidbcapabilities table id from iirelation, iirel_idx catalogs.
	*/
	table_id.db_tab_base = DM_B_RIDX_TAB_ID;
	table_id.db_tab_index = DM_I_RIDX_TAB_ID;
	tran_id.db_high_tran = 0;
	tran_id.db_low_tran = 0;

	status = dm2t_open(dcb, &table_id, DM2T_IS,   
	    DM2T_UDIRECT, DM2T_A_READ, (i4)0, (i4)20, 
	    0, 0, dmf_svcb->svcb_lock_list, (i4)0,(i4)0, 
	    0, &tran_id, &timestamp, &r_idx, (DML_SCB *)0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break;

	/*  
	** Find entry in iirelation via iirel_idx, with either case.
	*/
	for (;;)
	{
	    MEmove(sizeof(DU_DBA_DBDB) - 1, DU_DBA_DBDB, ' ', 
		sizeof(DB_OWN_NAME), dollar_ingres.db_own_name);
	    MEmove(sizeof(LOWER_DBCAP_NAME) - 1, LOWER_DBCAP_NAME, ' ', 
		sizeof(DB_TAB_NAME), dbcap_name.db_tab_name);

	    key_desc[0].attr_operator = DM2R_EQ;
	    key_desc[0].attr_number = DM_1_RELIDX_KEY;
	    key_desc[0].attr_value = dbcap_name.db_tab_name;
	    key_desc[1].attr_operator = DM2R_EQ;
	    key_desc[1].attr_number = DM_2_RELIDX_KEY;
	    key_desc[1].attr_value = dollar_ingres.db_own_name;

	    /* 
	    ** Retrieve records with this key, should find one with
	    ** either upper or lower case key. 
	    */
	    status = dm2r_position(r_idx, DM2R_QUAL, key_desc, 
		(i4)2, (DM_TID *)0, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
		break;

	    status = dm2r_get(r_idx, &tid, DM2R_GETNEXT, (char *)&rindexrecord, 
		&jsx->jsx_dberr);

	    if (status == E_DB_OK)
	    {
		rel_tid.tid_i4 = rindexrecord.tidp;
		break;
	    }

	    /* 
	    ** If not found, try again w upper case name and owner.
	    */
	    if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
	    {
		MEmove(sizeof(UPPER_DBCAP_NAME) - 1, UPPER_DBCAP_NAME, ' ', 
		    sizeof(DB_TAB_NAME), dbcap_name.db_tab_name);

	        cptr = dollar_ingres.db_own_name;
		for (i = 0; i < sizeof(DB_OWN_NAME); i++)
		{
		    CMtoupper(cptr, cptr);
		    CMnext(cptr);
		}

		status = dm2r_position(r_idx, DM2R_QUAL, key_desc, 
		    (i4)2, (DM_TID *)0, &jsx->jsx_dberr);
		if (status != E_DB_OK)
		    break;	    

		status = dm2r_get(r_idx, &tid, DM2R_GETNEXT, 
		    (char *)&rindexrecord, &jsx->jsx_dberr);
		if (status == E_DB_OK)
		{
		    rel_tid.tid_i4 = rindexrecord.tidp;
		    break;
		}
	    }

	    break;
	}

	if (status != E_DB_OK)
	{
	    if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
		SETDBERR(&jsx->jsx_dberr, 0, E_DM918A_LISTTAB_NONEXISTENT); /* XXX */
	    break;
	}

	/* 
	** iirel_idx no longer needed. 
	*/
	status = dm2t_close(r_idx, DM2T_NOPURGE, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break; 

	/*
	** Open iirelation to get iidbcapabilities table id
	*/
	table_id.db_tab_base = DM_B_RELATION_TAB_ID;
	table_id.db_tab_index = DM_I_RELATION_TAB_ID;
	tran_id.db_high_tran = 0;
	tran_id.db_low_tran = 0;

	status = dm2t_open(dcb, &table_id, DM2T_IS,    
	    DM2T_UDIRECT, DM2T_A_READ, (i4)0, (i4)20, 
	    0, 0, dmf_svcb->svcb_lock_list, (i4)0,(i4)0, 
	    0, &tran_id, &timestamp, &rel, (DML_SCB *)0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break; 

	status = dm2r_get(rel, &rel_tid, DM2R_BYTID, (char *)&relation,
	    &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break; 

	status = dm2t_close(rel, DM2T_NOPURGE, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break; 

	/* 
	** Now open iidbcapilities.
	*/
	table_id.db_tab_base = relation.reltid.db_tab_base;
	table_id.db_tab_index = relation.reltid.db_tab_index;

	status = dm2t_open(dcb, &table_id, DM2T_IS,  DM2T_UDIRECT, DM2T_A_READ,
	    (i4)0, (i4)20, (i4)0, (i4)0, 
	    dmf_svcb->svcb_lock_list, 0, 0, 0, &tran_id,
	    &timestamp, &dbc, (DML_SCB *)0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break; 

	status = dm2r_position(dbc, DM2R_ALL, (DM2R_KEY_DESC *)0, 
	    (i4)0, (DM_TID *)0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break; 

	/*
	** Search thru iidbcapabilities (its a heap) and extract delim id info.
	*/
	for (;;)
	{
	    status = dm2r_get(dbc, &tid, DM2R_GETNEXT, 
				(char *)&iidbc, &jsx->jsx_dberr);
	    if (status != E_DB_OK)  
		break; 

	    /*
	    ** There are three "case cases": 
	    **   regular (non-delim ids),
	    **   delimited id's 
	    **   real users ids (obtained from the operating system).
	    */
	    if (MEcmp(iidbc.cap_capability, "DB_NAME_CASE", 12) == 0)
	    {
		if (MEcmp((char *)iidbc.cap_value, "UPPER", 5) == 0)
		    jsx->jsx_reg_case = JSX_CASE_UPPER;
		else if (MEcmp((char *)iidbc.cap_value, "LOWER", 5) == 0)
		    jsx->jsx_reg_case = JSX_CASE_LOWER;
	    }
	    else if (MEcmp(iidbc.cap_capability, "DB_DELIMITED_CASE", 17) == 0)
	    {
		if (MEcmp((char *)iidbc.cap_value, "UPPER", 5) == 0)
		    jsx->jsx_delim_case = JSX_CASE_UPPER;
		else if (MEcmp((char *)iidbc.cap_value, "LOWER", 5) == 0)
		    jsx->jsx_delim_case = JSX_CASE_LOWER;
	    }
	    else if (MEcmp(iidbc.cap_capability, "DB_REAL_USER_CASE", 17) == 0)
	    {
		if (MEcmp((char *)iidbc.cap_value, "UPPER", 5) == 0)
		    jsx->jsx_real_user_case = JSX_CASE_UPPER;
		else if (MEcmp((char *)iidbc.cap_value, "LOWER", 5) == 0)
		    jsx->jsx_real_user_case = JSX_CASE_LOWER;
	    }
	}

	if (jsx->jsx_dberr.err_code != E_DM0055_NONEXT)
	    break;

	status = dm2t_close(dbc, DM2T_NOPURGE, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break; 

	return(E_DB_OK);
    }

    /*
    ** Error handling.
    */
    if (r_idx)
	loc_status = dm2t_close(r_idx, DM2T_NOPURGE, &local_dberr); 

    if (rel)
	loc_status = dm2t_close(rel, DM2T_NOPURGE, &local_dberr);

    if (dbc)
	loc_status = dm2t_close(dbc, DM2T_NOPURGE, &local_dberr); 

    if (jsx->jsx_dberr.err_code > E_DM_INTERNAL)
    {
	uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    }

    SETDBERR(&jsx->jsx_dberr, 0, E_DM104A_JSP_DELIM_SETUP_ERR);

    return(E_DB_ERROR);
}

/*{
** Name: jsp_set_case		- Apply delimited id case semantics
**
** Description:
**
**	Use delim id case semantics stored in jsp to apply proper case to
**	object passed in.  
**
**	It is best if obj and ret_obj do not overlap, but they may 
**	according to MEcopy() restrictions (dest is lower address than src).
**
** Inputs:
**      jsx                     jsx context
**	case_type		JSX_CASE_UPPER if upper case
**				JSX_CASE_LOWER if lower case
**				JSX_CASE_MIXED if mixed case
**      obj_len			len of object
**      obj			object to be masticated
**
** Outputs:
**      ret_obj			masticated object 
**
** Side Effects:
**	    none
**
** History:
**      20-sep-1993 (jnash)
**          Created in support of delim id case semantics in jsp utilites.
[@history_template@]...
*/
VOID 
jsp_set_case(
DMF_JSX 	*jsx, 
i4		case_type,
i4		obj_len,
char		*obj,
char		*ret_obj)
{
    char    	*cptr;
    char    	*last;

    /*
    ** Copy to destination, leave original unchanged.
    */
    MEcopy(obj, obj_len, ret_obj);

    cptr = ret_obj;
    last = ret_obj + obj_len - 1;

    /*
    ** Perform case translation
    */
    if (case_type == JSX_CASE_UPPER)
    {
	/*
	** Upper case name semantics.
	*/
	while (cptr <= last)
	{
	    CMtoupper(cptr, cptr);
	    CMnext(cptr);
	}
    }
    else if (case_type == JSX_CASE_LOWER)
    {
	/*
	** Lower case name semantics.
	*/
	while (cptr <= last)
	{
	    CMtolower(cptr, cptr);
	    CMnext(cptr);
	}
    }

    /* 
    ** Mixed left alone.
    */

    return;
}

/*{
** Name: jsp_file_maint	    - Checkpoint, dump and journal file maintenence.
**
** Description:
**      This routine manages the destruction of old checkpoints journal and
**	dump files.  It is used by ckpdb and alterdb.
**	
** 	Routine assumes that config file is locked at entry, and is updated
**	after return.
**
**	The routine is called by ckpdb and alterdb -delete_oldest_ckp.
**
** Inputs:
**      jsx                             Journal support context.
**	dcb				Pointer to DCB.
**	config				Pointer to pointer to config file.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      09-mar-1987 (Derek)
**          Created for Jupiter.
**	 7-dec-1989 (rogerk)
**	    When '-d' option is specified, delete old dmp copies of config file.
**	 9-jan-1989 (rogerk)
**	    Make sure we don't reference out-of-bounds in journal history
**	    when there are no previous entries to truncate.
**	    Truncate or delete old dump entries even if offline checkpoint.
**	31-oct-1990 (bryanp)
**	    Bug #34016:
**	    Now that we make a config file dump for offline as well as online
**	    checkpoints, config file dump deletion must be performed as part of
**	    checkpoint deletion, not as part of dump file deletion, otherwise
**	    we erroneously don't delete the config file dumps for offline
**	    checkpoints.
**	15-oct-1991 (mikem)
**	    Eliminated sun compiler warning "warning: statement not reached"
**	    by changing while loop which was only ever executed once to an
**	    "if" statement.
**	15-mar-1993 (rogerk)
**	    Added direction parameter to dm0j_open routine.
**	20-sep-1993 (jnash)
**	    Moved here from dmfcpp.c, called by ckpdb as well as 
**	    alterdb -delete_oldest_ckp.  Truncation of jnl and dump 
**	    files is no longer performed in this routine.
**	18-oct-1993 (jnash)
**	    Issue ALTERDB completion message even if no dump files deleted.
**	31-jan-1994 (jnash)
**	  - During alterdb -delete_oldest_ckp, only delete dump files 
**	    associated with deleted checkpoint.
**	  - Don't pass back nonzero status if dm0d_d_config returns warning.  
**	22-feb-1995 (shero03
**	    Bug #66987
**	    Pass in the index to delete, rather than assuming 0
**      07-mar-1995 (stial01)
**          Bug 67329: there may not be a ckp file for each data 
**          location, even if one doesn't exist, continue to delete others
**	21-aug-1995 (harpa06)
**	    Bug #70655 - It is possible that the checkpoint file where the 
**	    database tables are stored is in fact a directory. If so, delete 
**	    the files in the directory and then remove the directory.
**	 6-feb-1996 (nick)
**	    We weren't deleting the cssss000.lst files from the dump 
**	    location.
**	13-feb-96 (nick)
**	    Call to DIdirdelete() passed a DI_IO pointer which didn't
**	    actually point to anything ; DIdirdelete() sets the io_type
**	    element of the DI_IO as a side effect !
**	24-Jan-2002 (inifa01) bug 106914 INGSRV 1670.
**	    alterdb -delete_oldest_ckp fails to delete dump history
**	    information and dump files if for some reason the checkpoint
**	    and dump sequence numbers go out of sync.
**	    Fix was to search for and remove the corresponding checkpoint
**	    sequence if for an online checkpoint.
**	01-Dec-2003 (zhahu02) 
**          update for alterdb -delete_oldest_ckp INGSRV2570 (bug 111376).
**	29-Jul-2005 (sheco02) 
**	    Somehow the change 471341 (x-integration of 467394) did not get 
**	    through. Re-apply the change 467394 manually.
**      19-oct-06 (kibro01) bug 116436
**          Change jsp_file_maint parameter to be sequence number rather than
**          index into the jnl file array, and add a parameter to specify
**          whether to fully delete (partial deletion leaves journal and
**          checkpoint's logical existence)
**	31-Mar-08 (kibro01) b119961
**	    Remove orphaned dump files prior to the valid checkpoint when
**	    performing a "ckpdb -d".
[@history_template@]...
*/
DB_STATUS
jsp_file_maint(
DMF_JSX		*jsx,
DMP_DCB		*dcb,
DM0C_CNF	**config,
i4		ckp_seq,
bool		fully_delete)
{
    DM0C_CNF            *cnf = *config;
    DB_STATUS		status = E_DB_OK;
    i4		i;
    i4		j;
    i4		local_error;
    CL_ERR_DESC		sys_err;
    DM_FILE		filename;
    char		line_buffer[132 + MAX_LOC + MAX_LOC];
    DMP_EXT             *ext = dcb->dcb_ext;
    i4             ckp_cnt; 
    LOCATION		loc;
    LOCATION		temploc;
    LOINFORMATION 	loinf;
    LO_DIR_CONTEXT	lc;
    i4                  flagword;
    DELETE_INFO		delete_info;
    CL_ERR_DESC         *error_code;
    DI_IO	        f;
    char		ck_loc_buf[MAX_LOC + 1]; /* location string */
    char		ck_dir_buf[MAX_LOC + 1]; /* directory */
    char		save_dir_buf[MAX_LOC + 1]; /* directory */
    char		ck_fn_buf[MAX_LOC +1]; /* filename string */
    char		*ck_fname;
    char		*ck_dir;
    LOCATION		ck_loc;
    LOCATION		save_ck_loc;
    bool		alter_db = FALSE;
    i4			jnl_index;
    i4			dmp_index;
    i4			first_jnl;
    i4			last_jnl;
    i4			first_dmp;
    i4			last_dmp;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    if ((jsx->jsx_status & JSX_ALTDB_CKP_DELETE) ||
	(jsx->jsx_status2 & JSX2_ALTDB_CKP_INVALID))
	alter_db = TRUE;

    /* If ckp_seq_no is 0, this is the call from "delete all checkpoints", 
    ** so loop round until we can go no further
    */
    if (ckp_seq == 0)
    {
	i4 status = E_DB_OK;
	while ((status == E_DB_OK) &&
		(cnf->cnf_jnl->jnl_count > 1) &&
		(cnf->cnf_jnl->jnl_history[0].ckp_sequence != 0))
	{
		status = jsp_file_maint(jsx, dcb, config,
			cnf->cnf_jnl->jnl_history[0].ckp_sequence,
			fully_delete);
	}
	/* Now remove any left-over dump files prior to the first saved
	** checkpoint in the journal list.  This can happen if >100 checkpoints
	** have been taken and some of the journal entries have just been
	** abandoned, possibly leaving an orphan dump entry (kibro01) b119961
	*/
	while ((status == E_DB_OK) &&
		(cnf->cnf_dmp->dmp_count > 1) &&
		(cnf->cnf_dmp->dmp_history[0].ckp_sequence <
			cnf->cnf_jnl->jnl_history[0].ckp_sequence))
	{
		status = jsp_file_maint(jsx, dcb, config,
			cnf->cnf_dmp->dmp_history[0].ckp_sequence,
			fully_delete);
	}
	return (status);
    }

    /*
    ** BUG 67329: count number of data locations checkpointed
    **            For table checkpoints we may be missing some checkpoint files
    **            but we need to delete any that were created.
    */
    for (ckp_cnt = 0, i = 0; i < ext->ext_count; i++)
    {
	/*  Skip journal, dump, work, checkpoint and aliased locations. */
	if (ext->ext_entry[i].flags & (DCB_JOURNAL| DCB_CHECKPOINT |
			DCB_DUMP | DCB_ALIAS | DCB_WORK | DCB_AWORK))
	    continue;
	/* else this location checkpointed */
	ckp_cnt++;
    }

    for (jnl_index = 0; jnl_index < cnf->cnf_jnl->jnl_count; jnl_index++)
    {
	if (cnf->cnf_jnl->jnl_history[jnl_index].ckp_sequence == ckp_seq)
	    break;
    }

    /* If we found this sequence number, go on and delete it... */
    if (jnl_index < cnf->cnf_jnl->jnl_count)
    {
	/* Determine a journal number */
	first_jnl = cnf->cnf_jnl->jnl_history[jnl_index].ckp_f_jnl;
	last_jnl = cnf->cnf_jnl->jnl_history[jnl_index].ckp_l_jnl;

	if (first_jnl && fully_delete)
	{
	    /*  Delete journal files pertaining to this checkpoint. */
	    if (jsx->jsx_status & JSX_VERBOSE)
	    {
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%@ CPP/ALTDB: Deleting journal files for checkpoint %d.\n",
		    ckp_seq);
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%34* Journal file set %d to %d.\n",
		    first_jnl, last_jnl);
	    }

	    if (jsx->jsx_status & JSX_TRACE)
	    {
		TRdisplay("%@ CPP: Deleting journal files for checkpoint %d.\n",
		    ckp_seq);
		TRdisplay("%34* Journal file set %d to %d.\n",
		    first_jnl, last_jnl);
	    }

	    for (; first_jnl <= last_jnl; first_jnl++)
	    {
		status = dm0j_delete(DM0J_MERGE, 
		    (char *)&dcb->dcb_jnl->physical,
		    dcb->dcb_jnl->phys_length, first_jnl,
		    (DB_TAB_ID *)0, &jsx->jsx_dberr);
		if (status != E_DB_OK)
		{
		    dmfWriteMsg(NULL, E_DM1110_CPP_DELETE_JNL, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1112_CPP_JNLDMPCKP_NODEL);
		    return (E_DB_ERROR);
		}
	    }
	}
	
	/*
	**  Delete checkpoint set for this checkpoint, and the associated
	**  config file backup located in the dump directory. Note that we
	**  delete the config file backup as part of the CHECKPOINT deletion
	**  processing because config file backups are made for both online and
	**  offline checkpoints beginning with Ingres 6.3/02.
	*/

	filename.name[0] = 'c';
	filename.name[1] = (ckp_seq / 1000) + '0';
	filename.name[2] = ((ckp_seq / 100) % 10) + '0';
	filename.name[3] = ((ckp_seq / 10) % 10) + '0';
	filename.name[4] = (ckp_seq % 10) + '0';
	filename.name[8] = '.';
	filename.name[9] = 'c';
	filename.name[10] = 'k';
	filename.name[11] = 'p';

	if (jsx->jsx_status & JSX_VERBOSE)
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ CPP/ALTDB: Deleting checkpoint set %d.\n", ckp_seq);

	/*  Loop through all checkpoint save sets. */

	for (i = 1;; i++)
	{
	    /*	Construct file name. */

	    filename.name[5] = (i / 100) + '0';
	    filename.name[6] = ((i / 10) % 10) + '0';
	    filename.name[7] = (i % 10) + '0';
 
 	    /* Is this CKP REALLY a file? */

	    STncpy (ck_dir_buf, (char *) (dcb->dcb_ckp->physical.name),
		     dcb->dcb_ckp->phys_length);
	    ck_dir_buf[dcb->dcb_ckp->phys_length ] = '\0';
	    LOfroms (PATH, ck_dir_buf, &ck_loc);
            STncpy ( ck_fn_buf, filename.name, sizeof (DM_FILE));
	    ck_fn_buf[ sizeof (DM_FILE) ] = '\0';
	    LOfroms (FILENAME, ck_fn_buf, &temploc);
	    LOstfile (&temploc, &ck_loc);   
 	    flagword = LO_I_TYPE;
 	    if ((LOinfo (&ck_loc, &flagword, &loinf) == OK) && 
 		(loinf.li_type == LO_IS_DIR))   
 	    {
 	        /* Nope! It's a directory. Kill everything in it. */
		STncpy (ck_dir_buf, (char *) (dcb->dcb_ckp->physical.name),
			 dcb->dcb_ckp->phys_length);
		ck_dir_buf[dcb->dcb_ckp->phys_length ] = '\0';
		LOfroms (PATH, ck_dir_buf, &ck_loc);
		LOfaddpath(&ck_loc, ck_fn_buf, &ck_loc);
		LOcopy(&ck_loc, save_dir_buf, &save_ck_loc);

		if (LOwcard (&ck_loc, NULL, NULL, NULL, &lc) == OK)
	        do
		{
			LOtos (&ck_loc, &ck_fname);
			if (jsx->jsx_status & JSX_VERBOSE)
			   TRformat(dmf_put_line, 0, line_buffer, 
			   sizeof(line_buffer),
			   "%@ CPP/ALTDB: Deleting file %s.\n", ck_fname);
	    		CKdelete (ck_fname, STlength (ck_fname), NULL, 0, &sys_err);
	        } while (LOwnext (&lc, &ck_loc) == OK);
		LOwend (&lc);
	
		/* Remove the directory we thought was a file. */

		LOtos (&save_ck_loc, &ck_dir);
	        delete_info.path_name = ck_dir;
		delete_info.path_length = STlength (ck_dir);
		delete_info.error = 0;
		if (jsx->jsx_status & JSX_VERBOSE)
		   TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		   "%@ CPP/ALTDB: Deleting directory %s.\n", ck_dir);
		status = DIdirdelete (&f, delete_info.path_name, 
				      delete_info.path_length,
				      NULL, 0, &sys_err);
            } 
            else
	    {
		if (jsx->jsx_status & JSX_VERBOSE)
		   TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		   "%@ CPP/ALTDB: Deleting file %~t in directory %~t\n", 
		   sizeof(filename), filename.name,
		   dcb->dcb_ckp->phys_length, dcb->dcb_ckp->physical.name);
	    	status = CKdelete(dcb->dcb_ckp->physical.name,
				  dcb->dcb_ckp->phys_length,
				  filename.name, sizeof(filename), &sys_err);
		/* BUG 67329: don't stop, there may be more ckp files */
		if (status == CK_NOTFOUND)
		    status = OK;
	    }

	    if (status != OK)
	    {
		dmfWriteMsg(NULL, E_DM1111_CPP_DELETE_CKP, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1112_CPP_JNLDMPCKP_NODEL);
		return (E_DB_ERROR);
	    }

	    /* If this is alterdb, delete only one checkpoint saveset */
	    if (jsx->jsx_status2 & JSX2_ALTDB_CKP_INVALID)
		break;

	    if (i < ckp_cnt)
		continue;
	    else
		break;
	}

	/*
	** Bug 34016: Delete config file dump here.
	**
	** Check for DUMP copy of config file for this checkpoint and
	** delete it. If the config file is not present, dm0d_d_config returns
	** E_DB_WARN (in particular, this is the case for offline checkpoints
	** taken prior to Ingres 6.3/02).
	*/
	if (jsx->jsx_status & JSX_VERBOSE)
	{
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ CPP/ALTDB: Deleting dump config file for checkpoint %d.\n",
		ckp_seq);
	}

	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay("%@ CPP: Deleting dump config file for checkpoint %d.\n",
		ckp_seq);
	}

	/* 
	** Delete associated copy of the config file on the dump directory. 
	*/
	status = dm0d_d_config(dcb, cnf, ckp_seq, &jsx->jsx_dberr);
	if (DB_FAILURE_MACRO(status))
	{
	    dmfWriteMsg(NULL, E_DM1134_CPP_DELETE_DMP, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1112_CPP_JNLDMPCKP_NODEL);
	    return (E_DB_ERROR);
	}
	status = E_DB_OK;

	if (jsx->jsx_status & JSX_VERBOSE)
	{
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%@ CPP/ALTDB: Deleting dump table file for checkpoint %d.\n",
		ckp_seq);
	}

	if (jsx->jsx_status & JSX_TRACE)
	{
	    TRdisplay("%@ CPP: Deleting dump table file for checkpoint %d.\n",
		ckp_seq);
	}

	{
	    TBLLST_CTX	tblctx;

	    tblctx.seq = ckp_seq;
	    tblctx.phys_length = dcb->dcb_dmp->phys_length;
	    STRUCT_ASSIGN_MACRO(dcb->dcb_dmp->physical, tblctx.physical);

	    status = tbllst_delete(jsx, &tblctx);

	    if (DB_FAILURE_MACRO(status))
	    {
		uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
			(char * )0, 0L, (i4 *)NULL, &local_error, 0);
	    	dmfWriteMsg(NULL, E_DM116B_CPP_CHKPT_DEL_ERROR, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1112_CPP_JNLDMPCKP_NODEL);
	    	return (E_DB_ERROR);
	    }
	}
	status = E_DB_OK;

	/*  Update jnl in memory.  Note that this MEcopy moves all jnl_history
	**  records up one in the jnl_history array
	*/

	if (fully_delete)
	{
	    MEcopy((char *)&cnf->cnf_jnl->jnl_history[jnl_index+1],
		(CKP_HISTORY - jnl_index - 1) * sizeof(struct _JNL_CKP),
		(char *)&cnf->cnf_jnl->jnl_history[jnl_index]); 
	    cnf->cnf_jnl->jnl_count--;
	}
    }

    /*
    ** Delete old DUMP files for old checkpoints.
    ** Do this even if running offline checkpoint - we can still delete
    ** old portions of online checkpoint.
    **
    ** If this is Offline Checkpoint then delete all the entries in the
    ** dump history, even the latest one - as there is no dump entry for
    ** the current checkpoint.
    */
    for (dmp_index = 0; dmp_index < cnf->cnf_dmp->dmp_count; dmp_index++)
    {
	if (cnf->cnf_dmp->dmp_history[dmp_index].ckp_sequence == ckp_seq)
	    break;
    }

    /* If we found this sequence number, go on and delete it... */
    if (dmp_index < cnf->cnf_dmp->dmp_count)
    {
	/*  Delete dump files pertaining to this checkpoint. */
	first_dmp = cnf->cnf_dmp->dmp_history[dmp_index].ckp_f_dmp;
	last_dmp = cnf->cnf_dmp->dmp_history[dmp_index].ckp_l_dmp;

	if (first_dmp)
	{
	    if (jsx->jsx_status & JSX_VERBOSE)
	    {
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%@ CPP/ALTDB: Deleting dump files for checkpoint %d.\n",
		    ckp_seq);
		TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		    "%34* Dump file set %d to %d.\n",
		    first_dmp, last_dmp);
	    }

	    if (jsx->jsx_status & JSX_TRACE)
	    {
		TRdisplay("%@ CPP: Deleting dump files for checkpoint %d.\n",
		    ckp_seq);
		TRdisplay("%34* Dump file set %d to %d.\n",
		    first_dmp, last_dmp);
	    }

	    for (; first_dmp <= last_dmp; first_dmp++)
	    {
		status = dm0d_delete(0, (char *)&dcb->dcb_dmp->physical,
		    dcb->dcb_dmp->phys_length, first_dmp, &jsx->jsx_dberr);
		if (status != E_DB_OK)
		{
		    dmfWriteMsg(NULL, E_DM1134_CPP_DELETE_DMP, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1112_CPP_JNLDMPCKP_NODEL);
		    return (E_DB_ERROR);
		}
	    }
	}
	
	/*  Update dmp in memory. */

	MEcopy((char *)&cnf->cnf_dmp->dmp_history[dmp_index+1],
		(CKP_HISTORY - dmp_index - 1) * sizeof(struct _DMP_CKP),
		(char *)&cnf->cnf_dmp->dmp_history[dmp_index]); 
	cnf->cnf_dmp->dmp_count--;
    }

    /*
    ** If successful alterdb, echo message
    */
    if (alter_db && (status == E_DB_OK))
    {
	char		error_buffer[ER_MAX_LEN];
	i4		error_length;

	STncpy( line_buffer, dcb->dcb_name.db_db_name, DB_DB_MAXNAME);
	line_buffer[ DB_DB_MAXNAME ] = '\0';
	STtrmwhite(line_buffer);

	uleFormat(NULL, E_DM1405_ALT_CKP_DELETED, (CL_ERR_DESC *)NULL, ULE_LOOKUP, 
	    (DB_SQLSTATE *)NULL, error_buffer, ER_MAX_LEN, &error_length,
	    &local_error, 2, 0, line_buffer, 0, ckp_seq);

	error_buffer[error_length] = 0;
	SIprintf("%s\n", error_buffer);

	uleFormat(NULL, E_DM1405_ALT_CKP_DELETED, (CL_ERR_DESC *)NULL, ULE_LOG,
	    (DB_SQLSTATE *)0, (char *)NULL, (i4)0, (i4 *)NULL,
	    &local_error, 2, 0, line_buffer, 0, ckp_seq);
    }

    return (status);
}



/*{
** Name: open_iidbdb             - Open the iidbdb database
**
** Description:
**
**      This routine will allocate a dcb, initialize it and open the 
**      iidbdb database.
**
** Inputs:
**      access_mode             The access mode: DM2D_A_READ, DM2D_A_WRITE
**      lock_mode               The lock mode: DM2D_X, DM2D_IX...
**      iidb_flag               Special open flags. 
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**      iidcbp                  Pointer to DCB allocated for iidbdb database.
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
[@history_template@]...
*/
DB_STATUS
open_iidbdb(
DMF_JSX		  *jsx,
DMP_DCB                **iidcbp,
i4                access_mode,
i4                lock_mode,
i4                iidb_flag)
{
    char                error_buffer[ER_MAX_LEN];
    i4             error_length;
    DB_STATUS           status;
    DMP_DCB             *iidcb = 0;
    i4			local_err;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    *iidcbp = 0;
    do 
    {
	    /*
	    ** Allocate a DCB for the database database (iidbdb)    
	    */
	    status = dm0m_allocate((i4)sizeof(DMP_DCB),
		DM0M_ZERO, (i4)DCB_CB, (i4)DCB_ASCII_ID, (char *)NULL,
		(DM_OBJECT **)&iidcb, &jsx->jsx_dberr);

	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		dmfWriteMsg(NULL, E_DM150A_RELOC_MEM_ALLOC, 0);
		break;
	    }

	    /*  Fill the in DCB for the database database. */
	    MEmove(sizeof(DU_DBDBNAME) - 1, DU_DBDBNAME, ' ',
		sizeof(iidcb->dcb_name), iidcb->dcb_name.db_db_name);
	    MEmove(sizeof(DU_DBA_DBDB) - 1, DU_DBA_DBDB, ' ',
		sizeof(iidcb->dcb_db_owner), iidcb->dcb_db_owner.db_own_name);
	    iidcb->dcb_type = DCB_CB;
	    iidcb->dcb_access_mode = (access_mode == DM2D_A_READ) ? 
					 DCB_A_READ : DCB_A_WRITE;
	    iidcb->dcb_served = DCB_MULTIPLE;
	    iidcb->dcb_bm_served = DCB_MULTIPLE;
	    iidcb->dcb_db_type = DCB_PUBLIC;
	    iidcb->dcb_log_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;
	    dm0s_minit(&iidcb->dcb_mutex);
	    MEmove(8, "$default", ' ', sizeof(iidcb->dcb_location.logical),
		(PTR)&iidcb->dcb_location.logical);

	    if (build_loc(&iidcb->dcb_name, 11, "II_DATABASE", LO_DB,
			&iidcb->dcb_location) != E_DB_OK)
	    {
		status = E_DB_ERROR;
		uleFormat(&jsx->jsx_dberr, E_DM1017_JSP_II_DATABASE, 
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    error_buffer, ER_MAX_LEN, &error_length, &local_err, 0);
		dmf_put_line(0, error_length, error_buffer);
		break;
	    }

	    /*
	    ** Open the database database
	    */
	    status = dm2d_open_db(iidcb, access_mode, lock_mode,
			dmf_svcb->svcb_lock_list, iidb_flag, &jsx->jsx_dberr);

	    if (status != E_DB_OK)
	    {
		uleFormat(&jsx->jsx_dberr, E_DM1009_JSP_DBDB_OPEN, 
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    error_buffer, ER_MAX_LEN, &error_length, &local_err, 0);
		dmf_put_line(0, error_length, error_buffer);
		break;
	    }

    } while (FALSE);

    if (status == E_DB_OK)
	*iidcbp = iidcb;
    else
    {
	if (iidcb)
	    dm0m_deallocate((DM_OBJECT **)&iidcb);
    }

    return (status);
}



/*{
** Name: build_loc	- Build a physical location 
**
** Description:
**      This routine takes the database name and area name and creates  
**      a location entry
**
** Inputs:
**      dbname                          Pointer to database name.
**      l_path                          Length of location path.
**      path                            Pointer to location path.
**	loc_type			The "what" flag to LOingpath
**	location			Which location is to be filled in
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
[@history_line@]...
[@history_template@]...
*/
DB_STATUS
build_loc(
DB_DB_NAME          *dbname,
i4                   l_path,
char		    *path,
char		    *loc_type,
DMP_LOC_ENTRY	    *location)
{
    i4		    i;
    char	    *cp;
    LOCATION	    loc;
    char	    db_name[64];
    char	    area[256];
    DB_STATUS	    status;

    for (i = 0; i < sizeof(*dbname); i++)
    {
	db_name[i] = dbname->db_db_name[i];

	if (dbname->db_db_name[i] == ' ')
	    break;
    }

    db_name[i] = '\0';

    for (i = 0; i < l_path; i++)
    {
	area[i] = path[i];

	if (path[i] == ' ')
	    break;
    }

    area[i] = '\0';
    
    if ((status = LOingpath(area, db_name, loc_type, &loc)) == OK)
    {
	LOtos(&loc, &cp);

	if (*cp == '\0')
	{
	    status = E_DB_ERROR;
	}
	else
	{
	    MEmove(STlength(cp), cp, ' ', 
		   sizeof(location->physical),
		   (PTR)&location->physical);
	    location->phys_length = STlength(cp);
	}
    }

    return(status);
}


/*{
**
**  Name: tbllst_delete
**
**  Description:
**	Deletes file cssss000.lst ( where ssss is a checkpoint sequence 
**	number ) in the location specified by the table list context.
**
**  Inputs:
**	tblctx  -	table list context
**
**  Outputs:
**	tblctx	-	filename returned 
**
**  Returns:
**	E_DB_OK	-	file deleted or didn't exist.
**	E_DB_ERROR
**
** Side Effects:
**	    File deleted.
**	
**  History:
**       6-feb-1996 (nick)
**	    Created.
*/
DB_STATUS
tbllst_delete(
DMF_JSX		*jsx,
TBLLST_CTX	*tblctx)
{
    TBLLST_CTX	*t = tblctx;
    DI_IO	*d = &t->dio;
    DM_FILE	file;
    i4	number;
    STATUS	cl_status;
    CL_ERR_DESC	sys_err;
    i4	local_err;

    CLRDBERR(&jsx->jsx_dberr);

    number = t->seq;

    file.name[0] = 'c';
    file.name[1] = ((number / 1000) % 10) + '0';
    file.name[2] = ((number / 100) % 10) + '0';
    file.name[3] = ((number / 10) % 10) + '0';
    file.name[4] = (number % 10) + '0';
    file.name[5] = '0';
    file.name[6] = '0';
    file.name[7] = '0';
    file.name[8] = '.';
    file.name[9] = 'l';
    file.name[10] = 's';
    file.name[11] = 't';
    STRUCT_ASSIGN_MACRO(file, t->filename);

    cl_status = DIdelete(d, t->physical.name, t->phys_length, file.name, 
			sizeof(file.name), &sys_err);
    
    if (cl_status == OK || cl_status == DI_FILENOTFOUND)
    {
	t->blkno = -1;
	return(E_DB_OK);
    }
    
    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &local_err, 0);

    SETDBERR(&jsx->jsx_dberr, 0, E_DM1058_JSP_TBL_LST_ERR);
    return(E_DB_ERROR);
}


/*{
**
**  Name: tbllst_create
**
**  Description:
**	Creates file cssss000.lst ( where ssss is a checkpoint sequence 
**	number ) in the location specified by the table list context.
**
**  Inputs:
**	tblctx  -	table list context
**
**  Outputs:
**	tblctx	-	filename returned and various bits of the DI_IO set.
**
**  Returns:
**	E_DB_OK
**	E_DB_ERROR
**	E_DB_WARN	- file exists
**
** Side Effects:
**	    File created if E_DB_OK returned.
**	
**  History:
**       6-feb-1996 (nick)
**	    Created.
*/
DB_STATUS
tbllst_create(
DMF_JSX		*jsx,
TBLLST_CTX	*tblctx)
{
    TBLLST_CTX	*t = tblctx;
    DI_IO	*d = &t->dio;
    DM_FILE	file;
    i4	number;
    STATUS	cl_status;
    CL_ERR_DESC	sys_err;
    i4	local_err;

    CLRDBERR(&jsx->jsx_dberr);

    number = t->seq;

    file.name[0] = 'c';
    file.name[1] = ((number / 1000) % 10) + '0';
    file.name[2] = ((number / 100) % 10) + '0';
    file.name[3] = ((number / 10) % 10) + '0';
    file.name[4] = (number % 10) + '0';
    file.name[5] = '0';
    file.name[6] = '0';
    file.name[7] = '0';
    file.name[8] = '.';
    file.name[9] = 'l';
    file.name[10] = 's';
    file.name[11] = 't';
    STRUCT_ASSIGN_MACRO(file, t->filename);

    cl_status = DIcreate(d, t->physical.name, t->phys_length, file.name, 
		sizeof(file), t->blksz, &sys_err);
    
    if (cl_status == OK)
	return(E_DB_OK);
    else if (cl_status == DI_EXISTS)
	return(E_DB_WARN);
    
    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &local_err, 0);

    SETDBERR(&jsx->jsx_dberr, 0, E_DM1058_JSP_TBL_LST_ERR);
    return(E_DB_ERROR);
}


/*{
**
**  Name: tbllst_open
**
**  Description:
**	Opens file cssss000.lst ( where ssss is a checkpoint sequence 
**	number ) in the location specified by the table list context.
**
**  Inputs:
**	tblctx  -	table list context
**
**  Outputs:
**	tblctx	-	filename returned and various bits of the DI_IO set.
**
**  Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** Side Effects:
**	    File opened if E_DB_OK returned.
**	
**  History:
**       6-feb-1996 (nick)
**	    Created.
**	2-Mar-2007 (kschendel) SIR 122757
**	    table-list files can be PRIVATE.
*/
DB_STATUS
tbllst_open(
DMF_JSX		*jsx,
TBLLST_CTX	*tblctx)
{
    TBLLST_CTX	*t = tblctx;
    DI_IO	*d = &t->dio;
    DM_FILE	file;
    i4	number;
    STATUS	cl_status;
    CL_ERR_DESC	sys_err;
    i4	local_err;

    CLRDBERR(&jsx->jsx_dberr);

    number = t->seq;

    file.name[0] = 'c';
    file.name[1] = ((number / 1000) % 10) + '0';
    file.name[2] = ((number / 100) % 10) + '0';
    file.name[3] = ((number / 10) % 10) + '0';
    file.name[4] = (number % 10) + '0';
    file.name[5] = '0';
    file.name[6] = '0';
    file.name[7] = '0';
    file.name[8] = '.';
    file.name[9] = 'l';
    file.name[10] = 's';
    file.name[11] = 't';
    STRUCT_ASSIGN_MACRO(file, t->filename);

    cl_status = DIopen(d, t->physical.name, t->phys_length, file.name, 
		sizeof(file), t->blksz, DI_IO_WRITE,
		DI_SYNC_MASK | DI_PRIVATE_MASK, &sys_err);
    
    if (cl_status == OK)
    {
	t->blkno = 0;
	return(E_DB_OK);
    }
    
    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &local_err, 0);

    SETDBERR(&jsx->jsx_dberr, 0, E_DM1058_JSP_TBL_LST_ERR);
    return(E_DB_ERROR);
}


/*{
**
**  Name: tbllst_close
**
**  Description:
**	Closes file cssss000.lst ( where ssss is a checkpoint sequence 
**	number ) in the location specified by the table list context.
**
**  Inputs:
**	tblctx  -	table list context
**
**  Outputs:
**	tblctx	-	DI_IO contained in table liost context updated.
**
**  Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** Side Effects:
**	    File closed if E_DB_OK returned.
**	
**  History:
**       6-feb-1996 (nick)
**	    Created.
*/
DB_STATUS
tbllst_close(
DMF_JSX		*jsx,
TBLLST_CTX	*tblctx)
{
    TBLLST_CTX	*t = tblctx;
    DI_IO	*d = &t->dio;
    STATUS	cl_status;
    CL_ERR_DESC	sys_err;
    i4	local_err;

    CLRDBERR(&jsx->jsx_dberr);

    if (t->blkno == -1)
    {
        SETDBERR(&jsx->jsx_dberr, 0, E_DM1058_JSP_TBL_LST_ERR);
    	return(E_DB_ERROR);
    }
    cl_status = DIclose(d, &sys_err);
    
    if (cl_status == OK)
    {
	t->blkno = -1;
	return(E_DB_OK);
    }
    
    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &local_err, 0);

    SETDBERR(&jsx->jsx_dberr, 0, E_DM1058_JSP_TBL_LST_ERR);
    return(E_DB_ERROR);
}


/*{
**
**  Name: tbllst_read
**
**  Description:
**	Reads one page from file cssss000.lst ( where ssss is a checkpoint 
**	sequence number ) in the location specified by the table list context.
**
**	There is no ability to specify the specific page ; if you need to
**	read prior to the current position then the file must be closed and
**	reopened.
**
**  Inputs:
**	tblctx  -	table list context
**
**  Outputs:
**	tblctx	-	page returned in buffer
**
**  Returns:
**	E_DB_OK
**	E_DB_ERROR
**	E_DB_WARN	-	end of file reached
**
** Side Effects:
**	    Page to read next incremeted.
**	
**  History:
**       6-feb-1996 (nick)
**	    Created.
*/
DB_STATUS
tbllst_read(
DMF_JSX		*jsx,
TBLLST_CTX	*tblctx)
{
    TBLLST_CTX	*t = tblctx;
    DI_IO	*d = &t->dio;
    STATUS	cl_status = OK;
    CL_ERR_DESC	sys_err;
    i4	local_err;
    i4	num_pages = 1;

    CLRDBERR(&jsx->jsx_dberr);

    do
    {
    	if (t->blkno == -1)
	    break;

    	cl_status = DIread(d, &num_pages, t->blkno, (char *)&t->tbuf, 
			&sys_err);
    
    	if (cl_status == OK)
	{
	    t->blkno += num_pages;
	    return(E_DB_OK);
	}
    } while (FALSE);

    if (cl_status == DI_ENDFILE)
    {
	return(E_DB_WARN);
    }
    else if (cl_status != OK)
    {
    	uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &local_err, 0);
    }

    SETDBERR(&jsx->jsx_dberr, 0, E_DM1058_JSP_TBL_LST_ERR);
    return(E_DB_ERROR);
}


/*{
**
**  Name: tbllst_write
**
**  Description:
**	Writes one page to file cssss000.lst ( where ssss is a checkpoint 
**	sequence number ) in the location specified by the table list context.
**
**	There is no ability to specify the specific page ; if you need to
**	write prior to the current position then the file must be closed and
**	reopened.
**
**  Inputs:
**	tblctx  -	table list context
**
**  Outputs:
**	none
**
**  Returns:
**	E_DB_OK
**	E_DB_ERROR
**	E_DB_WARN	-	end of file reached
**
** Side Effects:
**	    Page to write next incremeted.
**	
**  History:
**       6-feb-1996 (nick)
**	    Created.
*/
DB_STATUS
tbllst_write(
DMF_JSX		*jsx,
TBLLST_CTX	*tblctx)
{
    TBLLST_CTX	*t = tblctx;
    DI_IO	*d = &t->dio;
    STATUS	cl_status = OK;
    CL_ERR_DESC	sys_err;
    i4	local_err;
    i4	num_pages = 1;
    i4	page;

    CLRDBERR(&jsx->jsx_dberr);

    do
    {
    	if (t->blkno == -1)
	    break;

    	cl_status = DIalloc(d, num_pages, &page, &sys_err);
    	if (cl_status != OK)
	    break;

    	cl_status = DIwrite(d, &num_pages, t->blkno, (char *)&t->tbuf, 
			&sys_err);
    	if (cl_status == OK)
	{
	    t->blkno += num_pages;
	    return(E_DB_OK);
	}
    } while (FALSE);

    if (cl_status != OK)
    {
    	uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, (char *)NULL, 0L, 
		(i4 *)NULL, &local_err, 0);
    }

    SETDBERR(&jsx->jsx_dberr, 0, E_DM1058_JSP_TBL_LST_ERR);
    return(E_DB_ERROR);
}
