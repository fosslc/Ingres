/*
**Copyright (c) 2004 Ingres Corporation
**
*/

# include    <compat.h>
# include    <gl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <dudbms.h>
# include    <bt.h>
# include    <me.h>
# include    <cs.h>
# include    <st.h>
# include    <tm.h>
# include    <tr.h>
# include    <adf.h>
# include    <scf.h>
# include    <ulm.h>
# include    <dmf.h>
# include    <dmrcb.h>
# include    <dmtcb.h>
# include    <dmucb.h>
# include    <sxf.h>

# include    <gwf.h>
# include    <gwfint.h>
# include    <gwftrace.h>
# include    "gwfsxa.h"
/*
** Name: GWSXA.C	- source for the C2 Security Audit Gateway.
**
** Description:
**	This file contains the routines which implement the SXA gateway.
**
** History:
**	14-sep-92 (robf)
**	    Created
**	18-nov-92 (robf)
**	    SXA gateway is now a IMA managed object, added startup
**          calls.
**	27-nov-92 (robf)
**	     Improved error handling when registering a table, plus
**	     better defaults.
**	18-dec-92 (robf)
**	     Emulate access by TID by performing a scan of the table,
**	     only restarting the scan if the TID is prior to the current
**	     position. 
**	4-feb-1993 (markg)
**		Changed the name of some symbolic constants user for
**		error messages to be 32 characters or less in length
**		as these were causing problems with ercompile.
**      05-apr-1993 (smc)
**              Added forward declaration of gwsxa_stats().
**	7-jul-93 (robf)
**	    Check AUDITOR rather than security privilege. Rewrote priv.
**	    checking to be more generic in case this ever changes again.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	03-aug-93 (shailaja)
**	    removed nested comments.
**      15-sep-93 (smc)
**          Made session ids CS_SID.
**	16-nov-93 (stephenb)
**	    changed audit writes for attempt to register tables from table 
**	    table events (SXF_E_TABLE) to security events (SXF_E_SECURITY),
**	    because we already audit a table event in DMF.
**	24-jan-1994 (gmanning)
**	    Give name to semaphores in order to take advantage of cross 
**	    process semaphores on NT.
**	11-oct-93 (swm)
**	    Bug #56448
**	    Made gwf_display() portable. The old gwf_display()
**	    took an array of variable-sized arguments. It could
**	    not be re-written as a varargs function as this
**	    practice is outlawed in main line code.
**	    The problem was solved by invoking a varargs function,
**	    TRformat, directly.
**	    The gwf_display() function has now been deleted, but
**	    for flexibilty gwf_display has been #defined to
**	    TRformat to accomodate future change.
**	    All calls to gwf_display() changed to pass additional
**	    arguments required by TRformat:
**	        gwf_display(gwf_scctalk,0,trbuf,sizeof(trbuf) - 1,...)
**	    this emulates the behaviour of the old gwf_display()
**	    except that the trbuf must be supplied.
**	    Note that the size of the buffer passed to TRformat
**	    has 1 subtracted from it to fool TRformat into NOT
**	    discarding a trailing '\n'.
**	28-Apr-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to gwfsxadata.c.
**      09-dec-1997 (hweho01)
**          Semaphores "GWsxamsg" and "GWsxa" are not in the shared memory, 
**          they should be CS_SEM_SINGLE type.  
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**      11-dec-2008 (stial01)
**          Redefine sxf_data as string and then blank pad to name size.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**     22-apr-2010 (stial01)
**          Use DB_EXTFMT_SIZE for register table newcolname IS 'ext_format'
**	11-Jun-2010 (kiria01) b123908
**	    Init ulm_streamid_p for ulm_openstream to fix potential segvs.
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**	13-Oct-2010 (kschendel) SIR 134544
**	    DMU char array replaced with indicator bitmap, fix here.
*/

GLOBALREF	GW_FACILITY	*Gwf_facility;
GLOBALREF CS_SEMAPHORE GWsxamsg_sem; /* Semaphore to protect msg list*/
GLOBALREF CS_SEMAPHORE GWsxa_sem; /* Semaphore to protect general increments*/
GLOBALREF ULM_RCB gwsxa_ulm;
GLOBALREF      GWSXA_XATT  Gwsxa_xatt_tidp;

FUNC_EXTERN	DB_STATUS	scf_call();

static    SIZE_TYPE sxa_ulm_memleft=0;

static    VOID gwsxa_stats();

/*
**	Gateway Statistics
*/
GLOBALREF GWSXASTATS GWsxastats ;

/*
**	Define TEST_SXF_OK to enable "test" SXF (trace point 6 turns on)
*/
# ifdef TEST_SXF_OK
static struct {
	int rec_no;
	i4 eventtype;
	char *ruserid;
	char *euserid;
	char *dbname;
	i4 messid;
	i4 auditstatus;
	i4 userpriv;
	i4 objpriv;
	i4 accesstype;
	char    *object;
} sxf_data[] = {
	{1, SXF_E_TABLE, 
		"robf_real", 
		"robf_eff",
		"dbname", 
		I_SX2801_SELECT, OK,2,2,SXF_A_SELECT,
		"tblname" },
	{2, SXF_E_DATABASE, 
		"robf_real", 
		"robf_eff",
		"dbname",
		I_SX201A_DATABASE_ACCESS , 1,6,6,SXF_A_SELECT,
		"database" },
	{3, SXF_E_TABLE, 
		"robf_real", 
		"robf_eff",
		"iidbdb",
		I_SX2804_UPDATE , OK,9,2,SXF_A_UPDATE,
		"table2" },
	{4, SXF_E_PROCEDURE, 
		"robf_real", 
		"robf_eff",
		"dbname",
		I_SX201D_DBPROC_EXECUTE , 1,9,2,SXF_A_EXECUTE,
		"dbproc" },
	{5, SXF_E_PROCEDURE, 
		"xxxx_real", 
		"xxxx_eff",
		"iidbdb",
		I_SX201D_DBPROC_EXECUTE , 1,9,8,SXF_A_EXECUTE,
		"dbproc_number2" },
	{-1}};
# endif
GLOBALREF   char    *GWsxa_version;
/*
**	Name: gwsxa_term - terminate a gateway session
*/

