/*
** Copyright (c) 1985, 2010 Ingres Corporation
**
**
*/

/**
** Name: DM.H - Internal DMF common definitions.
**
** Description:
**      This files has definitions that are common to header files
**	defined in DMF.  These definitions include common constants,
**	common types and forward types.
**
** History:
**      28-nov-1985 (derek)
**          Created new for jupiter.
**	26-dec-88 (greg)
**	    Add DM929[23]_DM2[RU]_BADLOGICAL
**	 6-feb-89 (rogerk)
**	    Added 8 bytes to DM_MUTEX struct as CS_SEMAPHORE size increased.
**	    Added constants for default values of buffer manager hash table
**	    sizes and #of pages to write during load.
**	    Added pointer to lbmcb in svcb.
**	    Added new error codes for shared buffer manager.
**	 29-Mar-89 (teg)
**	    Added new error message constants for DMM_LIST().
**	 27-apr-89 (mikem)
**	    Added new error codes for logical keys.
**	 3-may-89 (rogerk)
**	    Added error codes for secondary index update errors.
**	 7-jun-89 (rogerk)
**	    Added error codes for bad tuples encountered in database.
**	    Added cluster shared buffer manager connect/allocate errors.
**	12-jun-1989 (rogerk)
**	    Redefined DM_MUTEX to reference CS_SEMAPHORE since the semaphore
**	    structure became a non-fixed size struct that we could not mimic
**	    correctly.
**      21-jun-89 (jennifer)
**          Added identifier for shared memory used for storing
**          security auditing state.
**	15-aug-1989 (mikem)
**	    Added new errors for bug 4953: E_DM904C_ERROR_GETTING_RECORD, 
**	    E_DM904D_ERROR_PUTTING_RECORD.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.
**	    Added svcb_gateway and svcb_gw_xacts to server control block to
**	    indicate the existence of a gateway.  Added gateway error numbers.
**	18-sep-1989 (rogerk)
**	    Added error numbers for 7.0 bug fixes.
**	18-sep-1989 (teg)
**	    Added error numbers for patch operation.
**	 2-oct-1989 (rogerk)
**	    Added error numbers for DM0L_FSWAP operation.
**	24-oct-1989 (teg) 
**	    add error numbers for check table operation.
**	 5-dec-1989 (rogerk)
**	    Added error number for DMVE_SBIPAGE operation.
**	30-dec-1989 (greg)
**	    Add DM9711_ which replaces DM9700 with parameters
**	    Add DM928E which is 923C with parameters
**	02-feb-1990 (fred)
**	    Added E_DM9Bxx errors for DMPE objects (DMF code for dealing with
**	    peripheral objects).
**	23-apr-1990 (rogerk)
**	    Add system shutdown error defines (9395, 9424)
**	14-may-1990 (rogerk)
**	    Add checkpoint errors (9036)
**	    Add new server errros (9426).
**	14-aug-1990 (rogerk)
**	    Add new consistency point error (9397).
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	25-apr-1990 (teresa)
**	    add new type_def DM_MISC_FLAG
**	29-apr-1991 (Derek)
**	    Added new error message for allocation project.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    19-nov-1990 (rogerk)
**		Added timezone to the server control block.  This holds
**		the timezone in which the DBMS server is running.
**		This was done as part of the DBMS Timezone changes.
**	    27-nov-90 (linda)
**		Linda added E_DM904E_GW_DMF_DEADLOCK definition to 6.4 with
**		the following note:
**		    For the case where GWF got a deadlock error from one of 
**		    its DMF calls.  GWF will return E_GW0327_DMF_DEADLOCK to 
**		    the DMF calling routine; this will be translated into 
**		    E_DM904E_GW_DMF_DEADLOCK; and the routine which made the 
**		    call must check for this return code where appropriate.
**		The definition clashed with another 6.5 definition which used
**		904E; and since I could find no reference to the above mentioned
**		gateway error value in 6.4, I did not move the definition over
**		to the 6.5 system.
**	    18-dec-1990 (rogerk)
**		Added error messages for Btree allocate bug fixes.
**	    10-dec-1990 (rogerk)
**		Added svcb_scanfactor for the multi-block IO cost factor
**		to support the SCANFACTOR startup flag.
**	    17-dec-1990 (jas)
**		Added error definitions for the Smart Disk project.
**	     4-feb-1991 (rogerk)
**		Added error definitions for Set Nologging.
**	    25-feb-1991 (rogerk)
**		Added new memory allocate failure error message (9427).
**		Added new rollforward, auditdb errors for Archiver Stability.
**		Added error number for dm0l test routine.
**	    25-mar-1991 (bryanp)
**		Added error messages (93b0-93ef) for Btree Index Compression.
**	    23-jul-91 (markg)
**		Added new error codes 93a2, 93a3 and 93a4 for fix of bug 38679. 
**	    29-jul-1991 (bryanp)
**		Added error messages (9C00-9C08) for buffer manager & journaling
**		Added 9C09 and 9629 for fix to B38527.
**	    19-aug-91 (rogerk)
**		Added new svcb_status values SVCB_RCP, SVCB_ACP, SVCB_CSP to
**		be able to identify the context in which DMF is running.  These
**		states are now set by the respective processes after the SVCB
**		is initialized.
**          1-oct-1991 (jnash)
**              Add E_DM929A_DM0L_CRDB for dm0l_crdb logging error.
**              Also note that E_DM921E_DM0L_BALLOC, E_DM921F_DM0L_BDEALLOC,
**              E_DM9220_DM0L_CALLOC and E_DM9234_DM0L_BPEXTEND are no 
**              longer used. 
**	    17-oct-1991 (rmuth)
**		Add error E_DM93A5_INVALID_EXTEND_SIZE.
**          22-oct-1991 (jnash)
**              Add E_DM93A6_EXTEND_DB
**          22-oct-1991 (jnash)
**		Added new Buffer Manager error numbers.
**	    30-oct-1991 (jnash)
**		Added E_DM_93AA_READ_TCB
**	     3-nov-1991 (rogerk)
**		Added E_DM929B_DM0L_OVFL, E_DM962A_DMVE_OVFL defines.
**	    13-Dec-1991 (rmuth)
**		Added E_DM93AB_MAP_PAGE_TO_FMAP, E_DM92DD_DM1P_FIX_HEADER,
**		E_DM92DE_DM1P_UNFIX_HEADER, E_DM92EE_DM1P_FIX_MAP,
**		E_DM92EF_DM1P_UNFIX_MAP.
**	    30-jan-1992 (bonobo)
**		Removed the redundant typedef to quiesce the ANSI C 
**		compiler warnings.
**          6-jan-92 (jnash)
**              Added DM_GFAULT_MAX_PAGES, E_DM9428_GFAULT_CONFIG.
**	    10-feb-92 (rmuth)
**		Add E_DM92ED_DM1P_BAD_PAGE.
**	31-mar-1992 (bryanp)
**	    Added E_DM92F0_DM1X_WRITE_GROUP.
**	    Specify temporary table defaults for allocation and extend values.
**	    Added svcb_int_sort_size to control in-memory sorting.
**	13-apr-92 (jnash)
**	    Add E_DM93AC_BM_CACHE_LOCK.
**	15-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	    Add E_DM93AC_CONVERT_IIREL, E_DM93AD_IIREL_NOT_CONVERTED.
**	21-apr-1992 (jnash)
**	    Add E_DM9429_EXPAND_POOL.
**	07-jul-1992 (jnash)
**	    Add E_DM942A_RCP_DBINCON_FAIL and E_DM942B_CRASH_LOG.
**    	16-jul-1992 (kwatts)
**          Add TYPEDEFs for the accessor structures for MPF project.
**	29-August-1992 (rmuth)
**	    - Add defaults for table allocation and extend values.
**	    - Add a load of file extend error messages.
**	22-sep-1992 (bryanp)
**	    Add DM93AF message definition.
**      25-sep-1992 (stevet)
**          Changed svcb_timezone of SVCB to svcb_tzcb, which is a pointer
**          to the timezone structure TM_TZ_CB for the server.
**	30-October-1992 (rmuth)
**	    Add E_DM92CF_DM2F_GALLOC_ERROR, E_DM92CE_DM2F_GUARANTEE_ERROR,
**	    E_DM92CD_DM2U_CONVERT_ERR, E_DM92CC_ERR_CONVERT_TBL_INFO,
**	    E_DM92CB_DM1P_ERROR_INFO, E_DM92CA_SMS_CONV_NEEDED
**	8-nov-92 (ed)
**	    Add new errors for DB_MAXNAME upgradedb code
**	15-oct-1992 (jnash)
**	    Reduced logging project.  Add svcb_checksum to reflect
**	    II_DBMS_CHECKSUMS.
**	22-oct-92 (rickh)
**	    Removed DM_CMP_LIST.  It duplicates DB_CMP_LIST.
**	26-oct-1992 (rogerk & jnash)
**	    Reduced Logging Project: Added new error messages.
**	04-nov-92 (jrb)
**	    Added typedef for DML_LOC_MASKS.
**	9-nov-1992 (bryanp)
**	    Added svcb_check_dead_interval to the dmf_svcb.
**	    Add prototype for dmf_udt_lkinit().
**	16-nov-1992 (bryanp)
**	    Add prototypes for new dmfinit functions.
**	26-oct-1992 (rogerk & jnash)
**	    Reduced Logging Project: Added new error messages.
**	05-NOV-92 (rickh)
**	    Added E_DM91A7_BAD_CATALOG_TEMPLATES.
**	14-dec-92 (jrb)
**	    Removed definition of DM_M_DIREXISTS (not used).
**	17-dec-92 (jnash)
**	    Add E_DM9053_INCONS_ALLOC.
**	18-nov-1992 (robf)
**	    Removed definition of svcb_audit_state, no longer needed
**	    as this is now handled by SXF.
**	18-jan-1993 (bryanp)
**	    Add svcb_cp_timer to the DM_SVCB.
**	18-jan-1993 (rmuth)
**	    Add E_DM92BF_DM1P_TABLE_TOO_BIG
**	15-jan-1993 (jnash)
**	    Add E_DM9054_BAD_LOG_RESERVE.
**	22-jan-1993 (jnash)
**	    Add E_DM9055_AUDITDB_PRESCAN_FAIL.
**	30-feb-1993 (rmuth)
**	   - Shorten DM92EB, DM92F3, DM92FA and DM92FC.
**	   - Add E_DM93EF_DM1P_REPAIR, E_DM910A_DM1U_VERIFY_INFO.
**	30-mar-1993 (rmuth)
**	    Add E_DM92EC_DM1P_ADD_EXTEND.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN. The
**		    DM_LOG_ADDR type is now defunct. All new code must use
**		    either the LG_LA type, which is a Log Address, or the LG_LSN
**		    type, which is a Log Sequence Number. If you don't know
**		    which one to use, look further, because it's important:
**		    correct use of the Log Sequence Number and Log Address types
**		    are crucial for proper Ingres operations on shared disk
**		    systems such as the VAXCluster and the ICL Goldrush.
**		Moved prototypes for dmfinit.c functions to dmfinit.h since
**		    they don't really seem to belong in dm.h. Most of these
**		    functions are only used in a few places, whereas dm.h is
**		    seen by all of dmf.
**	15-may-1993 (rmuth)
**	    Add E_DM930D_DM2U_READONLY_UPD_CATS
**	24-may-1993 (bryanp)
**	    Added some new DMF message number definitions.
**	24-may-1993 (jnash)
**	    Add E_DM9037_BAD_JNL_CREATE.
**	24-may-1993 (rogerk)
**	    Added some new DMF message number definitions.
**	21-jun-1993 (bryanp)
**	    Added some new DMF message number definitions.
**	21-jun-1993 (rogerk)
**	    Added some new DMF message number definitions.
**	26-jul-1993 (bryanp)
**	    Remove DM_LKID; all of DMF now uses LK_LKID instead.
**	26-jul-1993 (jnash)
**	    Added some new DMF message number definitions.
**	23-aug-1993 (bryanp)
**	    Add svcb_c_buffers field to the DMF_SVCB for minimal star servers.
**      23-aug-1993 (mikem)
**          Changed E_DM9383_DM2T_UPDATE_LOGICAL_KEY_ERROR and
**          E_DM9384_DM2T_UPDATE_LOGICAL_KEY_ERROR, to
**          E_DM9383_DM2T_UPD_LOGKEY_ERR and E_DM9384_DM2T_UPD_LOGKEY_ERR to
**          bring them under the 32 character limit imposed by ERcompile().
**	23-aug-1993 (jnash)
**	    Added E_DM967A_DMVE_UNDO.
**	23-aug-1993 (rogerk)
**	    Added some new DMF message number definitions.
**      09-sep-1993 (smc)
**          Made obj_owner portable PTR type.
**	20-sep-1993 (andys)
**	    Add E_DM9059_TRAN_FORCE_ABORT.
**	18-oct-1993 (rmuth)
**	    Add E_DM930E_DM0P_MO_INIT_ERR.
**	18-oct-1993 (jnash)
**	    Add E_DM9509_DMXE_PASS_ABORT.
**	18-oct-93 (jrb)
**	    Added DM_WRKSPILL_SIZE and errors E_DM929E-92A1 for MLSorts.
**	18-oct-1993 (rogerk)
**	    Add new error messages.
**	18-oct-93 (swm)
**	    Added a new svcb_tcb_id field after the standard DMF header, for
**	    allocation of a unique id for each newly allocated DMP_TCB. The
**	    per DMP_TCB unique id is for use as a unique lock event value.
**	    This field is currently unused, it has been added as a hook for
**	    my next integration.
**	11-dec-1993 (rogerk)
**	    Added new error message: E_DM9442_REPLACE_COMPRESS.
**	26-oct-93 (swm)
**	    Bug #56440
**	    New svcb_tcb_id field now in use.
**	31-jan-1994 (bryanp)
**	    Added new error message: E_DM9C8D_BAD_FILE_FORCE.
**	31-jan-1994 (rogerk)
**	    Added new error message: E_DM9443_DMR_PREINIT_RECOVER.
**	31-jan-1994 (jnash)
**	    Added new error message: E_DM92A2_DM2D_LAST_TBID.
**	07-mar-1994 (pearl)
**	    Added new error messages: E_DM9060_SESSION_LOGGING_OFF,
**	    E_DM9061_JNLDB_SESSION_LOGGING_ON and 
**	    E_DM9062_NON_JNLDB_SESSION_LOGGING_ON.
**	    The last 2 messages are similar, except that DM9062 is for
**	    non-journaled databases, and does not advise users to perform
**	    a ckpdb.
**	18-apr-1994 (jnash)
**	    fsync project: Add svcb->svcb_load_optim, true if async
**	    writes performed.
**	20-apr-1994 (mikem)
**	    bug #62347
**	    Added new error E_DM91A8_DMM_LOCV_DILISTDIR which logs the pathname
**	    string as a parameter to the error message.  This error message 
**	    obsoletes use of E_DM91A4_DMM_LOCV_DILISTDIR.
**	15-apr-1994 (chiku)
**	    Bug56702: Added new error message: E_DM9070_LOG_LOGFULL.
**	23-may-1994 (andys)
**	    Add E_DM9444_DMD_DIR_NOT_PRESENT	[bug 60702]
**	20-jun-1994 (jnash)
**	    Add W_DM9A11_GATEWAY_PATCH.
**	03-may-1995 (cohmi01)
**	    Add svcb_status items: SVCB_BATCHMODE,SVCB_IOMASTER,SVCB_READAHEAD.
**	19-may-1995 (shero03)
**	    Added KLV
**	02-jun-1995 (cohmi01)
**	    Add svcb_prefetch_hdr, points to memory used by readahead threads.
**	06-mar-1996 (stial01 for bryanp)
**	    Added svcb_page_size.
**	06-mar-1996 (stial01 for bryanp)
**	    DM_MAXTUP is the DMF maximum tuple size. Currently, this is limited
**		by the page format to 16000 bytes on a 16K page (and shorter
**		on smaller pages).
**      06-mar-1996 (stial01) Changes for VPS (variable page size)
**          Maintain pointers for potentially multiple buffer manager
**          memory segments (svcb_bmseg). Maintain svcb_scanfactor and 
**          svcb_c_buffers for each page size.
**          Maintain user-defined maximum tuple length for installation
**          Added DM_MAX_CACHE, DM_MAX_BUF_SEGMENT
**          Added ROUND_UP_TO_EIGHT() and MAX_UNSCALED_OFFSET_PAGE_SIZE() macros
**	06-mar-96 (nanpr01)
**	    Bump up DM_MAXTUP to 32728.
**      13-mar-1996 (thaju02 & nanpr01)
**          New Page Format Project.
**            - Added typedefs for DM_PAGESTAT, DM_PAGESIZE, DM_PGSEQNO,
**              DM_PGCHKSUM.
**            - Changed DMPP_PAGE structure typedef to union typedef.
**	      - Defined DM_COMPAT_PGSIZE.
**	20-may-1996 (ramra01)
**	    Add typedef for DMPP_TUPLE_INFO. Needed for return info
**	    from accessor routines for alter table, version and row lvl
**	    lck projects.	   
**	16-may-96 (stephenb)
**	    Add new replication error codes (DM9550 thru DM95FF). also
**	    add proto for dmd_reptrace().
**	20-jun-1996 (thaju02)
**	    Removed struct _DM_V2_LINEID.  Defined DM_V2_LINEID to be 
**	    i4. Changed DM_REC_NEW and DM_FREE_SPACE to be long.
**	    Modified DM_GET_LINEID_MACRO. Added DM_MAXTUP_MACRO.
**	    Changed DM_OFFSET from u_i2 to i4.
**	17-jul-96 (nick)
**	    Add E_DM9063_LK_TABLE_MAXLOCKS.
**	24-jul-96 (stephenb)
**	    Add define for messages E_DM9568_REP_DBEVENT_ERROR, 
**	    E_DM9569_REP_INQ_CLEANUP and W_DM956A_REP_MANUAL_DIST.
**      26-jul-1996 (ramra01 for bryanp)
**          Added svcb_stat.utl_alter_tbl.
**	    Added error code for convert row for Alter table add/drop
**	    column (E_DM9445_DMR_ROW_CVT_ERROR).  
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Added E_DM93F2_NO_TCB, E_DM93F3_NOT_TCB_CACHE_LOCK,
**          E_DM93F5_NO_BUFFER and E_DMF030_CACHE_NULL for support of
**          the Distributed Multi-Cache Management (DMCM) Protocol.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Added E_DM93F5_ROW_LOCK_PROTOCOL
**          Added E_DM9318_DM1R_ALLOCATE
**      20-jan-97 (stial01)
**          Added E_DM905A_ROW_DEADLOCK, E_DM905B_BAD_LOCK_ROW,
**          E_DM905C_VALUE_DEADLOCK, E_DM905D_BAD_LOCK_VALUE
**	23-jan-97 (hanch04)
**	    Add new DMF_S4_VERSION for OI2.0
**	28-Jan-1997 (jenjo02)
**	    Split svcb_mutex in two yielding svcb_dq_mutex (for DCBs)
**	    and svcb_sq_mutex (for SCBs).
**      28-jan-97 (nicph02)
**          Add E_DM91A9_DMM_LOCV_WORKEXISTS
**	09-apr-97 (cohmi01)
**	    Allow readlock, timeout & maxlocks system defaults to be configured,
**	    add these items to svcb. Add defines for defaults. #74045, #8616.
**      26-feb-97 (dilma04)
**          Add E_DM9064_LK_ESCALATE_RELEASE.
**      27-feb-97 (stial01)
**          Added E_DM9C28_BXV2PAGE_ERROR,E_DM9C29_BXV2KEY_ERROR,
**          E_DM9C2A_NOROWLOCK, E_DM9C2B_REPL_ROWLK
**          Added svcb_ss_lock_list. When sole server, we want the SV_DATATASE
**          lock(s) on a separate lock list that the RCP will inherit for
**          online recovery of a server crash.
**	11-apr-97 (stephenb)
**	    Add error E_DM956D_REP_NO_SHAD_KEY_COL, E_DM956E_REP_BAD_KEY_SIZE
**	    and E_DM956F_REP_NO_ARCH_COL.
**      21-apr-97 (stial01)
**          Added defines for new errors.
**	9-may-97 (stephenb)
**	    Add define for W_DM9570_REP_NO_PATHS
**	20-jun-97 (stephenb)
**	    Add W_DM5571_REP_DISABLE_CDDS define
**	25-jun-97 (stephenb)
**	    Add messages E_DM9572_REP_CAPTURE_FAIL and E_DM9573_REP_NO_SHAD
**	    message W_DM5571_REP_DISABLE_CDDS should be 
**	    W_DM9571_REP_DISABLE_CDDS
**	26-jun-97 (stephenb)
**	    Add message E_DM9574_REP_BAD_CDDS_TYPE
**	26-jun-97 (stephenb)
**	    Add message E_DM9575_REP_BAD_PRIO_TYPE
**      30-jun-97 (stial01)
**          Added new btree recovery errors.
**	4-aug-97 (stephenb)
**	    Add E_DM9576_REP_CPN_CONVERT.
**      20-aug-97 (stial01)
**          Added btree reposition error messages
**      14-oct-97 (stial01)
**          Allow isolation level to be configured. Added svcb_isolation.
**          Added svcb_log_err, to hold flags on whether to log message
**          if readlock=nolock reduces isolation level.
**	28-oct-97 (stephenb)
**	    add E_DM9577_REP_DEADLOCK_ROLLBACK and E_DM9578_REP_QMAN_DEADLOCK.
**	27-aug-1997 (bobmart)
**	    Add svcb_lk_log_esc to svcb; member holds flags on whether t
**	    log lock escalations (on resource or transaction, for system
**	    catalogs or user tables).
**	20-nov-97 (stephenb)
**	    add E_DM957A_REP_BAD_DATE_CMP and W_DM9579_REP_MULTI_SHADOW
**	25-nov-97 (stephenb)
**	    Add replicator locking fields to DMF_SVCB.
**	03-feb-97 (hanch04)
**	    Add new DMF_S5_VERSION for OI2.5
**	17-feb-98 (inkdo01)
**	    Added fields for parallel sort project (svcb_sthread, svcb_strows,
**	    svcb_stnthreads).
**	5-mar-98 (stephenb)
**	    Add defines E_DM957B_REP_BAD_DATE, E_DM957C_REP_GET_ERROR,
**	    E_DM957D_REP_NO_DISTQ, E_DM957E_REP_DISTQ_UPDATE.
**	26-mar-98 (shust01)
**	    Add svcb_status item: SVCB_WB_FUTILITY which is used to keep a
**	    write-behind thread from spinning looking for work when there is
**	    no work for it to do. Bug 88988.
**	3-Apr-98 (thaju02)
**	    Added E_DM9B0B_DMPE_LENGTH_MISMATCH.
**	19-mar-98 (inkdo01)
**	    Add defines for E_DM9714_PSORT_CFAIL and E_DM9715_PSORT_PFAIL.
**	23-mar-98 (inkdo01)
**	    Add define for E_DM9715_PSORT_CRERR.
**	13-apr-98 (inkdo01)
**	    Reorganized parallel sort threads to use factota, rather than
**	    preallocated thread pool.
**	1-jun-98 (stephenb)
**	    Add E_DM957F_REP_DISTRIB_ERROR.
**	06-jun-1998 (nanpr01)
**	    Converted the macros to v1 and v2 macros so that from selective
**	    places they can be called directly to save the pagesize comparison.
**	26-may-1998 (nanpr01)
**	    Added default exchange buffer size in rows and number of exchange
**	    buffers.
**	27-jul-1998 (rigka01)
**	    Cross integrate fix for bug 90691 from 1.2 to 2.0 code line
**          In highly concurrent environment, temporary file names may not
**          be unique so improve algorithm for determining names
**	    Add svcb_DM2F_TempTblCnt 
**	 3-aug-98 (hayke02)
**	    Minor modification to previous change. svcb_DM2F_TempTblCnt now
**	    has type u_i4 (same as stmt_counter).
**	01-oct-1998 (somsa01)
**	    Added E_DM9448_DM2F_NO_DISK_SPACE.
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**	22-Feb_1999 (bonro01)
**	    Remove svcb_log_id.
**	25-apr-99 (stephenb)
**	    Add messages for new verifydb peripheral (coupon) checking 
**	    functionality
**      05-may-1999 (nanpr01,stial01)
**          Added DM_SAME_TRANID_MACRO
**	28-may-1999 (nanpr01)
**	    Add messages for timeout of DDL operation due to online_ckpoint
**	    is in progress. Error E_DM9071_LOCK_TIMEOUT_ONLINE_CKP.
**      31-aug-1999 (stial01)
**          Added svcb_etab_pgsize, svcb_etab_tmpsz
**	28-may-1999 (nanpr01)
**	    Add messages for timeout of DDL operation due to online_ckpoint
**	    is in progress. Error E_DM9071_LOCK_TIMEOUT_ONLINE_CKP.
**      21-oct-1999 (stial01)
**          Added DM9072-DM9076
**      09-feb-2000 (stial01)
**          Added E_DM9077, E_DM9078
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2001 (jenjo02)
**	    Deleted DML_SCF structure. Session's DML_SCB is now 
**	    embedded in SCF's SCB, not separately allocated.
**	    Added svcb_dmscb_offset, offset from SCF's SCB to
**	    DMF's session control block pointer (DML_SCB*).
**	18-Jan-2001 (jenjo02)
**	    Added SVCB_IS_SECURE define to DM_SVCB.
**      01-feb-2001 (stial01)
**          Define DMPP_TUPLE_HDR as a union (Variable Page Type SIR 102955)
**	02-Mar-2001 (hanch04)
**	    Bumped release id to external "II 2.6" internal 860 or 8.6
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      10-may-2001 (stial01)
**          Added btree statistics to svcb_stat in the DM_SVCB
**	05-Mar-2002 (jenjo02)
**	    Added typedefs, errors for Sequence Generators.
**      08-may-2002 (stial01)
**          Added E_DM9580_CONFIG_DBSERVICE_ERROR
**	20-Aug-2002 (jenjo02)
**	    Added E_DM9066_LOCK_BUSY for TIMEOUT=NOWAIT support.
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      23-Oct-2002 (hanch04)
**	    Changed DM_OFFSET and page_size to SIZE_TYPE to support
**	    buffer manager cache > 2 gig.
**	06-Feb-2003 (jenjo02)
**	    Added E_DM9D0B_SEQUENCE_LOCK_TIMEOUT,
**	    E_DM9D0C_SEQUENCE_LOCK_BUSY
**	07-Apr-2003 (hanch04)
**	    Bumped release id to external "II 2.65" internal 865 or 8.65
**	25-Apr-2003 (jenjo02) SIR 111028
**	    Redefined dmd_check() as a macro which calls
**	    dmd_check_fcn(), passing __FILE__ and __LINE__
**	    to improve diagnostics.
**	    Added E_DM9449_DMD_CHECK to log from whence
**	    dmd_check() was called.
**      28-may-2003 (stial01)
**          Added E_DM93C8_BXLOCKERROR_ERROR
**      30-Mar-2004 (hanal04) Bug 107828 INGSRV2701
**          Added E_DM9690_DMVE_IIREL_GET
**      20-sept-2004 (huazh01)
**          Remove the fix for 90691 as it has been rewritten. 
**          INGSRV2643, b111502. 
**      07-apr-2004 (horda03) Bug 96820/INGSRV850
**          To ensure consistency and future expansion, defined
**          DM_ATT. Initially this is a "unsigned char", to ensure
**          that attributes >127 are not negative. There is a need
**          to declare the type as an u_i2, to handle Index log records
**          where a key's base table attribute is >255; but this will
**          invalidate existing journals.
**      13-may-2005 (horda03) Bug 114508/INGSRV3301
**          Added E_DM9C2C_NOPAGELOCK.
**      16-sep-2003 (stial01)
**          Added svcb_etab_type (SIR 110923)
**      25-nov-2003 (stial01)
**          Added DM723, svcb_etab_logging to set etab logging on/off
**      02-jan-2004 (stial01)
**          Removed svcb fields for temporary trace points DM722, DM723
**	13-jan-2004 (sheco02)
**	    Bumped release id to external "II 3.0" internal 900 or 9.0
**	15-Jan-2004 (schka24)
**	    Add errors for partitioned tables.
**	21-Jan-2004 (jenjo02)
**	    Added typedef for DM_TID8 and defines.
**	6-Feb-2004 (schka24)
**	    Added name-generator number and ID to svcb; remove old
**	    temp table number.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Change DM_TUPLEN_MAX back to 32767 until we can support > max i2.
**      18-feb-2004 (stial01)
**          Define DM_TUPLEN_MAX_V5 for 256k row support
**      30-Mar-2004 (hanal04) Bug 107828 INGSRV2701
**          Added E_DM9690_DMVE_IIREL_GET.
**	5-Apr-2004 (schka24)
**	    Define max # of log rawdata types.
**	09-Apr-2004 (jenjo02)
**	    Added E_DM904F_ERROR_UNFIXING_PAGES
**      14-apr-2004 (stial01)
**          Added svcb_maxbtree (SIR 108315)
**      11-may-2004 (stial01)
**          Removed unnecessary svcb_maxbtreekey
**	11-may-2004 (thaju02)
**	    Added E_DM9CAE_FIND_COMPENSATED, E_DM9CAF_ONLINE_DUPCHECK, 
**	    E_DM9CB0_TEST_REDO_FAIL, E_DM9CB1_RNL_LSN_MISMATCH,
**	    E_DM9CB2_NO_RNL_LSN_ENTRY.
**	02-jun-2004 (thaju02)
**	    Added E_DM9CB3_ONLINE_MOD_RESTRICT.
**	10-Aug-2004 (jenjo02)
**	    Make dm0m_check() macro at least check for a non-NULL
**	    object of the correct type to guard against 
**	    QEF parallel query mistakes.
**      08-nov-2004 (stial01)
**          Added svcb_dop
**	15-oct-2004 (devjo01)
**	    Add E_DM9C91_BM_LSN_MISMATCH.
**	17-nov-2004 (devjo01)
**	    Add SVCB_CLASS_NODE_AFFINITY.
**      16-dec-2004 (stial01)
**          Changes for config param system_lock_level
**	28-Dec-2004 (jenjo02)
**	    Added E_DM9C92_UNEXTEND_DB.
**	30-mar-2005 (stial01)
**	    Added svcb_pad_bytes
**      06-apr-2005 (stial01)
**          Added E_DM944A_RCP_DBINCONSISTENT, E_DM944B_RCP_IDXINCONS
**      11-may-2005 (stial01)
**          Added svcb_csp_dbms_id for CSP class start/stop messages
**	4-Jun-2005 (schka24)
**	    Added hash-join temp file name generator.
**	13-Jul-2005 (schka24)
**	    Remove group-size limit msg E_DM9428.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Added defines for all those DM0M memory type structures,
**	    prefixed with "DM_" to consolidate "type" numbering
**	    in one place. Please continue the effort by adding
**	    new defines here (don't leave holes in the numbers)
**	    and mapping your new structure's ?_CB define to the
**	    corresponding DM_?_CB define.
**      27-Jul-2006 (kschendel)
**          (Datallegro) Reduce DM_MWRITE_PAGES to 2.  16 is nice,
**          but it eats memory like crazy when trying to build thousands
**          of partitions in a large page size.
**          NOTE: this wouldn't be necessary if mwrite_pages were exposed
**          as a config parameter!
**      30-Aug-2006 (kschendel)
**          DM_MWRITE_PAGES is indeed now a parameter, put the default back.
**      28-Apr-2006 (hweho01)
**          Changed the table level to 9.1 for new release. 
**      20-oct-2006 (stial01)
**          Table level should not advance for ingres2006r2.
**      16-oct-2006 (stial01)
**          Added DM_LLOC_CB
**	07-May-2007 (drivi01)
**	    Changed the table level to 9.2 for new release.
**	13-Jul-2007 (kibro01) b118695
**	    Add DM911E_ALLOC_FAILURE error
**      02-may-2008 (horda03) Bug 120349
**          Add DM_RAWD_TYPE_MEANING define.
**	18-Aug-2008 (kibro01) b120490
**	    Add E_DM9582_REP_NON_JNL error
**      25-Sep-2008 (hanal04) Bug 120942
**          AddedE_DM9717_SORT_COST_ERROR.
**      05-may-2009 (stial01)
**          Added E_DM944C_RCP_TABLEINCONS
**	29-May-2009 (kschendel) b122119
**	    Add E_DM905E for unexpected arithmetic exceptions.
**      10-sep-2009 (stial01)
**          Changed the table level (relcmptlvl) to 10.0. 
**	23-Oct-2009 (kschendel) SIR 122739
**	    Repurpose 9398 error for compression control problem.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Rework relcmptlvl version to be a simple integer.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Add DM5065 warning.
**	26-Feb-2010 (jonj)
**	    Define DM_CRIB_CB for cursor CRIBs.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Define DM_BQCB_CB for blob query context blocks.
*/