DB_STATUS
gwsxa_term( GWX_RCB *gwx_rcb )
{
    VOID gwsxa_stats(void);
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    TRdisplay("SXA C2 Audit Gateway Shutdown\n");
    /*
    **	Print stats on shutdown.
    */
    gwsxa_stats();
    return (E_DB_OK);
}
/*
** Name: gwsxa_stats - print SXA statistics to the trace log
**
** Description:
**	Print stats on the gateway. 
**
** History:
**	06-oct-92 (robf)
**	    Created
**	26-Oct-98 (kosma01)
**	    prototype of TRdisplay nolonger needed because it is 
**	    given in tr.h.
*/
static VOID
gwsxa_stats(void)
{
    i4 (*fptr)(char *, ...);

    fptr= TRdisplay;

    (*fptr)("%25*- SXA Gateway Statistics %25*-\n");
    (*fptr)(" SXA REGISTER operations: %8d total, %8d failures.\n",
			GWsxastats.sxa_num_register,
			GWsxastats.sxa_num_reg_fail);
    (*fptr)(" SXA OPEN operations    : %8d total, %8d failures.\n",
			GWsxastats.sxa_num_open,
			GWsxastats.sxa_num_open_fail);
    (*fptr)(" SXA CLOSE operations   : %8d total, %8d failures.\n",
			GWsxastats.sxa_num_close,
			GWsxastats.sxa_num_close_fail);
    (*fptr)(" SXA GET operations     : %8d total, %8d failures.\n",
			GWsxastats.sxa_num_get,
			GWsxastats.sxa_num_get_fail);
    (*fptr)(" SXA GET-BY-TID requests: %8d total, %8d causing rescan.\n",
			GWsxastats.sxa_num_tid_get,
			GWsxastats.sxa_num_tid_scanstart);
    (*fptr)(" SXA Distinct Mesg Ids  : %8d total, %8d not found.\n",
			GWsxastats.sxa_num_msgid,
			GWsxastats.sxa_num_msgid_fail);
    (*fptr)(" SXA Message Id lookups : %8d total.\n",
			GWsxastats.sxa_num_msgid_lookup);
    (*fptr)(" SXA Update attempts    : %8d.\n",
			GWsxastats.sxa_num_update_tries);
    (*fptr)(" SXA Keyed position attempts: %8d.\n",
			GWsxastats.sxa_num_position_keyed);
    (*fptr)(" SXA Index register attempts: %8d.\n",
			GWsxastats.sxa_num_reg_index_tries);

    (*fptr)("%75*-\n");
}
/*
**	Name: gwsxa_tabf - register an audit table
**
**	Description:
**		Makes an audit table known to the gateway.
**		This performs error checking, then returns the
**		the tuples required for the iigw06_relation/attribute
**		tables.
**
**	Inputs:
**		gwx_rcb.xrcb_column_cnt	    Number of columns in table
**		gwx_rcb.xrcb_column_attr    Column attributes
**		gwx_rcb.xrcb_var_data_list  Column "info" strings 
**		gwx_rcb.xrcb_tab_name	    Table Name
**		gwx_rcb.xrcb_tab_owner	    Table Owner
**		gwx_rcb.xrcb_tab_id	    Table ID
**		gwx_rcb.xrcb_gw_id	    Gateway ID
**		gwx_rcb.xrcb_var_data1	    Gateway options
**		gwx_rcb.xrcb_var_data2	    Gateway file (audit log)
**		gwx_rcb.xrcb_var_data3	    GWSXA_XREL relation info
**		gwx_rcb.xrcb_dmu_chars	    DMU characteristics info
**		
**	Outputs:
**		gwx_rcb.xrcb_mtuple_buffs   Place for attribute info
**				.ptr_size     Length of attribute tuple
**				.ptr_in_count Number of attributes
**				.ptr_address  Where attributes go.
**		gwx_rcb.xrcb_var_data3	    Place for relation info
**				.data_in_size  Length of relation attribute
**				.data_address  Where relation goes
**
** History:
**	14-sep-1992 (robf)
**		Created.
**	17-dec-1992 (robf)
**		Updated error handling, object owner passed to sxf_call.
**	4-feb-1993 (markg)
**		Changed the name of some symbolic constants user for
**		error messages to be 32 characters or less in length
**		as these were causing problems with ercompile.
**	2-jul-93 (robf)
**	        Detect detailinfo and security label flags and mark in
**	16-nov-93 (stephenb)
**	    changed audit writes for attempt to register tables from table 
**	    table events (SXF_E_TABLE) to security events (SXF_E_SECURITY),
**	    because we already audit a table event in DMF.
**	    only audit I_SX2703_REGISTER_AUDIT_LOG on successful register
**	    of audit log, we already audit I_SX2701_REGISTER_AUDIT_SECURITY
**	    earlier on in the routine for failure.
*/
DB_STATUS
gwsxa_tabf( GWX_RCB     *gwx_rcb )
{
    i4	    	*error = &gwx_rcb->xrcb_error.err_code;
    GWSXA_XREL *xrel = (GWSXA_XREL*)
    			gwx_rcb->xrcb_var_data3.data_address;
    GWSXA_XATT *xatt = (GWSXA_XATT*)
    			gwx_rcb->xrcb_mtuple_buffs.ptr_address;
    DB_STATUS 		status, local_status;
    i4		local_err;
    i4			regtime;
    i4			indicator;
    char		*auditfile;
    i2			auditflen;
    i2			colnum;
    char		extname[DB_EXTFMT_SIZE];
    SXF_RCB 		sxfrcb;
    CS_SID  		scf_session_id;
    char		trbuf[GWF_MAX_MSGLEN + 1]; /* last char for `\n' */

    
    status=E_DB_OK;
    gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_register);
    for(;;)	/* Something to break out of */
    {
	/*
	**	Sanity checks
	*/
	status=gwsxachkrcb (gwx_rcb,"GWSXA_TABF");
	if ( status!=E_DB_OK)
	{
		*error= E_GW050E_ALREADY_REPORTED;
		break;
	}
	/*
	**	Check if the size of the relation matches the expected
	**	SXA size
	*/
	if ( gwx_rcb->xrcb_var_data3.data_in_size != SXA_XREL_SIZE)
	{
		i4 tmp = SXA_XREL_SIZE;
		gwsxa_error(gwx_rcb,E_GW4052_SXA_BAD_XREL_LEN,
				SXA_ERR_INTERNAL,
				3,
				sizeof("GWSXA_TABF")-1,
				"GWSXA_TABF",
				sizeof(tmp),
				(PTR)&tmp,
				sizeof(gwx_rcb->xrcb_var_data3.data_in_size),
				(PTR)&(gwx_rcb->xrcb_var_data3.data_in_size));

		*error= E_GW050E_ALREADY_REPORTED;
		status=E_DB_ERROR;
		break;
	}
	/*
	**	Check if the size of the attribute entry matches the
	**	expected SXA size
	*/
	if ( gwx_rcb->xrcb_mtuple_buffs.ptr_size != SXA_XATTR_SIZE)
	{
		i4 tmp = SXA_XATTR_SIZE;
		gwsxa_error(gwx_rcb,E_GW4053_SXA_BAD_XATTR_LEN,
				SXA_ERR_INTERNAL,
				3,
				sizeof("GWSXA_TABF")-1,
				"GWSXA_TABF",
				sizeof(tmp),
				(PTR)&tmp,
				sizeof(gwx_rcb->xrcb_mtuple_buffs.ptr_size),
				(PTR)&(gwx_rcb->xrcb_mtuple_buffs.ptr_size));

		*error= E_GW050E_ALREADY_REPORTED;
		status=E_DB_ERROR;
		break;
	}
	/*
	**	This is the auditfile/SAL
	*/
	auditfile=gwx_rcb->xrcb_var_data2.data_address;
	/*
	**	No flags set initially
	*/
	xrel->flags=0;
	/*
	**	If the audit file is 'current' then map to 
	**	the SXF default of empty string
	*/
	if(STnlength(SXA_CUR_SAL_LEN+1,auditfile)==SXA_CUR_SAL_LEN)
	{
		/*
		**	Could be 'current'
		*/
		if(STbcompare(SXA_CURRENT_SAL,SXA_CUR_SAL_LEN,
			      auditfile,SXA_CUR_SAL_LEN,TRUE)==0)
		{
			/*
			**	Got 'current' sal
			*/
			if (GWF_SXA_SESS_MACRO(  1))
			{
				gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
				    "SXA TABF: Converting 'current' sal to default string\n");
			}
			xrel->flags|=SXA_FLAG_CURRENT;
		}
	}
	/*
	**	Verify column information/mappings, allow up to two
	**	mappings per column.
	*/
	if ( gwx_rcb->xrcb_column_cnt <1 ||
	    gwx_rcb->xrcb_column_cnt > SXA_MAX_COLUMNS * 2)
	{
		i4 tmp = SXA_MAX_COLUMNS* 2;
		gwsxa_error(gwx_rcb,E_GW4054_SXA_BAD_NUM_COLUMNS,
				SXA_ERR_INTERNAL,
				3,
				sizeof("GWSXA_TABF")-1,
				"GWSXA_TABF",
				sizeof(tmp),
				(PTR)&tmp,
				sizeof(gwx_rcb->xrcb_column_cnt),
				(PTR)&(gwx_rcb->xrcb_column_cnt));
		*error= E_GW050E_ALREADY_REPORTED;
		status=E_DB_ERROR;
		break;
	}
	/*
	**	Verify gateway registration information.
	**	
	**	We only check some, currently UPDATE and JOURNALING.
	**	plus make sure its a HEAP structure.
	*/

	indicator = -1;
	while (gwx_rcb->xrcb_dmu_chars != NULL
	  && (indicator = BTnext(indicator, gwx_rcb->xrcb_dmu_chars->dmu_indicators, DMU_CHARIND_LAST)) != -1)
	{
	    switch(indicator)
	    {
	    case DMU_JOURNALED:
		if (gwx_rcb->xrcb_dmu_chars->dmu_journaled != DMU_JOURNAL_OFF)
		{
		    gwsxa_error(gwx_rcb,
					E_GW4005_SXA_NO_REGISTER_JNL,
					SXA_ERR_USER,
					0);
		    *error= E_GW050E_ALREADY_REPORTED;
		    status=E_DB_ERROR;
		}
		break;
	    case DMU_STRUCTURE:
		if (gwx_rcb->xrcb_dmu_chars->dmu_struct != DB_HEAP_STORE)
		{
		    gwsxa_error(gwx_rcb,E_GW4067_SXA_NO_REGISTER_KEYED,
				SXA_ERR_USER,
				0);
		    *error= E_GW050E_ALREADY_REPORTED;
		    status=E_DB_ERROR;
		}
		break;
	    case DMU_GW_UPDT:
		if (gwx_rcb->xrcb_dmu_chars->dmu_flags & DMU_FLAG_UPDATE)
		{
		    gwsxa_error(gwx_rcb,E_GW4004_SXA_NO_REGISTER_UPDATE,
					SXA_ERR_USER,
					0);
		    *error= E_GW050E_ALREADY_REPORTED;
		    status=E_DB_ERROR;
		}
		break;
	    default:
		break;
	    }
	    if( status==E_DB_ERROR)
		break;
	}
	if(status==E_DB_ERROR)
		break;
	/*
	**	Check if user has appropriate privilege
	*/
	status= gwsxa_priv_user(DU_UAUDITOR);
	if (status!=E_DB_OK)
	{
		/*
		**	Send a message back to the user
		*/
		/*E_GW407D_SXA_NO_PRIV */
		gwsxa_error(gwx_rcb,E_GW407D_SXA_NO_PRIV,
				SXA_ERR_USER,
				2,
				sizeof("REGISTER")-1,
				"REGISTER",
				STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
				gwx_rcb->xrcb_tab_name->db_tab_name);
		/*
		**	Log the error in SXF
		*/
		_VOID_ gwsxa_scf_session(&scf_session_id);
		MEfill(sizeof(sxfrcb), 0, (PTR)&sxfrcb);
		sxfrcb.sxf_cb_type = SXFRCB_CB;
		sxfrcb.sxf_length = sizeof(sxfrcb);
		sxfrcb.sxf_sess_id = scf_session_id;
		sxfrcb.sxf_force   = 0;
		sxfrcb.sxf_objname = auditfile;
		sxfrcb.sxf_objlength= STlength(auditfile);
		sxfrcb.sxf_objowner= NULL;
		sxfrcb.sxf_msg_id   = I_SX2701_REGISTER_AUDIT_SECURITY;
		sxfrcb.sxf_accessmask = (SXF_A_FAIL|SXF_A_REGISTER);
		sxfrcb.sxf_auditevent = SXF_E_SECURITY;
		status = sxf_call(SXR_WRITE, &sxfrcb);
		if (status!=E_DB_OK)
		{
			/*
			**	Something went wrong trying to write this
			**	audit record, so log the error.
			*/
			gwsxa_error(gwx_rcb,sxfrcb.sxf_error.err_code,
					SXA_ERR_LOG,0);
		}
		*error= E_GW050E_ALREADY_REPORTED;
		status=E_DB_ERROR;
		break;
	}
	/*
	**	Create the relation tuple entry
	**
	**	The format is:
	**	table id
	**	auditfile[SXA_MAX_AUDIT_FILE_NAME]
	**	reg_date (timestamp)
	*/
	regtime=TMsecs();
	xrel->regtime=regtime;
	/*
	**	Copy the data for the extended relation info.
	*/
	STRUCT_ASSIGN_MACRO(*gwx_rcb->xrcb_tab_id, xrel->tab_id);

	STmove(auditfile,' ',sizeof(xrel->audit_file),
			xrel->audit_file);
	xrel->flags|= SXA_FLAG_NODB_REG; /* Assume no DB until actually found */
	gwsxa_zapblank(gwx_rcb->xrcb_tab_name->db_tab_name,
			 sizeof(gwx_rcb->xrcb_tab_name->db_tab_name));
	/*
	**	Create the attribute tuple entries
	**	Each attribute format is:
	**	table id
	**	attid
	**	attname
	**	audid
	**	audname
	*/
	status=E_DB_OK;
	for(colnum=0; colnum<gwx_rcb->xrcb_column_cnt && status==E_DB_OK; 
		colnum++,xatt++)
	{
		SXA_ATT_ID attid1;
		SXA_ATT_ID attid2;
		DMT_ATT_ENTRY *col;
		DMU_GWATTR_ENTRY **gwattrs;
		char	*attname;

		col = &(gwx_rcb->xrcb_column_attr[colnum]);
		/*
		**	nuke blanks & change length
		*/
		attname=col->att_nmstr;
		gwsxa_zapblank(attname, col->att_nmlen);
		if (GWF_SXA_SESS_MACRO(  1))
		{
			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "SXA TABF: Processing attribute %d '%s'\n",
			    colnum, attname);
		}
		attid1=gwsxa_find_attbyname(attname);
		/*
		**	Check for alternative mappings.
		**	In this case the info strings *must* have
		**	just the real column name
		*/
		gwattrs = (DMU_GWATTR_ENTRY **) gwx_rcb->xrcb_var_data_list.ptr_address;
		if (gwattrs[colnum]==NULL || 
		    !(gwattrs[colnum]->gwat_flags_mask& DMGW_F_EXTFMT) ||
		    gwattrs[colnum]->gwat_xsize==0 ||
		    gwattrs[colnum]->gwat_xbuffer[0]==EOS)
		{
			if (GWF_SXA_SESS_MACRO(  1))
			{
				gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
				    "SXA TABF: Alternate format for column %s is null/has zero size\n",
				    attname);
			}
			attid2=SXA_ATT_INVALID;
			/*
			**	Make the external name the regular name
			*/
			MEcopy(attname,sizeof(extname),extname);
		}
		else
		{
			attid2=gwsxa_find_attbyname(gwattrs[colnum]->gwat_xbuffer);
			if (attid2==SXA_ATT_INVALID)
			{
				/*
				**	Specified but illegal, so error
				*/
				gwsxa_error(gwx_rcb,
					E_GW4007_SXA_REGISTER_BAD_TYPE,
					SXA_ERR_USER,
					3,
					gwattrs[colnum]->gwat_xsize,
					gwattrs[colnum]->gwat_xbuffer,
					STlength(attname),
					attname,
					STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
					gwx_rcb->xrcb_tab_name->db_tab_name);
				*error= E_GW050E_ALREADY_REPORTED;
				status=E_DB_ERROR;
				break;
			}
			MEcopy(gwattrs[colnum]->gwat_xbuffer,
				sizeof(extname),extname);
		}
		/*
		 *	Check for errors/clashes
		 */
		if (attid1!= SXA_ATT_INVALID && 
		    attid2!= SXA_ATT_INVALID &&
		    attid1!=attid2)
		{
			gwsxa_error(gwx_rcb,E_GW4008_SXA_REGISTER_MISMATCH,
				SXA_ERR_USER,
				4,
				gwattrs[colnum]->gwat_xsize,
				gwattrs[colnum]->gwat_xbuffer,
				STlength(attname),
				attname,
				STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
				gwx_rcb->xrcb_tab_name->db_tab_name,
				gwattrs[colnum]->gwat_xsize,
				gwattrs[colnum]->gwat_xbuffer);
			*error= E_GW050E_ALREADY_REPORTED;
			status=E_DB_ERROR;
			break;
		}
		if (attid1==SXA_ATT_INVALID)	/* Use second if first bad */
			attid1=attid2;
		/*
		**	Check for invalid column
		**/
		if (attid1 == SXA_ATT_INVALID) 
		{
			gwsxa_error(gwx_rcb,E_GW4009_SXA_REGISTER_BAD_COLUMN,
				SXA_ERR_USER,
				2,
				STlength(attname),
				attname,
				STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
				gwx_rcb->xrcb_tab_name->db_tab_name);
			*error= E_GW050E_ALREADY_REPORTED;
			status=E_DB_ERROR;
			break;
			
		}
		/*
		**	Check for database
		*/
		if (attid1==SXA_ATT_DATABASE)
		{
			if (GWF_SXA_SESS_MACRO( 1))
			{
				gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
				    "SXA TABF: Found database-name column '%s'\n",
			    	    attname);
			}
			xrel->flags&= ~SXA_FLAG_NODB_REG;
		}
		else if(attid1==SXA_ATT_OBJLABEL)
		{
			if (GWF_SXA_SESS_MACRO( 1))
			{
				gwf_display(gwf_scctalk, 0, trbuf,
					sizeof(trbuf) -1,
				"SXA TABF: Found object label column '%s'\n",
					attname);
			}
			xrel->flags|= SXA_FLAG_OBJLABEL_REG;
			
		}
		else if(attid1==SXA_ATT_DETAILINFO)
		{
			if (GWF_SXA_SESS_MACRO( 1))
			{
				gwf_display(gwf_scctalk, 0, trbuf,
					sizeof(trbuf) -1,
				"SXA TABF: Found detailinfo column '%s'\n",
					attname);
			}
			xrel->flags|= SXA_FLAG_DETAIL_REG;
			
		}
		/*
		**	Now build the output tuple
		*/
		STRUCT_ASSIGN_MACRO(*gwx_rcb->xrcb_tab_id, xatt->xatt_tab_id);

		xatt->attid=colnum+1;
		MEcopy(attname,sizeof(xatt->attname),xatt->attname);

		xatt->audid=attid1;

		MEcopy(extname,sizeof(xatt->audname),xatt->audname);
	}
	if (status!=E_DB_OK)
		break;
	break;
    }/* End FOR */
   if ( status!=E_DB_OK)
   {
	/*
	**	Count number of failed registrations
	*/
	gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_reg_fail);
   }
   else /* only audit for success */
   {
       	/*
       	**	Log the fact we registered the SAL
   	*/
   	_VOID_ gwsxa_scf_session(&scf_session_id);
   	MEfill(sizeof(sxfrcb), 0, (PTR)&sxfrcb);
   	sxfrcb.sxf_cb_type = SXFRCB_CB;
   	sxfrcb.sxf_length = sizeof(sxfrcb);
   	sxfrcb.sxf_sess_id = scf_session_id;
   	sxfrcb.sxf_force  = 0;
   	sxfrcb.sxf_objname= auditfile;
   	sxfrcb.sxf_objlength= STlength(auditfile);
   	sxfrcb.sxf_objowner= NULL;
   	sxfrcb.sxf_msg_id = I_SX2703_REGISTER_AUDIT_LOG;
	sxfrcb.sxf_accessmask = (SXF_A_SUCCESS|SXF_A_REGISTER);
   	sxfrcb.sxf_auditevent = SXF_E_SECURITY;
   	/*
   	**	Make sure we preserve the regular status here
   	*/
   	local_status =sxf_call(SXR_WRITE, &sxfrcb);
   	if (local_status!=E_DB_OK)
   	{
	    /*
	    **	Something went wrong trying to write this
	    **	audit record, so log the error 
	    */
	    gwsxa_error(gwx_rcb,sxfrcb.sxf_error.err_code,
		SXA_ERR_LOG,0);
	
   	}
   }
   if (GWF_SXA_SESS_MACRO( 1))
   {
	gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
	    "SXA TABF: Final relation flags are %d\n", xrel->flags);
   }
   return status;
}