/*
**  Forward and/or External typedef/struct references.
*/

/*  Types defined in this file. */

typedef struct _DM_OBJECT   DM_OBJECT;
typedef struct _DM_SVCB	    DM_SVCB;
typedef struct _DM_STATS    DM_STATS;
typedef struct _DMP_MISC    DMP_MISC;
typedef struct _DM0M_STAT   DM0M_STAT;
typedef union  _DM_TID	    DM_TID;
typedef union  _DM_TID8     DM_TID8;
typedef struct _DM_TIMESTAMP  DM_TIMESTAMP;
typedef u_i2                DM_V1_LINEID;
typedef SIZE_TYPE           DM_OFFSET;
typedef u_i2                DM_LINE_IDX;
typedef i4 	    	    DM_V2_LINEID;
typedef union  _DM_LINEID   DM_LINEID;
typedef u_i4		    DM_PAGENO;
typedef i4		    DM_CID;
typedef u_i4                DM_PAGESTAT;
typedef u_i4		    DM_PAGESIZE;
typedef u_i4                DM_PGSZTYPE;
typedef u_i4                DM_PGSEQNO;
typedef u_i2                DM_PGCHKSUM;

/* DM_ATT type for referencing the a Base Table
** attribute.
**
** FIX ME: Currently defined as u_i1, this doesn't
**         allow a reference to an attribute >255.
**         Changing DM_ATT to a larger type (u_i2
**         say) will invalidate existing journal
**         records (INDEX/MODIFY).
*/
typedef u_i1                DM_ATT;

/*  Types with definitions in DML.H	*/

typedef struct _DML_ODCB	DML_ODCB;
typedef struct _DML_LOC_MASKS	DML_LOC_MASKS;
typedef struct _DML_SCB		DML_SCB;
typedef struct _DML_SLCB	DML_SLCB;
typedef struct _DML_SPCB	DML_SPCB;
typedef struct _DML_XCB		DML_XCB;
typedef struct _DML_XCCB	DML_XCCB;
typedef struct _DML_SEQ	        DML_SEQ;
typedef struct _DML_CSEQ        DML_CSEQ;

/*  Types with definitions in DMP.H  */

typedef struct _DMP_DCB	    DMP_DCB;
typedef struct _DMP_EXT	    DMP_EXT;
typedef struct _DMP_FCB	    DMP_FCB;
typedef struct _DMP_HCB     DMP_HCB;
typedef struct _DMP_RCB	    DMP_RCB;
typedef struct _DMP_TCB	    DMP_TCB;
typedef struct _DMP_PFH	    DMP_PFH;

/*  Types with definitions in DMPP.H */

typedef union  _DMPP_PAGE   DMPP_PAGE;
typedef union  _DMPP_TUPLE_HDR   DMPP_TUPLE_HDR;
typedef struct _DMPP_ACC_PLV DMPP_ACC_PLV;
typedef struct _DMPP_ACC_TLV DMPP_ACC_TLV;
typedef struct _DMPP_ACC_KLV DMPP_ACC_KLV;
typedef struct _DMPP_SEG_HDR	DMPP_SEG_HDR;

/*  Types with definitions is DM0PBMCB.H. */

typedef struct _DMP_BMSCB   DMP_BMSCB;
typedef struct _DMP_LBMCB   DMP_LBMCB;
typedef struct _DM0P_BM_COMMON DM0P_BM_COMMON;

/*  Types with definitions in DM1U.H. */

typedef struct  _DM1U_CB   DM1U_CB;

/*  Types with definitions in DM0LLCTX.H. */

typedef struct	_DMP_LCTX   DMP_LCTX;


/*
**  Defines of constants.
*/

#define		DM_PG_SIZE      2048L
#define		DM_NO_TUPINFO	-1

#define		DM_COMPAT_PGSIZE    2048L
#define		DM_COMPAT_PGTYPE    1

#define		DM_PG_INVALID	    0 /* same as DB_PG_INVALID */
#define		DM_PG_V1	    1 /* same as DB_PG_V1 */
#define		DM_PG_V2	    2 /* same as DB_PG_V2 */
#define		DM_PG_V3	    3 /* same as DB_PG_V3 */
#define		DM_PG_V4	    4 /* same as DB_PG_V4 */
#define		DM_PG_V5	    5 /* same as DB_PG_V5 */
#define		DM_PG_V6	    6 /* same as DB_PG_V6 */
#define		DM_PG_V7	    7 /* same as DB_PG_V7 */

#define		DM_MAXTUP	32728
#if DM_MAXTUP < DB_MAXTUP
blow chunks now!
#endif

/* added this constant due to iirelation.relwid datatype restriction
** of signed i2.  
*/
#define		DM_TUPLEN_MAX		32767
#define		DM_TUPLEN_MAX_V5	256*1024

#define        DM_REC_NEW        0x80000000        /* record new indicator */
#define        DM_FREE_SPACE     0x40000000        /* free space indicator */

#define	       DM_OFFSET_MASK	 0x0001FFFF	   /* vers2 */
#define	       DM_FLAGS_MASK	 0xFFFE0000	   /* vers2 */
/* 
** Logical defining name of memory segment used for auditing information.
*/
#define         DM_MEM_NAME     "iimemsec"

/*
** Default values used to initialize DMF when user does not give
** specific values.
*/

#define		DM_DBCACHE_SIZE	    20	    /* number of db cache locks
					    ** maintained by buffer manager */
#define		DM_TBLCACHE_SIZE    40	    /* number of table cache locks
					    ** maintained by buffer manager */
#define		DM_MWRITE_PAGES	    16	    /* number of pages written with one
					    ** write IO during load routines */
#define		DM_DEFAULT_CACHE_NAME	"CACH_DEF"  /* Shared cache name if
					    ** shared buffer manager option
					    ** is specified, but a cache name
					    ** is not given. */
#define		DM_WRKSPILL_SIZE   64	    /* if an object is small than this
					    ** number (in pages) then we put it
					    ** on a single location; if larger
					    ** then we start using more work
					    ** locations
					    */
#define		DM_DEFAULT_MAXLOCKS 10	    /* sys default - max # of page locks */
#define		DM_DEFAULT_WAITLOCK  0	    /* sys default - wait on lock forever */
#define		DM_DEFAULT_EXCH_BSIZE 5000  /* default exchange buffer size
					    ** in # of rows */
#define 	DM_DEFAULT_EXCH_NBUFF 3	    /* default # of exchange buffers */
/*
** Max server configuration values
*/


/*
** Maximum number of buffer caches.
** A separate buffer cache is managed for each page size the installation
** has been configured for.
*/
#define DM_MAX_CACHE DB_MAX_CACHE

/*
** Maximum number of memory segments allocated for buffer caches
** The number of memory segments allocated depends on configuration
** parameters which allow you to specify one buffer cache per memory segment.
*/
#define DM_MAX_BUF_SEGMENT DB_MAX_CACHE

/* Type codes for the RAWDATA log record.  These are somewhat generic
** and data structures may want or need to allocate members based on
** the number of types, so they're defined here rather than in dm0l.h.
** (Indeed, dm0l doesn't really care about the contents of RAWDATA records,
** as the name implies.)
** *Note* update DM_RAWD_TYPES if you add more type codes.
*/
#define DM_RAWD_PARTDEF		0	/* A partition definition */
#define DM_RAWD_PPCHAR		1	/* A ppchar array */

#define DM_RAWD_TYPE_MEANING "PARTDEF,PPCHAR"

#define DM_RAWD_TYPES		2	/* Number of types defined */


/*
**	Stuff for dm0m_allocate, here rather than in dm0m.h
**	for more robust exposure and define resolution:
**
**      Constants used for flag argument of dm0m_allocate().
*/

#define                 DM0M_ZERO       	0x0001L
#define                 DM0M_LOCKED		0x0010L

/*
**	Constants used for type argument of dm0m_allocate().
**
**	(default_memory_class | type) where
**	
**	 type is a unique number in the range 1-8191
**					(0x0001 - 0x1fff)
**	
**	 default_memory_class is either
**
**		DM0M_LONGTERM	(0x4000) memory that persists
**				beyond the life of a session,
**				typically for use by other
**				sessions.
**		DM0M_SHORTTERM	(0x2000) short-term session
**				memory. If any remains when
**				the session terminates, it 
**				will be deallocated, though
**				one is expected to do
**	 			explicit deallocations
**				(auto deallocation 
**				is -not- a "feature").
**
**	NB: any individual dm0m_allocate for an object
**	    may explicitly override its default_memory_class
**	    in the "flag" argument.
**
**	NB: in a SINGLEUSER or Ingres-threaded environment, 
**	    DM0M will allocate only LONGTERM memory.
**	
*/
#define                 DM0M_LONGTERM		0x4000L
#define                 DM0M_SHORTTERM		0x2000L

#define		DM_TCB_CB		( 1 | DM0M_LONGTERM)
#define         DM_RCB_CB		( 2 | DM0M_LONGTERM)
#define         DM_FCB_CB		( 3 | DM0M_LONGTERM)
#define         DM_DCB_CB		( 4 | DM0M_LONGTERM)
#define         DM_SVCB_CB		( 5 | DM0M_LONGTERM)
#define         DM_SCB_CB 		( 6 | DM0M_SHORTTERM)
#define         DM_XCB_CB		( 7 | DM0M_SHORTTERM)
#define         DM_ODCB_CB		( 8 | DM0M_SHORTTERM)
#define         DM_SLCB_CB		( 9 | DM0M_SHORTTERM)
#define         DM_SPCB_CB		(10 | DM0M_SHORTTERM)
#define         DM_BMSCB_CB		(11 | DM0M_LONGTERM)
#define         DM_HCB_CB		(12 | DM0M_LONGTERM)
#define         DM_MISC_CB		(13 | DM0M_SHORTTERM)
#define         DM_XCCB_CB		(14 | DM0M_SHORTTERM)
#define		DM_SORT_CB		(15 | DM0M_SHORTTERM)
#define		DM_MXCB_CB		(16 | DM0M_SHORTTERM)
#define         DM_LCTX_CB		(17 | DM0M_LONGTERM)
#define         DM_DMVECB_CB		(18 | DM0M_SHORTTERM)
#define         DM_LCT_CB		(19 | DM0M_SHORTTERM)
#define         DM_DM0J_CB		(20 | DM0M_LONGTERM)
#define         DM_RCP_CB		(21 | DM0M_LONGTERM)
#define         DM_RDB_CB		(22 | DM0M_LONGTERM)
#define         DM_RTX_CB		(23 | DM0M_LONGTERM)
#define         DM_CNF_CB		(24 | DM0M_SHORTTERM)
#define         DM_ACB_CB		(25 | DM0M_LONGTERM)
#define         DM_DBCB_CB		(26 | DM0M_LONGTERM)
#define         DM_EXT_CB		(27 | DM0M_LONGTERM)
#define         DM_RDMU_CB		(28 | DM0M_LONGTERM)
#define         DM_PFH_CB		(29 | DM0M_LONGTERM)
#define         DM_SEQ_CB		(30 | DM0M_LONGTERM)
#define         DM_LBMCB_CB		(31 | DM0M_LONGTERM)
#define         DM_CSEQ_CB		(32 | DM0M_SHORTTERM)
#define         DM_PCB_CB		(33 | DM0M_SHORTTERM)
#define         DM_DM0D_CB		(34 | DM0M_LONGTERM)
#define         DM_RLP_CB		(35 | DM0M_LONGTERM)
#define		DM_RTB_CB		(36 | DM0M_LONGTERM)
#define         DM_RTBL_CB		(37 | DM0M_LONGTERM)
#define         DM_LOCM_CB		(38 | DM0M_SHORTTERM)
#define	   	DM_TBIO_CB		(39 | DM0M_LONGTERM)
#define		DM_RFPTX_CB		(40 | DM0M_LONGTERM)
#define 	DM_RFPBQTX_CB		(41 | DM0M_LONGTERM)
#define        	DM_DM2U_IND_CB		(42 | DM0M_SHORTTERM)
#define        	DM_DM2U_FPG_CB		(43 | DM0M_SHORTTERM)
#define         DM_SIDEFILE_CB		(44 | DM0M_SHORTTERM)
#define		DM_REP_CB		(45 | DM0M_LONGTERM)
#define	    	DM_TBL_CB		(46 | DM0M_LONGTERM)
#define         DM_CPP_LOCM_CB		(47 | DM0M_LONGTERM)
#define         DM_RFP_LOCM_CB		(48 | DM0M_LONGTERM)
#define         DM_CPP_FNMS_CB		(49 | DM0M_LONGTERM)
#define         DM_RFP_OCTX_CB		(50 | DM0M_LONGTERM)
#define        	DM_RFP_LSN_WAIT_CB	(51 | DM0M_LONGTERM)
#define        	DM_ATX_CB		(52 | DM0M_LONGTERM)
#define        	DM_ABS_CB		(53 | DM0M_LONGTERM)
#define        	DM_TBL_HCB_CB		(54 | DM0M_LONGTERM)
#define        	DM_DM0C_T_EXT_CB	(55 | DM0M_LONGTERM)
#define        	DM_ETL_CB		(56 | DM0M_LONGTERM)
#define		DM_LLOC_CB		(57 | DM0M_SHORTTERM)
#define		DM_CRIB_CB		(58 | DM0M_SHORTTERM)
#define		DM_BQCB_CB		(59 | DM0M_LONGTERM)

/* Don't forget to update this... */
#define		DM0M_MAX_TYPE		 59

/*
**  Define macro for dm0m_check to avoid function call at entry point to all
**  dmf routines.
**
**  If xDEBUG is not turned on then don't define macro which means dm0m_check
**  routine will be called.
**  If no xDEBUG, minimally check for non-NULL object of the
**  correct type, call CS_breakpoint() for debugging if it looks bad.
*/
#ifdef xDEBUG
FUNC_EXTERN DB_STATUS dm0m_check(
			DM_OBJECT	*object,
			i4		type_id);
#else
#define			dm0m_check(arg1, arg2) \
	( (arg1 == NULL || ((DM_OBJECT*)arg1)->obj_type != (u_i2)arg2)\
	    && !CS_breakpoint() )
#endif /* xDEBUG */

/*
**  Vax Cluster defines of constants.
**  The assumption is that all node names are unique 
**  to six characters and that a cluster has a maximum
**  of six nodes.
*/

#define                 DM_CNODE_MAX    16
#define                 DM_CNAME_MAX     6

/*
**  Type and constant definitions for passing flag arguments to some of the dmf
**  database create/extend routines.
*/
typedef i4		    DM_MISC_FLAG;
   

/*
** Define DMF version of tables (iirelation.relcmptlvl).
**
** DMF_T0_VERSION : the initial value used for Ingres Release 6.
** Tables at this level utilize NULLS, multiple locations and the
** new 6.0 datatypes vchar, char.
**
** DMF_T1_VERSION : New Hash algorithm version.
** Used to distinguish which hashing algorithm to use.  All new hash tables
** created (or modified) at or after this version will use the new hash
** algorithm. All hash tables previous to this version must use the old hash
** function.
**
** *Note* T0 and T1 version tables are no longer supported as of 10.0.
** In the unlikely instance that someone has a pre-6.1 table, it will
** have to be dumped and reloaded using an older version of Ingres.
**
** The DMF_T2_VERSION : Compression algorithm version.
** Used to distinguish compression capabilities.  Versions previous to this
** only compressed non-nullable C and TEXT datatypes.  Tables modified to a 
** compressed structure since this version will compress all character 
** datatypes as well as nullable datatypes.  The system does not support
** compressed tables with 6.0 versions previous to DMF_T2_VERSION.  Users
** must modify the tables before using them (versions previous to 6.0 are
** supported since they contained no datatypes for which the compression
** algorithm was changed.
**
** DMF_T4_VERSION: Ingres 2.0 mistakenly included tuple header into
** index page keys-per-page calcs for btree.
**
** DMF_T5_VERSION: SI btrees changed to only carry keys on index pages.
** Prior to DMF_T5_VERSION, the leaf and index pages carried all columns
** of the secondary index.
**
** DMF_T8_VERSION: SI btree leaf page range entries changed to not carry
** non-key columns.
**
** DMF_T10_VERSION: version number changed from character string-ish
** thing to an integer xxxyyzz (xxx = major version, yy = minor version,
** zz = service pack / patch / build level).  Note that the relcmptlvl
** version *need not* advance with the Ingres version!  it only needs to
** change when table storage formats change.
** Also in T10: new MVCC page types;  all btrees use leaf overflow pages
** (previously was only done for V1/2k page btrees).
*/

#define		DMF_T0_VERSION		CV_C_CONST_MACRO('6', '.', '0', ' ')
#define		DMF_T1_VERSION		CV_C_CONST_MACRO('6', '.', '0', 'a')
#define		DMF_T2_VERSION		CV_C_CONST_MACRO('6', '.', '1', 'a')
#define		DMF_T3_VERSION		CV_C_CONST_MACRO('7', '.', '0', ' ')
#define		DMF_T4_VERSION		CV_C_CONST_MACRO('8', '.', '0', ' ')
#define		DMF_T5_VERSION		CV_C_CONST_MACRO('8', '.', '5', ' ')
#define		DMF_T6_VERSION		CV_C_CONST_MACRO('8', '.', '6', ' ')
#define		DMF_T7_VERSION		CV_C_CONST_MACRO('9', '.', '0', ' ')
#define		DMF_T8_VERSION		CV_C_CONST_MACRO('9', '.', '1', ' ')
#define		DMF_T9_VERSION		CV_C_CONST_MACRO('9', '.', '2', ' ')
/* This version was only used in development until the change-over to
** pure numeric versions.  It will never occur in production databases.
** Note that it's actually less than the older versions.
*/
#define		DMF_T10x_VERSION	CV_C_CONST_MACRO('1', '0', '.', '0')