/*
**	Name: gwsxa_idxf - register an audit index. This is currently
**	not allowed, so error.
*/
DB_STATUS
gwsxa_idxf(GWX_RCB     *gwx_rcb )
{
    i4	    *error = &gwx_rcb->xrcb_error.err_code;
    gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_reg_index_tries);
    gwsxa_error(gwx_rcb,E_GW4001_SXA_NO_INDEXES,
		SXA_ERR_USER|SXA_ERR_LOG,
		0);
    *error= E_GW050E_ALREADY_REPORTED;
    return (E_DB_ERROR);
}

/*
** Name: gwsxa_open	- gateway exit to open table
**
** Description:
**	Opens an audit file via SXF  and sets up to read it.
**
** History:
**	14-sep-92 (robf)
**		First created.
**	17-dec-92 (robf)
**		Updated error handling/sxf_call parameters.
**	2-jul-93 (robf)
**	        Copy all xrel flags into rsb flags.
**	4-feb-94 (stephenb/robf)
**	    added case E_SX002E_OSAUDIT_NO_READ when checking SXF errors.
**      9-Sept-2001 (hayke02) Bug 97794 INGSRV 1008
**          We now return error status if the SXA_OPEN sxf_call() returns
**          an sxfrcb.sxf_error.err_code of E_SX0015_FILE_NOT_FOUND or
**          E_SX100D_SXA_OPEN. This ensures that a register table will
**          fail if the audit file is not defined using as absolute
**          pathname, or the audit file does not exist. 
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
**      04-Dec-2008 (coomi01) b121323
**          Replace call to gwsxa_zapblank() with gwsxa_rightTrimBlank().
**          When the File Path has embeded spaces we want to leave them intact.
*/
DB_STATUS
gwsxa_open( GWX_RCB     *gwx_rcb)
{
    i4	*error = &gwx_rcb->xrcb_error.err_code;
    DB_STATUS	status;
    GW_RSB	*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GW_TCB      *tcb = rsb->gwrsb_tcb;
    GWSXA_XREL  *xrel = (GWSXA_XREL*)tcb->gwt_xrel.data_address;
    SXF_RCB 	sxfrcb;
    GWSXA_RSB	*gwsxa_rsb;
    char	*filename=xrel->audit_file;
    i4		access_mode=gwx_rcb->xrcb_access_mode;
    CS_SID      scf_session_id;
    char	trbuf[GWF_MAX_MSGLEN + 1]; /* last char for `\n' */
    DB_STATUS   scf_info_status;

    status=E_DB_OK;
    /*
    **	Remove trailing blanks & NULL terminate the SAL, otherwise open
    **  in SXF may fail
    */
    gwsxa_rightTrimBlank(filename, SXA_MAX_AUDIT_FILE_NAME);

    if (GWF_SXA_SESS_MACRO(  4))
    {
	gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
	    "SXA_OPEN: Opening SAL '%s' in mode %d (%s)\n", filename,
	    access_mode, (access_mode==GWT_WRITE?"WRITE":
			    access_mode==GWT_READ?"READ":
			    access_mode==GWT_MODIFY?"MODIFY":
			    "OTHER"));
    }
    gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_open);
    for(;;)	/* Something to break out of */
    {
	/*
	**	Sanity checks
	*/
	if ( gwsxachkrcb (gwx_rcb,"GWSXA_OPEN") != E_DB_OK)
	{
		*error= E_GW050E_ALREADY_REPORTED;
		status=E_DB_ERROR;
		break;
	}
	/*
	**	Validate the access mode
	*/
	if ( access_mode==GWT_WRITE)
	{
		gwsxa_zapblank(gwx_rcb->xrcb_tab_name->db_tab_name,
				sizeof(gwx_rcb->xrcb_tab_name->db_tab_name));
		gwsxa_error(gwx_rcb,E_GW4057_SXA_OPEN_WRITE,
			SXA_ERR_INTERNAL,
			1,
			STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
			gwx_rcb->xrcb_tab_name->db_tab_name);
		gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_update_tries);
		*error= E_GW050E_ALREADY_REPORTED;
		status=E_DB_ERROR;
		break;
		
	}
	/*
	**	Validate its not a secondary - we don't allow SALs as
	**	secondary indexes yet (they are sequential).
	*/
	if( gwx_rcb->xrcb_tab_id->db_tab_index > 0)
	{
		gwsxa_zapblank(gwx_rcb->xrcb_tab_name->db_tab_name,
				sizeof(gwx_rcb->xrcb_tab_name->db_tab_name));
		/*E_GW4011_SXA_OPEN_NO_INDEXES */
		gwsxa_error(gwx_rcb,E_GW4011_SXA_OPEN_NO_INDEXES,
				SXA_ERR_USER|SXA_ERR_LOG,
				1,
				STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
				gwx_rcb->xrcb_tab_name->db_tab_name);
		*error= E_GW050E_ALREADY_REPORTED;
		status=E_DB_ERROR;
		break;
	}
	/*
	**	Validate if user has security priv
	*/
	status= gwsxa_priv_user(DU_UAUDITOR);
	if (status!=E_DB_OK)
	{
		/*
		**	Failed, 
		*/
		_VOID_ gwsxa_scf_session(&scf_session_id);
	        MEfill(sizeof(sxfrcb), 0, (PTR)&sxfrcb);
		sxfrcb.sxf_cb_type = SXFRCB_CB;
		sxfrcb.sxf_length   = sizeof(sxfrcb);
		sxfrcb.sxf_sess_id  = scf_session_id;
		sxfrcb.sxf_force    = 0;
		sxfrcb.sxf_objname  = gwx_rcb->xrcb_tab_name->db_tab_name;
		sxfrcb.sxf_objlength= DB_MAXNAME;
		sxfrcb.sxf_objowner= NULL;
		sxfrcb.sxf_msg_id = I_SX2702_QUERY_AUDIT_SECURITY;
		sxfrcb.sxf_accessmask = (SXF_A_FAIL|SXF_A_SELECT);
		sxfrcb.sxf_auditevent = SXF_E_SECURITY;

		status = sxf_call(SXR_WRITE, &sxfrcb);
		if(status!=E_DB_OK)
		{
			/*
			**	Something went wrong trying to write this
			**	audit record, so log the error 
			*/
			gwsxa_error(gwx_rcb,sxfrcb.sxf_error.err_code,
					SXA_ERR_LOG,0);
		}
		/*E_GW407D_SXA_NO_PRIV */
		gwsxa_error(gwx_rcb,E_GW407D_SXA_NO_PRIV,
				SXA_ERR_USER,
				2,
				sizeof("OPEN")-1,
				"OPEN",
				STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
				gwx_rcb->xrcb_tab_name->db_tab_name);
		*error= E_GW050E_ALREADY_REPORTED;
		status=E_DB_ERROR;
		break;
	}
	/*
        ** Allocate and initialize the SXA-gateway specific RSB. This 
	** control information is allocated from the memory stream 
	** associated with  the record stream control block.
	** 
	** We do this now so on close there is an RSB available
        */
        rsb->gwrsb_rsb_mstream.ulm_psize = sizeof(GWSXA_RSB);

	if (GWF_SXA_SESS_MACRO(  5))
	{
		gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
		    "SXA_OPEN: Allocating %d bytes for RSB\n",
		    rsb->gwrsb_rsb_mstream.ulm_psize);
	}
        status = ulm_palloc(&rsb->gwrsb_rsb_mstream);
	/*
	**	Check for null pointer, just in case...
	*/
        if (status != E_DB_OK ||
	    rsb->gwrsb_rsb_mstream.ulm_pptr==NULL)
        {
		gwsxa_error(gwx_rcb,E_GW405A_SXA_BAD_PALLOC,
				SXA_ERR_LOG,
				1,
				sizeof("SXA_OPEN")-1,
				"SXA_OPEN");
		*error = rsb->gwrsb_rsb_mstream.ulm_error.err_code;
		break;
	    
        }
        gwsxa_rsb = (GWSXA_RSB *)rsb->gwrsb_rsb_mstream.ulm_pptr;
	gwsxa_rsb->tups_read=0;
	gwsxa_rsb->flags=0;
        rsb->gwrsb_internal_rsb = (PTR)gwsxa_rsb;
	/*
	**	Set up SXF control block for call
	*/
	sxfrcb.sxf_cb_type=SXFRCB_CB;
	sxfrcb.sxf_length=sizeof(sxfrcb);
	sxfrcb.sxf_ascii_id=SXFRCB_ASCII_ID;
	/*
	**	Check for CURRENT file, map to appropriate
	**	SXF internal name (empty currently)
	*/

	if (xrel->flags&SXA_FLAG_CURRENT)
	{
		if (GWF_SXA_SESS_MACRO(  5))
			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "SXA_OPEN: Using the CURRENT audit file\n");
		sxfrcb.sxf_filename=NULL;
		/*
		**	Flush the current log just in case.
		**	If this fails quitely continue, but may be
		**	missing the latest records.
		*/
		status = sxf_call(SXR_FLUSH, &sxfrcb);
		if ( status!=E_DB_OK)
		{
			if (GWF_SXA_SESS_MACRO(  5))
				gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
				    "SXA_OPEN: Flushing the CURRENT audit file Failed, ignoring\n");
			status=E_DB_OK;
		}
	}
	else
	{
		sxfrcb.sxf_filename=filename;
	}
	/*
	**	Call SXF to open the file
	*/