/* All the "old" versions are larger than this as a u_i4: */
#define		DMF_T_OLD_VERSION	DU_OLD_DBCMPTLVL

#define		DMF_T10_VERSION		DU_MAKE_VERSION(10,0,0)
#define		DMF_T_VERSION		DMF_T10_VERSION

/*
** Default values for table allocation and extend amounts
*/
#define         DM_TBL_DEFAULT_ALLOCATION  4
#define         DM_TBL_DEFAULT_EXTEND      16
#define         DM_TBL_DEFAULT_FHDR_PAGENO 1  
#define         DM_TBL_INVALID_FHDR_PAGENO 0

/*
** Default values for temporary table allocation and extend amounts
*/
#define		DM_TTBL_DEFAULT_ALLOCATION  32
#define		DM_TTBL_DEFAULT_EXTEND	    32

/*
** Macros
*/

#define DM_V1_SIZEOF_LINEID_MACRO		\
	((i4)sizeof(DM_V1_LINEID)) 	

#define DM_V2_SIZEOF_LINEID_MACRO		\
	((i4)sizeof(DM_V2_LINEID)) 	

#define DM_VPT_SIZEOF_LINEID_MACRO(page_type)	\
	(page_type == DM_COMPAT_PGTYPE ?		\
	 DM_V1_SIZEOF_LINEID_MACRO :		\
	 DM_V2_SIZEOF_LINEID_MACRO )

#define DM_V1_GET_LINEID_MACRO(line_tab, index)	\
	(((DM_V1_LINEID *)line_tab)[(index)])

#define DM_V2_GET_LINEID_MACRO(line_tab, index)	\
	(((DM_V2_LINEID *)line_tab)[(index)]) 

#define DM_VPT_GET_LINEID_MACRO(page_type, line_tab, index)	\
	(page_type == DM_COMPAT_PGTYPE ?			\
	  DM_V1_GET_LINEID_MACRO(line_tab, index) :	\
	  DM_V2_GET_LINEID_MACRO(line_tab, index) )

#define DM_SAME_TRANID_MACRO(t1, t2) \
	((t1.db_low_tran == t2.db_low_tran && \
	 t1.db_high_tran == t2.db_high_tran) ? 1 : 0)

#define DM_VALID_PAGE_TYPE(page_type)			\
((page_type == DM_PG_V1 || page_type == DM_PG_V2 ||	\
page_type == DM_PG_V3 || page_type == DM_PG_V4 || 	\
page_type == DM_PG_V5 || page_type == DM_PG_V6 ||	\
page_type == DM_PG_V7) ? 1 : 0) 



/* 
** Name: DM_LINEID - union of line table entry formats.
**
** History:
**	04-apr-1996 (thaju02)
**	    New Page Format Project.
**		Created.
*/
union _DM_LINEID
{
    DM_V1_LINEID	dm_v1_lineid;
    DM_V2_LINEID	dm_v2_lineid;
};

/*}
** Name: DM_TIMESTAMP - A general 8 bytes timestamp.
**
** Description:
**      This structure describes the type of structure used to contain
**	a generic timestamp.
**
** History:
**     02-jun-1987 (jennifer)
**          Created new for jupiter
*/
struct _DM_TIMESTAMP
{
    u_i4         high;              /* High value of timestamp. */
    u_i4         low;               /* Low value of timestamp. */
};

/*}
** Name: DMP_MISC - Unspecified data control block.
**
** Description:
**      This control block is used to allocate unspecified type of
**	memory.
**
** History:
**     06-dec-1985 (derek)
**          Created new for jupiter.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	15-Aug-2005 (jenjo02)
**	    Map MISC_CB to DM_MISC_CB
*/
struct _DMP_MISC
{
    PTR	    misc_next;             /* Not used. */
    PTR	    misc_prev;		    /* Not used. */
    SIZE_TYPE	    misc_length;	    /* Length of control block. */
    i2              misc_type;              /* Type of control block for
                                            ** memory manager. */
#define                 MISC_CB             DM_MISC_CB
    i2              misc_s_reserved;        /* Reserved for future use. */
    PTR             misc_l_reserved;        /* Reserved for future use. */
    PTR             misc_owner;             /* Owner of control block for
                                            ** memory manager.  SVCB will be
                                            ** owned by the server. */
    i4         misc_ascii_id;          /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 MISC_ASCII_ID       CV_C_CONST_MACRO('M', 'I', 'S', 'C')
    char	    *misc_data;		    /* Pointer to allocate data. */
};

/*}
** Name: DM_MUTEX - Definition of a mutex.
**
** Description:
**	This is the DMF structure to use for SEMAPHORE calls.  It is defined
**	to be identical to the CS_SEMAPHORE structure which is required for
**	all semaphore calls.
**
**	This structure definition is left around mostly for historical reasons.
**
** History:
**      01-oct-1986 (Derek)
**	    Created for Jupiter.
**	12-jun-1989 (rogerk)
**	    Redefined to reference CS_SEMAPHORE since the semaphore structure
**	    became a non-fixed size struct that we could not mimic correctly.
[@history_template@]...
*/
typedef CS_SEMAPHORE	DM_MUTEX;

/*
**  SVCB - SECTION
**
**       This section defines the structure of the DMF server static  variables
**       which are all located in the server control block (SVCB).
**       The following structs are defined:
**           
**           DM_SVCB                 Server Control Block.
*/

/*}
** Name:  DM_SVCB - DMF internal control block for static server information.
**
** Description:
**      This typedef defines the control block required to keep information
**      about the server and its data structures.
**
**      Most of the information in the SVCB comes from static information
**      compiled with DMF.  This is a fixed length control block and is
**      located on the server stack.  The SVCB includes such information 
**      as a pointer to a database control block (DCB)  for each database 
**      this server has access to, as a pointer to the session control
**      block (SCB) for each user connection to the server, a pointer 
**      to the hash array used to cache table control blocks (TCBs), and
**      numerous configuration parameters and runtime statistics for
**      measuring and tuning DMF.
**       
**      A pictorial representation of SVCBs in an active server enviroment is:
**   
**           SVCB      
**       +-----------+ 
**       |           |             DCB                      DCB
**       |           |          +-------+              +--------+
**       |  DCB   Q  |--------->| DCB Q |------------->| DCB Q  |
**       |           |          |       |              |        |
**   +---|  SCB  Q   |          +-------+              +--------+  
**   |   |           |
**   |   |HASH ARRAY |------------------+
**   |   +-----------+                  |        TCB HASH ARRAY
**   |      SCB               SCB       |       +----------------------------+  
**   |   +--------+        +--------+   +------>|bucket | bucket | ...|bucket|
**   +-->| SCB Q  |------->| SCB Q  |           |   1   |   2    |    |  n   |
**       |        |        |        |           +----------------------------+
**       +--------+        +--------+  
**
**

**     The SVCB layout is as follows:
**                                                        
**                     +----------------------------------+
**                     | svcb_q_next                      |  First bytes are
**                     | svcb_q_prev                      |  standard DMF CB
**                     | svcb_length                      |  header. 
**                     | svcb_type   | svcb_reserved      |
**                     | svcb_reserved                    |
**                     | svcb_owner                       |
**                     | svcb_ascii_id                    |
**                     |------ End of Header -------------|
**		       | svcb_tcb_id                      |
**		       | svcb_status                      |
**		       | svcb_dq_mutex			  |
**                     | svcb_d_next                      |
**                     | svcb_d_prev                      |
**		       | svcb_sq_mutex			  |
**                     | svcb_s_next                      |
**                     | svcb_s_prev                      |
**		       | svcb_bmcb_ptr                    |
**                     | svcb_hcb_ptr                     |
**                     | svcb_lctx_ptr			  |
**		       | svcb_lock_list                   |
**                     | svcb_ss_lock_list                |
**                     | svcb_mem_mutex                   |
**                     | svcb_pool_list                   |
**                     | svcb_free_list                   |
**                     | svcb_id                          |
**                     | svcb_name                        |
**                     | svcb_mwrite_blocks               |
**                     | svcb_s_max_session               |
**                     | svcb_d_max_database              |
**                     | svcb_m_max_memory                |
**                     | svcb_x_max_transaction           |
**		       | svcb_gateway                     |
**		       | svcb_gw_xacts                    |
**		       | svcb_tzcb                        |
**		       | svcb_scanfactor                  |
**		       | svcb_cnt                         |
**		       | svcb_stat                        |
**		       | svcb_trace                       |
**                     +----------------------------------+
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    19-nov-90 (rogerk)
**		Added timezone to the server control block.  This holds
**		the timezone in which the DBMS server is running.
**		This was done as part of the DBMS Timezone changes.
**	    10-dec-1990 (rogerk)
**		Added svcb_scanfactor for the multi-block IO cost factor.
**	    19-aug-91 (rogerk)
**		Added new svcb_status values SVCB_RCP, SVCB_ACP, SVCB_CSP to
**		be able to identify the context in which DMF is running.  These
**		states are now set by the respective processes after the SVCB
**		is initialized.
**	13-may-1992 (bryanp)
**	    Added svcb_int_sort_size to control in-memory sorting.
**	15-oct-1992 (jnash)
**	    Reduced logging project.  Add svcb_checksum to reflect
**	    II_DBMS_CHECKSUMS.
**	9-nov-1992 (bryanp)
**	    Added svcb_check_dead_interval to the dmf_svcb.
**	14-dec-1992 (rogerk)
**	    Integrated walt's change of svcb_trace from i4 to u_i4
**	    for alpha port.
**	18-jan-1993 (bryanp)
**	    Add svcb_cp_timer to the DM_SVCB.
**	26-apr-1993 (jnash)
**	    Add E_DM9056_CHECKSUM_DISABLE, E_DM9057_PM_CHKSM_INVALID.
**	25-may-1993 (robf)
**	    Add E_DM9058_BAD_FILE_BUILD
**	23-aug-1993 (bryanp)
**	    Add svcb_c_buffers field to the DMF_SVCB for minimal star servers.
**	    In a star server, DMF is not fully started; the only operation
**	    permitted is DMT_SORT_COST. To support this, the number of single
**	    page buffers that we would have used had we started a buffer 
**          manager is saved in svcb_c_buffers to be returned by DMT_SORT_COST.
**      13-sep-1993 (smc)
**          Removed non-portable byte offset comments.
**	09-oct-93 (swm)
**	    Bug #56440
**	    Changed svcb_id to be i4 as it is used only to store
**	    and compare dmc_id values, since dmc_id has changed back to
**	    i4 (and dmc_session_id split off from it to avoid
**	    overloading).
**	05-nov-1993 (smc/swm)
**	    Bug 58635
**          Made unused obj_maxaddr a i4 and changed the name to
**	    obj_unused. Type change required to make the DMF header size
**	    consistent across platforms.
**	    Changed type of obj_unused in DM_OBJECT from i4 to PTR *
**	    to make the DMF header size consistent.
**	18-apr-1994 (jnash)
**	    fsync project: Add svcb->svcb_load_optim, true if async
**	    writes performed.
**	09-apr-97 (cohmi01)
**	    Allow readlock, timeout & maxlocks system defaults to be 
**	    configured, add these items to svcb. #74045, #8616.
**	06-mar-1996 (stial01 for bryanp)
**	    Added svcb_page_size.
**      06-mar-1996 (stial01)
**          Changes for variable page size project: added svcb_bmseg
**	28-Jan-1997 (jenjo02)
**	    Split svcb_mutex in two yielding svcb_dq_mutex (for DCBs)
**	    and svcb_sq_mutex (for SCBs).
**      28-feb-1997 (stial01)
**          Added svcb_ss_lock_list. When sole server, we want the SV_DATATASE
**          lock(s) on a separate lock list that the RCP will inherit for
**          online recovery of a server crash.
**	27-aug-1997 (bobmart)
**	    Add svcb_lk_log_esc to svcb; member holds flags on whether t
**	    log lock escalations (on resource or transaction, for system
**	    catalogs or user tables).
**	17-feb-98 (inkdo01)
**	    Added fields for parallel sort project (svcb_sthread, svcb_strows,
**	    svcb_stnthreads).
**	26-mar-1998 (nanpr01)
**	    Added default blob structure in the server control block.
**	26-may-1998 (nanpr01)
**	    Added fields for parallel index project(svcb_pind_nbuffers,
**	    svcb_pind_bsize).
**	18-Jan-2001 (jenjo02)
**	    Added SVCB_IS_SECURE define.
**	05-Mar-2002 (jenjo02)
**	    Added svcb_seq, list of Sequence Generators for closed
**	    databases.
**	03-Nov-2004 (jenjo02)
**	    Added SVCB_IS_MT flag, set if running with OS threads.
**	15-Aug-2005 (jenjo02)
**	    Map MISC_CB to DM_MISC_CB
**	    Add multiple svcb_pool/svcb_free lists, redefine
**	    obj_invalid as obj_pool, obj_unused as obj_scb.
**	    Add svcb_st_hint feedback array, svcb_dm0m_stat
**	    for ShortTerm session memory, statistics by "type".
**	    svcb_st_ialloc defines size of SCF-allocated session
**	    pool (allocated when DML_SCB is allocated).
**	9-Nov-2005 (schka24)
**	    Add "never replicate" status to be set from II_DBMS_REP
**	    environment variable.
**	27-july-06 (dougi)
**	    Add svcb_sortcpu_factor, svcb_sortio_factor for sort cost
**	    estimate adjustment.
**	04-Apr-2008 (jonj)
**	    Add SVCB_ULOCKS (Update locks supported), SVCB_STATUS define.
**	5-Nov-2009 (kschendel) SIR 122757
**	    Rename load-optim to directio-load.  Add directio-tables
**	    and directio_align.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Add total DMF pool size and table show's to DMF stats.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: add svcb_xid_array_size, svcb_xid_array_ptr,
**	    svcb_xid_lg_id_max, rc_retry stat, svcb_last_tranid,
**	    SVCB_CONFIG_V6, SVCB_CONFIG_V7
**	08-Mar-2010 (thaju02)
**	    svcb_maxtuplen is obsolete; not used.
*/
struct _DM_SVCB
{
    DM_SVCB        *svcb_q_next;            /* Not used, no queue of SVCBs. */
    DM_SVCB        *svcb_q_prev;
    SIZE_TYPE	    svcb_length;	    /* Length of control block. */
    i2              svcb_type;              /* Type of control block for
                                            ** memory manager. */
#define                 SVCB_CB             DM_SVCB_CB
    i2              svcb_s_reserved;        /* Reserved for future use. */
    PTR             svcb_l_reserved;        /* Reserved for future use. */
    PTR             svcb_owner;             /* Owner of control block for
                                            ** memory manager.  SVCB will be
                                            ** owned by the server. */
    i4         svcb_ascii_id;          /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 SVCB_ASCII_ID       CV_C_CONST_MACRO('S', 'V', 'C', 'B')
    u_i4       svcb_tcb_id;            /* Next unique identifier for 
                                            ** newly allocated DMP_TCB. */
    i4	    svcb_status;	    /* Server status. */
#define		SVCB_ACTIVE		0x0001L
#define		SVCB_SINGLEUSER		0x0002L
#define		SVCB_CHECK		0x0004L
#define		SVCB_C2SECURE		0x0008L
#define		SVCB_IS_SECURE		SVCB_C2SECURE
#define		SVCB_RCP		0x0020L
#define		SVCB_ACP		0x0040L
#define		SVCB_CSP		0x0080L
#define		SVCB_BATCHMODE		0x0100L	/* batch mode server */
#define		SVCB_IOMASTER		0x0200L	/* this is IO server */
#define		SVCB_READAHEAD		0x0400L	/* Readahead enabled */
#define		SVCB_IS_MT		0x0800L	/* OS-threaded server*/
#define		SVCB_CLASS_NODE_AFFINITY  0x1000L     /* one node only */
#define		SVCB_NO_REP		0x2000L /* Never replicate */
#define		SVCB_ULOCKS		0x4000L /* Update locks supported */
/* String values of above bits */
#define SVCB_STATUS "\
ACTIVE,SINGLEUSER,CHECK,C2SECURE,10,RCP,ACP,CSP,\
BATCHMODE,IOMASTER,READAHEAD,MT,AFFINITY,NO_REP,ULOCKS"

    DM_MUTEX	    svcb_dq_mutex;	    /* Mutex to protect DCB queue */
    DMP_DCB         *svcb_d_next;           /* Queue of database control 
                                            ** blocks associated with this
                                            ** server. */
    DMP_DCB         *svcb_d_prev;
    DM_MUTEX	    svcb_sq_mutex;	    /* Mutex to protect SCB queue */
    DML_SCB         *svcb_s_next;           /* Queue of session control 
                                            ** blocks associated with this
                                            ** server. */
    DML_SCB         *svcb_s_prev;
  
    DMP_BMSCB       *svcb_bmseg[DM_MAX_BUF_SEGMENT];
					    /* Pointer to buffer manager 
					    ** memory segment(s) */
    DMP_LBMCB	    *svcb_lbmcb_ptr;        /* Pointer to the local buffer 
					    ** manager control block. */
    DMP_PFH	    *svcb_prefetch_hdr;     /* ptr to readahead/prefetch
					    ** header control block */
    DMP_HCB	    *svcb_hcb_ptr;	    /* Pointer to TCB hash control
					    ** block used to locate and
					    ** cache TCB's. */
    DMP_LCTX	    *svcb_lctx_ptr;	    /* Pointer to logging system context
					    ** Used to abort transactions. */
    i4	    svcb_lock_list;	    /* Lock list for server objects. */
    i4         svcb_ss_lock_list;      /* SINGLE SERVER lock list */
    i4	    svcb_strows;	    /* min # rows to trigger || sort */
    i4	    svcb_stnthreads;	    /* # threads per sort (for || sorts) */
    i4	    svcb_stbsize;	    /* parallel sort exchange buffer
					    ** size (in rows) */
    i4	    svcb_pind_bsize;	    /* parallel index exchange buffer
					    ** size in rows */
    i4	    svcb_pind_nbuffers;	    /* number of exchange buffers
					    ** for parallel index building */
    f4	    svcb_sort_cpufactor;	/* factor by which sort CPU estimate
					** is to be reduced */
    f4	    svcb_sort_iofactor;		/* factor by which sort IO estimate
					** is to be reduced */
#define		SVCB_MAXPOOLS	16
    i4		    svcb_pools;	       /* How many are there, really */
    i4		    svcb_next_pool;    /* Next to allocate from */
    i4		    svcb_pad_bytes;    /* dm0m pad bytes */
    SIZE_TYPE	    svcb_st_ialloc;    /* Initial ST SCB pool size, or 0 */
    DM_MUTEX	    svcb_mem_mutex[SVCB_MAXPOOLS]; /* Multi-thread synchronization
					    ** mutex for DM0M. */
    struct _DM_OBJECT
    {
	DM_OBJECT   *obj_next;		    /* Next item on free list. */
	DM_OBJECT   *obj_prev;		    /* Previous item on free list. */
	SIZE_TYPE    obj_size;		    /* Size of object. */
	u_i2	    obj_type;		    /* Type of object. */
#define			OBJ_FREE	    0
#define			OBJ_POOL	    0xffff
	u_i2	    obj_pool;	    	    /* Which pool */
#define			OBJ_P_STATIC	    0xffff /* Static SCB pool */
#define			OBJ_P_SCF	    0xfffe /* SCB pool from SCF */
	PTR	    obj_scb;		    /* SCB if shortterm memory */
	PTR	    obj_owner;		    /* Control block owner. */
	i4	    obj_tag;		    /* Hex dump visible tag. */
#define			OBJ_T_POOL	    CV_C_CONST_MACRO('P', 'O', 'O', 'L')
#define			OBJ_T_FREE	    CV_C_CONST_MACRO('F', 'R', 'E', 'E')
	i4	    obj_pad2;		    /* NOT USED. */
    }		    svcb_pool_list[SVCB_MAXPOOLS]; /* List of allocation pools. */
    DM_OBJECT	    svcb_free_list[SVCB_MAXPOOLS]; /* List of free blocks. */
    i4	    svcb_id;                /* Identifier for server. */
    DB_DB_NAME      svcb_name;              /* Name of server . */
    i4		    svcb_mwrite_blocks;	    /* Number of blocks to write per IO
					    ** during table builds. */
    i4         svcb_s_max_session;     /* Maximum number of sessions
                                            ** allowed per server. */
    i4         svcb_d_max_database;    /* Maximum number of databases
                                            ** allowed per server. */
    i4         svcb_m_max_memory;      /* Maximum amount of memory
                                            ** allowed to server. */
    i4         svcb_x_max_transactions;/* Maximum number of transactions
                                            ** allowed per server. */
    u_i4	    *svcb_xid_array_ptr;    /* Pointer to LG's xid_array */
    i4		    svcb_xid_array_size;    /* Size of LG xid array */
    i4		    svcb_xid_lg_id_max;	    /* Upper bound of xid_array */
    volatile u_i4   svcb_last_tranid;	    /* Last assigned db_low_tran */
    i4		    svcb_gateway;	    /* Indicates presence of Gateway */
    i4		    svcb_gw_xacts;	    /* Indicates Gateway needs xacts */
    PTR	            svcb_tzcb;	            /* Timezone in which server runs */
    i4	    svcb_int_sort_size;	    /* Size of sorts which should be
					    ** performed internally if possible
					    */
    /* Raw I/O may need specially aligned addresses.  Since raw is very rare,
    ** keep track of open databases that have raw locations.  This way, if
    ** no raw-location db's are open, no raw alignment is needed.
    ** Note that this counter is maintained by db OPEN/CLOSE, not db ADD.
    */
    i4		svcb_rawdb_count;	/* Number of open DB's with raw locs */
    u_i4	svcb_directio_align;	/* Address alignment needed if doing
					** direct I/O. */
    bool	svcb_checksum;		/* Checksum enabled/disabled flag */
    bool	svcb_directio_tables;	/* TRUE to pass direct I/O hint
					** to DI for table files. */
    bool	svcb_directio_load;	/* TRUE to pass direct I/O hint
					** to DI for load files (tables
					** being loaded or modified */
    i4	    svcb_check_dead_interval;
					    /* interval btwn check-dead calls */
    i4	    svcb_gc_interval;	    /* interval btwn group commit
					    ** calls.
					    */
    i4	    svcb_gc_numticks;	    /* number of clock ticks until
					    ** group commit expiration
					    */
    i4	    svcb_gc_threshold;	    /* group commit start threshold */

    i4	    svcb_lk_readlock;	    /* sys default for readlock mode */
    i4	    svcb_lk_maxlocks;	    /* sys default for max page locks */
    i4	    svcb_lk_waitlock;	    /* sys default for waitlock secs */
    i4	    svcb_lk_level;	    /* sys default for lock level */
    i4         svcb_isolation;         /* sys default for isolation level*/
    i4         svcb_log_err;           /* Flags specify if new messages
					    ** should be logged,default is no.
					    */
#define SVCB_LOG_READNOLOCK     0x01        /* log readnolock lowers isolation*/
#define SVCB_LOG_LPR_SC		0x02        /* log resource escalation on sys 
					    ** cat */
#define SVCB_LOG_LPR_UT		0x04        /* log resource escalation on user 
					    ** tab. */
#define SVCB_LOG_LPT_SC		0x08        /* log transaction escalation in 
					    ** sys cat */
#define SVCB_LOG_LPT_UT		0x10        /* log transaction escalation on 
					    ** usr tab.*/

    i4	    svcb_cp_timer;	    /* Consistency Point Timer */
    i4	    svcb_c_buffers[DM_MAX_CACHE]; /* number of single page 
					    ** buffers we would have allocated 
					    ** in our buffer if we had allocated
					    ** a buffer manager (MINIMAL STAR
					    ** SERVER ONLY).
					    */
    SIZE_TYPE	    svcb_page_size;	    /* Default page size */
    i4	    svcb_page_type;	    /* Default page type (DM721 tracepoint) */