# ifdef TEST_SXF_OK
	
	if (GWF_SXA_SESS_MACRO(  6))
		status = E_DB_OK;
	else
# endif
	status=sxf_call(SXA_OPEN, &sxfrcb);
	if (status!=E_DB_OK)
	{
		/*
		**	Check if MODIFY - if so silently clean up 
		**	and fill in the page count.
		*/
		if (access_mode==GWT_MODIFY &&
		    (sxfrcb.sxf_error.err_code != E_SX0015_FILE_NOT_FOUND) &&
		    (sxfrcb.sxf_error.err_code != E_SX100D_SXA_OPEN))
		{
			/*
			**	Call SXF for default file size and
			**	use that (assume user is linking 
			**	an old SAL, and the default size hasn't
			**	changed)
			**/
			/*
			** SXF fills in the current file name
			** here so make sure empty buffer
			*/
			sxfrcb.sxf_filename=NULL;
			sxfrcb.sxf_file_size=100;/* Default */
			status=sxf_call(SXS_SHOW,&sxfrcb);
			if (status==E_DB_OK)
			{
				/*
				**	ASSUMPTION: Page size is 2K, 
				**	and sxf_file_size is in KB
				*/	
				rsb->gwrsb_page_cnt=sxfrcb.sxf_file_size/2;
				if (rsb->gwrsb_page_cnt<10)
					rsb->gwrsb_page_cnt=10;
				
				if (GWF_SXA_SESS_MACRO(  4))
				{
					gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
					    "SXA_OPEN: Failed to access file on MODIFY action, using default SXF file size.\n");
				}
				*error=0;
				break;
			}
			else
			{
				if (GWF_SXA_SESS_MACRO(  4))
				{
					gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
					    "SXA OPEN, Unable to access SXF, using default size of 100 pages\n");
				}
				rsb->gwrsb_page_cnt= 100;
				*error=0;
				status=E_DB_OK;
			}
		}
		else
		{
			/*
			**	Something went wrong opening the SAL, so 
			**	check on the return status
			*/
			/*
			**	E_SX0010_SXA_NOT_ENABLED - No C2 enabled 
			*/
			gwsxa_zapblank(filename,SXA_MAX_AUDIT_FILE_NAME);
			gwsxa_zapblank(gwx_rcb->xrcb_tab_name->db_tab_name,
				sizeof(gwx_rcb->xrcb_tab_name->db_tab_name));

			switch(sxfrcb.sxf_error.err_code)
			{
			case E_SX0010_SXA_NOT_ENABLED:
				/*
				**	SXF not enabled - no security
				*/
				gwsxa_error(gwx_rcb,E_GW4012_SXA_NO_AUDITING,
						SXA_ERR_USER|SXA_ERR_LOG,
						2,
						STlength(filename),
						filename,
						STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
						gwx_rcb->xrcb_tab_name->db_tab_name);
				*error= E_GW050E_ALREADY_REPORTED;
				break;
			case E_SX0015_FILE_NOT_FOUND:
			case E_SX002E_OSAUDIT_NO_READ:
				/*
				**	File doesn't exist, or cannot be
				**	read.
				*/
				gwsxa_error(gwx_rcb,E_GW407B_SXA_LOG_NOT_EXIST,
						SXA_ERR_USER|SXA_ERR_LOG,
						2,
						STlength(filename),
						filename,
						STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
						gwx_rcb->xrcb_tab_name->db_tab_name);
				*error= E_GW050E_ALREADY_REPORTED;
				break;
			default:
				/*
				**	Some other open error. Also log
				**	the SXF error.
				*/
				gwsxa_error(gwx_rcb,sxfrcb.sxf_error.err_code,
						SXA_ERR_LOG, 0 );
				gwsxa_error(gwx_rcb,E_GW4058_SXA_OPEN_ERROR,
						SXA_ERR_USER|SXA_ERR_LOG,
						2,
						STlength(filename),
						filename,
						STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
						gwx_rcb->xrcb_tab_name->db_tab_name);
				*error= E_GW050E_ALREADY_REPORTED;
				break;
			}
		}
	}
	/*
	**	Now save the control block info
	**	for later reads/access
	*/
	/*
 	** Copy the data over for later use
	*/
	gwsxa_rsb->sxf_access_id=sxfrcb.sxf_access_id; /* SXF cookie */
	gwsxa_rsb->flags=xrel->flags;
	gwsxa_rsb->flags|=SXA_FLAG_OPEN;       /* This RSB is open */
	/*
	**	Check if database is a registered column. If not, then
	**	get the current database name and save for later use
	*/
	if (xrel->flags&SXA_FLAG_NODB_REG)
	{
		SCF_CB		scf_cb;
		SCF_SCI		sci_list;
		
		gwsxa_rsb->flags|=SXA_FLAG_NODB_REG;
		scf_cb.scf_length = sizeof(SCF_CB);
		scf_cb.scf_type = SCF_CB_TYPE;
		scf_cb.scf_facility = DB_GWF_ID;
		scf_cb.scf_session = DB_NOSESSION;
		scf_cb.scf_len_union.scf_ilength = 1;
		/* may cause lint message */
		scf_cb.scf_ptr_union.scf_sci = (SCI_LIST*)&sci_list;
		sci_list.sci_code = SCI_DBNAME;
		sci_list.sci_length = sizeof(gwsxa_rsb->database);
		sci_list.sci_aresult = (PTR) &gwsxa_rsb->database;
		sci_list.sci_rlength = NULL;
		scf_info_status = scf_call(SCU_INFORMATION, &scf_cb);
		if (scf_info_status != E_DB_OK)
		{
		    status = scf_info_status;
		    gwsxa_error(gwx_rcb,E_GW4078_SXA_BAD_SCF_INFO,
			SXA_ERR_INTERNAL,1,
				sizeof("DATABASE NAME"),"DATABASE NAME");
		    *error = scf_cb.scf_error.err_code;
		    break;
		}
		if (GWF_SXA_SESS_MACRO(4))
		{
			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "SXA OPEN: Current database is '%s'\n",
			    gwsxa_rsb->database.db_db_name);
		}
	}
	/*
	**	Save current file size in the rsb
	**	ASSUMPTION: Page size is 2K, 
	**	and sxf_file_size is in KB. 
	*/	
	rsb->gwrsb_page_cnt= sxfrcb.sxf_file_size/2;
	if (rsb->gwrsb_page_cnt<10)
		rsb->gwrsb_page_cnt=10;
	break;	
    }
    if(status!=E_DB_OK)
    {
	    gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_open_fail);
    }
    return status;
}


/*
** Name: gwsxa_close	- gateway exit to close table
**
** Description:
**	Closes a previously opened SXF audit file
**
** History:
**	14-sep-92 (robf)
**		First created.
**	30-nov-92 (robf)
**	        Update error handling for non-C2 servers. Silently allow
**		SXF failures when E_SX0010_SXA_NOT_ENABLED is returned.
*/
DB_STATUS
gwsxa_close( GWX_RCB     *gwx_rcb )
{
	GW_RSB          *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
	GW_TCB      	*tcb = rsb->gwrsb_tcb;
	GWSXA_XREL  	*xrel = (GWSXA_XREL*)tcb->gwt_xrel.data_address;
	i4         *error = &gwx_rcb->xrcb_error.err_code;
	GWSXA_RSB       *gwsxa_rsb = (GWSXA_RSB *)rsb->gwrsb_internal_rsb;
	DB_STATUS       status;
	SXF_RCB 	sxfrcb;
        char		trbuf[GWF_MAX_MSGLEN + 1]; /* last char for `\n' */

	if (GWF_SXA_SESS_MACRO(3))
	{
		gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
		    "SXA_CLOSE called\n");

	}

        gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_close);
	status=E_DB_OK;

	for(;;)	/* Something to break out of */
	{
		/*
		**	Sanity checks
		*/
		if ( gwsxachkrcb (gwx_rcb,"SXA_CLOSE") != E_DB_OK)
		{
			*error= E_GW050E_ALREADY_REPORTED;
			status=E_DB_ERROR;		       
			break;
		}
		/*
		**	Make sure the extended rsb is still around
		*/
		if ( gwsxa_rsb==NULL)
		{
			gwsxa_error(gwx_rcb,E_GW4079_SXA_NULL_EXTENDED_RSB,
					SXA_ERR_INTERNAL,
					1,
					sizeof("GWSXA_CLOSE")-1,
					"GWSXA_CLOSE");
			*error= E_GW050E_ALREADY_REPORTED;
			status=E_DB_OK;
				
			gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_close_fail);
			/*
			**	Can't continue close since don't know the RSB info
			*/
			break;
		}
		/*
		**	Make sure this is open
		*/
		if (! gwsxa_rsb->flags&SXA_FLAG_OPEN)
		{
			gwsxa_zapblank(xrel->audit_file,sizeof(xrel->audit_file));
			gwsxa_error(gwx_rcb,E_GW4060_SXA_CLOSE_NOT_OPEN,
					SXA_ERR_INTERNAL,
					2,
					STlength(xrel->audit_file),
					xrel->audit_file,
					STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
					gwx_rcb->xrcb_tab_name->db_tab_name);
			*error= E_GW050E_ALREADY_REPORTED;
			status=E_DB_ERROR;
			break;
		}
		/*
		**	Set up SXF control block for call
		*/
		sxfrcb.sxf_cb_type=SXFRCB_CB;
		sxfrcb.sxf_length=sizeof(sxfrcb);
		sxfrcb.sxf_access_id=gwsxa_rsb->sxf_access_id;
		/*
		**	Call SXF to close the file
		*/
# ifdef TEST_SXF_OK
		if (GWF_SXA_SESS_MACRO(6))
			status = E_DB_OK;
		else
# endif
		status=sxf_call(SXA_CLOSE, &sxfrcb);
		if (status!=E_DB_OK)
		{
			/*
			**	Something went wrong closing the SAL, so 
			**	log a warning but continue (since we are just
			**	trying to close it). 
			**
			**	For the case of	auditing not enabled we 
			**	silently continue since this is the expected 
			**	behaviour. (Shouldn't really be trying to
			**	close an audit file when no auditing, but
			**	higher level code can still call us, so
			**	handle it)
			**
			**	The switch here allows for other errors 
			**	to be handled if desired.
			*/
			switch(sxfrcb.sxf_error.err_code)
			{
			case E_SX0010_SXA_NOT_ENABLED:
				/*
				**	OK errors, ignore.
				*/
				break;
			default:
				/*
				**	Log a message to be friendly then
				** 	continue.
				*/
				gwsxa_error(gwx_rcb,sxfrcb.sxf_error.err_code,
						SXA_ERR_LOG, 0 );
				gwsxa_error(gwx_rcb,sxfrcb.sxf_error.err_code,
						SXA_ERR_LOG,0);
				gwsxa_zapblank(xrel->audit_file,sizeof(xrel->audit_file));
				gwsxa_error(gwx_rcb,E_GW4059_SXA_CLOSE_ERROR,
						SXA_ERR_LOG,
						2,
						STlength(xrel->audit_file),
						xrel->audit_file,
						STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
						gwx_rcb->xrcb_tab_name->db_tab_name);
				gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_close_fail);
			};
			/*
			**	Mark status as OK to continue
			*/
			status=E_DB_OK;
		}
		gwsxa_rsb->flags&= ~SXA_FLAG_OPEN;
		gwsxa_rsb->sxf_access_id=0;
		break;
	}
	if(status!=E_DB_OK)
	{
	 	gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_close_fail);
	}
	return status;
}