    i4	    svcb_config_pgtypes;    /* configurable dbms server parameter */
#define	SVCB_CONFIG_V1		0x01
#define	SVCB_CONFIG_V2		0x02
#define	SVCB_CONFIG_V3		0x04
#define	SVCB_CONFIG_V4		0x08
#define SVCB_CONFIG_V5		0x10
#define SVCB_CONFIG_V6		0x20
#define SVCB_CONFIG_V7		0x40
    SIZE_TYPE      svcb_etab_tmpsz;        /* Default etab temps page size */
    SIZE_TYPE      svcb_etab_pgsize;       /* Default etab page size */
    i4		svcb_maxtuplen;         /* Maximum tuple length (not used) */
    i4		svcb_blob_etab_struct;  /* Default blob etab structure */
    double	svcb_scanfactor[DM_MAX_CACHE]; /* Multi-block IO cost factor */
    i4		svcb_dop;		/* Degree of parallelism */
    i4	    svcb_rep_salock;	    /* replicator shad/arch lockmode */
    i4	    svcb_rep_iqlock;	    /* replicator input que lockmode */
    i4	    svcb_rep_dqlock;	    /* replicator dist que lockmode */
    i4	    svcb_rep_dtmaxlock;	    /* rep dist thread maxlocks */
    struct
    {
     i4        cur_session;            /* Current number of sessions. */
     i4        peak_session;           /* Highest number of sessions. */
     i4        cur_database;           /* Current number of databases. */
     i4        peak_database;          /* Highest number of databases. */
     i4	    cur_transaction;	    /* Current number of 
                                            ** transactions. */
     i4	    peak_transaction;	    /* Highest number of 
                                            ** transactions. */
    }               svcb_cnt; 
    struct
    {
      u_i4	    ses_begin;
      u_i4	    ses_end;
      u_i4	    ses_mem_alloc;
      u_i4	    mem_alloc[SVCB_MAXPOOLS];
      u_i4	    mem_dealloc[SVCB_MAXPOOLS];
      u_i4	    mem_expand[SVCB_MAXPOOLS];
      SIZE_TYPE	    mem_totalloc;
      u_i4	    db_add;
      u_i4	    db_delete;
      u_i4	    db_open;
      u_i4          db_close;
      u_i4	    tbl_open;
      u_i4          tbl_close;
      u_i4          tbl_build;
      u_i4          tbl_validate;
      u_i4	    tbl_invalid;
      u_i4	    tbl_update;
      u_i4	    tbl_create;
      u_i4	    tbl_destroy;
      u_i4	    tbl_show;
      u_i4	    tx_begin;
      u_i4	    tx_commit;
      u_i4	    tx_abort;
      u_i4	    tx_savepoint;
      u_i8	    rec_get;
      u_i4	    rec_put;
      u_i4	    rec_replace;
      u_i4	    rec_delete;
      u_i4	    rec_position;
      u_i4	    rec_load;
      u_i4	    utl_create;
      u_i4          utl_destroy;
      u_i4	    utl_modify;
      u_i4          utl_index;
      u_i4	    utl_relocate;
      u_i4	    utl_alter_tbl;		
      u_i4	    rec_p_all;
      u_i4	    rec_p_range;
      u_i4	    rec_p_exact;
      u_i4	    btree_leaf_split;
      u_i4	    btree_index_split;
      u_i4	    btree_repos;
      u_i4	    btree_pglock;
      u_i4	    btree_nolog;
      u_i4	    btree_repos_chk_range;
      u_i4	    btree_repos_search;
      u_i4	    rc_retry;
    }		    svcb_stat;
    u_i4	    svcb_trace[2048 / BITS_IN(i4)];
					    /* Trace bits. */
    /* See dm2f.c for how these temporary-file name generators are used. */
    i2		    svcb_tfname_id;	/* Temp name ID (1 to 511)
					** 0 for rollforwarddb
					** -1 for other jsp, acp.
					*/ 

    i2		    svcb_csp_dbms_id;	/* dbms id for CSP msgs */
    i4		    svcb_tfname_gen;	/* Temp name generator number */
    i4		    svcb_hfname_gen;	/* Name generator for dmh temps */
    i4		    svcb_dmscb_offset;	/* Offset from SCF's SCB to DML_SCB* */
    DML_SEQ	    *svcb_seq;		/* List of Sequence Generators
					** of closed databases */
    /*
    ** This feedback array is used by DM0M to collect ShortTerm (SCB)
    ** memory allocations and predict the initial
    ** sizes of future SCB memory pools.
    */
/* Initial/default ShortTerm allocation */
#define DM0M_PageSize	8192
/* The max number of pages we track */
#define	DM0M_MaxPages	(1000*1024 / DM0M_PageSize)
/* 0th element is 'best" hint */
#define DM0M_Hint	0
/* First array is for non-factotum sessions, second for factota */
#define	DM0M_User	0
#define	DM0M_Factotum	1
#define DM0M_Hints	2	/* There are 2 arrays */
    u_i4	    svcb_st_hint[2][DM0M_MaxPages+1];

    /* Used to collect "type" stats via trace point DM105 */
    struct _DM0M_STAT
    {
	i4		tag;		/* ASCII_ID of type */
	u_i4		longterm;	/* # LongTerm allocs */
	u_i4		shortterm;	/* # ShortTerm allocs */
	i4		alloc_now;	/* How many alloc'd now? */
	i4		alloc_hwm;	/* Hwm alloc'd */
	SIZE_TYPE	min;		/* Smallest alloc size */
	SIZE_TYPE	max;		/* Max alloc size */
    } svcb_dm0m_stat[DM0M_MAX_TYPE+1];
};

GLOBALREF   DM_SVCB *dmf_svcb;

/*}
** Name: DM_TID - The structure of a tuple identifier.
**
** Description:
**      This structure defines a tuple identifier and it associated constants.
**
** History:
**     22-oct-1985 (derek)
**          Created new for Jupiter.
*/
union _DM_TID
{
    struct
    {
#define                 DM_TIDEOF      511
#ifndef TID_SWAP
	u_i4        tid_line:9;         /* Index into offset table on page. */
	u_i4	    tid_page:23;	/* The page number. */
#else
	u_i4	    tid_page:23;	/* The page number. */
	u_i4        tid_line:9;         /* Index into offset table on page. */
#endif
    }	tid_tid;
    u_i4	    tid_i4;		/* Above as an i4. */
};

/* Constants associated with DM_TIDs: */
#define		DM_TID_TYPE	DB_INT_TYPE
#define		DM_TID_LENGTH	4

/*}
** Name: DM_TID8 - The structure of a big tuple identifier.
**
** Description:
**      This structure defines a big tuple identifier, i.e., 
**	one which prefixes DM_TID with partitioning 
**	and extension information.
**
**	The intent is to map the tid so that the fields look like
**	an i8 constructed as:
**	partno<<48 + page_ext<<40 + row_ext<<32 + tid4
**
**	Unfortunately it doesn't seem to be possible to do that in
**	a portable manner without the arch specific TID_SWAP definition.
**
** History:
**	19-Dec-2003 (jenjo02)
**	    Created for Partitioned Table project.
**	15-Jul-2004 (schka24)
**	    Apply tid-swap so that tid8 looks like a tid4 extension
**	    even on annoying backwards address spaces.
*/
union _DM_TID8
{
#ifdef TID_SWAP
    /* The big-endian flavor, written in normal left to right */
    struct
    {
	u_i4	    tid_partno:16;	/* Partition number */
	u_i4	    tid_page_ext:8;	/* Page extension (not used) */
	u_i4	    tid_row_ext:8;	/* Row extension (not used) */
	u_i4	    tid_page:23;	/* The page number. */
	u_i4        tid_line:9;         /* Index into offset table on page. */
    }	tid;
    struct				/* Above as 2 i4's: */
    {
	u_i4	    tpf;		/* TID prefix */
	u_i4	    tid;		/* The TID only */
    }   tid_i4;
#else
    /* The little-endian flavor, members go right to left */
    struct
    {
	u_i4        tid_line:9;         /* Index into offset table on page. */
	u_i4	    tid_page:23;	/* The page number. */
	u_i4	    tid_row_ext:8;	/* Row extension (not used) */
	u_i4	    tid_page_ext:8;	/* Page extension (not used) */
	u_i4	    tid_partno:16;	/* Partition number */
    }	tid;
    struct				/* Above as 2 i4's: */
    {
	u_i4	    tid;		/* The TID only */
	u_i4	    tpf;		/* TID prefix */
    }   tid_i4;
#endif
};

/* Constants associated with DM_TID8s: */
#define		DM_TID8_TYPE	DB_INT_TYPE
#define		DM_TID8_LENGTH	8


/*}
** Name:  DM_FILENAME - Data type for standard file name.
**
** Description:
**      This typedef defines the structure for the layout of a 
**      standard file name returned by DMF.  It returns a fully
**      qualified host filename.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
*/
typedef struct
{
    char            file_name[DB_AREA_MAX];   /* Standard file name */
}   DM_FILENAME;

/*}
** Name:  DM_FILE - Data type for physical file name.
**
** Description:
**      This typedef defines the structure for the layout of a 
**      physical file name used for I/O. 
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	25-Aug-2006 (jonj)
**	    Pushed length define into iicommon.h as DB_FILE_MAX
*/
typedef struct
{
    char            name[DB_FILE_MAX];	    /* Standard file name */
}   DM_FILE;

/*}
** Name:  DM_PATH- Data type for physical path name.
**
** Description:
**      This typedef defines the structure for the layout of a 
**      physical path name used for I/O. 
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
*/
typedef struct
{
    char            name[DB_AREA_MAX];	    /* Standard file name */
}   DM_PATH;


/*
** Error codes definition for DMF internal use.
*/

/* Shared by buffer mgr and JSP */
#define		E_DM101A_JSP_CANT_CONNECT_BM		(E_DM_MASK + 0x101AL)

#define		W_DM5065_NO_FHDR_FOUND			(E_DM_MASK + 0x5065)
/* 
** Use I_DM8000-8FFF for Informational messages, that need to 
** be internationalized.
*/
#define		    I_DM_INFORMATIONAL              (E_DM_MASK + 0x8000L - 1)
#define             I_DM8000_SEC_TABLE_ACCESS		(E_DM_MASK + 0x8000L)
#define             I_DM8001_SEC_RECORD_ACCESS		(E_DM_MASK + 0x8001L)
#define             I_DM8002_SEC_DATABASE_ACCESS	(E_DM_MASK + 0x8002L)
#define             I_DM8003_SEC_RECBYTID_ACCESS	(E_DM_MASK + 0x8003L)
#define             I_DM8004_SEC_RECBYKEY_ACCESS	(E_DM_MASK + 0x8004L)
#define             I_DM8005_SEC_USER_ACCESS	        (E_DM_MASK + 0x8005L)


/* 
** Internal Error codes 9000-FFFF
*/
#define		    E_DM_INTERNAL		    (E_DM_MASK + 0x9000L - 1)
#define             E_DM9000_BAD_FILE_ALLOCATE		(E_DM_MASK + 0x9000L)
#define             E_DM9001_BAD_FILE_CLOSE		(E_DM_MASK + 0x9001L)
#define             E_DM9002_BAD_FILE_CREATE		(E_DM_MASK + 0x9002L)
#define             E_DM9003_BAD_FILE_DELETE		(E_DM_MASK + 0x9003L)
#define             E_DM9004_BAD_FILE_OPEN		(E_DM_MASK + 0x9004L)
#define             E_DM9005_BAD_FILE_READ		(E_DM_MASK + 0x9005L)
#define             E_DM9006_BAD_FILE_WRITE		(E_DM_MASK + 0x9006L)
#define             E_DM9007_BAD_FILE_SENSE		(E_DM_MASK + 0x9007L)
#define             E_DM9008_BAD_FILE_FLUSH		(E_DM_MASK + 0x9008L)
#define             E_DM9009_BAD_FILE_RENAME		(E_DM_MASK + 0x9009L)
#define             E_DM900A_BAD_LOG_DBADD		(E_DM_MASK + 0x900AL)
#define             E_DM900B_BAD_LOG_ALTER		(E_DM_MASK + 0x900BL)
#define             E_DM900C_BAD_LOG_BEGIN		(E_DM_MASK + 0x900CL)
#define             E_DM900D_BAD_LOG_CLOSE		(E_DM_MASK + 0x900DL)
#define             E_DM900E_BAD_LOG_END		(E_DM_MASK + 0x900EL)
#define             E_DM900F_BAD_LOG_EVENT		(E_DM_MASK + 0x900FL)
#define             E_DM9010_BAD_LOG_FORCE		(E_DM_MASK + 0x9010L)
#define             E_DM9011_BAD_LOG_INIT		(E_DM_MASK + 0x9011L)
#define             E_DM9012_BAD_LOG_OPEN		(E_DM_MASK + 0x9012L)
#define             E_DM9013_BAD_LOG_POSITION		(E_DM_MASK + 0x9013L)
#define             E_DM9014_BAD_LOG_READ		(E_DM_MASK + 0x9014L)
#define             E_DM9015_BAD_LOG_WRITE		(E_DM_MASK + 0x9015L)
#define             E_DM9016_BAD_LOG_REMOVE		(E_DM_MASK + 0x9016L)
#define             E_DM9017_BAD_LOG_SHOW		(E_DM_MASK + 0x9017L)
#define             E_DM9018_BAD_LOG_ERASE		(E_DM_MASK + 0x9018L)
#define             E_DM9019_BAD_LOCK_ALTER		(E_DM_MASK + 0x9019L)
#define             E_DM901A_BAD_LOCK_CREATE		(E_DM_MASK + 0x901AL)
#define             E_DM901B_BAD_LOCK_RELEASE		(E_DM_MASK + 0x901BL)
#define             E_DM901C_BAD_LOCK_REQUEST		(E_DM_MASK + 0x901CL)
#define             E_DM901D_BAD_LOCK_SHOW		(E_DM_MASK + 0x901DL)
#define             E_DM901E_BAD_LOCK_INIT		(E_DM_MASK + 0x901EL)
#define             E_DM901F_BAD_TABLE_CREATE		(E_DM_MASK + 0x901FL)
#define             E_DM9020_BM_BAD_UNLOCK_PAGE		(E_DM_MASK + 0x9020L)
#define             E_DM9021_BM_BAD_LOCK_PAGE		(E_DM_MASK + 0x9021L)
#define             E_DM9022_BM_BAD_PROTOCOL		(E_DM_MASK + 0x9022L)
#define             E_DM9023_ERROR_OPENING_TABLE	(E_DM_MASK + 0x9023L)
#define             E_DM9024_ERROR_CLOSING_TABLE	(E_DM_MASK + 0x9024L)
#define             E_DM9025_BAD_TABLE_DESTROY          (E_DM_MASK + 0x9025L)
#define             E_DM9026_REL_UPDATE_ERR             (E_DM_MASK + 0x9026L)
#define             E_DM9027_INDEX_UPDATE_ERR           (E_DM_MASK + 0x9027L)
#define             E_DM9028_ATTR_UPDATE_ERR            (E_DM_MASK + 0x9028L)
#define             E_DM9029_UNEXPECTED_FILE		(E_DM_MASK + 0x9029L)
#define		    E_DM902A_BAD_JNL_CLOSE		(E_DM_MASK + 0x902AL)
#define		    E_DM902B_BAD_JNL_CREATE		(E_DM_MASK + 0x902BL)
#define		    E_DM902C_BAD_JNL_OPEN		(E_DM_MASK + 0x902CL)
#define		    E_DM902D_BAD_JNL_READ		(E_DM_MASK + 0x902DL)
#define		    E_DM902E_BAD_JNL_WRITE		(E_DM_MASK + 0x902EL)
#define		    E_DM902F_BAD_JNL_UPDATE		(E_DM_MASK + 0x902FL)
#define		    E_DM9030_BAD_JNL_FORMAT		(E_DM_MASK + 0x9030L)
#define		    E_DM9031_BAD_JNL_LENGTH		(E_DM_MASK + 0x9031L)
#define		    E_DM9032_BAD_JNL_DELETE		(E_DM_MASK + 0x9032L)
#define		    E_DM9033_BAD_JNL_DIRCREATE		(E_DM_MASK + 0x9033L)
#define		    E_DM9034_BAD_JNL_NOT_FOUND		(E_DM_MASK + 0x9034L)
#define		    E_DM9035_BAD_JNL_TRUNCATE		(E_DM_MASK + 0x9035L)
#define		    E_DM9036_JNLCKP_CREATE_ERROR	(E_DM_MASK + 0x9036L)
#define		    E_DM9037_BAD_JNL_CREATE		(E_DM_MASK + 0x9037L)
#define		    E_DM9040_BAD_CKP_DIRCREATE		(E_DM_MASK + 0x9040L)
#define		    E_DM9041_LK_ESCALATE_TABLE		(E_DM_MASK + 0x9041L)
#define		    E_DM9042_PAGE_DEADLOCK		(E_DM_MASK + 0x9042L)
#define		    E_DM9043_LOCK_TIMEOUT		(E_DM_MASK + 0x9043L)
#define		    E_DM9044_ESCALATE_DEADLOCK		(E_DM_MASK + 0x9044L)
#define		    E_DM9045_TABLE_DEADLOCK		(E_DM_MASK + 0x9045L)
#define		    E_DM9046_TABLE_NOLOCKS		(E_DM_MASK + 0x9046L)
#define		    E_DM9047_LOCK_RETRY	    		(E_DM_MASK + 0x9047L)
#define		    E_DM9048_DEV_UPDATE_ERROR	        (E_DM_MASK + 0x9048L)
#define		    E_DM9049_UNKNOWN_EXCEPTION		(E_DM_MASK + 0x9049L)
#define		    E_DM904A_FATAL_EXCEPTION		(E_DM_MASK + 0x904AL)
#define		    E_DM904B_BAD_LOCK_EVENT		(E_DM_MASK + 0x904BL)
#define		    E_DM904C_ERROR_GETTING_RECORD	(E_DM_MASK + 0x904CL)
#define		    E_DM904D_ERROR_PUTTING_RECORD	(E_DM_MASK + 0x904DL)
#define		    E_DM904E_BAD_LOG_COPY		(E_DM_MASK + 0x904EL)
#define		    E_DM904F_ERROR_UNFIXING_PAGES	(E_DM_MASK + 0x904FL)
#define		    E_DM9050_TRANSACTION_NOLOGGING	(E_DM_MASK + 0x9050L)
#define		    E_DM9051_ABORT_NOLOGGING		(E_DM_MASK + 0x9051L)
#define		    E_DM9052_WORK_PATH_INVALID		(E_DM_MASK + 0x9052L)
#define		    E_DM9053_INCONS_ALLOC		(E_DM_MASK + 0x9053L)
#define		    E_DM9054_BAD_LOG_RESERVE		(E_DM_MASK + 0x9054L)
#define		    E_DM9055_AUDITDB_PRESCAN_FAIL	(E_DM_MASK + 0x9055L)
#define		    E_DM9056_CHECKSUM_DISABLE		(E_DM_MASK + 0x9056L)
#define		    E_DM9057_PM_CHKSM_INVALID		(E_DM_MASK + 0x9057L)
#define		    E_DM9058_BAD_FILE_BUILD		(E_DM_MASK + 0x9058L)
#define		    E_DM9059_TRAN_FORCE_ABORT		(E_DM_MASK + 0x9059L)
#define             E_DM905A_ROW_DEADLOCK               (E_DM_MASK + 0x905AL)
#define             E_DM905B_BAD_LOCK_ROW               (E_DM_MASK + 0x905BL)
#define             E_DM905C_VALUE_DEADLOCK             (E_DM_MASK + 0x905CL)
#define             E_DM905D_BAD_LOCK_VALUE             (E_DM_MASK + 0x905DL)
#define             E_DM905E_ARITH_EXCEPTION            (E_DM_MASK + 0x905EL)
#define             E_DM905F_OLD_TABLE_NOTSUPP          (E_DM_MASK + 0x905FL)
#define		    E_DM9060_SESSION_LOGGING_OFF	(E_DM_MASK + 0x9060L)
#define		    E_DM9061_JNLDB_SESSION_LOGGING_ON 	(E_DM_MASK + 0x9061L)
#define		    E_DM9062_NON_JNLDB_SESSION_LOGGING_ON (E_DM_MASK + 0x9062L)
#define		    E_DM9063_LK_TABLE_MAXLOCKS		(E_DM_MASK + 0x9063L)
#define             E_DM9064_LK_ESCALATE_RELEASE        (E_DM_MASK + 0x9064L)
#define             E_DM9065_READNOLOCK_READUNCOMMITTED (E_DM_MASK + 0x9065L)
#define             E_DM9066_LOCK_BUSY 			(E_DM_MASK + 0x9066L)
#define		    E_DM9070_LOG_LOGFULL		(E_DM_MASK + 0x9070L)
#define		    E_DM9071_LOCK_TIMEOUT_ONLINE_CKP	(E_DM_MASK + 0x9071L)
#define             E_DM9072_BAD_UNLOCK_ROW             (E_DM_MASK + 0x9072L)
#define             E_DM9073_BAD_UNLOCK_ROW_BY_LKID     (E_DM_MASK + 0x9073L)
#define             E_DM9074_BAD_UNLOCK_VALUE           (E_DM_MASK + 0x9074L)
#define             E_DM9075_BAD_UNLOCK_VALUE_BY_LKID   (E_DM_MASK + 0x9075L)
#define             E_DM9076_ERROR_RELEASING_LOCK       (E_DM_MASK + 0x9076L)
#define             E_DM9077_ERROR_LOCK_DOWNGRADE       (E_DM_MASK + 0x9077L)
#define             E_DM9078_ROW_LOCK_ERROR             (E_DM_MASK + 0x9078L)
#define		    E_DM9079_CONFIG_NAME_MISMATCH	(E_DM_MASK + 0x9079L)
/* DMU internal error conditions. */
#define             E_DM9100_LOAD_TABLE_ERROR		(E_DM_MASK + 0x9100L)
#define             E_DM9101_UPDATE_CATALOGS_ERROR	(E_DM_MASK + 0x9101L)
#define             E_DM9102_BAD_MSG_FORMAT             (E_DM_MASK + 0x9102L)
#define             E_DM9103_ERR_SENDING_MSG            (E_DM_MASK + 0x9103L)
#define             E_DM9104_ERR_CK_PATCH_TBL           (E_DM_MASK + 0x9104L)
#define             E_DM9105_ERR_DEALLOC_MEMORY         (E_DM_MASK + 0x9105L)
#define             E_DM9106_ERR_ALLOC_MEMORY           (E_DM_MASK + 0x9106L)
#define             E_DM9107_BAD_DM1U_COMPRESS          (E_DM_MASK + 0x9107L)
#define		    E_DM9108_FIXED_PAGE			(E_DM_MASK + 0x9108L)
#define		    E_DM9109_BAD_RELMAIN		(E_DM_MASK + 0x9109L)
#define		    E_DM910A_DM1U_VERIFY_INFO		(E_DM_MASK + 0x910AL)
#define	            E_DM910B_BAD_SEC_LBL_ATT	        (E_DM_MASK + 0x910BL)
#define	            E_DM910C_ERR_ROW_SEC_LABEL	        (E_DM_MASK + 0x910CL)
#define	            E_DM910D_ROW_MAC_ERROR		(E_DM_MASK + 0x910DL)
#define	            E_DM910E_ROW_SEC_AUDIT_ERROR	(E_DM_MASK + 0x910EL)
#define		    E_DM910F_TABLE_MAC_ERROR		(E_DM_MASK + 0x910FL)
#define		    E_DM9110_ROW_DEF_LABEL_ERR		(E_DM_MASK + 0x9110L)
#define		    E_DM9111_DEF_LABEL_TYPE		(E_DM_MASK + 0x9111L)
#define		    E_DM9112_DEF_LABEL_ERR		(E_DM_MASK + 0x9112L)
#define		    E_DM9113_PERPH_POS_ERR		(E_DM_MASK + 0x9113L)
#define		    E_DM9114_VERIFY_MEM_ERR		(E_DM_MASK + 0x9114L)
#define		    E_DM9115_PERPH_GET_ERR		(E_DM_MASK + 0x9115L)
#define		    E_DM9116_REL_POS_ERR		(E_DM_MASK + 0x9116L)
#define		    E_DM9117_REL_GET_ERR		(E_DM_MASK + 0x9117L)
#define		    E_DM9118_ETAB_OPEN_ERR		(E_DM_MASK + 0x9118L)
#define		    E_DM9119_ETAB_POS_ERR		(E_DM_MASK + 0x9119L)
#define		    E_DM911A_NO_UNDER_DV		(E_DM_MASK + 0x911AL)
#define		    E_DM911B_NO_DT_INFO			(E_DM_MASK + 0x911BL)
#define		    E_DM911C_RCB_ALLOC_ERR		(E_DM_MASK + 0x911CL)
#define		    E_DM911D_NO_NULL_INFO		(E_DM_MASK + 0x911DL)
#define		    E_DM911E_ALLOC_FAILURE		(E_DM_MASK + 0x911EL)

/* DMM internal error conditions. 9180 - 91ff */
#define	     	    E_DM9180_CANT_ADD_LIST_TUPLE	(E_DM_MASK + 0x9180L)
#define		    E_DM9181_SPECIFY_DEL_FILE		(E_DM_MASK + 0x9181L)
#define	    	    E_DM9182_CANT_MAKE_LIST_TUPLE	(E_DM_MASK + 0x9182L)
#define	    	    E_DM9183_DILIST_ERR			(E_DM_MASK + 0x9183L)
#define	            E_DM9184_DMMLIST_FAILED		(E_DM_MASK + 0x9184L)
#define	            E_DM9185_CANT_LIST_DIR		(E_DM_MASK + 0x9185L)
#define	            E_DM9186_LISTDIR_NONEXISTENT	(E_DM_MASK + 0x9186L)
#define	            E_DM9187_INVALID_LIST_TBL_FMT	(E_DM_MASK + 0x9187L)
#define	            E_DM9188_DMM_ALLOC_ERROR		(E_DM_MASK + 0x9188L)
#define	            E_DM9189_DMM_LISTTAB_OPEN_ERR	(E_DM_MASK + 0x9189L)
#define	            E_DM918A_LISTTAB_NONEXISTENT	(E_DM_MASK + 0x918AL)
#define	            E_DM918B_DMM_SYSCAT_ERROR		(E_DM_MASK + 0x918BL)
#define	            E_DM918C_SPECIFY_LIST_COLNAME	(E_DM_MASK + 0x918CL)
#define	            E_DM918D_BAD_LISTFILE_FORMAT	(E_DM_MASK + 0x918DL)
#define	            E_DM918E_SPECIFY_LIST_TABLE		(E_DM_MASK + 0x918EL)
#define		    E_DM918F_ILLEGAL_DEL_FIL		(E_DM_MASK + 0x918FL)
#define		    E_DM9190_BAD_FILE_DELETE		(E_DM_MASK + 0x9190L)
#define             E_DM9191_BADOPEN_IIPHYSDB           (E_DM_MASK + 0x9191L)
#define             E_DM9192_BADOPEN_IIPHYSEXT          (E_DM_MASK + 0x9192L)
#define             E_DM9193_BAD_PHYSDB_TUPLE           (E_DM_MASK + 0x9193L)
#define             E_DM9194_BAD_PHYSEXT_TUPLE          (E_DM_MASK + 0x9194L)
#define             E_DM9195_BAD_MESSAGESEND            (E_DM_MASK + 0x9195L)
#define             E_DM9196_BADOPEN_IITEMPLOC          (E_DM_MASK + 0x9196L)
#define             E_DM9197_BADREAD_IITEMPLOC          (E_DM_MASK + 0x9197L)
#define             E_DM9198_BADCLOSE_IITEMPLOC         (E_DM_MASK + 0x9198L)
#define             E_DM9199_BADOPEN_TEMPCODEMAP        (E_DM_MASK + 0x9199L)
#define             E_DM919A_BADREAD_TEMPCODEMAP        (E_DM_MASK + 0x919AL)
#define             E_DM919B_BADCLOSE_TEMPCODEMAP       (E_DM_MASK + 0x919BL)
#define             E_DM919C_BAD_IIPHYS_EXTEND          (E_DM_MASK + 0x919CL)
#define             E_DM919D_BAD_IIPHYS_DATABASE        (E_DM_MASK + 0x919DL)
#define             E_DM919E_DMM_LOCV_BADPARAM		(E_DM_MASK + 0x919EL)
#define             E_DM919F_DMM_LOCV_CKPEXISTS		(E_DM_MASK + 0x919FL)
#define             E_DM91A0_DMM_LOCV_DMPEXISTS		(E_DM_MASK + 0x91A0L)
#define             E_DM91A1_DMM_LOCV_DATAEXISTS	(E_DM_MASK + 0x91A1L)
#define             E_DM91A2_DMM_LOCV_JNLEXISTS		(E_DM_MASK + 0x91A2L)
#define             E_DM91A3_DMM_LOCV_CKLISTFILE	(E_DM_MASK + 0x91A3L)
#define             E_DM91A5_DMM_LOCV_DILISTFILE	(E_DM_MASK + 0x91A4L)
#define             E_DM91A6_DMM_LOCV_JFLISTFILE	(E_DM_MASK + 0x91A6L)
#define             E_DM91A7_BAD_CATALOG_TEMPLATES      (E_DM_MASK + 0x91A7L)
#define             E_DM91A8_DMM_LOCV_DILISTDIR		(E_DM_MASK + 0x91A8L)
#define             E_DM91A9_DMM_LOCV_WORKEXISTS        (E_DM_MASK + 0x91A9L)

/* DMP internal error conditions. */
#define		    E_DM9200_BM_BUSY			(E_DM_MASK + 0x9200L)
#define		    E_DM9201_BM_CLOSE_PAGE_ERROR	(E_DM_MASK + 0x9201L)
#define		    E_DM9202_BM_BAD_FILE_PAGE_ADDR	(E_DM_MASK + 0x9202L)
#define		    E_DM9203_DM2R_RCB_ALLOC		(E_DM_MASK + 0x9203L)
#define		    E_DM9204_BM_FIX_PAGE_ERROR		(E_DM_MASK + 0x9204L)
#define		    E_DM9205_BM_INVALIDATE_PAGE_ERR	(E_DM_MASK + 0x9205L)
#define		    E_DM9206_BM_BAD_PAGE_NUMBER		(E_DM_MASK + 0x9206L)
#define		    E_DM9207_BM_INTERNAL_ERROR		(E_DM_MASK + 0x9207L)
#define		    E_DM9208_BM_UNFIX_PAGE_ERROR	(E_DM_MASK + 0x9208L)
#define		    E_DM9209_BM_NO_CACHE_LOCKS		(E_DM_MASK + 0x9209L)
#define		    E_DM920A_BM_ESCALATE_LOCK_ERR	(E_DM_MASK + 0x920AL)
#define		    E_DM920B_BM_NO_BUFFERS		(E_DM_MASK + 0x920BL)
#define		    E_DM920C_BM_BAD_FAULT_PAGE		(E_DM_MASK + 0x920CL)
#define		    E_DM920D_BM_BAD_GROUP_FAULTPAGE	(E_DM_MASK + 0x920DL)
#define		    E_DM920E_BM_FORCE_PAGE_ERROR	(E_DM_MASK + 0x920EL)
#define		    E_DM920F_BM_GROUP_FORCEPAGE_ERR	(E_DM_MASK + 0x920FL)
#define		    E_DM9210_BM_VALIDATE_PAGE_ERROR	(E_DM_MASK + 0x9210L)
#define		    E_DM9211_DM0L_BT			(E_DM_MASK + 0x9211L)
#define		    E_DM9212_DM0L_ROBT			(E_DM_MASK + 0x9212L)
#define		    E_DM9213_DM0L_ET			(E_DM_MASK + 0x9213L)
#define		    E_DM9214_DM0L_BI			(E_DM_MASK + 0x9214L)
#define		    E_DM9215_DM0L_PUT			(E_DM_MASK + 0x9215L)
#define		    E_DM9216_DM0L_DEL			(E_DM_MASK + 0x9216L)
#define		    E_DM9217_DM0L_REP			(E_DM_MASK + 0x9217L)
#define		    E_DM9218_DM0L_SBI			(E_DM_MASK + 0x9218L)
#define		    E_DM9219_DM0L_SPUT			(E_DM_MASK + 0x9219L)
#define		    E_DM921A_DM0L_SDEL			(E_DM_MASK + 0x921AL)
#define		    E_DM921B_DM0L_SREP			(E_DM_MASK + 0x921BL)
#define		    E_DM921C_DM0L_BM			(E_DM_MASK + 0x921CL)
#define		    E_DM921D_DM0L_EM			(E_DM_MASK + 0x921DL)
#define		    E_DM921E_DM0L_BALLOC		(E_DM_MASK + 0x921EL)
#define		    E_DM921F_DM0L_BDEALLOC		(E_DM_MASK + 0x921FL)
#define		    E_DM9220_DM0L_CALLOC		(E_DM_MASK + 0x9220L)
#define		    E_DM9221_DM0L_SAVEPOINT		(E_DM_MASK + 0x9221L)
#define		    E_DM9222_DM0L_ABORTSAVE		(E_DM_MASK + 0x9222L)
#define		    E_DM9223_DM0L_FRENAME		(E_DM_MASK + 0x9223L)
#define		    E_DM9224_DM0L_FCREATE		(E_DM_MASK + 0x9224L)
#define		    E_DM9225_DM0L_OPENDB		(E_DM_MASK + 0x9225L)
#define		    E_DM9226_DM0L_CLOSEDB		(E_DM_MASK + 0x9226L)
#define		    E_DM9227_DM0L_PHASE1		(E_DM_MASK + 0x9227L)
#define		    E_DM9228_DM0L_CONFIG		(E_DM_MASK + 0x9228L)
#define		    E_DM9229_DM0L_CREATE		(E_DM_MASK + 0x9229L)
#define		    E_DM922A_DM0L_DESTROY		(E_DM_MASK + 0x922AL)
#define		    E_DM922B_DM0L_RELOCATE		(E_DM_MASK + 0x922BL)
#define		    E_DM922C_DM0L_MODIFY		(E_DM_MASK + 0x922CL)
#define		    E_DM922D_DM0L_INDEX			(E_DM_MASK + 0x922DL)
#define		    E_DM922E_DM0L_POSITION		(E_DM_MASK + 0x922EL)
#define		    E_DM922F_DM0L_READ			(E_DM_MASK + 0x922FL)
#define		    E_DM9230_DM0L_FORCE			(E_DM_MASK + 0x9230L)
#define		    E_DM9231_DM0L_LOGFORCE		(E_DM_MASK + 0x9231L)
#define		    E_DM9232_DM0L_ALLOCATE		(E_DM_MASK + 0x9232L)
#define		    E_DM9233_DM0L_DEALLOCATE		(E_DM_MASK + 0x9233L)
#define		    E_DM9234_DM0L_BPEXTEND		(E_DM_MASK + 0x9234L)
#define		    E_DM9235_DM0L_BCP			(E_DM_MASK + 0x9235L)
#define		    E_DM9236_DM0L_SM1_RENAME		(E_DM_MASK + 0x9236L)
#define		    E_DM9237_DM0L_SM2_CONFIG		(E_DM_MASK + 0x9237L)
#define		    E_DM9238_LOG_READ_EOF		(E_DM_MASK + 0x9238L)
#define		    E_DM9239_BM_BAD_PARAMETER		(E_DM_MASK + 0x9239L)
#define		    E_DM923A_CONFIG_OPEN_ERROR		(E_DM_MASK + 0x923AL)
#define		    E_DM923B_CONFIG_CLOSE_ERROR		(E_DM_MASK + 0x923BL)
#define		    E_DM923C_CONFIG_FORMAT_ERROR	(E_DM_MASK + 0x923CL)
#define		    E_DM923D_DM0M_NOMORE		(E_DM_MASK + 0x923DL)
#define		    E_DM923E_DM0M_BAD_SIZE		(E_DM_MASK + 0x923EL)
#define		    E_DM923F_DM2F_OPEN_ERROR		(E_DM_MASK + 0x923FL)
#define		    E_DM9240_DM2F_CLOSE_ERROR		(E_DM_MASK + 0x9240L)
#define		    E_DM9241_DM2F_BAD_LOCATION		(E_DM_MASK + 0x9241L)
#define		    E_DM9242_DM1R_EXTEND		(E_DM_MASK + 0x9242L)
#define		    E_DM9243_DM1R_PUT			(E_DM_MASK + 0x9243L)
#define		    E_DM9244_DM1R_REPLACE		(E_DM_MASK + 0x9244L)
#define		    E_DM9245_DM1S_ALLOCATE		(E_DM_MASK + 0x9245L)
#define		    E_DM9246_DM1S_DEALLOCATE		(E_DM_MASK + 0x9246L)
#define		    E_DM9247_DM1H_ALLOCATE		(E_DM_MASK + 0x9247L)
#define		    E_DM9248_DM1H_SEARCH		(E_DM_MASK + 0x9248L)
#define		    E_DM9249_DM1I_ALLOCATE		(E_DM_MASK + 0x9249L)
#define		    E_DM924A_DM1I_SEARCH		(E_DM_MASK + 0x924AL)
#define		    E_DM924B_DM1S_BBEGIN		(E_DM_MASK + 0x924BL)
#define		    E_DM924C_DM1S_BEND			(E_DM_MASK + 0x924CL)
#define		    E_DM924D_DM1S_BPUT			(E_DM_MASK + 0x924DL)
#define		    E_DM924E_DM1H_BBEGIN		(E_DM_MASK + 0x924EL)
#define		    E_DM924F_DM1H_BEND			(E_DM_MASK + 0x924FL)
#define		    E_DM9250_DM1H_BPUT			(E_DM_MASK + 0x9250L)
#define		    E_DM9251_DM1I_BADD			(E_DM_MASK + 0x9251L)
#define		    E_DM9252_DM1I_BEND			(E_DM_MASK + 0x9252L)
#define		    E_DM9253_DM1I_BOTTOM		(E_DM_MASK + 0x9253L)
#define		    E_DM9254_DM1B_EXTEND		(E_DM_MASK + 0x9254L)
#define		    E_DM9255_DM1B_FREEDATA		(E_DM_MASK + 0x9255L)
#define		    E_DM9256_DM1B_FINDFREE		(E_DM_MASK + 0x9256L)
#define		    E_DM9257_DM1B_FINDDATA		(E_DM_MASK + 0x9257L)
#define		    E_DM9258_DM1B_MERGE			(E_DM_MASK + 0x9258L)
#define		    E_DM9259_DM1B_BEND			(E_DM_MASK + 0x9259L)
#define		    E_DM925A_DM1B_BEGIN			(E_DM_MASK + 0x925AL)
#define		    E_DM925B_DM1B_BPUT			(E_DM_MASK + 0x925BL)
#define		    E_DM925C_DM1B_BADD			(E_DM_MASK + 0x925CL)
#define		    E_DM925D_DM1B_BOTTOM		(E_DM_MASK + 0x925DL)
#define		    E_DM925E_DM1B_ALLOCATE		(E_DM_MASK + 0x925EL)
#define		    E_DM925F_DM1B_GETBYBID		(E_DM_MASK + 0x925FL)
#define		    E_DM9260_DM1B_DELETE		(E_DM_MASK + 0x9260L)
#define		    E_DM9261_DM1B_GET			(E_DM_MASK + 0x9261L)
#define		    E_DM9262_DM1B_PUT			(E_DM_MASK + 0x9262L)
#define		    E_DM9263_DM1B_REPLACE		(E_DM_MASK + 0x9263L)
#define		    E_DM9264_DM1B_SEARCH		(E_DM_MASK + 0x9264L)
#define		    E_DM9265_ADD_DB			(E_DM_MASK + 0x9265L)
#define		    E_DM9266_UNLOCK_CLOSE_DB		(E_DM_MASK + 0x9266L)
#define		    E_DM9267_CLOSE_DB			(E_DM_MASK + 0x9267L)
#define		    E_DM9268_OPEN_DB			(E_DM_MASK + 0x9268L)
#define		    E_DM9269_LOCK_OPEN_DB		(E_DM_MASK + 0x9269L)
#define		    E_DM926A_TBL_CACHE_LOCK		(E_DM_MASK + 0x926AL)
#define		    E_DM926B_BUILD_TCB			(E_DM_MASK + 0x926BL)
#define		    E_DM926C_TBL_CLOSE			(E_DM_MASK + 0x926CL)
#define		    E_DM926D_TBL_LOCK			(E_DM_MASK + 0x926DL)
#define		    E_DM926E_RECLAIM_TCB		(E_DM_MASK + 0x926EL)
#define		    E_DM926F_TBL_CACHE_CHANGE		(E_DM_MASK + 0x926FL)
#define		    E_DM9270_RELEASE_TCB		(E_DM_MASK + 0x9270L)
#define		    E_DM9271_BUILD_TEMP_TCB		(E_DM_MASK + 0x9271L)
#define		    E_DM9272_REL_UPDATE_ERROR		(E_DM_MASK + 0x9272L)
#define		    E_DM9273_TBL_UPDATE			(E_DM_MASK + 0x9273L)
#define		    E_DM9274_TBL_VERIFY			(E_DM_MASK + 0x9274L)
#define		    E_DM9275_DM2R_LOAD			(E_DM_MASK + 0x9275L)
#define		    E_DM9276_TBL_OPEN			(E_DM_MASK + 0x9276L)
#define		    E_DM9277_DM0L_LOCATION		(E_DM_MASK + 0x9277L)
#define		    E_DM9278_DM0L_SETBOF		(E_DM_MASK + 0x9278L)
#define		    E_DM9279_DM0L_JNLEOF		(E_DM_MASK + 0x9279L)
#define		    E_DM927A_DM0L_ARCHIVE		(E_DM_MASK + 0x927AL)
#define		    E_DM927B_DM0L_STARTDRAIN		(E_DM_MASK + 0x927BL)
#define		    E_DM927C_DM0L_ENDDRAIN		(E_DM_MASK + 0x927CL)
#define		    E_DM927D_DM0L_LOAD			(E_DM_MASK + 0x927DL)
#define		    E_DM927E_UPDATE_TCB			(E_DM_MASK + 0x927EL)
#define		    E_DM927F_DCB_INVALID		(E_DM_MASK + 0x927FL)
#define		    E_DM9280_DM0J_CLOSE_ERROR		(E_DM_MASK + 0x9280L)
#define		    E_DM9281_DM0J_CREATE_ERROR		(E_DM_MASK + 0x9281L)
#define		    E_DM9282_DM0J_OPEN_ERROR		(E_DM_MASK + 0x9282L)
#define		    E_DM9283_DM0J_READ_ERROR		(E_DM_MASK + 0x9283L)
#define		    E_DM9284_DM0J_WRITE_ERROR		(E_DM_MASK + 0x9284L)
#define		    E_DM9285_DM0J_UPDATE_ERROR		(E_DM_MASK + 0x9285L)
#define		    E_DM9286_DM0J_FORMAT_ERROR		(E_DM_MASK + 0x9286L)
#define		    E_DM9287_DM0J_LENGTH_ERROR		(E_DM_MASK + 0x9287L)
#define		    E_DM9288_TBL_CTRL_LOCK		(E_DM_MASK + 0x9288L)
#define		    E_DM9289_BM_BAD_MUTEX_PROTOCOL	(E_DM_MASK + 0x9289L)
#define		    E_DM928A_CONFIG_EXTEND_ERROR	(E_DM_MASK + 0x928AL)
#define		    E_DM928B_CONFIG_NOT_COMPATABLE	(E_DM_MASK + 0x928BL)
#define		    E_DM928C_CONFIG_CONVERT_ERROR	(E_DM_MASK + 0x928CL)
#define		    E_DM928D_NO_DUMP_LOCATION		(E_DM_MASK + 0x928DL)
#define		    E_DM928E_CONFIG_FORMAT_ERROR	(E_DM_MASK + 0x928EL)
#define		    E_DM928F_DM2F_ADD_FCB_ERROR		(E_DM_MASK + 0x928FL)
#define		    E_DM9290_DM2F_DELETE_ERROR		(E_DM_MASK + 0x9290L)
#define		    E_DM9291_DM2F_FILENOTFOUND		(E_DM_MASK + 0x9291L)
#define		    E_DM9292_DM2F_DIRNOTFOUND		(E_DM_MASK + 0x9292L)
#define		    E_DM9293_DM2F_BADDIR_SPEC		(E_DM_MASK + 0x9293L)
#define		    E_DM9294_DM2R_BADLOGICAL		(E_DM_MASK + 0x9294L)
#define		    E_DM9295_DM2U_BADLOGICAL		(E_DM_MASK + 0x9295L)
#define		    E_DM9296_DM0L_ASSOC			(E_DM_MASK + 0x9296L)
#define		    E_DM9297_DM0L_ALLOC			(E_DM_MASK + 0x9297L)
#define		    E_DM9298_DM0L_DEALLOC		(E_DM_MASK + 0x9298L)
#define		    E_DM9299_DM0L_EXTEND		(E_DM_MASK + 0x9299L)
#define             E_DM929A_DM0L_CRDB                  (E_DM_MASK + 0x929AL)
#define             E_DM929B_DM0L_OVFL                  (E_DM_MASK + 0x929BL)
#define             E_DM929C_DM1S_EMPTYTABLE		(E_DM_MASK + 0x929CL)
#define             E_DM929D_BAD_DIR_CREATE		(E_DM_MASK + 0x929DL)
#define             E_DM929E_DM0L_EXT_ALTER		(E_DM_MASK + 0x929EL)
#define             E_DM929F_DM2D_ALTER_DB 		(E_DM_MASK + 0x929FL)
#define             E_DM92A0_DMVE_ALTER_UNDO		(E_DM_MASK + 0x92A0L)
#define             E_DM92A1_DM2D_EXT_MISMATCH		(E_DM_MASK + 0x92A1L)
#define             E_DM92A2_DM2D_LAST_TBID		(E_DM_MASK + 0x92A2L)
#define		    E_DM92A3_NAME_GEN_OVF		(E_DM_MASK + 0x92A3L)
#define		    E_DM92BF_DM1P_TABLE_TOO_BIG		(E_DM_MASK + 0x92BFL)
#define		    E_DM92CA_SMS_CONV_NEEDED		(E_DM_MASK + 0x92CAL)
#define		    E_DM92CB_DM1P_ERROR_INFO		(E_DM_MASK + 0x92CBL)
#define		    E_DM92CC_ERR_CONVERT_TBL_INFO	(E_DM_MASK + 0x92CCL)
#define		    E_DM92CD_DM2U_CONVERT_ERR		(E_DM_MASK + 0x92CDL)
#define		    E_DM92CE_DM2F_GUARANTEE_ERR		(E_DM_MASK + 0x92CEL)
#define		    E_DM92CF_DM2F_GALLOC_ERROR		(E_DM_MASK + 0x92CFL)
#define		    E_DM92D0_DM1P_GETFREE		(E_DM_MASK + 0x92d0L)
#define		    E_DM92D1_DM1P_PUTFREE		(E_DM_MASK + 0x92d1L)
#define		    E_DM92D2_DM1P_SETFREE		(E_DM_MASK + 0x92d2L)
#define		    E_DM92D3_DM1P_TESTFREE		(E_DM_MASK + 0x92d3L)
#define		    E_DM92D4_DM1P_CHECKHWM		(E_DM_MASK + 0x92d4L)
#define		    E_DM92D5_DM1P_LASTUSED		(E_DM_MASK + 0x92d5L)
#define		    E_DM92D6_DM1P_REBUILD		(E_DM_MASK + 0x92d6L)
#define		    E_DM92D7_DM1P_DUMP			(E_DM_MASK + 0x92d7L)
#define		    E_DM92D8_DM1P_VERIFY		(E_DM_MASK + 0x92d8L)
#define		    E_DM92D9_DM1P_XFREE			(E_DM_MASK + 0x92d9L)
#define		    E_DM92DA_DM1P_XUSED			(E_DM_MASK + 0x92dAL)
#define		    E_DM92DB_DM1P_FREE			(E_DM_MASK + 0x92dBL)
#define		    E_DM92DC_DM1P_EXTEND		(E_DM_MASK + 0x92dCL)
#define		    E_DM92DD_DM1P_FIX_HEADER		(E_DM_MASK + 0x92dDL)
#define		    E_DM92DE_DM1P_UNFIX_HEADER		(E_DM_MASK + 0x92dEL)
#define		    E_DM92DF_FHFM_CHECKSUM_ERROR	(E_DM_MASK + 0x92dfL)
#define		    E_DM92E0_DM1X_START			(E_DM_MASK + 0x92E0L)
#define		    E_DM92E1_DM1X_FINISH		(E_DM_MASK + 0x92E1L)
#define		    E_DM92E2_DM1X_READPAGE		(E_DM_MASK + 0x92E2L)
#define		    E_DM92E3_DM1X_NEWPAGE		(E_DM_MASK + 0x92E3L)
#define		    E_DM92E4_DM1X_RESERVE		(E_DM_MASK + 0x92E4L)
#define		    E_DM92E5_DM1X_ALLOCATE		(E_DM_MASK + 0x92E5L)
#define		    E_DM92E6_DM1X_FREE			(E_DM_MASK + 0x92E6L)
#define		    E_DM92E7_DM1X_WRITE_ONE		(E_DM_MASK + 0x92E7L)
#define		    E_DM92E8_DM1X_READ_ONE		(E_DM_MASK + 0x92E8L)
#define		    E_DM92E9_DM1X_BUILD_SMS		(E_DM_MASK + 0x92E9L)
#define		    E_DM92EA_DM1X_READ_GROUP		(E_DM_MASK + 0x92EAL)
#define		    E_DM92EB_DM1P_CVT_TABLE_TO_65	(E_DM_MASK + 0x92EBL)
#define             E_DM92EC_DM1P_ADD_EXTEND           	(E_DM_MASK + 0x92ECL)
#define             E_DM92ED_DM1P_BAD_PAGE           	(E_DM_MASK + 0x92EDL)
#define		    E_DM92EE_DM1P_FIX_MAP		(E_DM_MASK + 0x92EEL)
#define		    E_DM92EF_DM1P_UNFIX_MAP		(E_DM_MASK + 0X92EFL)
#define		    E_DM92F0_DM1X_WRITE_GROUP		(E_DM_MASK + 0X92F0L)
#define		    E_DM92F1_DM1P_BUILD_SMS		(E_DM_MASK + 0X92F1L)
#define		    E_DM92F2_DM1P_SINGLE_FMAP_FREE	(E_DM_MASK + 0X92F2L)
#define		    E_DM92F3_DM1P_MARK_FREE_BLD_SMS	(E_DM_MASK + 0X92F3L)
#define		    E_DM92F4_DM1P_DISPLAY_PAGETYPES	(E_DM_MASK + 0X92F4L)
#define		    E_DM92F5_DM1P_DISPLAY_BITMAPS	(E_DM_MASK + 0X92F5L)
#define		    E_DM92F6_DM1P_USED_RANGE		(E_DM_MASK + 0X92F6L)
#define		    E_DM92F7_DM1P_SCAN_MAP		(E_DM_MASK + 0X92F7L)
#define		    E_DM92F8_DM1P_GET_PAGE_BUILD_SMS	(E_DM_MASK + 0X92F8L)
#define		    E_DM92F9_DM1P_PUT_PAGE_BUILD_SMS	(E_DM_MASK + 0X92F9L)
#define		    E_DM92FA_DM1P_CREATE_PG_BLD_SMS	(E_DM_MASK + 0X92FAL)
#define		    E_DM92FB_DMM_ADD_SMS		(E_DM_MASK + 0X92FBL)
#define		    E_DM92FC_DMM_SETUP_TBL_RELFHDR	(E_DM_MASK + 0X92FCL)
#define		    E_DM92FD_DM2D_CONV_CORE_TABLE 	(E_DM_MASK + 0X92FDL)
#define		    E_DM92FE_DM2D_CONV_DBMS_CATS 	(E_DM_MASK + 0X92FEL)
#define		    E_DM92FF_DM2D_CONV_TO_65_ERROR 	(E_DM_MASK + 0X92FFL)
#define		    E_DM9300_DM0P_CLOSE_PAGE		(E_DM_MASK + 0x9300L)
#define		    E_DM9301_DM0P_FORCE_PAGES		(E_DM_MASK + 0x9301L)
#define		    E_DM9302_DM0P_INVALIDATE		(E_DM_MASK + 0x9302L)
#define		    E_DM9303_DM0P_MUTEX			(E_DM_MASK + 0x9303L)
#define		    E_DM9304_DM0P_UNMUTEX		(E_DM_MASK + 0x9304L)
#define		    E_DM9305_DM0P_TRAN_INVALIDATE	(E_DM_MASK + 0x9305L)
#define		    E_DM9306_DM0P_UNFIX_PAGE	        (E_DM_MASK + 0x9306L)
#define		    E_DM9307_DM0P_WBEHIND_SIGNAL	(E_DM_MASK + 0x9307L)
#define		    E_DM9308_DM0P_TOSS_PAGES		(E_DM_MASK + 0x9308L)
#define		    E_DM9309_DM0P_RECLAIM		(E_DM_MASK + 0x9309L)
#define		    E_DM930A_DM0P_ALLOCATE		(E_DM_MASK + 0x930AL)
#define		    E_DM930B_DM0P_DEALLOCATE		(E_DM_MASK + 0x930BL)
#define		    E_DM930C_DM0P_CHKSUM		(E_DM_MASK + 0x930CL)
#define		    E_DM930D_DM2U_READONLY_UPD_CATS	(E_DM_MASK + 0x930DL)
#define		    E_DM930E_DM0P_MO_INIT_ERR		(E_DM_MASK + 0x930EL)
#define		    E_DM9310_DM2R_RELEASE_RCB           (E_DM_MASK + 0x9310L)
#define		    E_DM9311_DM2R_DELETE		(E_DM_MASK + 0x9311L)
#define		    E_DM9312_DM2R_GET			(E_DM_MASK + 0x9312L)
#define		    E_DM9313_DM2R_POSITION		(E_DM_MASK + 0x9313L)
#define		    E_DM9314_DM2R_PUT			(E_DM_MASK + 0x9314L)
#define		    E_DM9315_DM2R_REPLACE		(E_DM_MASK + 0x9315L)
#define		    E_DM9316_DM2R_UNFIX_PAGES		(E_DM_MASK + 0x9316L)
#define		    E_DM9317_DM2R_TID_ACCESS_ERROR	(E_DM_MASK + 0x9317L)
#define             E_DM9318_DM1R_ALLOCATE              (E_DM_MASK + 0x9318L)
#define		    E_DM931F_DM2T_RENAME_FILE		(E_DM_MASK + 0x931FL)
#define		    E_DM9320_DM2T_BUILD_TCB		(E_DM_MASK + 0x9320L)
#define		    E_DM9321_DM2T_CLOSE			(E_DM_MASK + 0x9321L)
#define		    E_DM9322_DM2T_FIND_TCB		(E_DM_MASK + 0x9322L)
#define		    E_DM9323_DM2T_OPEN   		(E_DM_MASK + 0x9323L)
#define		    E_DM9324_DM2T_DEALLOCATE_TCB	(E_DM_MASK + 0x9324L)
#define             E_DM9325_DM2T_UPDATE_REL            (E_DM_MASK + 0x9325L)
#define             E_DM9326_DM2T_VERIFY_TCB            (E_DM_MASK + 0x9326L)
#define             E_DM9327_BAD_DB_OPEN_COUNT          (E_DM_MASK + 0x9327L)
#define             E_DM9328_DM2T_NOFREE_TCB            (E_DM_MASK + 0x9328L)
#define             E_DM9329_DM0J_M_ALLOCATE		(E_DM_MASK + 0x9329L)
#define             E_DM932A_DM0J_M_LIST_FILES		(E_DM_MASK + 0x932AL)
#define             E_DM932B_DM0J_M_NO_FILES		(E_DM_MASK + 0x932BL)
#define             E_DM932C_DM0J_M_BAD_NODE_JNL	(E_DM_MASK + 0x932CL)
#define             E_DM932D_DM0J_M_BAD_READ		(E_DM_MASK + 0x932DL)
#define             E_DM932E_DM0J_M_BAD_RECORD		(E_DM_MASK + 0x932EL)
#define             E_DM932F_DM0L_ALTER                 (E_DM_MASK + 0x932FL)
#define             E_DM9330_DM2T_PURGE_TCB		(E_DM_MASK + 0x9330L)
#define		    E_DM9331_DM2T_TBL_IDX_MISMATCH	(E_DM_MASK + 0x9331L)
#define		    E_DM9332_DM2T_IDX_DOMAIN_ERROR	(E_DM_MASK + 0x9332L)
#define		    E_DM9333_DM2T_EXTRA_IDX		(E_DM_MASK + 0x9333L)
#define		    E_DM9334_DM2T_MISSING_IDX		(E_DM_MASK + 0x9334L)
#define		    E_DM9335_DM2F_ENDFILE	        (E_DM_MASK + 0x9335L)
#define		    E_DM9336_DM2F_BUILD_ERROR	        (E_DM_MASK + 0x9336L)
#define		    E_DM9337_DM2F_CREATE_ERROR	        (E_DM_MASK + 0x9337L)
#define		    E_DM9338_DM2F_INIT_ERROR	        (E_DM_MASK + 0x9338L)
#define		    E_DM9339_DM2F_ALLOC_ERROR	        (E_DM_MASK + 0x9339L)
#define		    E_DM933A_DM2F_FORCE_ERROR	        (E_DM_MASK + 0x933AL)
#define		    E_DM933B_DM2F_FLUSH_ERROR	        (E_DM_MASK + 0x933BL)
#define		    E_DM933C_DM2F_READ_ERROR	        (E_DM_MASK + 0x933CL)
#define		    E_DM933D_DM2F_SENSE_ERROR	        (E_DM_MASK + 0x933DL)
#define		    E_DM933E_DM2F_RENAME_ERROR	        (E_DM_MASK + 0x933EL)
#define		    E_DM933F_DM2F_WRITE_ERROR	        (E_DM_MASK + 0x933FL)
#define		    E_DM9340_DM2F_RELEASE_ERROR	        (E_DM_MASK + 0x9340L)
#define		    E_DM9341_DM1I_FLUSH			(E_DM_MASK + 0x9341L)
#define		    E_DM9342_DM2U_REORG			(E_DM_MASK + 0x9342L)
#define		    E_DM9343_DM0L_ECP			(E_DM_MASK + 0x9343L)
#define		    E_DM9344_BM_CP_FLUSH_ERROR		(E_DM_MASK + 0x9344L)
#define		    E_DM9345_BM_WBFLUSH_WAIT		(E_DM_MASK + 0x9345L)
#define		    E_DM9346_DIRTY_PAGE_NOT_FIXED	(E_DM_MASK + 0x9346L)
#define		    E_DM9347_CONFIG_FILE_PATCHED	(E_DM_MASK + 0x9347L)
#define		    E_DM9348_BM_FLUSH_PAGES_ERROR	(E_DM_MASK + 0x9348L)
#define		    E_DM9349_DM1H_NOFULL		(E_DM_MASK + 0x9349L)
#define		    E_DM934A_BTREE_NOROOM		(E_DM_MASK + 0x934AL)
#define		    E_DM934B_DM1B_DUPCHECK		(E_DM_MASK + 0x934BL)
#define		    E_DM934C_DM0P_BAD_ADDRESS		(E_DM_MASK + 0x934CL)
#define		    E_DM934D_DM0P_BAD_PAGE		(E_DM_MASK + 0x934DL)
#define		    E_DM934E_DM0P_PAGE_FIXED		(E_DM_MASK + 0x934EL)
#define		    E_DM934F_DM0P_NO_TCB		(E_DM_MASK + 0x934FL)
#define		    E_DM9350_BTREE_INCONSIST		(E_DM_MASK + 0x9350L)
#define		    E_DM9351_BTREE_BAD_TID		(E_DM_MASK + 0x9351L)
#define		    E_DM9352_BAD_DMP_NOT_FOUND		(E_DM_MASK + 0x9352L)
#define		    E_DM9353_BAD_DMP_CLOSE		(E_DM_MASK + 0x9353L)
#define		    E_DM9354_BAD_DMP_CREATE		(E_DM_MASK + 0x9354L)
#define		    E_DM9355_BAD_DMP_OPEN		(E_DM_MASK + 0x9355L)
#define		    E_DM9356_BAD_DMP_READ		(E_DM_MASK + 0x9356L)
#define		    E_DM9357_BAD_DMP_WRITE		(E_DM_MASK + 0x9357L)
#define		    E_DM9358_BAD_DMP_UPDATE		(E_DM_MASK + 0x9358L)
#define		    E_DM9359_BAD_DMP_FORMAT		(E_DM_MASK + 0x9359L)
#define		    E_DM935A_BAD_DMP_LENGTH		(E_DM_MASK + 0x935AL)
#define		    E_DM935B_BAD_DMP_DELETE		(E_DM_MASK + 0x935BL)
#define		    E_DM935C_BAD_DMP_DIRCREATE		(E_DM_MASK + 0x935CL)
#define		    E_DM935D_BAD_DMP_TRUNCATE		(E_DM_MASK + 0x935DL)
#define		    E_DM935E_DM0D_CLOSE_ERROR		(E_DM_MASK + 0x935EL)
#define		    E_DM935F_DM0D_CREATE_ERROR		(E_DM_MASK + 0x935FL)
#define		    E_DM9360_DM0D_OPEN_ERROR		(E_DM_MASK + 0x9360L)
#define		    E_DM9361_DM0D_READ_ERROR		(E_DM_MASK + 0x9361L)
#define		    E_DM9362_DM0D_WRITE_ERROR		(E_DM_MASK + 0x9362L)
#define		    E_DM9363_DM0D_UPDATE_ERROR		(E_DM_MASK + 0x9363L)
#define		    E_DM9364_DM0D_FORMAT_ERROR		(E_DM_MASK + 0x9364L)
#define		    E_DM9365_DM0D_LENGTH_ERROR		(E_DM_MASK + 0x9365L)
#define             E_DM9366_DM0D_M_ALLOCATE		(E_DM_MASK + 0x9366L)
#define             E_DM9367_DM0D_M_LIST_FILES		(E_DM_MASK + 0x9367L)
#define             E_DM9368_DM0D_M_NO_FILES		(E_DM_MASK + 0x9368L)
#define             E_DM9369_DM0D_M_BAD_NODE_DMP	(E_DM_MASK + 0x9369L)
#define             E_DM936A_DM0D_M_BAD_READ		(E_DM_MASK + 0x936AL)
#define             E_DM936B_DM0D_M_BAD_RECORD		(E_DM_MASK + 0x936BL)
#define		    E_DM936C_DM0L_SBACKUP		(E_DM_MASK + 0x936CL)
#define		    E_DM936D_DM0L_EBACKUP		(E_DM_MASK + 0x936DL)
#define		    E_DM936E_DM0C_GETENV		(E_DM_MASK + 0x936EL)
#define		    E_DM936F_DM0C_LOINGPATH		(E_DM_MASK + 0x936FL)
#define		    E_DM9370_DM0L_SECURE		(E_DM_MASK + 0x9370L)
#define		    E_DM9371_RELEASE_TCB_WARNING	(E_DM_MASK + 0x9371L)
#define		    E_DM9372_BM_CANT_CREATE		(E_DM_MASK + 0x9372L)
#define		    E_DM9373_BM_CANT_CONNECT		(E_DM_MASK + 0x9373L)
#define		    E_DM9374_BM_CANT_DESTROY		(E_DM_MASK + 0x9374L)
#define		    E_DM9375_BM_PARMS_MISMATCH		(E_DM_MASK + 0x9375L)
#define		    E_DM9376_BM_LOCK_CONNECT		(E_DM_MASK + 0x9376L)
#define		    E_DM9377_BM_LOCK_RELEASE		(E_DM_MASK + 0x9377L)
#define		    E_DM9378_BM_NO_DBCACHE_LOCKS	(E_DM_MASK + 0x9378L)
#define		    E_DM9379_BM_CANT_OPEN_DB		(E_DM_MASK + 0x9379L)
#define		    E_DM937A_BM_NO_TBLCACHE_LOCKS	(E_DM_MASK + 0x937AL)
#define		    E_DM937B_BM_CANT_CHANGE_TABLE	(E_DM_MASK + 0x937BL)
#define		    E_DM937C_BM_BUFMGR_LOCK_ERROR	(E_DM_MASK + 0x937CL)
#define		    E_DM937D_BM_CONNECTIONS		(E_DM_MASK + 0x937DL)
#define		    E_DM937E_TCB_BUSY			(E_DM_MASK + 0x937EL)
#define		    E_DM937F_BM_NOT_A_BUFMGR		(E_DM_MASK + 0x937FL)
#define		    E_DM9380_TAB_KEY_OVERFLOW		(E_DM_MASK + 0x9380L)
#define		    E_DM9381_DM2R_PUT_RECORD		(E_DM_MASK + 0x9381L)
#define		    E_DM9382_OBJECT_KEY_OVERFLOW	(E_DM_MASK + 0x9382L)
#define		    E_DM9383_DM2T_UPD_LOGKEY_ERR 	(E_DM_MASK + 0x9383L)
#define		    E_DM9384_DM2T_UPD_LOGKEY_ERR 	(E_DM_MASK + 0x9384L)
#define		    E_DM9385_NO_INDEX_TUPLE		(E_DM_MASK + 0x9385L)
#define		    E_DM9386_SEC_INDEX_ERROR		(E_DM_MASK + 0x9386L)
#define		    E_DM9387_INDEX_UPDATE_ERROR		(E_DM_MASK + 0x9387L)
#define		    E_DM9388_BAD_DATA_LENGTH		(E_DM_MASK + 0x9388L)
#define		    E_DM9389_BAD_DATA_VALUE		(E_DM_MASK + 0x9389L)
#define		    E_DM938A_BAD_DATA_ROW		(E_DM_MASK + 0x938AL)
#define		    E_DM938B_INCONSISTENT_ROW		(E_DM_MASK + 0x938BL)
#define		    E_DM938C_BM_CLUSTER_CONNECT		(E_DM_MASK + 0x938CL)
#define		    E_DM938D_SHARED_MEM_ALLOCATE	(E_DM_MASK + 0x938DL)
#define		    E_DM938E_BM_MEDESTROY		(E_DM_MASK + 0x938EL)
#define		    E_DM938F_DM0L_DMU			(E_DM_MASK + 0x938FL)
#define		    E_DM9390_DM0A_BAD_AUDIT_WRITE	(E_DM_MASK + 0x9390L)
#define		    E_DM9391_DM0L_FSWAP			(E_DM_MASK + 0x9391L)
#define		    E_DM9392_DM0D_CONFIG_RESTORE	(E_DM_MASK + 0x9392L)
#define		    E_DM9393_DM0D_NO_DUMP_CONFIG	(E_DM_MASK + 0x9393L)
#define		    E_DM9394_DM0D_CONFIG_DELETE		(E_DM_MASK + 0x9394L)
#define		    E_DM9395_SYSTEM_SHUTDOWN_TEST	(E_DM_MASK + 0x9395L)
#define		    E_DM9396_BT_WRONG_PAGE_FIXED	(E_DM_MASK + 0x9396L)
#define		    E_DM9397_CP_INCOMPLETE_ERROR	(E_DM_MASK + 0x9397L)
#define		    E_DM9398_CMPCONTROL_SPACE		(E_DM_MASK + 0x9398L)
#define		    E_DM9399_SD_POSITION_TYPE		(E_DM_MASK + 0x9399L)
#define		    E_DM939A_SD_PAGE_ALLOC		(E_DM_MASK + 0x939AL)
#define		    E_DM939B_SD_FORCE			(E_DM_MASK + 0x939BL)
#define		    E_DM939C_SD_NOT_POSITIONED		(E_DM_MASK + 0x939CL)
#define		    E_DM939D_SD_UNPACK			(E_DM_MASK + 0x939DL)
#define		    E_DM939E_SD_PAGE_READ		(E_DM_MASK + 0x939EL)
#define		    E_DM939F_DM0L_TEST			(E_DM_MASK + 0x939FL)
#define		    E_DM93A0_ROLLFORWARD_JNLDISABLE	(E_DM_MASK + 0x93A0L)
#define		    E_DM93A1_AUDITDB_JNLDISABLE		(E_DM_MASK + 0x93A1L)
#define 	    E_DM93A2_DM1R_GET			(E_DM_MASK + 0x93A2L)
#define 	    E_DM93A3_QUALIFY_ROW_ERROR		(E_DM_MASK + 0x93A3L)
#define 	    E_DM93A4_DM1SD_GET			(E_DM_MASK + 0x93A4L)
#define		    E_DM93A5_INVALID_EXTEND_SIZE	(E_DM_MASK + 0x93A5L)
#define             E_DM93A6_EXTEND_DB                  (E_DM_MASK + 0x93A6L)
#define             E_DM93A7_BAD_FILE_PAGE_ADDR		(E_DM_MASK + 0x93A7L)
#define             E_DM93A8_DM0P_WPASS_ABORT		(E_DM_MASK + 0x93A8L)
#define             E_DM93A9_BM_FORCE_PAGE_EOF		(E_DM_MASK + 0x93A9L)
#define             E_DM93AA_READ_TCB                   (E_DM_MASK + 0x93AAL)
#define		    E_DM93AB_MAP_PAGE_TO_FMAP		(E_DM_MASK + 0X93ABL)
#define		    E_DM93AC_CONVERT_IIREL		(E_DM_MASK + 0X93ACL)
#define		    E_DM93AD_IIREL_NOT_CONVERTED	(E_DM_MASK + 0X93ADL)
#define		    E_DM93AE_BM_CACHE_LOCK		(E_DM_MASK + 0x93AEL)
#define		    E_DM93AF_BAD_PAGE_CNT		(E_DM_MASK + 0x93AFL)