/*
**	Name: gwsxa_position - position gateway table to a record
**
**	Description:
**		Right now, only position for a scan is allowed,
**		due to SXF. Any other kind of position will generate
**		an error.
**
**	History:
** 	    14-sep-92 (robf)
**		First created
**	    18-dec-92 (robf)
**	        Various error handling cleanups.
**
*/
DB_STATUS
gwsxa_position( GWX_RCB     *gwx_rcb )
{
	GW_RSB          *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
	GW_TCB      	*tcb = rsb->gwrsb_tcb;
	GWSXA_XREL  	*xrel = (GWSXA_XREL*)tcb->gwt_xrel.data_address;
	i4         *error = &gwx_rcb->xrcb_error.err_code;
	GWSXA_RSB       *gwsxa_rsb = (GWSXA_RSB *)rsb->gwrsb_internal_rsb;
	DB_STATUS   status;
	SXF_RCB 	sxfrcb;
        char		trbuf[GWF_MAX_MSGLEN + 1]; /* last char for `\n' */

	if (GWF_SXA_SESS_MACRO(12))
	{
		gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
		    "SXA_POSITION called\n");
	}
	/*
	**	Sanity checks
	*/
	if (gwsxachkrcb (gwx_rcb,"GWSXA_POSITION") != E_DB_OK)
	{
		*error= E_GW050E_ALREADY_REPORTED;
		return E_DB_ERROR;
	}

	if (gwsxa_rsb == NULL)
	{
		gwsxa_error(gwx_rcb,E_GW4079_SXA_NULL_EXTENDED_RSB,
				SXA_ERR_INTERNAL,
				1,
				sizeof("GWSXA_POSITION")-1,
				"GWSXA_POSITION");
		*error= E_GW050E_ALREADY_REPORTED;
		return E_DB_ERROR;
	}
	/*
	**	Make sure this is open
	*/
	if (! gwsxa_rsb->flags&SXA_FLAG_OPEN)
	{
		gwsxa_error(gwx_rcb,E_GW4061_SXA_POSITION_NOT_OPEN,
				SXA_ERR_INTERNAL,
				2,
				sizeof(xrel->audit_file),
				xrel->audit_file,
				STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
				gwx_rcb->xrcb_tab_name->db_tab_name);
		*error= E_GW050E_ALREADY_REPORTED;
		return E_DB_ERROR;
	}
	/*
	**	data1 is the low key. This is NULL(empty) for a scan,
	**	otherwise error
	**/
	if (gwx_rcb->xrcb_var_data1.data_in_size>0)
	{
		gwsxa_error (gwx_rcb,E_GW4003_SXA_SCAN_ONLY,
			SXA_ERR_USER|SXA_ERR_LOG,
			0);
		*error= E_GW050E_ALREADY_REPORTED;
		gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_position_keyed);
		return E_DB_ERROR;
	}
	/*
	**	Set up SXF control block for call
	*/
	sxfrcb.sxf_cb_type=SXFRCB_CB;
	sxfrcb.sxf_length=sizeof(sxfrcb);
	sxfrcb.sxf_access_id=gwsxa_rsb->sxf_access_id;
	/*
	**	Call SXF to position the file
	*/
# ifdef TEST_SXF_OK
	if (GWF_SXA_SESS_MACRO(6))
		status = E_DB_OK;
	else
# endif
	status=sxf_call(SXR_POSITION, &sxfrcb);
	if (status!=E_DB_OK)
	{
		/*
		**	Something went wrong positioning the SAL, so 
		**	check on the return status
		*/
		gwsxa_zapblank(xrel->audit_file,sizeof(xrel->audit_file));
		gwsxa_zapblank(gwx_rcb->xrcb_tab_name->db_tab_name,
			sizeof(gwx_rcb->xrcb_tab_name->db_tab_name));
		gwsxa_error(gwx_rcb,E_GW4062_SXA_POSITION_ERROR,
				SXA_ERR_INTERNAL,
				2,
				sizeof(xrel->audit_file),
				xrel->audit_file,
				STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
				gwx_rcb->xrcb_tab_name->db_tab_name);
		*error= E_GW050E_ALREADY_REPORTED;
		return status;
	}
        return (E_DB_OK);
}