/*
** Messages 93b0 ... 93ef added by the Btree Index Compression project:
*/
#define		    E_DM93B0_BXDUPKEY_ERROR		(E_DM_MASK + 0x93B0L)
#define		    E_DM93B1_BXSEARCH_ERROR		(E_DM_MASK + 0x93B1L)
#define		    E_DM93B2_BXSPLIT_ERROR		(E_DM_MASK + 0x93B2L)
#define		    E_DM93B3_BXBINSRCH_ERROR		(E_DM_MASK + 0x93B3L)
#define		    E_DM93B4_BXDIVISION_ERROR		(E_DM_MASK + 0x93B4L)
#define		    E_DM93B5_BXIDXDIV_ERROR		(E_DM_MASK + 0x93B5L)
#define		    E_DM93B6_BXLFDIV_ERROR		(E_DM_MASK + 0x93B6L)
#define		    E_DM93B7_BXDELETE_ERROR		(E_DM_MASK + 0x93B7L)
#define		    E_DM93B8_BXFORMAT_ERROR		(E_DM_MASK + 0x93B8L)
#define		    E_DM93B9_BXINSERT_ERROR		(E_DM_MASK + 0x93B9L)
#define		    E_DM93BA_BXSEARCH_BADPARAM		(E_DM_MASK + 0x93BAL)
#define		    E_DM93BB_BXJOIN_ERROR		(E_DM_MASK + 0x93BBL)
#define		    E_DM93BC_BXSEARCH_ERROR		(E_DM_MASK + 0x93BCL)
#define		    E_DM93BD_BXNEWROOT_ERROR		(E_DM_MASK + 0x93BDL)
#define		    E_DM93BE_BXGETRANGE_ERROR		(E_DM_MASK + 0x93BEL)
#define		    E_DM93BF_BXUPDRANGE_ERROR		(E_DM_MASK + 0x93BFL)
#define		    E_DM93C0_BBPUT_ERROR		(E_DM_MASK + 0x93C0L)
#define		    E_DM93C1_BBBEGIN_ERROR		(E_DM_MASK + 0x93C1L)
#define		    E_DM93C2_BBEND_ERROR		(E_DM_MASK + 0x93C2L)
#define		    E_DM93C3_BBBEGIN_BADPARAM		(E_DM_MASK + 0x93C3L)
#define		    E_DM93C4_BXUPDOVFL_ERROR		(E_DM_MASK + 0x93C4L)
/* msgs 93c5-93cf left available for dm1bbuild.c,dm1bindex.c & dm1bmerge.c */
#define             E_DM93C5_BXSPLIT_RESERVE            (E_DM_MASK + 0x93C5L)
#define             E_DM93C6_ALLOC_RESERVE              (E_DM_MASK + 0x93C6L)
#define             E_DM93C7_BXCLEAN_ERROR              (E_DM_MASK + 0x93C7L)
#define		    E_DM93C8_BXLOCKERROR_ERROR		(E_DM_MASK + 0x93C8L)
#define		    E_DM93D0_BALLOCATE_ERROR		(E_DM_MASK + 0x93D0L)
#define		    E_DM93D1_BDUPKEY_ERROR		(E_DM_MASK + 0x93D1L)

#define		    E_DM93E0_BAD_INDEX_ALLOC		(E_DM_MASK + 0x93E0L)
#define		    E_DM93E1_BAD_INDEX_COMP		(E_DM_MASK + 0x93E1L)
#define		    E_DM93E2_BAD_INDEX_DEL		(E_DM_MASK + 0x93E2L)
#define		    E_DM93E3_BAD_INDEX_GET		(E_DM_MASK + 0x93E3L)
#define		    E_DM93E4_BAD_INDEX_PUT		(E_DM_MASK + 0x93E4L)
#define		    E_DM93E5_BAD_INDEX_FORMAT		(E_DM_MASK + 0x93E5L)
#define		    E_DM93E6_BAD_INDEX_REPLACE		(E_DM_MASK + 0x93E6L)
#define		    E_DM93E7_INCONSISTENT_ENTRY		(E_DM_MASK + 0x93E7L)
#define		    E_DM93E8_BAD_INDEX_LSHIFT		(E_DM_MASK + 0x93E8L)
#define		    E_DM93E9_BAD_INDEX_RSHIFT		(E_DM_MASK + 0x93E9L)
#define		    E_DM93EA_NOT_A_LEAF_PAGE		(E_DM_MASK + 0x93EAL)
#define		    E_DM93EB_BAD_INDEX_TPUT		(E_DM_MASK + 0x93EBL)
#define		    E_DM93EC_BAD_INDEX_LENGTH		(E_DM_MASK + 0x93ECL)
#define		    E_DM93ED_WRONG_PAGE_TYPE		(E_DM_MASK + 0x93EDL)
#define             E_DM93EE_BAD_INDEX_RESERVE          (E_DM_MASK + 0x93EEL)
#define		    E_DM93EF_DM1P_REPAIR		(E_DM_MASK + 0x93EFL)
/* msgs 93Ee-93Ef left available for dm1cxlog_error msgs */
#define		    E_DM93F0_CONVERT_IIATT		(E_DM_MASK + 0X93F0L)
#define		    E_DM93F1_IIATT_NOT_CONVERTED	(E_DM_MASK + 0X93F1L)
#define             E_DM93F2_NO_TCB                     (E_DM_MASK + 0x93F2L)
#define             E_DM93F3_NOT_TCB_CACHE_LOCK         (E_DM_MASK + 0x93F3L)
#define             E_DM93F4_NO_BUFFER                  (E_DM_MASK + 0x93F4L)
#define             E_DM93F5_ROW_LOCK_PROTOCOL          (E_DM_MASK + 0x93F5L)
#define             E_DM93F6_BAD_REPOSITION             (E_DM_MASK + 0x93F6L)
#define             E_DM93F7_DM1B_REPOSITION            (E_DM_MASK + 0x93F7L)
#define		    E_DM93F8_DM2T_NOT_MASTER		(E_DM_MASK + 0x93F8L)
#define		    E_DM93F9_DM2T_MISSING_PP		(E_DM_MASK + 0x93F9L)


/* DMF internal errors -- 9400 - 94FF */

#define		    E_DM9400_RCP_LOGSTAT		(E_DM_MASK + 0x9400L)
#define		    E_DM9401_RCP_LOGINIT		(E_DM_MASK + 0x9401L)
#define		    E_DM9402_RCP_BLDHDR			(E_DM_MASK + 0x9402L)
#define		    E_DM9403_RCP_GETBLKSIZE		(E_DM_MASK + 0x9403L)
#define		    E_DM9404_RCP_GETLASTBLK		(E_DM_MASK + 0x9404L)
#define		    E_DM9405_RCP_GETCP			(E_DM_MASK + 0x9405L)
#define		    E_DM9406_RCP_GETAP			(E_DM_MASK + 0x9406L)
#define		    E_DM9407_RCP_RECOVER		(E_DM_MASK + 0x9407L)
#define		    E_DM9408_RCP_P0			(E_DM_MASK + 0x9408L)
#define		    E_DM9409_RCP_P1			(E_DM_MASK + 0x9409L)
#define		    E_DM940A_RCP_P2			(E_DM_MASK + 0x940AL)
#define		    E_DM940B				(E_DM_MASK + 0x940BL)
#define		    E_DM940C_RCP_ABORT			(E_DM_MASK + 0x940CL)
#define		    E_DM940E_RCP_RCPRECOVER		(E_DM_MASK + 0x940EL)
#define		    E_DM940F_RCP_CHKEVENT		(E_DM_MASK + 0x940FL)
#define		    E_DM9410_RCP_SHUTDOWN		(E_DM_MASK + 0x9410L)
#define		    E_DM9411_DMF_INIT			(E_DM_MASK + 0x9411L)
#define		    E_DM9412_DMF_TERM			(E_DM_MASK + 0x9412L)
#define		    E_DM9413_RCP_COUNT_OPENS		(E_DM_MASK + 0x9413L)
#define		    E_DM9414_RCP_REDO_RECOV		(E_DM_MASK + 0x9414L)
#define		    E_DM9415_RCP_GENDB			(E_DM_MASK + 0x9415L)
#define		    E_DM9416_RCP_REDOINIT		(E_DM_MASK + 0x9416L)
#define		    E_DM9417_RCP_REDO_PROC		(E_DM_MASK + 0x9417L)
#define		    E_DM9418_RCP_RDMUINS		(E_DM_MASK + 0x9418L)
#define		    E_DM9419_RCP_RDDBINS		(E_DM_MASK + 0x9419L)
#define		    E_DM941A_RCP_RBIINS			(E_DM_MASK + 0x941AL)
#define		    E_DM941B_RCP_REDOFINALE		(E_DM_MASK + 0x941BL)
#define		    E_DM941C_RCP_GENDB			(E_DM_MASK + 0x941CL)
#define		    E_DM941D_RCP_MP0			(E_DM_MASK + 0x941DL)
#define		    E_DM941E_RCP_MP1			(E_DM_MASK + 0x941EL)
#define		    E_DM941F_PROC_REDO			(E_DM_MASK + 0x941FL)
#define		    E_DM9420_RCP_WILLING_COMMIT		(E_DM_MASK + 0x9420L)
#define		    E_DM9421_RECOVER_WILLING_COMMIT	(E_DM_MASK + 0x9421L)
#define		    E_DM9422_RCP_COMMIT			(E_DM_MASK + 0x9422L)
#define		    E_DM9423_RCP_DIS_DUAL_LOGGING	(E_DM_MASK + 0x9423L)
#define		    E_DM9424_RCP_HARD_SHUTDOWN		(E_DM_MASK + 0x9424L)
#define		    E_DM9425_MEMORY_ALLOCATE		(E_DM_MASK + 0x9425L)
#define		    E_DM9426_LGADD_SIZE_MISMATCH	(E_DM_MASK + 0x9426L)
#define		    E_DM9427_DM0M_MEMORY_ALLOCATE	(E_DM_MASK + 0x9427L)
/*	DM9428 available */
#define             E_DM9429_EXPAND_POOL 		(E_DM_MASK + 0x9429L)
#define		    E_DM942A_RCP_DBINCON_FAIL		(E_DM_MASK + 0x942AL)
#define		    E_DM942B_CRASH_LOG			(E_DM_MASK + 0x942BL)
#define		    E_DM942C_DMPP_ROW_UNCOMP		(E_DM_MASK + 0x942CL)
#define		    E_DM942D_RCP_ONLINE_CONTEXT		(E_DM_MASK + 0x942DL)
#define		    E_DM942E_RCP_INCOMPLETE_CP		(E_DM_MASK + 0x942EL)
#define		    E_DM942F_RCP_CHECK_RECOVERY		(E_DM_MASK + 0x942FL)
#define		    E_DM9430_RCP_CHECK_REC_ERROR	(E_DM_MASK + 0x9430L)
#define		    E_DM9431_RCP_OPENDB_ERROR		(E_DM_MASK + 0x9431L)
#define		    E_DM9432_RCP_OPENTX_ERROR		(E_DM_MASK + 0x9432L)
#define		    E_DM9433_RCP_INCOMP_TX_ERROR	(E_DM_MASK + 0x9433L)
#define		    E_DM9434_DMR_COMPLETE_ERROR		(E_DM_MASK + 0x9434L)
#define		    E_DM9435_DMR_ANALYSIS_PASS		(E_DM_MASK + 0x9435L)
#define		    E_DM9436_DMR_UPDATE_BOF		(E_DM_MASK + 0x9436L)
#define		    E_DM9437_RCP_P3			(E_DM_MASK + 0x9437L)
#define		    E_DM9438_RCP_CLOSE_CONFIG		(E_DM_MASK + 0x9438L)
#define		    E_DM9439_APPLY_REDO			(E_DM_MASK + 0x9439L)
#define		    E_DM943A_APPLY_UNDO			(E_DM_MASK + 0x943AL)
#define		    E_DM943B_RCP_DBINCONSISTENT		(E_DM_MASK + 0x943BL)
#define		    E_DM943C_DMR_RECOVER		(E_DM_MASK + 0x943CL)
#define		    E_DM943D_RCP_DBREDO_ERROR		(E_DM_MASK + 0x943DL)
#define		    E_DM943E_RCP_DBUNDO_ERROR		(E_DM_MASK + 0x943EL)
#define		    E_DM943F_RCP_TX_WO_DB		(E_DM_MASK + 0x943FL)
#define		    E_DM9440_SD_BIND			(E_DM_MASK + 0x9440L)
#define		    E_DM9441_DM2U_FILE_CREATE		(E_DM_MASK + 0x9441L)
#define		    E_DM9442_REPLACE_COMPRESS		(E_DM_MASK + 0x9442L)
#define		    E_DM9443_DMR_PREINIT_RECOVER	(E_DM_MASK + 0x9443L)
#define		    E_DM9444_DMD_DIR_NOT_PRESENT	(E_DM_MASK + 0x9444L)
#define		    E_DM9445_DMR_ROW_CVT_ERROR		(E_DM_MASK + 0x9445L)
#define             E_DM9446_RCP_NO_SVDB_LOCK           (E_DM_MASK + 0x9446L)
#define             E_DM9447_RCP_SVDB_LOCK_ERROR        (E_DM_MASK + 0x9447L)
#define             E_DM9448_DM2F_NO_DISK_SPACE         (E_DM_MASK + 0x9448L)
#define             E_DM9449_DMD_CHECK         		(E_DM_MASK + 0x9449L)
#define		    E_DM944A_RCP_DBINCONSISTENT		(E_DM_MASK + 0x944AL)
#define		    E_DM944B_RCP_IDXINCONS		(E_DM_MASK + 0x944BL)
#define		    E_DM944C_RCP_TABLEINCONS		(E_DM_MASK + 0x944CL)
#define		    E_DM944D_RCP_INCONS_PAGE		(E_DM_MASK + 0x944DL)
#define		    E_DM944E_RCP_INCONS_TABLE		(E_DM_MASK + 0x944EL)

/* DMXE internal error conditions. */
#define		    E_DM9500_DMXE_BEGIN			(E_DM_MASK + 0x9500L)
#define		    E_DM9501_DMXE_COMMIT		(E_DM_MASK + 0x9501L)
#define		    E_DM9502_DMXE_SAVE			(E_DM_MASK + 0x9502L)
#define		    E_DM9503_DMXE_ABORT			(E_DM_MASK + 0x9503L)
#define		    E_DM9504_DMXE_WRITEBT		(E_DM_MASK + 0x9504L)
#define		    E_DM9505_DMXE_WILLING_COMMIT	(E_DM_MASK + 0x9505L)
#define		    E_DM9506_DMXE_DISCONNECT		(E_DM_MASK + 0x9506L)
#define		    E_DM9507_DMXE_RESUME		(E_DM_MASK + 0x9507L)
#define		    E_DM9508_DMXE_TRAN_INFO		(E_DM_MASK + 0x9508L)
#define		    E_DM9509_DMXE_PASS_ABORT		(E_DM_MASK + 0x9509L)

/* DBMS replication internal errors */
#define		    E_DM9550_NO_LOCAL_DB		(E_DM_MASK + 0x9550L)
#define		    E_DM9551_REP_DB_NOMEM		(E_DM_MASK + 0x9551L)
#define		    E_DM9552_REP_NO_ATTRIBUTE		(E_DM_MASK + 0x9552L)
#define		    E_DM9553_REP_NO_SHADOW		(E_DM_MASK + 0x9553L)
#define		    E_DM9554_REP_BAD_OPERATION		(E_DM_MASK + 0x9554L)
#define		    E_DM9555_REP_NOMEM			(E_DM_MASK + 0x9555L)
#define		    E_DM9556_REP_NO_SHAD_REC		(E_DM_MASK + 0x9556L)
#define		    E_DM9557_REP_NO_SHAD_MEM		(E_DM_MASK + 0x9557L)
#define		    E_DM9558_REP_NO_ARCH_MEM		(E_DM_MASK + 0x9558L)
#define		    E_DM9559_REP_NO_IN_ARCH		(E_DM_MASK + 0x9559L)
#define		    E_DM955A_REP_BAD_SHADOW		(E_DM_MASK + 0x955AL)
#define		    E_DM955B_REP_BAD_LOCAL_DB		(E_DM_MASK + 0x955BL)
#define		    E_DM955C_REP_NAME_TOO_LONG		(E_DM_MASK + 0x955CL)
#define		    E_DM955D_REP_BAD_DATE_CVT		(E_DM_MASK + 0x955DL)
#define		    E_DM955E_REP_NO_INPUT_QUEUE		(E_DM_MASK + 0x955EL)
#define		    E_DM955F_REP_BAD_INPUT_QUEUE	(E_DM_MASK + 0x955FL)
#define		    E_DM9560_REP_BAD_DIST_QUEUE		(E_DM_MASK + 0x9560L)
#define		    W_DM9561_REP_TXQ_FULL		(E_DM_MASK + 0x9561L)
#define		    W_DM9562_REP_NO_DD_DB_CDDS		(E_DM_MASK + 0x9562L)
#define		    W_DM9563_REP_TXQ_WORK		(E_DM_MASK + 0x9563L)
#define		    E_DM9564_REP_SCF_DBUSERADD		(E_DM_MASK + 0x9564L)
#define		    E_DM9565_REP_BAD_PRIO_LOOKUP	(E_DM_MASK + 0x9565L)
#define		    E_DM9566_REP_NO_PRIO_MATCH		(E_DM_MASK + 0x9566L)
#define		    E_DM9567_REP_BAD_TID		(E_DM_MASK + 0x9567L)
#define		    E_DM9568_REP_DBEVENT_ERROR		(E_DM_MASK + 0x9568L)
#define		    E_DM9569_REP_INQ_CLEANUP		(E_DM_MASK + 0x9569L)
#define		    W_DM956A_REP_MANUAL_DIST		(E_DM_MASK + 0x956AL)
#define		    E_DM956B_REP_BAD_CDDS_LOOKUP	(E_DM_MASK + 0x956BL)
#define		    E_DM956C_REP_NO_CDDS_MATCH		(E_DM_MASK + 0x956CL)
#define		    E_DM956D_REP_NO_SHAD_KEY_COL	(E_DM_MASK + 0x956DL)
#define		    E_DM956E_REP_BAD_KEY_SIZE		(E_DM_MASK + 0x956EL)
#define		    E_DM956F_REP_NO_ARCH_COL		(E_DM_MASK + 0x956FL)
#define		    W_DM9570_REP_NO_PATHS		(E_DM_MASK + 0x9570L)
#define		    W_DM9571_REP_DISABLE_CDDS		(E_DM_MASK + 0x9571L)
#define		    E_DM9572_REP_CAPTURE_FAIL		(E_DM_MASK + 0x9572L)
#define		    E_DM9573_REP_NO_SHAD		(E_DM_MASK + 0x9573L)
#define		    E_DM9574_REP_BAD_CDDS_TYPE		(E_DM_MASK + 0x9574L)
#define		    E_DM9575_REP_BAD_PRIO_TYPE		(E_DM_MASK + 0x9575L)
#define		    E_DM9576_REP_CPN_CONVERT		(E_DM_MASK + 0x9576L)
#define		    E_DM9577_REP_DEADLOCK_ROLLBACK	(E_DM_MASK + 0x9577L)
#define		    W_DM9578_REP_QMAN_DEADLOCK		(E_DM_MASK + 0x9578L)
#define		    W_DM9579_REP_MULTI_SHADOW		(E_DM_MASK + 0x9579L)
#define		    E_DM957A_REP_BAD_DATE_CMP		(E_DM_MASK + 0x957AL)
#define		    E_DM957B_REP_BAD_DATE		(E_DM_MASK + 0x957BL)
#define		    E_DM957C_REP_GET_ERROR		(E_DM_MASK + 0x957CL)
#define		    E_DM957D_REP_NO_DISTQ		(E_DM_MASK + 0x957DL)
#define		    E_DM957E_REP_DISTQ_UPDATE		(E_DM_MASK + 0x957EL)
#define		    E_DM957F_REP_DISTRIB_ERROR		(E_DM_MASK + 0x957FL)
#define		    E_DM9580_CONFIG_DBSERVICE_ERROR	(E_DM_MASK + 0x9580L)
#define		    E_DM9582_REP_NON_JNL		(E_DM_MASK + 0x9582L)

/* DMVE internal error conditions. */
#define		    E_DM9600_DMVE_BALLOC		(E_DM_MASK + 0x9600L)
#define		    E_DM9601_DMVE_BAD_PARAMETER		(E_DM_MASK + 0x9601L)
#define		    E_DM9602_DMVE_BDEALLOC		(E_DM_MASK + 0x9602L)
#define		    E_DM9603_DMVE_BIPAGE		(E_DM_MASK + 0x9603L)
#define		    E_DM9604_DMVE_BPEXTEND		(E_DM_MASK + 0x9604L)
#define		    E_DM9605_DMVE_CALLOC		(E_DM_MASK + 0x9605L)
#define		    E_DM9606_DMVE_CREATE		(E_DM_MASK + 0x9606L)
#define		    E_DM9607_DMVE_DEL			(E_DM_MASK + 0x9607L)
#define		    E_DM9608_DMVE_DESTROY		(E_DM_MASK + 0x9608L)
#define		    E_DM9609_DMVE_FCREATE		(E_DM_MASK + 0x9609L)
#define		    E_DM960A_DMVE_FRENAME		(E_DM_MASK + 0x960AL)
#define		    E_DM960B_DMVE_INDEX			(E_DM_MASK + 0x960BL)
#define		    E_DM960C_DMVE_MODIFY		(E_DM_MASK + 0x960CL)
#define		    E_DM960D_DMVE_PUT			(E_DM_MASK + 0x960DL)
#define		    E_DM960E_DMVE_RELOC			(E_DM_MASK + 0x960EL)
#define		    E_DM960F_DMVE_REP			(E_DM_MASK + 0x960FL)
#define		    E_DM9610_DMVE_SDEL			(E_DM_MASK + 0x9610L)
#define		    E_DM9611_DMVE_SM1RENAME		(E_DM_MASK + 0x9611L)
#define		    E_DM9612_DMVE_SM2CONFIG		(E_DM_MASK + 0x9612L)
#define		    E_DM9613_DMVE_SPUT			(E_DM_MASK + 0x9613L)
#define		    E_DM9614_DMVE_SREP			(E_DM_MASK + 0x9614L)
#define		    E_DM9615_DMVE_LK_FILE_EXTEND	(E_DM_MASK + 0x9615L)
#define		    E_DM9616_DMVE_UNLK_FILE_EXTEND	(E_DM_MASK + 0x9616L)
#define		    E_DM9617_DMVE_LOCATION      	(E_DM_MASK + 0x9617L)
#define		    E_DM9618_DMVE_SETBOF		(E_DM_MASK + 0x9618L)
#define		    E_DM9619_DMVE_JNLEOF		(E_DM_MASK + 0x9619L)
#define		    E_DM961A_DMVE_DBPURGE		(E_DM_MASK + 0x961AL)
#define		    E_DM961B_DMVE_ALTER			(E_DM_MASK + 0x961BL)
#define		    E_DM961C_DMVE_LOAD			(E_DM_MASK + 0x961CL)
#define		    E_DM961D_DMVE_BAD_LOADTAB		(E_DM_MASK + 0x961DL)
#define		    E_DM961E_DMVE_REDEL			(E_DM_MASK + 0x961EL)
#define		    E_DM961F_DMVE_REPUT			(E_DM_MASK + 0x961FL)
#define		    E_DM9620_DMVE_REREP			(E_DM_MASK + 0x9620L)
#define		    E_DM9621_DMVE_BAD_BEFORE_IMAGE	(E_DM_MASK + 0x9621L)
#define		    E_DM9622_DMVE_DMU			(E_DM_MASK + 0x9622L)
#define		    E_DM9623_DMVE_FSWAP			(E_DM_MASK + 0x9623L)
#define		    E_DM9624_DMVE_SBIPAGE		(E_DM_MASK + 0x9624L)
#define		    E_DM9625_DMVE_ALLOC			(E_DM_MASK + 0x9625L)
#define		    E_DM9626_DMVE_DEALLOC		(E_DM_MASK + 0x9626L)
#define		    E_DM9627_DMVE_ASSOC			(E_DM_MASK + 0x9627L)
#define		    E_DM9628_DMVE_EXTEND		(E_DM_MASK + 0x9628L)
#define		    E_DM9629_DMVE_SM0CLOSEPURGE		(E_DM_MASK + 0x9629L)
#define		    E_DM962A_DMVE_OVFL			(E_DM_MASK + 0x962AL)
#define		    E_DM962B_DMVE_NOFULL		(E_DM_MASK + 0x962BL)
#define		    E_DM9630_DMVE_UPDATE_OVFL		(E_DM_MASK + 0x9630L)
#define		    E_DM9631_UNDO_UPDATE_OVFL		(E_DM_MASK + 0x9631L)
#define		    E_DM9632_REDO_UPDATE_OVFL		(E_DM_MASK + 0x9632L)
#define		    E_DM9633_DMVE_SPLIT			(E_DM_MASK + 0x9633L)
#define		    E_DM9634_UNDO_SPLIT			(E_DM_MASK + 0x9634L)
#define		    E_DM9635_REDO_SPLIT			(E_DM_MASK + 0x9635L)
#define		    E_DM9636_UNDO_REP			(E_DM_MASK + 0x9636L)
#define		    E_DM9637_REDO_REP			(E_DM_MASK + 0x9637L)
#define		    E_DM9638_DMVE_REDO			(E_DM_MASK + 0x9638L)
#define		    E_DM9639_DMVE_UNDO			(E_DM_MASK + 0x9639L)
#define		    E_DM963A_UNDO_PUT			(E_DM_MASK + 0x963AL)
#define		    E_DM963B_REDO_PUT			(E_DM_MASK + 0x963BL)
#define		    E_DM963C_UNDO_OVFL			(E_DM_MASK + 0x963CL)
#define		    E_DM963D_REDO_OVFL			(E_DM_MASK + 0x963DL)
#define		    E_DM963E_DMVE_NOFULL		(E_DM_MASK + 0x963EL)
#define		    E_DM963F_UNDO_NOFULL		(E_DM_MASK + 0x963FL)
#define		    E_DM9640_REDO_NOFULL		(E_DM_MASK + 0x9640L)
#define		    E_DM9641_DMVE_FMAP			(E_DM_MASK + 0x9641L)
#define		    E_DM9642_UNDO_FMAP			(E_DM_MASK + 0x9642L)
#define		    E_DM9643_REDO_FMAP			(E_DM_MASK + 0x9643L)
#define		    E_DM9644_DMVE_EXTEND		(E_DM_MASK + 0x9644L)
#define		    E_DM9645_UNDO_EXTEND		(E_DM_MASK + 0x9645L)
#define		    E_DM9646_REDO_EXTEND		(E_DM_MASK + 0x9646L)
#define		    E_DM9647_DMVE_DISASSOC		(E_DM_MASK + 0x9647L)
#define		    E_DM9648_UNDO_DISASSOC		(E_DM_MASK + 0x9648L)
#define		    E_DM9649_REDO_DISASSOC		(E_DM_MASK + 0x9649L)
#define		    E_DM964A_UNDO_DEL			(E_DM_MASK + 0x964AL)
#define		    E_DM964B_REDO_DEL			(E_DM_MASK + 0x964BL)
#define		    E_DM964C_UNDO_ALLOC			(E_DM_MASK + 0x964CL)
#define		    E_DM964D_REDO_ALLOC			(E_DM_MASK + 0x964DL)
#define		    E_DM964E_DMVE_BTREE_PUT		(E_DM_MASK + 0x964EL)
#define		    E_DM964F_UNDO_BTREE_PUT		(E_DM_MASK + 0x964FL)
#define		    E_DM9650_REDO_BTREE_PUT		(E_DM_MASK + 0x9650L)
#define		    E_DM9651_DMVE_BTREE_DEL		(E_DM_MASK + 0x9651L)
#define		    E_DM9652_UNDO_BTREE_DEL		(E_DM_MASK + 0x9652L)
#define		    E_DM9653_REDO_BTREE_DEL		(E_DM_MASK + 0x9653L)
#define		    E_DM9654_DMVE_BTREE_FREE		(E_DM_MASK + 0x9654L)
#define		    E_DM9655_UNDO_BTREE_FREE		(E_DM_MASK + 0x9655L)
#define		    E_DM9656_REDO_BTREE_FREE		(E_DM_MASK + 0x9656L)
#define		    E_DM9657_DMVE_BTREE_OVFL		(E_DM_MASK + 0x9657L)
#define		    E_DM9658_UNDO_BTREE_OVFL		(E_DM_MASK + 0x9658L)
#define		    E_DM9659_REDO_BTREE_OVFL		(E_DM_MASK + 0x9659L)
#define		    E_DM965A_DMVE_BIPAGE		(E_DM_MASK + 0x965AL)
#define		    E_DM965B_UNDO_BIPAGE		(E_DM_MASK + 0x965BL)
#define		    E_DM965C_DMVE_AIPAGE		(E_DM_MASK + 0x965CL)
#define		    E_DM965D_REDO_AIPAGE		(E_DM_MASK + 0x965DL)
#define		    E_DM965E_UNDO_ASSOC			(E_DM_MASK + 0x965EL)
#define		    E_DM965F_REDO_ASSOC			(E_DM_MASK + 0x965FL)
#define		    W_DM9660_DMVE_TABLE_OFFLINE		(E_DM_MASK + 0x9660L)
#define		    E_DM9661_DMVE_FIX_TABIO_RETRY	(E_DM_MASK + 0x9661L)
#define		    E_DM9662_DMVE_FIX_TABIO		(E_DM_MASK + 0x9662L)
#define		    E_DM9663_DMVE_FIX_TABIO		(E_DM_MASK + 0x9663L)
#define		    E_DM9664_DMVE_INFO_UNKNOWN		(E_DM_MASK + 0x9664L)
#define		    E_DM9665_PAGE_OUT_OF_DATE		(E_DM_MASK + 0x9665L)
#define		    E_DM9666_PAGE_LSN_MISMATCH		(E_DM_MASK + 0x9666L)
#define		    E_DM9667_NOPARTIAL_RECOVERY		(E_DM_MASK + 0x9667L)
#define		    E_DM9668_TABLE_NOT_RECOVERED	(E_DM_MASK + 0x9668L)
#define		    E_DM9669_DMVE_BTUNDO_CHECK		(E_DM_MASK + 0x9669L)
#define		    E_DM966A_DMVE_KEY_MISMATCH		(E_DM_MASK + 0x966AL)
#define		    E_DM966B_DMVE_KEY_MISMATCH		(E_DM_MASK + 0x966BL)
#define		    E_DM966C_DMVE_TUPLE_MISMATCH	(E_DM_MASK + 0x966CL)
#define		    E_DM966D_DMVE_ROW_MISSING		(E_DM_MASK + 0x966DL)
#define		    E_DM966E_DMVE_ROW_OVERLAP		(E_DM_MASK + 0x966EL)
#define		    E_DM966F_DMVE_BTOVFL_NOROOM		(E_DM_MASK + 0x966FL)
#define		    E_DM9670_DMVE_BTUNDO_WARN		(E_DM_MASK + 0x9670L)
#define		    E_DM9671_DMVE_BTUNDO_FAILURE	(E_DM_MASK + 0x9671L)
#define		    E_DM9672_DMVE_ALLOC_WRONGMAP	(E_DM_MASK + 0x9672L)
#define		    E_DM9673_DMVE_ALLOC_MAPSTATE	(E_DM_MASK + 0x9673L)
#define		    E_DM9674_DMVE_PAGE_NOROOM		(E_DM_MASK + 0x9674L)
#define		    E_DM9675_DMVE_EXT_FHDR_STATE	(E_DM_MASK + 0x9675L)
#define		    E_DM9676_DMVE_FMAP_FHDR_STATE	(E_DM_MASK + 0x9676L)
#define		    E_DM9677_DMVE_FMAP_FMAP_STATE	(E_DM_MASK + 0x9677L)
#define		    E_DM9678_DMVE_OVFL_STATE		(E_DM_MASK + 0x9678L)
#define		    E_DM9679_DMVE_TABIO_BUILD		(E_DM_MASK + 0x9679L)
#define		    E_DM967A_DMVE_UNDO			(E_DM_MASK + 0x967AL)
#define		    E_DM967B_UNDO_FCREATE		(E_DM_MASK + 0x967BL)
#define		    E_DM967C_REDO_CREATE		(E_DM_MASK + 0x967CL)
#define		    E_DM967D_UNDO_CREATE		(E_DM_MASK + 0x967DL)
#define		    E_DM967E_REDO_DESTROY		(E_DM_MASK + 0x967EL)
#define		    E_DM967F_UNDO_DESTROY		(E_DM_MASK + 0x967FL)
#define		    E_DM9680_REDO_FRENAME		(E_DM_MASK + 0x9680L)
#define		    E_DM9681_UNDO_FRENAME		(E_DM_MASK + 0x9681L)
#define		    E_DM9682_REDO_MODIFY		(E_DM_MASK + 0x9682L)
#define		    E_DM9683_UNDO_MODIFY		(E_DM_MASK + 0x9683L)
#define		    E_DM9684_REDO_INDEX			(E_DM_MASK + 0x9684L)
#define		    E_DM9685_UNDO_INDEX			(E_DM_MASK + 0x9685L)
#define		    E_DM9686_REDO_RELOCATE		(E_DM_MASK + 0x9686L)
#define		    E_DM9687_UNDO_RELOCATE		(E_DM_MASK + 0x9687L)
#define		    E_DM9688_REDO_ALTER			(E_DM_MASK + 0x9688L)
#define		    E_DM9689_UNDO_ALTER			(E_DM_MASK + 0x9689L)
#define		    E_DM968A_REDO_FCREATE		(E_DM_MASK + 0x968AL)
#define		    E_DM968B_DMVE_UNRESERVE		(E_DM_MASK + 0x968BL)
#define             E_DM968D_DMVE_BTREE_INIT            (E_DM_MASK + 0x968DL)
#define             E_DM968E_DMVE_BTREE_FIND            (E_DM_MASK + 0x968EL)
#define             E_DM968F_DMVE_BID_CHECK             (E_DM_MASK + 0x968FL)
#define             E_DM9690_DMVE_IIREL_GET             (E_DM_MASK + 0x9690L)
#define             E_DM9691_DMVE_PAGE_FIX              (E_DM_MASK + 0x9691L)