/*
** Name: gwsxa_get - get a gateway record. 
**
** Description:
**	Calls SXF to get the next audit record and converts 
**        requested columns to Ingres format.
**
** History:
**    14-sep-92 (robf)
**	First created
**    18-dec-92 (robf)
**      Emulate access by TID by performing a scan of the table,
**      only restarting the scan if the TID is prior to the current
**	position. 
**      Various error handling cleanups. 
**    2-jul-93 (robf)
**      Check if need security label or detailinfo items and
**	only get from SXF if we need them.
*/
DB_STATUS
gwsxa_get( GWX_RCB *gwx_rcb )
{
	GW_RSB          *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
	i4         *error = &gwx_rcb->xrcb_error.err_code;
	GWSXA_RSB       *gwsxa_rsb = (GWSXA_RSB *)rsb->gwrsb_internal_rsb;
	GW_TCB      	*tcb = rsb->gwrsb_tcb;
	GWSXA_XREL  	*xrel = (GWSXA_XREL*)tcb->gwt_xrel.data_address;
	DB_STATUS   	status;
	SXF_RCB 	sxfrcb;
	SXF_AUDIT_REC	sxf_record;
	char	        *database= gwsxa_rsb->database.db_db_name;
	char	        sxfdetail[SXF_DETAIL_TXT_LEN];
        char		trbuf[GWF_MAX_MSGLEN + 1]; /* last char for `\n' */

	/*
	**	Sanity checks
	*/
	if ( gwsxachkrcb (gwx_rcb,"GWSXA_GET") != E_DB_OK)
	{
		*error= E_GW050E_ALREADY_REPORTED;
		return E_DB_ERROR;
	}
        gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_get);
	/*
	**	Make sure the extended rsb is still around
	*/
	if ( gwsxa_rsb==NULL)
	{
		gwsxa_error(gwx_rcb,E_GW4079_SXA_NULL_EXTENDED_RSB,
				SXA_ERR_INTERNAL,
				1,
				sizeof("GWSXA_GET")-1,
				"GWSXA_GET");
                gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_get_fail);
		/*
		**	Can't continue GET since don't know the RSB info
		*/
		*error= E_GW050E_ALREADY_REPORTED;
		return E_DB_ERROR;
	}
	/*
	**	Make sure this is open
	*/
	if (! gwsxa_rsb->flags&SXA_FLAG_OPEN)
	{
		gwsxa_error(gwx_rcb,E_GW4063_SXA_GET_NOT_OPEN,
				SXA_ERR_INTERNAL,
				2,
				sizeof(xrel->audit_file),
				xrel->audit_file,
				STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
				gwx_rcb->xrcb_tab_name->db_tab_name);
                gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_get_fail);
		*error= E_GW050E_ALREADY_REPORTED;
		return E_DB_ERROR;
	}
	/*
	**	Set up SXF control block 
	*/
	MEfill(sizeof(sxf_record), 0, (PTR)&sxf_record);
	MEfill(sizeof(sxfrcb), 0, (PTR)&sxfrcb);
	sxfrcb.sxf_cb_type=SXFRCB_CB;
	sxfrcb.sxf_length=sizeof(sxfrcb);
	sxfrcb.sxf_access_id=gwsxa_rsb->sxf_access_id;
	sxfrcb.sxf_record= &sxf_record;
	sxf_record.sxf_detail_len=0;
	/*
	** If getting object detail set pointer, otherwise set empty
	*/
	if (gwsxa_rsb->flags&SXA_FLAG_DETAIL_REG)
	{
		/* Place to retrieve detail */
		sxf_record.sxf_detail_txt= sxfdetail; 
	}
	else
		sxf_record.sxf_detail_txt= 0; 
	/*
	**	Check if by TID. We may not be positioned at this point,
	**	so position ready for scan
	*/
	if (gwx_rcb->xrcb_flags & GWX_BYTID)
	{
		/*
		**	If not got rows yet, or reading earlier need to
		**	re-position at the start of the scan.
		*/
                gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_tid_get);
		if(!gwsxa_rsb->tups_read  ||
		    gwsxa_rsb->tups_read > rsb->gwrsb_tid)
		{
			if (GWF_SXA_SESS_MACRO(  2))
			{
				gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
				    "SXA GET: Repositioning to start due to TID request.\n",
					NULL);
			}
			status=sxf_call(SXR_POSITION, &sxfrcb);
			if (status!=E_DB_OK)
			{
				/*
				**	Something went wrong positioning the SAL, so 
				**	check on the return status
				*/
				gwsxa_zapblank(xrel->audit_file,sizeof(xrel->audit_file));
				gwsxa_zapblank(gwx_rcb->xrcb_tab_name->db_tab_name,
					sizeof(gwx_rcb->xrcb_tab_name->db_tab_name));
				gwsxa_error(gwx_rcb,E_GW4062_SXA_POSITION_ERROR,
						SXA_ERR_INTERNAL,
						2,
						sizeof(xrel->audit_file),
						xrel->audit_file,
						STlength(gwx_rcb->xrcb_tab_name->db_tab_name),
						gwx_rcb->xrcb_tab_name->db_tab_name);
				*error= E_GW050E_ALREADY_REPORTED;
				return status;
			}
			gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_tid_scanstart);
		}
		if (!(gwsxa_rsb->flags&SXA_FLAG_POSTID))
		{
			/*
			**	Only issue the warning to the log
			**	once per open.
			*/
			gwsxa_error(gwx_rcb,E_GW407C_SXA_GET_BY_TID,
					SXA_ERR_LOG,0);
			gwsxa_rsb->flags|=SXA_FLAG_POSTID;
		}
	}

	for(;;)	/* Loop looking for records */
	{
		/*
		**	Call SXF to read the file
		*/
# ifdef TEST_SXF_OK
	     if (GWF_SXA_SESS_MACRO(  6))
	     {
		if( sxf_data[gwsxa_rsb->tups_read].rec_no== -1)
		{
			status=E_DB_WARN;
			sxfrcb.sxf_error.err_code=E_SX0007_NO_ROWS;
		}
		else
		{
			int rec=gwsxa_rsb->tups_read;
			status = E_DB_OK;
			sxfrcb.sxf_record->sxf_eventtype=sxf_data[rec].eventtype;
			TMnow(&sxfrcb.sxf_record->sxf_tm);
			STmove(sxf_data[rec].ruserid, ' ',
				sizeof(sxfrcb.sxf_record->sxf_ruserid.db_own_name),
				sxfrcb.sxf_record->sxf_ruserid.db_own_name);
			STmove(sxf_data[rec].euserid, ' ',
				sizeof(sxfrcb.sxf_record->sxf_euserid.db_own_name),
				sxfrcb.sxf_record->sxf_euserid.db_own_name);
			STmove(sxf_data[rec].dbname, ' ',
				sizeof(sxfrcb.sxf_record->sxf_dbname.db_db_name),
				sxfrcb.sxf_record->sxf_dbname.db_db_name
				);
			sxfrcb.sxf_record->sxf_messid=sxf_data[rec].messid;
			sxfrcb.sxf_record->sxf_auditstatus=sxf_data[rec].auditstatus;
			sxfrcb.sxf_record->sxf_userpriv=sxf_data[rec].userpriv;
			sxfrcb.sxf_record->sxf_objpriv=sxf_data[rec].objpriv;
			sxfrcb.sxf_record->sxf_accesstype=sxf_data[rec].accesstype;
			STmove(sxf_data[rec].object, ' ',
				sizeof(sxfrcb.sxf_record->sxf_object);
				sxfrcb.sxf_record->sxf_object);
		}
	    }
	    else
# endif
		status=sxf_call(SXR_READ, &sxfrcb);
		if (status==E_DB_WARN && 
		    sxfrcb.sxf_error.err_code== E_SX0007_NO_ROWS)
		{
			/*
			**	End of data
			*/
			if (GWF_SXA_SESS_MACRO(  2))
			{
				gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
				    "SXA GET: SXF returned no-more-rows after reading %d tuples\n",
				    gwsxa_rsb->tups_read);
			}
			*error=E_GW0641_END_OF_STREAM;
			return status;
			
		}
		else if (status!=E_DB_OK)
		{
			char *tname;
			i4   tlen;
			/*
			**	Something went wrong reading the SAL, so 
			**	check on the return status. Log the
			**	SXF error for traceback.
			*/
			gwsxa_error(gwx_rcb,sxfrcb.sxf_error.err_code,
						SXA_ERR_LOG,0);
			/*
			**	Now print gateway error.  
			**	the check here is in case GWF neglected to
			**	fill in the table name.
			*/
			gwsxa_zapblank(xrel->audit_file,sizeof(xrel->audit_file));
			if (gwx_rcb->xrcb_tab_name==NULL)
			{
				tname="<unknown>";
				tlen=STlength(tname);
			}
			else
			{
				gwsxa_zapblank(gwx_rcb->xrcb_tab_name->db_tab_name,
					sizeof(gwx_rcb->xrcb_tab_name->db_tab_name));
				tname=gwx_rcb->xrcb_tab_name->db_tab_name;
				tlen=STlength(gwx_rcb->xrcb_tab_name->db_tab_name);
			}
			gwsxa_error(gwx_rcb,E_GW4064_SXA_READ_ERROR,
					SXA_ERR_INTERNAL,
					3,
					sizeof(gwsxa_rsb->tups_read),
					(PTR)&(gwsxa_rsb->tups_read),
					STlength(xrel->audit_file),
					xrel->audit_file,
					tlen,
					(PTR)tname);
			*error= E_GW050E_ALREADY_REPORTED;
                        gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_get_fail);
			return status;
		}

		/*
		**	Update other values
		*/
		gwsxa_rsb->tups_read++;	/* Increment number of rows read */
		/*
		**	Dump out the record if trace point is set
		*/
		if (GWF_SXA_SESS_MACRO(7))
		{
			struct TMhuman tmh;
			char   audittmstr[27];
			_VOID_ TMbreak(&(sxfrcb.sxf_record->sxf_tm),&tmh);
			STprintf(audittmstr,"%s-%s-%s %s:%s:%s",
				tmh.day,
				tmh.month,
				tmh.year,
				tmh.hour,
				tmh.mins,
				tmh.sec
				);
			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "=========== Dump of SXF record %d ===========\n",
			    gwsxa_rsb->tups_read);

			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "Time Stamp   : %s\n",
			    audittmstr);

			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "Real User    : %t\n", DB_OWN_MAXNAME,
			    sxfrcb.sxf_record->sxf_ruserid.db_own_name);

			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "Effective Usr: %t\n", DB_OWN_MAXNAME,
			    sxfrcb.sxf_record->sxf_euserid.db_own_name);

			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "Database     : %t\n", DB_DB_MAXNAME,
			    sxfrcb.sxf_record->sxf_dbname.db_db_name);

			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "Usr Priv Mask: %d\n",
			    sxfrcb.sxf_record->sxf_userpriv);

			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "Obj Priv Mask: %d\n",
			    sxfrcb.sxf_record->sxf_objpriv);

			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "Audit Status : %d\n",
			    sxfrcb.sxf_record->sxf_auditstatus);

			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "Event Type   : %d\n",
			    sxfrcb.sxf_record->sxf_eventtype);

			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "Access Type  : %d\n",
			    sxfrcb.sxf_record->sxf_accesstype);

			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "Object Name  : %t\n",
			    DB_MAXNAME, sxfrcb.sxf_record->sxf_object);

			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "Object Owner : %t\n",
			    DB_OWN_MAXNAME,
			    sxfrcb.sxf_record->sxf_ruserid.db_own_name);

			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "Message ID   : %d\n",
			    sxfrcb.sxf_record->sxf_messid);

			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "Detail       : %d '%t'\n",
			    sxfrcb.sxf_record->sxf_detail_int,
			    sxfrcb.sxf_record->sxf_detail_len,
			    sxfrcb.sxf_record->sxf_detail_txt);
		}
	
		/*
		** Check here to see if we have "selective restriction"
		** Basically, if DATABASE is missing from the list
		** of registered columns the we only show audit records
		** for the CURRENT database
		*/
		if (gwsxa_rsb->flags&SXA_FLAG_NODB_REG)
		{
			/*
			**	Check if this record matches the current
			**	database. Compare the two lengths.
			*/
			if (STbcompare(sxf_record.sxf_dbname.db_db_name, 
					sizeof(sxf_record.sxf_dbname.db_db_name),
					database,
					sizeof(gwsxa_rsb->database.db_db_name),
					TRUE)!=0)
			{
				
				if (GWF_SXA_SESS_MACRO(  2))
				{
					gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
					    "SXA GET: Skipping record due to mismatch on database ids '%s', '%s'\n",
					    sxf_record.sxf_dbname.db_db_name,
					    database);
				}
				continue;
			}
		}
		/*
		** Check BYTID
		*/
		if(gwx_rcb->xrcb_flags&GWX_BYTID)
		{
			if (rsb->gwrsb_tid==gwsxa_rsb->tups_read)
			{
				/*
				**	Found the requested tid
				*/
				break;
			}
			else
			{
				/*
				**	Carry on looking
				*/
				continue;
			}
		}
		break;
	}
	/*
	**	And format the output according to the columns needed
	*/
	status=gwsxa_sxf_to_ingres(gwx_rcb,&sxf_record);

	if ( status!=E_DB_OK)
	{
		/*
		**	Conversion should already have printed 
		**	an error message, so just return an
		**	error status. Make sure we return STATUS in case
		**	its ERROR or FATAL
		*/
		*error = E_GW050D_DATA_CVT_ERROR;
		gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_get_fail);
		return status;
	}
	/*
	**	Return a pseudo-TID to the caller
	*/
	rsb->gwrsb_tid = gwsxa_rsb->tups_read;
        return (E_DB_OK);
}

/*
** Name: gwsxa_put	- gateway 'put record' exit
**
**	This generates an error right now, since the
**	audit file records are read-only currently
**
*/
DB_STATUS
gwsxa_put( GWX_RCB *gwx_rcb )
{
    i4	    *error = &gwx_rcb->xrcb_error.err_code;
    *error=E_GW4002_SXA_NO_UPDATE;
    return (E_DB_ERROR);

}