/* DMSE internal error conditions. */
#define		    E_DM9700_SR_OPEN_ERROR		(E_DM_MASK + 0x9700L)
#define		    E_DM9701_SR_CLOSE_ERROR		(E_DM_MASK + 0x9701L)
#define		    E_DM9702_SR_READ_ERROR		(E_DM_MASK + 0x9702L)
#define		    E_DM9703_SR_WRITE_ERROR		(E_DM_MASK + 0x9703L)
#define		    E_DM9704_SORT_ALLMEM_ERROR		(E_DM_MASK + 0x9704L)
#define		    E_DM9706_SORT_PROTOCOL_ERROR	(E_DM_MASK + 0x9706L)
#define		    E_DM9707_SORT_BEGIN			(E_DM_MASK + 0x9707L)
#define		    E_DM9708_SORT_END			(E_DM_MASK + 0x9708L)
#define		    E_DM9709_SORT_INPUT			(E_DM_MASK + 0x9709L)
#define		    E_DM970A_SORT_GET			(E_DM_MASK + 0x970AL)
#define		    E_DM970B_SORT_PUT			(E_DM_MASK + 0x970BL)
#define		    E_DM970C_SORT_DOMERGE		(E_DM_MASK + 0x970CL)
#define		    E_DM970D_SORT_WRITE_RECORD		(E_DM_MASK + 0x970DL)
#define		    E_DM970E_SORT_READ_RECORD		(E_DM_MASK + 0x970EL)
#define		    E_DM970F_SORT_OUTPUT_RUN		(E_DM_MASK + 0x970FL)
#define		    E_DM9710_SORT_INPUT_RUN		(E_DM_MASK + 0x9710L)
#define		    E_DM9711_SR_OPEN_ERROR		(E_DM_MASK + 0x9711L)
#define		    E_DM9712_SR_READ_ERROR		(E_DM_MASK + 0x9712L)
#define		    E_DM9713_SR_WRITE_ERROR		(E_DM_MASK + 0x9713L)
#define		    E_DM9714_PSORT_CFAIL		(E_DM_MASK + 0x9714L)
#define		    E_DM9715_PSORT_PFAIL		(E_DM_MASK + 0x9715L)
#define		    E_DM9716_PSORT_CRERR		(E_DM_MASK + 0x9716L)
#define             E_DM9717_SORT_COST_ERROR		(E_DM_MASK + 0x9717L)

/* DMF Archiver internal error conditions:  9800 range. */
/* DMF CSP internal error conditions:  9900 range. */

/* DMF Gateway internal error conditions. */
#define		    E_DM9A00_GATEWAY_RECOVER		(E_DM_MASK + 0x9A00L)
#define		    E_DM9A01_GATEWAY_INIT		(E_DM_MASK + 0x9A01L)
#define		    E_DM9A02_GATEWAY_TERM		(E_DM_MASK + 0x9A02L)
#define		    E_DM9A03_GATEWAY_SESS_INIT		(E_DM_MASK + 0x9A03L)
#define		    E_DM9A04_GATEWAY_SESS_TERM		(E_DM_MASK + 0x9A04L)
#define		    E_DM9A05_GATEWAY_REGISTER		(E_DM_MASK + 0x9A05L)
#define		    E_DM9A06_GATEWAY_REMOVE		(E_DM_MASK + 0x9A06L)
#define		    E_DM9A07_GATEWAY_OPEN		(E_DM_MASK + 0x9A07L)
#define		    E_DM9A08_GATEWAY_CLOSE		(E_DM_MASK + 0x9A08L)
#define		    E_DM9A09_GATEWAY_POSITION		(E_DM_MASK + 0x9A09L)
#define		    E_DM9A0A_GATEWAY_GET		(E_DM_MASK + 0x9A0AL)
#define		    E_DM9A0B_GATEWAY_PUT		(E_DM_MASK + 0x9A0BL)
#define		    E_DM9A0C_GATEWAY_REPLACE		(E_DM_MASK + 0x9A0CL)
#define		    E_DM9A0D_GATEWAY_DELETE		(E_DM_MASK + 0x9A0DL)
#define		    E_DM9A0E_GATEWAY_BEGIN		(E_DM_MASK + 0x9A0EL)
#define		    E_DM9A0F_GATEWAY_COMMIT		(E_DM_MASK + 0x9A0FL)
#define		    E_DM9A10_GATEWAY_ABORT		(E_DM_MASK + 0x9A10L)
#define		    W_DM9A11_GATEWAY_PATCH		(E_DM_MASK + 0x9A11L)

/* DMPE (Peripheral Object Support) Error Codes */
#define		    E_DM9B00_DELETE_DMPE_ERROR		(E_DM_MASK + 0x9B00L)
#define		    E_DM9B01_GET_DMPE_ERROR		(E_DM_MASK + 0x9B01L)
#define		    E_DM9B02_PUT_DMPE_ERROR		(E_DM_MASK + 0x9B02L)
#define		    E_DM9B03_REPLACE_DMPE_ERROR		(E_DM_MASK + 0x9B03L)
#define		    E_DM9B04_LOAD_DMPE_ERROR		(E_DM_MASK + 0x9B04L)
#define		    E_DM9B05_DMPE_PUT_CLOSE_TABLE	(E_DM_MASK + 0x9B05L)
#define		    E_DM9B06_DMPE_BAD_CAT_SCAN		(E_DM_MASK + 0x9B06L)
#define		    E_DM9B07_DMPE_ADD_EXTENSION		(E_DM_MASK + 0x9B07L)
#define		    E_DM9B08_DMPE_NEED_TRAN		(E_DM_MASK + 0x9B08L)
#define		    E_DM9B09_DMPE_TEMP			(E_DM_MASK + 0x9B09L)
#define		    E_DM9B0A_DMPE_CREATE_CATALOG	(E_DM_MASK + 0x9B0AL)
#define		    E_DM9B0B_DMPE_LENGTH_MISMATCH	(E_DM_MASK + 0x9B0BL)

/* Additional DMP internal error messages */
#define		    E_DM9C00_JNL_FILE_INFO		(E_DM_MASK + 0x9C00L)
#define		    E_DM9C01_FIXED_BUFFER_INFO		(E_DM_MASK + 0x9C01L)
#define		    E_DM9C02_MISSING_TCB_INFO		(E_DM_MASK + 0x9C02L)
#define		    E_DM9C03_INDEX_TCB_INFO		(E_DM_MASK + 0x9C03L)
#define		    E_DM9C04_BUFFER_LOCK_INFO		(E_DM_MASK + 0x9C04L)
#define		    E_DM9C05_CACHE_CVT_ERROR		(E_DM_MASK + 0x9C05L)
#define		    E_DM9C06_JNL_BLOCK_INFO		(E_DM_MASK + 0x9C06L)
#define		    E_DM9C07_OUT_OF_LOCKLISTS		(E_DM_MASK + 0x9C07L)
#define		    E_DM9C08_OUT_OF_LOCKS		(E_DM_MASK + 0x9C08L)
#define		    E_DM9C09_DM0L_SM0_CLOSEPURGE	(E_DM_MASK + 0x9C09L)
#define		    E_DM9C0A_RSRV_NOT_ONLINE		(E_DM_MASK + 0x9C0AL)
#define		    E_DM9C0B_DM0L_AI			(E_DM_MASK + 0x9C0BL)
#define		    E_DM9C0C_DM0L_CRVERIFY		(E_DM_MASK + 0x9C0CL)
#define		    E_DM9C0D_DM0L_FMAP			(E_DM_MASK + 0x9C0DL)
#define		    E_DM9C0E_DM0L_BTPUT			(E_DM_MASK + 0x9C0EL)
#define		    E_DM9C0F_DM0L_BTDEL			(E_DM_MASK + 0x9C0FL)
#define		    E_DM9C10_DM0L_BTSPLIT		(E_DM_MASK + 0x9C00L)
#define		    E_DM9C11_DM0L_BTOVFL		(E_DM_MASK + 0x9C11L)
#define		    E_DM9C12_DM0L_BTFREE		(E_DM_MASK + 0x9C12L)
#define		    E_DM9C13_DM0L_BTUPDOVFL		(E_DM_MASK + 0x9C13L)
#define		    E_DM9C14_DM0L_DISASSOC		(E_DM_MASK + 0x9C14L)
#define		    E_DM9C15_DM0L_NOFULL		(E_DM_MASK + 0x9C15L)
#define		    E_DM9C16_DM1B_FIND_OVFL_SP		(E_DM_MASK + 0x9C16L)
#define		    E_DM9C17_DM1B_NEWDUP		(E_DM_MASK + 0x9C17L)
#define		    E_DM9C18_DM1B_NEWDUP_NOKEY		(E_DM_MASK + 0x9C18L)
#define		    E_DM9C19_UPDATE_LOAD		(E_DM_MASK + 0x9C19L)
#define		    E_DM9C1A_RENAME_LOAD		(E_DM_MASK + 0x9C1AL)
#define		    E_DM9C1B_DM1BXCHAIN			(E_DM_MASK + 0x9C1BL)
#define		    E_DM9C1C_BTDELBYPASS_WARN		(E_DM_MASK + 0x9C1CL)
#define		    E_DM9C1D_BXBADKEY			(E_DM_MASK + 0x9C1DL)
#define		    E_DM9C1E_BXKEY_ALLOC		(E_DM_MASK + 0x9C1EL)
#define		    E_DM9C1F_BXOVFL_ALLOC		(E_DM_MASK + 0x9C1FL)
#define		    E_DM9C20_CRITICAL_SECTION		(E_DM_MASK + 0x9C20L)
#define		    E_DM9C21_BXOVFL_ALLOC		(E_DM_MASK + 0x9C21L)
#define		    E_DM9C22_BXOVFL_INCONSIST		(E_DM_MASK + 0x9C22L)
#define		    E_DM9C23_BXOVFL_FREE		(E_DM_MASK + 0x9C23L)
#define		    E_DM9C24_BXGETDUP_KEY		(E_DM_MASK + 0x9C24L)
#define		    E_DM9C25_DM1B_RCB_UPDATE		(E_DM_MASK + 0x9C25L)
/* E_DM9C26, E_DM9C27 have messages in erdmf.msg already */
#define             E_DM9C28_BXV2PAGE_ERROR             (E_DM_MASK + 0x9C28L)
#define             E_DM9C29_BXV2KEY_ERROR              (E_DM_MASK + 0x9C29L)
#define             E_DM9C2A_NOROWLOCK                  (E_DM_MASK + 0x9C2AL)
#define             E_DM9C2B_REPL_ROWLK                 (E_DM_MASK + 0x9C2BL)
#define             E_DM9C2C_NOPAGELOCK                 (E_DM_MASK + 0x9C2CL)


#define		    W_DM9C50_DM2T_FIX_NOTFOUND		(E_DM_MASK + 0x9C50L)
#define		    W_DM9C51_DM2T_FIX_BUSY		(E_DM_MASK + 0x9C51L)
#define		    E_DM9C52_VALID_COUNT_MISMATCH	(E_DM_MASK + 0x9C52L)
#define		    E_DM9C53_DM2T_UNFIX_TCB		(E_DM_MASK + 0x9C53L)
#define		    E_DM9C54_DM2T_RELEASE_TCB		(E_DM_MASK + 0x9C54L)
#define		    E_DM9C55_DM2T_FIX_TABIO		(E_DM_MASK + 0x9C55L)
#define		    E_DM9C56_DM2T_UFX_TABIO		(E_DM_MASK + 0x9C56L)
#define		    E_DM9C57_DM2T_INIT_TABIO		(E_DM_MASK + 0x9C57L)
#define		    E_DM9C58_DM2T_ADD_TABIO		(E_DM_MASK + 0x9C58L)
#define		    E_DM9C59_DM2T_READ_TCB		(E_DM_MASK + 0x9C59L)
#define		    E_DM9C5A_DM2T_BUILD_INDX		(E_DM_MASK + 0x9C5AL)
#define		    E_DM9C5B_DM2T_OPEN_TABIO		(E_DM_MASK + 0x9C5BL)
#define		    E_DM9C5C_DM2T_ADD_TBIO_LOCERR	(E_DM_MASK + 0x9C5CL)
#define		    E_DM9C5D_DM2T_PURGEFIXED_WARN	(E_DM_MASK + 0x9C5DL)
#define		    E_DM9C5E_DM2T_ADD_RETRY_CNT		(E_DM_MASK + 0x9C5EL)
#define		    E_DM9C5F_DM2T_PURGEBUSY_WARN	(E_DM_MASK + 0x9C5FL)

#define		    E_DM9C60_DM1R_DELETE		(E_DM_MASK + 0x9C60L)

#define		    E_DM9C70_DM2T_RELEASE_IDXERR	(E_DM_MASK + 0x9C70L)
#define		    E_DM9C71_DM2T_ADD_TBIO_INFO		(E_DM_MASK + 0x9C71L)
#define		    E_DM9C72_DM2T_LOCCOUNT_MISMATCH	(E_DM_MASK + 0x9C72L)
#define		    E_DM9C73_DM2T_LOCCOUNT_ERROR	(E_DM_MASK + 0x9C73L)
#define		    E_DM9C74_DM2T_LOCNAME_ERROR		(E_DM_MASK + 0x9C74L)
#define		    E_DM9C75_DM2D_CLOSE_TCBBUSY		(E_DM_MASK + 0x9C75L)
#define		    E_DM9C76_DM2D_CLOSE_TCBERR		(E_DM_MASK + 0x9C76L)

#define		    E_DM9C80_ERROR_FINDING_TABIO_PTR	(E_DM_MASK + 0x9C80L)
#define		    E_DM9C81_DM0P_GET_TABIO		(E_DM_MASK + 0x9C81L)
#define		    E_DM9C82_DM0P_UNFIX_TABIO_INFO	(E_DM_MASK + 0x9C82L)
#define		    E_DM9C83_DM0P_CACHEFIX_PAGE		(E_DM_MASK + 0x9C83L)
#define		    E_DM9C84_DM0P_FIX_TABIO		(E_DM_MASK + 0x9C84L)
#define		    E_DM9C85_DM0P_BICHECK_INFO		(E_DM_MASK + 0x9C85L)
#define		    E_DM9C86_DM0P_BICHECK		(E_DM_MASK + 0x9C86L)
#define		    E_DM9C87_DM0P_UNCACHE_FIX		(E_DM_MASK + 0x9C87L)
#define		    E_DM9C88_DM0P_FORCE_PAGES		(E_DM_MASK + 0x9C88L)
#define		    E_DM9C89_DM2T_BUILD_TCB		(E_DM_MASK + 0x9C89L)
#define		    E_DM9C8A_DM2T_FIX_TCB		(E_DM_MASK + 0x9C8AL)
#define		    E_DM9C8B_DM2T_TBL_INFO		(E_DM_MASK + 0x9C8BL)
#define		    E_DM9C8C_BM_ESCALATE_FAIL		(E_DM_MASK + 0x9C8CL)
#define             E_DM9C8D_BAD_FILE_FORCE		(E_DM_MASK + 0x9C8DL)
/* 9c8e and 9c8f are used in the msg file but I don't see them... */
#define		    E_DM9C90_DM0L_RAWDATA		(E_DM_MASK + 0x9C90L)
#define		    E_DM9C91_BM_LSN_MISMATCH		(E_DM_MASK + 0x9C91L)
#define             E_DM9C92_UNEXTEND_DB                (E_DM_MASK + 0x9C92L)

/* Online Modify */
#define		    E_DM9CA0_DM0L_BSF			(E_DM_MASK + 0x9CA0L)
#define		    E_DM9CA1_DM0L_ESF			(E_DM_MASK + 0x9CA1L)
#define		    E_DM9CA2_SIDEFILE_TABLE		(E_DM_MASK + 0x9CA2L)
#define		    E_DM9CA3_SIDEFILE			(E_DM_MASK + 0x9CA3L)
#define		    E_DM9CA4_ONLINE_REDO		(E_DM_MASK + 0x9CA4L)
#define		    E_DM9CA5_ONLINE_ARCHIVER		(E_DM_MASK + 0x9CA5L)
#define		    E_DM9CA6_ONLINE_MODIFY		(E_DM_MASK + 0x9CA6L)
#define		    E_DM9CA7_ONLINE_INIT		(E_DM_MASK + 0x9CA7L)
#define		    E_DM9CA8_SIDEFILE_CREATE		(E_DM_MASK + 0x9CA8L)
#define		    E_DM9CA9_SIDEFILE_OPEN		(E_DM_MASK + 0x9CA9L)
#define		    E_DM9CAA_ARCH_SFCB_NOTFOUND		(E_DM_MASK + 0x9CAAL)
#define		    E_DM9CAB_ARCH_BAD_SIDEFILE		(E_DM_MASK + 0x9CABL)
#define		    E_DM9CAC_SIDEFILE_CLOSE		(E_DM_MASK + 0x9CACL)
#define		    E_DM9CAD_ARCH_FILTER_SIDEFILE	(E_DM_MASK + 0x9CADL)
#define		    E_DM9CAE_FIND_COMPENSATED		(E_DM_MASK + 0x9CAEL)
#define		    E_DM9CAF_ONLINE_DUPCHECK		(E_DM_MASK + 0x9CAFL)
#define		    E_DM9CB0_TEST_REDO_FAIL		(E_DM_MASK + 0x9CB0L)
#define		    E_DM9CB1_RNL_LSN_MISMATCH		(E_DM_MASK + 0x9CB1L)
#define		    E_DM9CB2_NO_RNL_LSN_ENTRY		(E_DM_MASK + 0x9CB2L)
#define		    E_DM9CB3_ONLINE_MOD_RESTRICT	(E_DM_MASK + 0x9CB3L)

/* Sequence Generator errors */
#define		    E_DM9D00_IISEQUENCE_NOT_FOUND	(E_DM_MASK + 0x9D00L)
#define		    E_DM9D02_SEQUENCE_NOT_FOUND		(E_DM_MASK + 0x9D02L)
#define		    E_DM9D03_SEQUENCE_NO_LOCKS		(E_DM_MASK + 0x9D03L)
#define		    E_DM9D04_SEQUENCE_DEADLOCK		(E_DM_MASK + 0x9D04L)
#define		    E_DM9D05_SEQUENCE_OPEN_FAILURE	(E_DM_MASK + 0x9D05L)
#define		    E_DM9D06_SEQUENCE_LOCK_INTERRUPT	(E_DM_MASK + 0x9D06L)
#define		    E_DM9D07_SEQUENCE_LOCK_ERROR	(E_DM_MASK + 0x9D07L)
#define		    E_DM9D08_SEQUENCE_UNLOCK_ERROR	(E_DM_MASK + 0x9D08L)
#define		    E_DM9D09_SEQUENCE_CLOSE_FAILURE	(E_DM_MASK + 0x9D09L)
#define		    E_DM9D0B_SEQUENCE_LOCK_TIMEOUT	(E_DM_MASK + 0x9D0BL)
#define		    E_DM9D0C_SEQUENCE_LOCK_BUSY		(E_DM_MASK + 0x9D0CL)

/* DMD_CHECK messages. */
#define		    E_DMF000_MUTEX_INIT			(E_DM_MASK + 0xF000L)
#define		    E_DMF001_MUTEX_LOCK			(E_DM_MASK + 0xF001L)
#define		    E_DMF002_MUTEX_FREE			(E_DM_MASK + 0xF002L)
#define		    E_DMF003_MUTEX_UNLOCK		(E_DM_MASK + 0xF003L)
#define		    E_DMF004_EVENT_LRELEASE		(E_DM_MASK + 0xF004L)
#define		    E_DMF005_EVENT_LPREPARE		(E_DM_MASK + 0xF005L)
#define		    E_DMF006_EVENT_RELEASE		(E_DM_MASK + 0xF006L)
#define		    E_DMF007_EVENT_WAIT			(E_DM_MASK + 0xF007L)
#define		    E_DMF008_DM0M_BAD_OBJECT		(E_DM_MASK + 0xF008L)
#define		    E_DMF009_DM0M_FREE_OVERLAP_PREV	(E_DM_MASK + 0xF009L)
#define		    E_DMF00A_DM0M_FREE_OVERLAP_NEXT	(E_DM_MASK + 0xF00AL)
#define		    E_DMF00B_DM0M_PROTECT		(E_DM_MASK + 0xF00BL)
#define		    E_DMF00C_DM0M_POOL_CORRUPT		(E_DM_MASK + 0xF00CL)
#define		    E_DMF00D_NOT_DCB			(E_DM_MASK + 0xF00DL)
#define		    E_DMF00E_NOT_FCB			(E_DM_MASK + 0xF00EL)
#define		    E_DMF00F_NOT_TCB			(E_DM_MASK + 0xF00FL)
#define		    E_DMF010_DM1R_NO_PAGE		(E_DM_MASK + 0xF010L)
#define		    E_DMF011_DM1R_BAD_TID		(E_DM_MASK + 0xF011L)
#define		    E_DMF012_ADT_FAIL			(E_DM_MASK + 0xF012L)
#define		    E_DMF013_DM1B_CURRENT_INDEX		(E_DM_MASK + 0xF013L)
#define		    E_DMF014_DM1B_PARENT_INDEX		(E_DM_MASK + 0xF014L)
#define		    E_DMF015_EVENT_SET			(E_DM_MASK + 0xF015L)
#define		    E_DMF016_EVENT_LWAIT		(E_DM_MASK + 0xF016L)
#define		    E_DMF020_DM0P_VALIDATE		(E_DM_MASK + 0xF020L)
#define		    E_DMF022_DM0P_CACHE_IX_IS		(E_DM_MASK + 0xF022L)
#define		    E_DMF023_BM_CP_FLUSH_SEVERE		(E_DM_MASK + 0xF023L)
#define		    E_DMF024_INCOMPLETE_CP		(E_DM_MASK + 0xF024L)
#define		    E_DMF025_BM_CONNECT_COUNT		(E_DM_MASK + 0xF025L)
#define		    E_DMF026_DMPP_PAGE_INCONS		(E_DM_MASK + 0xF026L)
#define		    E_DMF027_DM0P_PASS_ABORT		(E_DM_MASK + 0xF027L)
#define		    E_DMF028_DM0P_UNFIX_TABIO		(E_DM_MASK + 0xF028L)
#define		    E_DMF029_DMXE_PASS_ABORT		(E_DM_MASK + 0xF029L)
#define             E_DMF030_CACHE_NULL                 (E_DM_MASK + 0xF030L)

/* Function prototypes definitions. Function prototypes which are placed here
** should only be those functions that are truly used by ALL of dmf and which
** have a special excuse for being here.
*/

#define	dmd_check(error_code) \
	dmd_check_fcn(error_code, __FILE__, __LINE__)

FUNC_EXTERN VOID dmd_check_fcn(
    i4		error_code,
    char	*file,
    i4		line);