/*
**	Name: gwsxa_replace, update a record in the audit file
**
**	This generates an error right now, since the
**	audit file records are read-only currently
*/
DB_STATUS
gwsxa_replace( GWX_RCB *gwx_rcb )
{
    GW_RSB	    *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    i4	    *error = &gwx_rcb->xrcb_error.err_code;
    GWSXA_RSB	    *gwdmf_rsb = (GWSXA_RSB *)rsb->gwrsb_internal_rsb;
    DMR_CB		dmrcb;
    DB_STATUS		status;

    gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_update_tries);
    *error=E_GW4002_SXA_NO_UPDATE;
    return (E_DB_ERROR);
}

/*
**	Delete a record. This generates an error right now, since the
**	audit file records are read-only currently.
*/
DB_STATUS
gwsxa_delete( GWX_RCB *gwx_rcb )
{
    GW_RSB	    *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    i4	    *error = &gwx_rcb->xrcb_error.err_code;
    GWSXA_RSB	    *gwdmf_rsb = (GWSXA_RSB *)rsb->gwrsb_internal_rsb;
    DMR_CB		dmrcb;
   
    
    gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_update_tries);
    *error=E_GW4002_SXA_NO_UPDATE;
    return (E_DB_ERROR);
}

/*
**	Transaction handling statements, silent no-op right now
*/
DB_STATUS
gwsxa_begin( GWX_RCB *gwx_rcb )
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return (E_DB_OK);
}


DB_STATUS
gwsxa_abort( GWX_RCB *gwx_rcb )
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return (E_DB_OK);
}


DB_STATUS
gwsxa_commit( GWX_RCB *gwx_rcb )
{
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return (E_DB_OK);
}

/*
** Name: gwsxa_info
**
** Description:
**	return info about the gateway
** History:
**	14-sep-92 (robf)
**	    Created.
*/

DB_STATUS
gwsxa_info( GWX_RCB *gwx_rcb )
{
    gwx_rcb->xrcb_var_data1.data_address = GWsxa_version;
    gwx_rcb->xrcb_var_data1.data_in_size = STlength(GWsxa_version) + 1;
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return (E_DB_OK);
}
/*
** Name: gwsxa_sterm 	- Session termination
**
** Description:
**	This routine terminates a gateway session
**
** History:
**	30-oct-92 (robf)
**		Created.
*/
gwsxa_sterm( GWX_RCB *gwx_rcb )
{
    char	trbuf[GWF_MAX_MSGLEN + 1]; /* last char for `\n' */

    if (GWF_SXA_SESS_MACRO(10))
    {
	gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
	    "SXA: Session being terminated\n");
    }
    gwx_rcb->xrcb_error.err_code = E_DB_OK;
    return (E_DB_OK);
}

/*
** Name: gwsxa_init	- SXA Gateway initialization
**
** Description:
**	This routine initializes the SXA Gateway.
**
** History:
**	14-sep-92 (robf)
**	    Created.
**	17-sep-92 (robf)
**	    Added private ULF memory stream to cache message ids
**	30-oct-92 (robf)
**	    Added exit for session termination (VSTERM).
**	24-jan-1994 (gmanning)
**	    Give name to semaphores in order to take advantage of cross 
**	    process semaphores on NT.
**      09-dec-1997 (hweho01)
**          Semaphores "GWsxamsg" and "GWsxa" are not in the shared memory, 
**          they should be CS_SEM_SINGLE type.  
*/
DB_STATUS
gwsxa_init( GWX_RCB *gwx_rcb )
{
    DB_STATUS status;
    DB_STATUS GWSXAmo_attach(void);
    if (gwx_rcb->xrcb_gwf_version != GWX_VERSION)
    {
	gwx_rcb->xrcb_error.err_code = E_GW0654_BAD_GW_VERSION;
	return (E_DB_ERROR);
    }
    for(;;) /* Something to break out of */
    {
	    (*gwx_rcb->xrcb_exit_table)[GWX_VTERM]	= gwsxa_term;
	    (*gwx_rcb->xrcb_exit_table)[GWX_VTABF]	= gwsxa_tabf;
	    (*gwx_rcb->xrcb_exit_table)[GWX_VOPEN]	= gwsxa_open;
	    (*gwx_rcb->xrcb_exit_table)[GWX_VCLOSE]	= gwsxa_close;
	    (*gwx_rcb->xrcb_exit_table)[GWX_VPOSITION]	= gwsxa_position;
	    (*gwx_rcb->xrcb_exit_table)[GWX_VGET]	= gwsxa_get;
	    (*gwx_rcb->xrcb_exit_table)[GWX_VPUT]	= gwsxa_put;
	    (*gwx_rcb->xrcb_exit_table)[GWX_VREPLACE]	= gwsxa_replace;
	    (*gwx_rcb->xrcb_exit_table)[GWX_VDELETE]	= gwsxa_delete;
	    (*gwx_rcb->xrcb_exit_table)[GWX_VBEGIN]	= gwsxa_begin;
	    (*gwx_rcb->xrcb_exit_table)[GWX_VCOMMIT]	= gwsxa_commit;
	    (*gwx_rcb->xrcb_exit_table)[GWX_VABORT]	= gwsxa_abort;
	    (*gwx_rcb->xrcb_exit_table)[GWX_VINFO]	= gwsxa_info;
	    (*gwx_rcb->xrcb_exit_table)[GWX_VIDXF]	= gwsxa_idxf;
	    (*gwx_rcb->xrcb_exit_table)[GWX_VSTERM]	= gwsxa_sterm;

	    gwx_rcb->xrcb_exit_cb_size = 0;
	    gwx_rcb->xrcb_xrelation_sz = SXA_XREL_SIZE;
	    gwx_rcb->xrcb_xattribute_sz = SXA_XATTR_SIZE;
	    gwx_rcb->xrcb_xindex_sz = SXA_XNDX_SIZE;

	    /*
	    **	Allocate private memory stream for message ids
	    */
	    gwsxa_ulm.ulm_facility=DB_GWF_ID;
	    /* Cache 350 messages, objects, access types */
	    gwsxa_ulm.ulm_sizepool=sizeof(SXA_MSG_DESC)*350; 
	    gwsxa_ulm.ulm_blocksize=sizeof(SXA_MSG_DESC); 
	    status=ulm_startup(&gwsxa_ulm);
	    if (status!=E_DB_OK)
	    {
		gwsxa_error(NULL,E_GW0310_ULM_STARTUP_ERROR,
			SXA_ERR_INTERNAL,
			1,
			sizeof(gwsxa_ulm.ulm_sizepool),
			(PTR)gwsxa_ulm.ulm_sizepool);
		break;
	    }
	    sxa_ulm_memleft=gwsxa_ulm.ulm_sizepool;
	    gwsxa_ulm.ulm_memleft= &sxa_ulm_memleft;
	    gwsxa_ulm.ulm_streamid_p = NULL;
	    /*
	    **	Initialize semaphores
	    */
	    status = CSw_semaphore( &GWsxamsg_sem, CS_SEM_SINGLE, "GWsxamsg" );
	    if (status != OK)
	    {
		    gwf_error(status, GWF_INTERR, 0);
		    gwx_rcb->xrcb_error.err_code = E_GW0200_GWF_INIT_ERROR;
		    status = E_DB_ERROR;
		    break;
	    }

	    status = CSw_semaphore( &GWsxa_sem, CS_SEM_SINGLE, "GWsxa" );
	    if (status != OK)
	    {
		    gwf_error(status, GWF_INTERR, 0);
		    gwx_rcb->xrcb_error.err_code = E_GW0200_GWF_INIT_ERROR;
		    status = E_DB_ERROR;
		    break;
	    }

	    /*
	    **	Now open a stream to work with
	    */
	    status=ulm_openstream(&gwsxa_ulm);
	    if (status!=E_DB_OK)
	    {
		gwsxa_error(NULL,E_GW0312_ULM_OPENSTREAM_ERROR,
			SXA_ERR_INTERNAL,
			3,
			sizeof(gwsxa_ulm.ulm_memleft),(PTR)&gwsxa_ulm.ulm_memleft,
			sizeof(gwsxa_ulm.ulm_sizepool),(PTR)&gwsxa_ulm.ulm_sizepool,
			sizeof(gwsxa_ulm.ulm_blocksize),(PTR)&gwsxa_ulm.ulm_blocksize);
		break;
	    }
	    gwx_rcb->xrcb_error.err_code = 0;
	    /* 
	    **  Set up tidp tuple for this gateway's extended attribute 
	    **	table for indexes.
	    */
	    gwx_rcb->xrcb_xatt_tidp = (PTR) &Gwsxa_xatt_tidp;
	    /*
	    **	Attach SXA as a MIB managed object
	    */
	    status=GWSXAmo_attach();
	    if (status!=E_DB_OK)
		break;

	    TRdisplay("SXA C2 Audit Gateway Version %s startup\n",GWsxa_version);
	    break;

    }
    return (status);
}

/*
**	Name: gwsxa_incr - increment a counter protected by a semaphore
**
**	Description:
**	    This provides a standard way to increment statistics 
**	    counter variables in a multi-user environment.
**
**	History:
**	   06-oct-92 (robf)
**		  Created.
*/
VOID
gwsxa_incr( CS_SEMAPHORE *sem, i4 *counterp )
{
    STATUS stat;

    if ( OK == (stat = CSp_semaphore( TRUE, sem ) ) )
    {
        (*counterp)++;
        stat = CSv_semaphore( sem );
    }
    if( stat!=OK)
    {
	TRdisplay("GWSXA_INCR: Semaphore error\n");
    }
}

