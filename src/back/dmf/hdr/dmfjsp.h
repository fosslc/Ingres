/*
** Copyright (c) 1986, 2005 Ingres Corporation
**
**
*/

/**
** Name: DMFJSP.H - DMF Jounaling Support Program definitions.
**
** Description:
**      This file contains the structures and constants used by the journaling
**	support routines.
**
** History:
**      06-nov-1986 (Derek)
**          Created.
**	21-jan-1989 (Edhsu)
**	    Add code to support online backup
**	 1-dec-1989 (rogerk)
**	    Added JSX_CKP_STALL flag.  This is used to facilitate online
**	    checkpoint testing by causing checkpoint to stall in the middle
**	    of the backup phase.
**	    Added new online checkpoint error numbers.
**	08-dec-1989 (mikem)
**	    Added 4 new errors:
** 		E_DM1144_CPP_DEAD_ARCHIVER
** 		E_DM1145_CPP_DEAD_ARCHIVER
** 		E_DM1146_CPP_DEAD_ARCHIVER
** 		E_DM1147_CPP_DEAD_ARCHIVER
**	03-jan-1989 (mikem)
**	     Adding 5 new errors:
**		E_DM1149_BACKUP_FAILURE.
**		E_DM1150_BACKUP_FAILURE.
**		E_DM1151_CPP_CPNEEDED
**		E_DM1152_CPP_ONLINE_PURGE
**		E_DM1153_CPP_CLUSTER_MERGE
**	17-may-1990 (rogerk)
**	    Added new auditdb error definitions.
**	21-may-1990 (bryanp)
**	    Added support for new #c flag to request rollforward from last
**	    valid checkpoint. Added JSX_CKP_SELECTED, JSX_LAST_VALID_CKP, and
**	    jsx_ckp_number to the DMF_JSX definition.
**	    Added E_DM133C_RFP_UNUSABLE_CKP message.
**	    Added E_DM1155_CPP_TEMPLATE_MISSING message.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	30-oct-1990 (bryanp)
**	    For Bug #34120:
**	    E_DM1022_JSP_NO_DISTRIB_DB
**	    E_DM1023_JSP_NO_LOC_INFO
**	    E_DM1024_JSP_BAD_LOC_INFO
**	    Also corrected a typo in E_DM1020
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    25-feb-1991 (rogerk)
**		Added support for Archiver Stability project.
**		Added alterdb option JSX_O_ALTER and JSX_JDISABLE, JSX_NOLOGGING
**		Added alterdb error codes.
**	    22-apr-1991 (rogerk)
**		Added E_DM1156_CPP_NOLOGGING message for error of trying to
**		backup database while Set Nologging sessions are running.
**	20-dec-1991 (bryanp) integrated: 6-sep-1991 (markg)
**	    Added messages E_DM1025 for errors in begin and end date
**	    processing in auditdb. Bug 39150.
**	07-jul-1992 (ananth)
**	    Prototyping DMF.
**	03-nov-1992 (robf)
**	    Added new JSP errors for SXF operations
**	30-nov-1992 (robf)
**	    Removed definition for JSX_AUDIT, no longer needed since
**	            SXF handles it.
**	    Removed definition for JSX_SUPERUSER since Superuser priv
**	    now longer exists, and it doesn't do anything useful.
**	17-dec-1992 (robf)
**	    Add handling for PM errors.
**	05-feb-93 (jnash)
**	    Reduced logging project.  Add JSX_AUDIT_ALL and
**	    JSX_AUDIT_VERBOSE.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added JSX_NORFP_UNDO flag to bypass undo phase of rollforward.
**	    Added new error codes.
**	21-apr-93 (jnash)
**	    For AUDITDB:
**	      	Add JSX_TBL_LIST, JSX_FNAME_LIST, jsx_tbl_list, jsx_fname_cnt,
**		jsx_fname_list.  Remove unused jsx_audit_file and jsx_t_name.
**		Add errors E_DM1044_JSP_FILE_TBL_EQ, E_DM1045_JSP_FILE_REQ_TBL.
**		E_DM1215_ATP_TBL_NONEXIST.
**	    For ALTERDB:
**	      	Add jsx_status1, jsx_max_jnl_blocks, jsx_jnl_block_size,
**		jsx_init_jnl_blocks, JSX_JNL_BLK_SIZE, JSX_JNL_NUM_BLOCKS,
**		Add errors E_DM103D_JSP_JNL_SIZE_CHGD,
**		E_DM103E_JSP_JNL_BKSZ_CHGD, E_DM103F_JSP_JNL_BKCNT_CHGD.
**	  	E_DM1040_JSP_JNL_SIZE_ERR, E_DM1041_JSP_JNL_BKSZ_ERR,
**		E_DM1042_JSP_JNL_BKCNT_ERR, E_DM1043_JSP_ALTER_ONEDB,
**	    For ROLLFORWARD:
**	        Add JSX_NORFP_LOGGING to support rollforwarddb -nologging,
**	        Add E_DM1346_RFP_NOLOGGING and E_DM1347_RFP_NOROLLBACK.
**	21-jun-93 (rogerk)
**	    Add new rollforward errors.
**	28-jun-93 (robf)
**	    Add new Secure 2.0 errors
**	26-jul-93 (jnash)
**	    Add new rollforward errors.
**	    Add JSX_ATP_INCONS, JSX_ATP_WAIT, JSX_HELP.
**	26-jul-93 (rogerk)
**	    Add new rollforward errors.
**	20-sep-1993 (jnash)
**	    Delim ids:
**	      - Add jsx_name_case, jsx_delim_case, jsx_real_user_case.
**	      - Add jsp_set_case() and jsp_get_case() FUNC_EXTERNs.
**	      - New errors.
**	    ALTERDB -delete_oldest_ckp:
**	      - Move DMFRFP_VALID_CHECKPOINT here from dmfrfp.c, rename
**	        to DMFJSP_VALID_CHECKPOINT for use by alterdb.
**	      - Add JSX_CKP_DELETE.
**	      - New errors.
**	20-sep-93 (rogerk)
**	    Add new rollforward errors.
**	1-oct-93 (robf)
**          Prototypes for JSP xact wrappers
**	18-oct-93 (jnash)
**	    Add E_DM1218_ATP_OROW_COMPRESS.
**	22-nov-93 (jnash)
**	    Add E_DM134C_RFP_INV_CHECKPOINT.
**	31-jan-94 (jnash)
**	    Add W_DM134D_RFP_NONJNL_TAB_DEL, E_DM134E_RFP_LAST_TABID,
**	    E_DM134F_RFP_INV_JNL, JSX_FORCE.
**	15-feb-1994 (andyw)
**	    Added new error messages E_DM1051_JSP_NO_INSTALL &
**	    E_DM1052_JSP_VALID_USAGE also corrected  E_DM1050_JSP_CPP_ONE_DB
**	    (E_DM_MASK + 0x1050F) to (E_DM_MASK + 0x1050L)
**	21-feb-94 (jnash)
**	  - Add jsx_start_lsn, jsx_end_lsn.
**	  - Add new errors.
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Table Level Rollforwarddb:
**		    Added jsx_status1 for table level processing on
**		    	-nosecondary_index
**			-on_error_continue
**			-online
**			-status_count=
**			-noblobs
**			-force_logical_consistent
**			-statistics
**			-Default behaviour for rollforwarddb at db level
**	16-nov-1994 (juliet)
**	    Added :
**              RFP_INV_ARG_FOR_DB_RECOVERY,
**              RFP_NO_CORE_TBL_RECOVERY,
**              RFP_INVALID_ARGUMENT,
**              RFP_INCONSIS_DB_NO_TBLRECV,
**              RFP_VIEW_TBL_NO_RECV,
**              RFP_GATEWAY_TBL_NO_RECV,
**              RFP_RECOVERING_MAX_TBLS,
**              RFP_DUPLICATE_TBL_SPECIFIED,
**              RFP_CANNOT_OEPN_IIRELATION,
**              RFP_CANNOT_OPEN_IIRELIDX.
**	22-nov-1994 (andyw)
**	    Added RFP_USR_INVALID_TABLE
**      03-jan-1995 (alison)
**          Added relocation errors:
**              RFP_OLDLOC_NOTFOUND
**              RFP_CANT_OPEN_NEWDB
**              RFP_CANT_UPDATE_IIDEVICES
**              RFP_CANT_UPDATE_IIRELATION
**              JSP_RELOC_ONE_DB
**          Added new dmfjsp operation JSX_O_RELOC
**          Renamed rfp_relocate() to dmfreloc()
**	06-jan-1995 (shero03)
**	    Added Partial Checkpoint Support:
**		Added jsx_status1 for table level checkpoint
**      07-jan-1995 (alison)
**          Added E_DM1161_CPP_NO_CORE_TBL_CKPT
**                E_DM1162_CPP_USR_INVALID_TBL
**                E_DM1372_RFP_NO_TBL_RECOV
**          Fixed E_DM136D_RFP_USR_INVALID_TABLE
**      17-jan-1995 (stial01)
**          Added E_DMxxxx_RELOC... errors
**          Re-numbered some recently added RFP errors because of RFP relocation
**          errors deleted (now they are RELOC errors).
**          BUG 66473: added E_DM1361_RFP_NO_DB_RECOV_TBL_CKPT
**	10-apr-95 (angusm)
**          Added JSX_O_IN1 for "DBALERT sponsors for INGRES" project.
**      05-apr-1995 (stial01)
**          BUG 67848: Added E_DM1518_RELOC_IIDBDB_ERR
**          BUG 67385: Added E_DM1163_CPP_TMPL_ERROR
**                           E_DM1164_CPP_TMPL_CMD_MISSING
**                           E_DM1362_RFP_TMPL_ERROR
**                           E_DM1363_RFP_TMPL_CMD_MISSING
**	05-jun-1995 (wonst02)
**	    Bug 68886, 68866
**	    Added E_DM1165 - E_DM1169 and E_DM1364 - E_DM1367
**	29-jun-1995 (thaju02)
**	    Modified DMF_JSX structure.  Added field jsx_stall_time and
**	    definition for JSX1_STALL_TIME for "ckpdb -max_stall_time=..."
**	    feature. 
**      30-jun-1995 (shero03)
**          Added -ignore flag for rollforwarddb
**      10-aug-1995 (sarjo01)
**          Added JSX_O_FLOAD and fastload error codes. 
**	20-july-1995 (thaju02)
**	    bug #69981 - added jsx_ignored_errors flag.
**	    jsx_ignored_errors = TRUE when -ignore argument specified
**	    and errors have occurred during application of journal records.
**	 7-feb-1996 (nick)
**	    Changes to support fix for #74453.
**      01-sep-1995 (tchdi01)
**          Added production mode related error codes
**                E_DM1519_PMODE_IIDBDB_ERR
**                E_DM1520_PMODE_DB_BUSY
**          Added new jsx status flag JSX1_PMODE
**	12-jun-96 (nick)
**	    Add E_DM105A_JSP_INC_ARG.
**	 2-jul-96 (nick)
**	    Add E_DM105B_JSP_NUM_LOCATIONS
**	08-jul-1996 (hanch04)
**	    Added support for parallel checkpointing
**	15-jan-1997 (nanpr01)
**	    Added E_DM105C_JSP_INV_ARG_VAL for bugs 80038 & 80001.
**	30-jan-97 (stephenb)
**	    Add E_DM1618_FLOAD_INIT_SCF, E_DM1619_FLOAD_BAD_READ and
**	    E_DM161A_FLOAD_COUPONIFY_ERROR.
** 	14-apr-1997 (wonst02)
** 	    Added JSX_READ_ONLY and JSX_READ_WRITE for readonly databases.
**	13-Nov-1997 (jenjo02)
**	    Added JSX_CKP_STALL_RW flag for online checkpoint.
**	23-jun-1998(angusm)
**	    Add I_DM116D_CPP_NOT_DEF_TEMPLATE, I_DM1369_RFP_NOT_DEF_TEMPLATE
**	30-jun-98 (thaju02)
**	    Add E_DM116E_CPP_TBL_CKP_DISALLOWED. (B91356)
**	10-oct-1998 (nanpr01)
**	    Added E_DM1370_RFP_NO_SEC_INDEX for bug 80014.
**	15-Mar-1999 (jenjo02)
**	    Removed much-maligned "-s" option, JSX_CKP_STALL_RW define.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	18-jan-2000 (gupsh01)
**	    Added flags JSX2_INTERNAL_DATA and JX2_ABORT_TRANS for auditdb
**	    switches -internal_data and -aborted_transactions.  
**      26-jan-2000 (gupsh01)
**          Added flags JSX2_NO_HEADER and JX2_LIMIT_OUTPUT for auditdb
**          switches -no_header and -limit_output.
**      28-feb-2000 (gupsh01)
**          Added support for table names with format owner_name.table_name.
**          modified the DMF_JSX structure.                  
**	15-sep-2000 (gupsh01)
**	    Added new error E_DM121B_ATP_NOEMPTY_TUPLE. for -aborted_transaction 
**	    for auditdb.
**	27-mar-2001 (somsa01)
**	    In DMF_JSX, added JSX2_EXPAND_LOBJS. This flag, when set, tells
**	    us that we need to not print out information on extension tables
**	    but expand a large object coupon into the actual blob.
**	30-may-2001 (devjo01)
**	    Add proto for jsp_file_maint.
**	01-Aug-2001 (bolke01)
**	     Adding 2 new errors:
**		    E_DM116F_CPP_DRN_PURGE
**		    E_DM1170_NO_RECOVERY_SERVER
**      22-jan-2003 (stial01)
**          Added E_DM1371_RFP_INFO
**	5-Apr-2004 (schka24)
**	    Swipe unused 1323 error for rawdata consistency check.
**      13 -Apr-2004(nansa02)   
**          Added E_DM1406_ALT_INV_UCOLLATION and E_DM1407_ALT_CON_UNICODE for Alterdb -convert_unicode.
**      01-sep-2004 (stial01)
**          Support cluster online checkpoint
**      30-sep-2004 (stial01)
**          Moved cluster online checkpoint defines here for non-cluster builds
**      21-dec-2004 (stial01)
**          Added E_DM1372_RFP_JNL_WARNING
**	23-feb-05 (inkdo01)
**	    Added E_DM1408_ALT_ALREADY_UNICODE.
**      01-apr-2005 (stial01)
**          Added jsx_ckp_phase
**      07-apr-2005 (stial01)
**          Changes for new rollforwarddb flags
**      05-may-2005 (stial01)
**          Added JSX2_ON_ERR_PROMPT
**      25-oct-2005 (stial01)
**          Added jsx_ignored_tbl_err, jsx_ignored_idx_err
**          Removed JSX2_ON_ERR_IX_IGNORE (it is new default index err handling)
**	16-nov-2005 (devjo01)
**	    Added dmf_write_msg_with_args.
**      12-apr-2007 (stial01)
**          Added support for rollforwarddb -incremental
**      04-may-2007 (stial01)
**          Added E_DM105D_JSP_INFO 
**      09-oct-2007 (stial01)
**          Added JSX2_RFP_ROLLBACK
**	10-Dec-2007 (jonj)
**	    Moved CKP_CSP_? defines to csp.h
**      10-jan-2008 (stial01)
**          Added E_DM161B_FLOAD_DB_BUSY
**      04-mar-2008 (stial01)
**          Added E_DM1377_RFP_INCR_JNL_WARNING
**      11-mar-2008 (stial01)
**	    Added E_DM1409_JNLSWITCH
**	18-jun-2008 (gupsh01)
**	    Added E_DM1410_ALT_UCOL_NOTEXIST.
**      27-jun-2008 (stial01)
**          Added E_DM1373_RFP_INCR_LAST_LSN
**	08-Sep-2008 (jonj)
**	    SIR 120874: Macroized dmf_write_msg, dmf_write_msg_with_args 
**	    to call common function dmfWriteMsgFcn(), passing caller's 
**	    __FILE__ and __LINE__.
**	29-Oct-2008 (jonj)
**	    SIR 120874: Add non-variadic macros and functions for
**	    compilers that don't support them.
**	04-Nov-2008 (jonj)
**	    Define dmfWriteMsg as preferred form, including DB_ERROR *.
**	    Add non-variadic form prototype, dmfWriteMsgNV.
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	    Delete function prototypes for dmf_write_msg, dmf_write_msg_with_args,
**	    all code now uses dmfWriteMsg.
**	    Add missing prototype for dmffload.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Added E_DM1411, E_DM1412, E_DM1413, E_DM1414.
[@history_template@]...
**/


/*
**  Defines of other constants.
*/

/*
** Name: DMFJSP_VALID_CHECKPOINT    - is this checkpoint valid?
**
** Description:
**	A checkpoint is considered valid and usable for roll-forward if:
**	    1) the ckp_jnl_valid bit is set in the journal history and
**	    2) the checkpoint mode (ckp_mode) in the journal history is OFFLINE
**		OR
**	    2) ckp_mode is ONLINE and the ckp_dmp_valid bit is set in the
**		dump history.
**
**	(It would have been nicer to have one validity check for either type
**	of checkpoint, and perhaps we can enhance checkpoint to operate that
**	way in the future).
**
**	This macro returns non-zero if the checkpoint is valid and 0 if it
**	is not.
**
**	The ckp_jnl_valid bit really has nothing to do with the validity of
**	the journals. All journals are valid. If ckp_jnl_valid is set in an
**	OFFLINE checkpoint, then the checkpoint is valid. Yuck.
**
** Inputs:
**	cnf	- config pointer
**	j_hist	- journal history number (index into journal history array)
**	d_hist	- dump history number (index into dump history array)
**
** Outputs:
**	None
**
** Returns:
**	valid	- 0 if checkpoint is INVALID, non-zero if it is valid
**
** History:
**	4-may-90 (bryanp)
**	    Created.
**	20-sep-93 (jnash)
**	    Moved here from dmfrfp.c, renamed DMFJSP_VALID_CHECKPOINT.
*/
#define DMFJSP_VALID_CHECKPOINT(cnf,j_hist,d_hist) \
    ( (cnf->cnf_jnl->jnl_history[j_hist].ckp_jnl_valid == 1) &&\
     ((cnf->cnf_jnl->jnl_history[j_hist].ckp_mode != CKP_ONLINE) ||\
      (d_hist != -1 && (cnf->cnf_dmp->dmp_history[d_hist].ckp_dmp_valid == 1)))\
    )
/*
**      Error codes for journaling support program and routines.
*/
#define             E_DM1001_JSP_INIT		(E_DM_MASK + 0x1001L)
#define		    E_DM1002_JSP_MISS_ARGS	(E_DM_MASK + 0x1002L)
#define		    E_DM1003_JSP_UNRECOGNIZED	(E_DM_MASK + 0x1003L)
#define		    E_DM1004_JSP_BAD_ARGUMENT	(E_DM_MASK + 0x1004L)
#define		    E_DM1005_JSP_BAD_ARG_COMBO	(E_DM_MASK + 0x1005L)
#define		    E_DM1006_JSP_TAPE_ONE_DB	(E_DM_MASK + 0x1006L)
#define		    E_DM1007_JSP_MEM_ALLOC	(E_DM_MASK + 0x1007L)
#define		    E_DM1008_CANT_OPEN_DB	(E_DM_MASK + 0x1008L)
#define		    E_DM1009_JSP_DBDB_OPEN	(E_DM_MASK + 0x1009L)
#define		    E_DM100A_JSP_OPEN_IIUSER	(E_DM_MASK + 0x100AL)
#define		    E_DM100B_JSP_NOT_USER	(E_DM_MASK + 0x100BL)
#define		    E_DM100C_JSP_NOT_OVERUSER	(E_DM_MASK + 0x100CL)
#define		    E_DM100D_JSP_CLOSE_IIUSER	(E_DM_MASK + 0x100DL)
#define		    E_DM100E_JSP_NO_SUPERUSER	(E_DM_MASK + 0x100EL)
#define		    E_DM100F_JSP_OPEN_IIDB	(E_DM_MASK + 0x100FL)
#define		    E_DM1010_JSP_CLOSE_IIDB	(E_DM_MASK + 0x1010L)
#define		    E_DM1011_JSP_DB_NOTFOUND	(E_DM_MASK + 0x1011L)
#define		    E_DM1013_JSP_CLOSE_IILOC	(E_DM_MASK + 0x1013L)
#define		    E_DM1014_JSP_NOT_DB_OWNER	(E_DM_MASK + 0x1014L)
#define		    E_DM1015_JSP_OPEN_IILOC	(E_DM_MASK + 0x1015L)
#define		    E_DM1016_JSP_MUST_USE_TABLE	(E_DM_MASK + 0x1016L)
#define		    E_DM1017_JSP_II_DATABASE	(E_DM_MASK + 0x1017L)
#define		    E_DM1018_JSP_II_LOCATION	(E_DM_MASK + 0x1018L)
#define		    E_DM1019_JSP_NO_ATP_DB	(E_DM_MASK + 0x1019L)
/* 101A defined in dm.h */
#define		    E_DM1020_JSP_NO_DTP_DB	(E_DM_MASK + 0x1020L)
#define		    E_DM1021_JSP_SEC_LAB_ERROR	(E_DM_MASK + 0x1021L)
#define		    E_DM1022_JSP_NO_DISTRIB_DB	(E_DM_MASK + 0x1022L)
#define		    E_DM1023_JSP_NO_LOC_INFO	(E_DM_MASK + 0x1023L)
#define		    E_DM1024_JSP_BAD_LOC_INFO	(E_DM_MASK + 0x1024L)
#define 	    E_DM1025_JSP_BAD_DATE_RANGE	(E_DM_MASK + 0x1025L)
#define		    E_DM1030_JSP_SXF_INIT_ERR   (E_DM_MASK + 0x1030L)
#define		    E_DM1031_JSP_SXF_SES_INIT   (E_DM_MASK + 0x1031L)
#define		    E_DM1032_JSP_SXF_SES_TERM   (E_DM_MASK + 0x1032L)
#define		    E_DM1033_JSP_SXF_SES_FLUSH  (E_DM_MASK + 0x1033L)
#define		    E_DM1034_JSP_OPEN_SECSTATE  (E_DM_MASK + 0x1034L)
#define		    E_DM1035_JSP_CLOSE_SECSTATE (E_DM_MASK + 0x1035L)
#define		    E_DM1036_JSP_ALTER_SXF      (E_DM_MASK + 0x1036L)
#define	            E_DM1037_JSP_NO_PRIV_UNAME  (E_DM_MASK + 0x1037L)
#define		    W_DM1038_JSP_SUPU_IGNORED   (E_DM_MASK + 0x1038L)
#define		    E_DM1039_JSP_BAD_PM_FILE	(E_DM_MASK + 0x1039L)
#define		    E_DM103A_JSP_NO_PM_FILE	(E_DM_MASK + 0x103AL)
#define             E_DM103B_JSP_INVALID_ARG    (E_DM_MASK + 0x103BL)
#define             E_DM103C_JSP_INCON_ARG      (E_DM_MASK + 0x103CL)
#define             E_DM103D_JSP_JNL_SIZE_CHGD	(E_DM_MASK + 0x103DL)
#define             E_DM103E_JSP_JNL_BKSZ_CHGD	(E_DM_MASK + 0x103EL)
#define             E_DM103F_JSP_JNL_BKCNT_CHGD	(E_DM_MASK + 0x103FL)
#define             E_DM1040_JSP_JNL_SIZE_ERR	(E_DM_MASK + 0x1040L)
#define             E_DM1041_JSP_JNL_BKSZ_ERR	(E_DM_MASK + 0x1041L)
#define             E_DM1042_JSP_JNL_BKCNT_ERR	(E_DM_MASK + 0x1042L)
#define             E_DM1044_JSP_FILE_TBL_EQ	(E_DM_MASK + 0x1044L)
#define             E_DM1045_JSP_FILE_REQ_TBL	(E_DM_MASK + 0x1045L)
#define		    E_DM1046_JSP_B1_NEEDS_C2	  (E_DM_MASK + 0x1046L)
#define		    E_DM1047_JSP_B1_NO_SECLABELS  (E_DM_MASK + 0x1047L)
#define		    E_DM1048_JSP_BAD_SECURE_LEVEL (E_DM_MASK + 0x1048L)
#define		    E_DM1049_JSP_SXF_MAC_ERR     (E_DM_MASK  + 0x1049L)
#define             E_DM104A_JSP_DELIM_SETUP_ERR (E_DM_MASK + 0x104AL)
#define             E_DM104B_JSP_ALTDB_ONE_DB	(E_DM_MASK + 0x104BL)
#define             E_DM104C_JSP_ATP_ONE_DB	(E_DM_MASK + 0x104CL)
#define             E_DM104D_JSP_RFP_ONE_DB	(E_DM_MASK + 0x104DL)
#define             E_DM104E_JSP_NORMALIZE_ERR	(E_DM_MASK + 0x104EL)
#define             E_DM104F_JSP_NAME_LEN	(E_DM_MASK + 0x104FL)
#define             E_DM1050_JSP_CPP_ONE_DB	(E_DM_MASK + 0x1050L)
#define             E_DM1051_JSP_NO_INSTALL     (E_DM_MASK + 0x1051L)
#define             E_DM1052_JSP_VALID_USAGE    (E_DM_MASK + 0x1052L)
#define             E_DM1053_JSP_INV_START_LSN	(E_DM_MASK + 0x1053L)
#define             E_DM1054_JSP_SLSN_GT_ELSN	(E_DM_MASK + 0x1054L)
#define             E_DM1055_JSP_SXF_DMF_ERR    (E_DM_MASK + 0x1055L)
#define             E_DM1056_JSP_RELOC_ONE_DB   (E_DM_MASK + 0x1056L)
#define             E_DM1057_JSP_FLOAD_ONE_DB   (E_DM_MASK + 0x1057L)
#define		    E_DM1058_JSP_TBL_LST_ERR    (E_DM_MASK + 0x1058L)
#define		    E_DM1059_JSP_INFODB_ERR     (E_DM_MASK + 0x1059L)
#define		    E_DM105A_JSP_INC_ARG        (E_DM_MASK + 0x105AL)
#define		    E_DM105B_JSP_NUM_LOCATIONS  (E_DM_MASK + 0x105BL)
#define             E_DM105C_JSP_INV_ARG_VAL    (E_DM_MASK + 0x105CL)
#define		    E_DM105D_JSP_INFO		(E_DM_MASK + 0x105DL)

#define		    E_DM1100_CPP_BAD_LOCATION	(E_DM_MASK + 0x1100L)
#define		    E_DM1101_CPP_WRITE_ERROR	(E_DM_MASK + 0x1101L)
#define		    E_DM1102_CPP_FAILED		(E_DM_MASK + 0x1102L)
#define		    E_DM1103_CPP_CONFIG_UPDATE	(E_DM_MASK + 0x1103L)
#define		    E_DM1104_CPP_CKP_ERROR	(E_DM_MASK + 0x1104L)
#define		    E_DM1105_CPP_CNF_OPEN	(E_DM_MASK + 0x1105L)
#define		    E_DM1106_CPP_JNL_CREATE	(E_DM_MASK + 0x1106L)
#define		    E_DM1107_CPP_CATALOG_ERROR	(E_DM_MASK + 0x1107L)
#define		    E_DM1108_CPP_OPEN_DB	(E_DM_MASK + 0x1108L)
#define		    E_DM1109_CPP_MEM_ALLOC	(E_DM_MASK + 0x1109L)
#define		    E_DM110A_CPP_CLOSE_DB	(E_DM_MASK + 0x110AL)
#define		    E_DM110B_CPP_FAILED		(E_DM_MASK + 0x110BL)
#define		    E_DM110C_CPP_CNF_CLOSE	(E_DM_MASK + 0x110CL)
#define		    E_DM110D_CPP_DB_BUSY	(E_DM_MASK + 0x110DL)
#define		    E_DM110E_CPP_FAIL_NO_JNL	(E_DM_MASK + 0x110EL)
#define		    E_DM110F_CPP_FAIL_NO_CHANGE	(E_DM_MASK + 0x110FL)
#define		    E_DM1110_CPP_DELETE_JNL	(E_DM_MASK + 0x1110L)
#define		    E_DM1111_CPP_DELETE_CKP	(E_DM_MASK + 0x1111L)
#define		    E_DM1112_CPP_JNLDMPCKP_NODEL (E_DM_MASK + 0x1112L)
#define		    E_DM1113_CPP_NOARCHIVER	(E_DM_MASK + 0x1113L)
#define		    E_DM1114_CPP_LOCATION	(E_DM_MASK + 0x1114L)
#define		    E_DM1115_CPP_II_CHECKPOINT	(E_DM_MASK + 0x1115L)
#define		    E_DM1116_CPP_II_JOURNAL	(E_DM_MASK + 0x1116L)
#define		    E_DM1117_CPP_II_DATABASE	(E_DM_MASK + 0x1117L)
#define		    E_DM1118_CPP_JNL_OPEN	(E_DM_MASK + 0x1118L)
#define		    E_DM1119_CPP_JNL_TRUNCATE	(E_DM_MASK + 0x1119L)
#define		    E_DM1120_CPP_JNL_CLOSE	(E_DM_MASK + 0x1120L)
#define		    E_DM1121_CPP_BEGIN_ERROR	(E_DM_MASK + 0x1121L)
#define		    E_DM1122_CPP_END_ERROR	(E_DM_MASK + 0x1122L)
#define		    E_DM1123_CPP_NOCLUSTER	(E_DM_MASK + 0x1123L)
#define		    E_DM1124_CPP_SBACKUP	(E_DM_MASK + 0x1124L)
#define		    E_DM1125_CPP_LGBGN_ERROR	(E_DM_MASK + 0x1125L)
#define		    E_DM1126_CPP_DM0LSBACKUP	(E_DM_MASK + 0x1126L)
#define		    E_DM1127_CPP_LGEND_ERROR	(E_DM_MASK + 0x1127L)
#define		    E_DM1128_CPP_RESUME		(E_DM_MASK + 0x1128L)
#define		    E_DM1129_CPP_DM0LEBACKUP	(E_DM_MASK + 0x1129L)
#define		    E_DM1130_CPP_EBACKUP	(E_DM_MASK + 0x1130L)
#define		    E_DM1131_CPP_LK_FAIL	(E_DM_MASK + 0x1131L)
#define		    E_DM1132_CPP_FAIL_NO_DMP	(E_DM_MASK + 0x1132L)
#define		    E_DM1133_CPP_II_DUMP	(E_DM_MASK + 0x1133L)
#define		    E_DM1134_CPP_DELETE_DMP	(E_DM_MASK + 0x1134L)
#define		    E_DM1135_CPP_UPD2_JNLDMP	(E_DM_MASK + 0x1135L)
#define		    E_DM1136_CPP_PROT_E_SBACKUP	(E_DM_MASK + 0x1136L)
#define		    E_DM1137_CPP_FBACKUP	(E_DM_MASK + 0x1137L)
#define		    E_DM1138_CPP_PROT_E_FBACKUP	(E_DM_MASK + 0x1138L)
#define		    E_DM1139_CPP_DBACKUP	(E_DM_MASK + 0x1139L)
#define		    E_DM1140_CPP_UPD1_JNLDMP	(E_DM_MASK + 0x1140L)
#define		    E_DM1141_CPP_DMP_OPEN	(E_DM_MASK + 0x1141L)
#define		    E_DM1142_CPP_DMP_TRUNCATE	(E_DM_MASK + 0x1142L)
#define		    E_DM1143_CPP_DMP_CLOSE	(E_DM_MASK + 0x1143L)
#define		    E_DM1144_CPP_DEAD_ARCHIVER	(E_DM_MASK + 0x1144L)
#define		    E_DM1145_CPP_DEAD_ARCHIVER	(E_DM_MASK + 0x1145L)
#define		    E_DM1146_CPP_DEAD_ARCHIVER	(E_DM_MASK + 0x1146L)
#define		    E_DM1147_CPP_DEAD_ARCHIVER	(E_DM_MASK + 0x1147L)
#define		    E_DM1148_CPP_CNF_DUMP	(E_DM_MASK + 0x1148L)
#define		    E_DM1149_CPP_BACKUP_FAILURE	(E_DM_MASK + 0x1149L)
#define		    E_DM1150_CPP_BACKUP_FAILURE	(E_DM_MASK + 0x1150L)
#define		    E_DM1151_CPP_CPNEEDED	(E_DM_MASK + 0x1151L)
#define		    E_DM1152_CPP_ONLINE_PURGE	(E_DM_MASK + 0x1152L)
#define		    E_DM1153_CPP_CLUSTER_MERGE	(E_DM_MASK + 0x1153L)
#define		    E_DM1154_CPP_CLUSTER_BUSY	(E_DM_MASK + 0x1154L)
#define		    E_DM1155_CPP_TMPL_MISSING	(E_DM_MASK + 0x1155L)
#define		    E_DM1156_CPP_NOLOGGING	(E_DM_MASK + 0x1156L)
#define		    E_DM1157_CPP_DISABLE_JNL	(E_DM_MASK + 0x1157L)
#define		    E_DM1158_CPP_BEGIN_ERROR	(E_DM_MASK + 0x1158L)
#define		    E_DM1159_CPP_END_ERROR	(E_DM_MASK + 0x1159L)
#define		    E_DM1160_CPP_SAVE   	(E_DM_MASK + 0x1160L)
#define             E_DM1161_CPP_NO_CORE_TBL_CKPT (E_DM_MASK + 0x1161L)
#define             E_DM1162_CPP_USR_INVALID_TBL  (E_DM_MASK + 0x1162L)
#define             E_DM1163_CPP_TMPL_ERROR       (E_DM_MASK+0x1163L)
#define             E_DM1164_CPP_TMPL_CMD_MISSING (E_DM_MASK+0x1164L)
#define		    E_DM1165_CPP_CHKPT_LST_CREATE (E_DM_MASK+0x1165L)
#define		    E_DM1166_CPP_CHKPT_LST_OPEN   (E_DM_MASK+0x1166L)
#define		    E_DM1167_CPP_CHKPT_LST_ALLOC  (E_DM_MASK+0x1167L)
#define		    E_DM1168_CPP_CHKPT_LST_WRITE  (E_DM_MASK+0x1168L)
#define		    E_DM1169_CPP_CHKPT_LST_CLOSE  (E_DM_MASK+0x1169L)
#define		    E_DM116A_CPP_CHKPT_LST_ERROR  (E_DM_MASK+0x116AL)
#define		    E_DM116B_CPP_CHKPT_DEL_ERROR  (E_DM_MASK+0x116BL)
#define		    E_DM116C_CPP_NO_ONLINE_CKP    (E_DM_MASK+0x116CL)
#define		    I_DM116D_CPP_NOT_DEF_TEMPLATE (E_DM_MASK+0x116DL)
#define		    E_DM116E_CPP_TBL_CKP_DISALLOWED (E_DM_MASK+0x116EL)
#define		    E_DM116F_CPP_DRN_PURGE	  (E_DM_MASK + 0x116FL)
#define		    E_DM1170_NO_RECOVERY_SERVER	  (E_DM_MASK + 0x1170L)

#define		    E_DM1200_ATP_DB_NOT_JNL	(E_DM_MASK + 0x1200L)
#define		    E_DM1201_ATP_WRITING_AUDIT  (E_DM_MASK + 0x1201L)
#define		    E_DM1202_ATP_JNL_OPEN	(E_DM_MASK + 0x1202L)
#define		    E_DM1203_ATP_JNL_READ	(E_DM_MASK + 0x1203L)
#define		    E_DM1204_ATP_JNL_CLOSE	(E_DM_MASK + 0x1204L)
#define		    E_DM1205_ATP_NO_BT_FOUND	(E_DM_MASK + 0x1205L)
#define		    E_DM1206_ATP_TOO_MANY_TX	(E_DM_MASK + 0x1206L)
#define		    E_DM1207_ATP_NON_DB		(E_DM_MASK + 0x1207L)
#define		    E_DM1208_ATP_OPEN_DB	(E_DM_MASK + 0x1208L)
#define		    E_DM1209_ATP_CNF_OPEN	(E_DM_MASK + 0x1209L)
#define		    E_DM120A_ATP_CNF_CLOSE	(E_DM_MASK + 0x120AL)
#define		    E_DM120B_ATP_CATALOG_ERROR	(E_DM_MASK + 0x120BL)
#define		    E_DM120C_ATP_ADF_ERROR	(E_DM_MASK + 0x120CL)
#define		    E_DM120D_ATP_DESCRIBE	(E_DM_MASK + 0x120DL)
#define		    E_DM120E_ATP_DB_BUSY	(E_DM_MASK + 0x120EL)
#define		    E_DM120F_ATP_TBL_NOTFOUND	(E_DM_MASK + 0x120FL)
#define		    E_DM1210_ATP_BAD_TRL_CREATE	(E_DM_MASK + 0x1210L)
#define		    E_DM1211_ATP_MEM_ALLOCATE	(E_DM_MASK + 0x1211L)
#define		    E_DM1212_ATP_DISPLAY	(E_DM_MASK + 0x1212L)
#define		    E_DM1213_ATP_COLUMN_DISPLAY	(E_DM_MASK + 0x1213L)
#define		    E_DM1214_ATP_DISPLAY_OVFL	(E_DM_MASK + 0x1214L)
#define		    E_DM1215_ATP_TBL_NONEXIST	(E_DM_MASK + 0x1215L)
#define		    E_DM1216_ATP_WAIT		(E_DM_MASK + 0x1216L)
#define             E_DM1217_ATP_DLM_FNAME 	(E_DM_MASK + 0x1217L)
#define             E_DM1218_ATP_OROW_COMPRESS 	(E_DM_MASK + 0x1218L)
#define             E_DM1219_ATP_INV_CKP 	(E_DM_MASK + 0x1219L)
#define             E_DM121A_ATP_LSN_NO_ALL	(E_DM_MASK + 0x121AL)
#define             E_DM121B_ATP_NOEMPTY_TUPLE  (E_DM_MASK + 0x121BL)

#define		    E_DM1300_RFP_OPEN_DB	(E_DM_MASK + 0x1300L)
#define		    E_DM1301_RFP_ROLL_FORWARD	(E_DM_MASK + 0x1301L)
#define		    E_DM1302_RFP_CNF_OPEN	(E_DM_MASK + 0x1302L)
#define		    E_DM1303_RFP_CNF_CLOSE	(E_DM_MASK + 0x1303L)
#define		    E_DM1304_RFP_MEM_ALLOC	(E_DM_MASK + 0x1304L)
#define		    E_DM1305_RFP_JNL_OPEN	(E_DM_MASK + 0x1305L)
#define		    E_DM1306_RFP_APPLY_RECORD	(E_DM_MASK + 0x1306L)
#define		    E_DM1307_RFP_RESTORE	(E_DM_MASK + 0x1307L)
#define		    E_DM1308_RFP_BAD_RECORD	(E_DM_MASK + 0x1308L)
#define		    E_DM1309_RFP_RFC_DELETE	(E_DM_MASK + 0x1309L)
#define		    E_DM130A_RFP_NO_CNFRFC	(E_DM_MASK + 0x130AL)
#define		    E_DM130B_RFP_RENAME_CNF	(E_DM_MASK + 0x130BL)
#define		    E_DM130C_RFP_DELETE_ALL	(E_DM_MASK + 0x130CL)
#define		    E_DM130D_RFP_BAD_RESTORE	(E_DM_MASK + 0x130DL)
#define		    E_DM130E_RFP_ALIAS_DELETE	(E_DM_MASK + 0x130EL)
#define		    E_DM130F_RFP_RESTORED	(E_DM_MASK + 0x130FL)
#define		    E_DM1310_RFP_CLOSE_DB	(E_DM_MASK + 0x1310L)
#define		    E_DM1311_RFP_MUST_RESTORE	(E_DM_MASK + 0x1311L)
#define		    E_DM1312_RFP_JNL_CREATE	(E_DM_MASK + 0x1312L)
#define		    E_DM1313_RFP_CKP_HISTORY	(E_DM_MASK + 0x1313L)
#define		    E_DM1314_RFP_DRAIN_ERROR	(E_DM_MASK + 0x1314L)
#define		    E_DM1315_RFP_DB_BUSY	(E_DM_MASK + 0x1315L)
#define		    E_DM1316_RFP_NO_CHECKPOINT	(E_DM_MASK + 0x1316L)
#define		    E_DM1317_RFP_CKP_MISSING    (E_DM_MASK + 0x1317L)
#define		    E_DM1318_RFP_NO_CKP_FILE	(E_DM_MASK + 0x1318L)
#define		    E_DM1319_RFP_NO_BT_FOUND	(E_DM_MASK + 0x1319L)
#define		    E_DM1320_RFP_TOO_MANY_TX	(E_DM_MASK + 0x1320L)
#define		    E_DM1321_RFP_BEGIN_ERROR	(E_DM_MASK + 0x1321L)
#define		    E_DM1322_RFP_END_ERROR	(E_DM_MASK + 0x1322L)
#define		    E_DM1323_RFP_BAD_RAWDATA	(E_DM_MASK + 0x1323L)
#define		    E_DM1324_RFP_DMP_OPEN	(E_DM_MASK + 0x1324L)
#define		    E_DM1325_RFP_APPLY_DRECORD	(E_DM_MASK + 0x1325L)
#define		    E_DM1326_RFP_BAD_DRECORD	(E_DM_MASK + 0x1326L)
#define		    E_DM1327_RFP_INV_CHECKPOINT	(E_DM_MASK + 0x1327L)
#define		    E_DM1328_DTP_DB_NOT_DMP	(E_DM_MASK + 0x1328L)
#define		    E_DM1329_DTP_WRITING_AUDIT  (E_DM_MASK + 0x1329L)
#define		    E_DM132A_DTP_DMP_OPEN	(E_DM_MASK + 0x132AL)
#define		    E_DM132B_DTP_DMP_READ	(E_DM_MASK + 0x132BL)
#define		    E_DM132C_DTP_DMP_CLOSE	(E_DM_MASK + 0x132CL)
#define		    E_DM132D_DTP_NO_BT_FOUND	(E_DM_MASK + 0x132DL)
#define		    E_DM132E_DTP_TOO_MANY_TX	(E_DM_MASK + 0x132EL)
#define		    E_DM132F_DTP_NON_DB		(E_DM_MASK + 0x132FL)
#define		    E_DM1330_DTP_OPEN_DB	(E_DM_MASK + 0x1330L)
#define		    E_DM1331_DTP_CNF_OPEN	(E_DM_MASK + 0x1331L)
#define		    E_DM1332_DTP_CNF_CLOSE	(E_DM_MASK + 0x1332L)
#define		    E_DM1333_DTP_CATALOG_ERROR	(E_DM_MASK + 0x1333L)
#define		    E_DM1334_DTP_ADF_ERROR	(E_DM_MASK + 0x1334L)
#define		    E_DM1335_DTP_DESCRIBE	(E_DM_MASK + 0x1335L)
#define		    E_DM1336_DTP_DB_BUSY	(E_DM_MASK + 0x1336L)
#define		    E_DM1337_DTP_TBL_NOTFOUND	(E_DM_MASK + 0x1337L)
#define		    E_DM1338_DTP_BAD_TRL_CREATE	(E_DM_MASK + 0x1338L)
#define		    E_DM1339_RFP_DMP_READ	(E_DM_MASK + 0x1339L)
#define		    E_DM133A_RFP_DMP_SCAN	(E_DM_MASK + 0x133AL)
#define		    E_DM133B_RFP_DUMP_CONFIG	(E_DM_MASK + 0x133BL)
#define		    E_DM133C_RFP_UNUSABLE_CKP	(E_DM_MASK + 0x133CL)
#define		    E_DM133D_RFP_UNDO_INCOMPLETE (E_DM_MASK + 0x133DL)
#define		    E_DM133E_RFP_ROLLBACK_ERROR	(E_DM_MASK + 0x133EL)
#define		    E_DM133F_RFP_UNDO_BEGIN_XACT (E_DM_MASK + 0x133FL)
#define		    E_DM1340_RFP_PREPARE_UNDO	(E_DM_MASK + 0x1340L)
#define		    E_DM1341_RFP_CLEANUP_UNDO	(E_DM_MASK + 0x1341L)
#define		    E_DM1342_RFP_INCOMPLETE_UNDO (E_DM_MASK + 0x1342L)
#define		    E_DM1343_RFP_JNL_UNDO	(E_DM_MASK + 0x1343L)
#define		    E_DM1344_RFP_APPLY_UNDO	(E_DM_MASK + 0x1344L)
#define		    E_DM1345_RFP_XACT_UNDO_FAIL	(E_DM_MASK + 0x1345L)
#define             E_DM1346_RFP_NOLOGGING	(E_DM_MASK + 0x1346L)
#define             E_DM1347_RFP_NOROLLBACK	(E_DM_MASK + 0x1347L)
#define             E_DM1348_RFP_CHECK_NOCKP	(E_DM_MASK + 0x1348L)
#define             E_DM1349_RFP_END_TIME	(E_DM_MASK + 0x1349L)
#define             E_DM134A_RFP_SPURIOUS_FILE	(E_DM_MASK + 0x134AL)
#define             E_DM134B_RFP_ACTION_LIST	(E_DM_MASK + 0x134BL)
#define             E_DM134C_RFP_INV_CHECKPOINT	(E_DM_MASK + 0x134CL)
#define             W_DM134D_RFP_NONJNL_TAB_DEL (E_DM_MASK + 0x134DL)
#define             E_DM134E_RFP_LAST_TABID 	(E_DM_MASK + 0x134EL)
#define             E_DM134F_RFP_INV_JNL 	(E_DM_MASK + 0x134FL)
#define             E_DM1350_RFP_INV_START_LSN 	(E_DM_MASK + 0x1350L)
#define             E_DM1351_RFP_BTIME_NOFORCE 	(E_DM_MASK + 0x1351L)
#define             E_DM1352_RFP_NOCKP_NOFORCE 	(E_DM_MASK + 0x1352L)
#define             E_DM1353_RFP_SLSN_NOFORCE 	(E_DM_MASK + 0x1353L)

#define		    E_DM1354_RFP_INV_ARG_FOR_DB_RECOVERY (E_DM_MASK+0x1354L)
#define		    E_DM1355_RFP_NO_CORE_TBL_RECOVERY    (E_DM_MASK+0x1355L)
#define		    E_DM1356_RFP_INVALID_ARGUMENT	 (E_DM_MASK+0x1356L)
#define		    E_DM1357_RFP_INCONSIST_DB_NO_TBLRECV (E_DM_MASK+0x1357L)
#define		    E_DM1358_RFP_VIEW_TBL_NO_RECV	 (E_DM_MASK+0x1358L)
#define		    E_DM1359_RFP_GATEWAY_TBL_NO_RECV     (E_DM_MASK+0x1359L)
#define		    E_DM135A_RFP_RECOVERING_MAX_TBLS	 (E_DM_MASK+0x135AL)
#define		    E_DM135B_RFP_DUPLICATE_TBL_SPECIFIED (E_DM_MASK+0x135BL)
#define		    E_DM135C_RFP_CANNOT_OPEN_IIRELATION  (E_DM_MASK+0x135CL)
#define		    E_DM135D_RFP_CANNOT_OPEN_IIRELIDX    (E_DM_MASK+0x135DL)
#define             E_DM135E_RFP_USR_INVALID_TABLE       (E_DM_MASK+0x135EL)
#define             E_DM135F_RFP_NO_TBL_RECOV            (E_DM_MASK+0x135FL)
#define             E_DM1360_RFP_TBL_NOTFOUND            (E_DM_MASK+0x1360L)
#define             E_DM1361_RFP_NO_DB_RECOV_TBL_CKPT    (E_DM_MASK+0x1361L)
#define             E_DM1362_RFP_TMPL_ERROR              (E_DM_MASK+0x1362L)
#define             E_DM1363_RFP_TMPL_CMD_MISSING        (E_DM_MASK+0x1363L)
#define		    E_DM1364_RFP_NO_TBL_CHKPT		 (E_DM_MASK+0x1364L)
#define		    E_DM1365_RFP_CHKPT_LST_OPEN		 (E_DM_MASK+0x1365L)
#define		    E_DM1366_RFP_CHKPT_LST_READ		 (E_DM_MASK+0x1366L)
#define		    E_DM1367_RFP_CHKPT_LST_CLOSE	 (E_DM_MASK+0x1367L)
#define		    E_DM1368_RFP_CHKPT_LST_ERROR	 (E_DM_MASK+0x1368L)
#define		    I_DM1369_RFP_NOT_DEF_TEMPLATE        (E_DM_MASK+0x1369L)
#define		    E_DM1370_RFP_NO_SEC_INDEX		 (E_DM_MASK+0x1370L)
#define		    E_DM1371_RFP_INFO			 (E_DM_MASK+0x1371L)
#define		    E_DM1372_RFP_JNL_WARNING		 (E_DM_MASK+0x1372L)
#define             E_DM1373_RFP_INCR_ARG                (E_DM_MASK+0x1373L)
#define             E_DM1374_RFP_INCR_TXN_SAVE           (E_DM_MASK+0x1374L)
#define             E_DM1375_RFP_INCR_TXN_RESTORE        (E_DM_MASK+0x1375L)
#define             E_DM1376_RFP_INCREMENTAL_FAIL        (E_DM_MASK+0x1376L)
#define		    E_DM1377_RFP_INCR_JNL_WARNING	 (E_DM_MASK+0x1377L)
#define		    E_DM1378_RFP_INCR_LAST_LSN		 (E_DM_MASK+0x1378L)
/*
** Alterdb errors.
*/
#define		    E_DM1400_ALT_ERROR_ALTERING_DB	(E_DM_MASK + 0x1400L)
#define		    E_DM1401_ALT_JOURNAL_DISABLED	(E_DM_MASK + 0x1401L)
#define		    E_DM1402_ALT_DB_NOT_JOURNALED	(E_DM_MASK + 0x1402L)
#define		    E_DM1403_ALT_JNL_NOT_ENABLED	(E_DM_MASK + 0x1403L)
#define		    E_DM1404_ALT_JNL_DISABLE_ERR	(E_DM_MASK + 0x1404L)
#define		    E_DM1405_ALT_CKP_DELETED		(E_DM_MASK + 0x1405L)
#define             E_DM1406_ALT_INV_UCOLLATION         (E_DM_MASK + 0x1406L)
#define             E_DM1407_ALT_CON_UNICODE            (E_DM_MASK + 0x1407L)
#define             E_DM1408_ALT_ALREADY_UNICODE        (E_DM_MASK + 0x1408L)
#define		    E_DM1409_JNLSWITCH			(E_DM_MASK + 0x1409L)
#define             E_DM1410_ALT_UCOL_NOTEXIST		(E_DM_MASK + 0x1410L)
#define             E_DM1411_ALT_MVCC_NOT_ENABLED	(E_DM_MASK + 0x1411L)
#define             E_DM1412_ALT_MVCC_NOT_DISABLED	(E_DM_MASK + 0x1412L)
#define             E_DM1413_ALT_MVCC_DISABLED		(E_DM_MASK + 0x1413L)
#define             E_DM1414_ALT_MVCC_ENABLED		(E_DM_MASK + 0x1414L)
/*
** Relocatedb  (and some RFP relocate) errors.
*/
#define             E_DM1500_RELOC_OLDLOC_EQ_NEW        (E_DM_MASK + 0x1500L)
#define             E_DM1501_RELOC_CP_FI_ERR            (E_DM_MASK + 0x1501L)
#define             E_DM1502_RELOC_DEL_FI_ERR           (E_DM_MASK + 0x1502L)
#define             E_DM1503_RELOC_NEWLOC_NOTFOUND      (E_DM_MASK + 0x1503L)
#define             E_DM1504_RELOC_UPDT_IIDB_ERR        (E_DM_MASK + 0x1504L)
#define             E_DM1505_RELOC_DISABLE_JNL          (E_DM_MASK + 0x1505L)
#define             E_DM1506_RELOC_INVL_USAGE           (E_DM_MASK + 0x1506L)
#define             E_DM1507_RELOC_ERR                  (E_DM_MASK + 0x1507L)
#define             E_DM1508_RELOC_OLDDB_EQ_NEW         (E_DM_MASK + 0x1508L)
#define             E_DM1509_RELOC_DEL_NEWFI_ERR        (E_DM_MASK + 0x1509L)
#define             E_DM150A_RELOC_MEM_ALLOC            (E_DM_MASK + 0x150AL)
#define             E_DM150B_RELOC_CP_FI_ERR            (E_DM_MASK + 0x150BL)
#define             E_DM150C_RELOC_LOC_CONFIG_ERR       (E_DM_MASK + 0x150CL)
#define             E_DM150D_RELOC_IIEXTEND_ERR         (E_DM_MASK + 0x150DL)
#define             E_DM150E_RELOC_IILOCATIONS_ERR      (E_DM_MASK + 0x150EL)
#define             E_DM150F_RELOC_CNF_OPEN             (E_DM_MASK + 0x150FL)
#define             E_DM1510_RELOC_CNF_CLOSE            (E_DM_MASK + 0x1510L)
#define             E_DM1511_RELOC_NO_CORE_GATE_VIEW    (E_DM_MASK + 0x1511L)
#define             E_DM1512_RELOC_CANT_OPEN_NEWDB      (E_DM_MASK + 0x1512L)
#define             E_DM1513_RELOC_CANT_UPDT_IIDEVICES  (E_DM_MASK + 0x1513L)
#define             E_DM1514_RELOC_CANT_UPDT_IIRELATION (E_DM_MASK + 0x1514L)
#define             E_DM1515_RELOC_USAGE_ERR            (E_DM_MASK + 0x1515L)
#define             E_DM1516_RELOC_DB_LOC_ERR           (E_DM_MASK + 0x1516L)
#define             E_DM1517_RELOC_TBL_LOC_ERR          (E_DM_MASK + 0x1517L)
#define             E_DM1518_RELOC_IIDBDB_ERR           (E_DM_MASK + 0x1518L)

/*
** production mode errors
*/
#define             E_DM1519_PMODE_IIDBDB_ERR           (E_DM_MASK + 0x1519L)
#define             E_DM1520_PMODE_DB_BUSY              (E_DM_MASK + 0x1520L)

/*
** fastload errors
*/
#define             E_DM1600_FLOAD_OPEN_FILE            (E_DM_MASK + 0x1600L)
#define             E_DM1601_FLOAD_OPEN_DB              (E_DM_MASK + 0x1601L)
#define             E_DM1602_FLOAD_START_XACT           (E_DM_MASK + 0x1602L)
#define             E_DM1603_FLOAD_SHOW                 (E_DM_MASK + 0x1603L)
#define             E_DM1604_FLOAD_OPEN_TABLE           (E_DM_MASK + 0x1604L)
#define             E_DM1605_FLOAD_CLOSE_TABLE          (E_DM_MASK + 0x1605L)
#define             E_DM1606_FLOAD_COMMIT_XACT          (E_DM_MASK + 0x1606L)
#define             E_DM1607_FLOAD_CLOSE_DB             (E_DM_MASK + 0x1607L)
#define             E_DM1608_FLOAD_CLOSE_FILE           (E_DM_MASK + 0x1608L)
#define             E_DM1609_FLOAD_ABORT_XACT           (E_DM_MASK + 0x1609L)
#define             E_DM160A_FLOAD_FAILED               (E_DM_MASK + 0x160AL)
#define             E_DM160B_FLOAD_NO_TCB               (E_DM_MASK + 0x160BL)
#define             E_DM160C_FLOAD_NO_TABLE             (E_DM_MASK + 0x160CL)
#define             E_DM160D_FLOAD_NOMEM                (E_DM_MASK + 0x160DL)
#define             E_DM160E_FLOAD_INIT_LOAD            (E_DM_MASK + 0x160EL)
#define             E_DM160F_FLOAD_LOAD_ERR             (E_DM_MASK + 0x160FL)
#define             E_DM1610_FLOAD_END_LOAD             (E_DM_MASK + 0x1610L)
#define             E_DM1611_FLOAD_UNFIX_TCB            (E_DM_MASK + 0x1611L)
#define             E_DM1612_FLOAD_BADARGS              (E_DM_MASK + 0x1612L)
#define             E_DM1613_FLOAD_READ_FILE            (E_DM_MASK + 0x1613L)
#define		    E_DM1615_FLOAD_BAD_TABLE_FORMAT	(E_DM_MASK + 0x1615L)
#define		    E_DM1616_FLOAD_TABLE_NOT_EMPTY	(E_DM_MASK + 0x1616L)
#define		    E_DM1617_FLOAD_TABLE_ERROR		(E_DM_MASK + 0x1617L)
#define		    E_DM1618_FLOAD_INIT_SCF		(E_DM_MASK + 0x1618L)
#define		    E_DM1619_FLOAD_BAD_READ		(E_DM_MASK + 0x1619L)
#define		    E_DM161A_FLOAD_COUPONIFY_ERROR	(E_DM_MASK + 0x161AL)
#define		    E_DM161B_FLOAD_DB_BUSY		(E_DM_MASK + 0x161BL)

typedef struct _JSX_LOC
    {                                           /* ROLLDB/RELOCATE         */
	DB_LOC_NAME             loc_name;       /* -location = name        */
	DMP_LOC_ENTRY          *loc_ext;        /* extent information      */
	bool                    loc_init;       /* TRUE if initialized     */
    } JSX_LOC;

/*}
** Name: DMF_JSX - DMF journaling support context.
**
** Description:
**      This structure contains information about the operations to be performed
**	by the journaling support program.
**
** History:
**      02-nov-1986 (Derek)
**          Created.
**	24-jan-1989 (EdHsu)
**	    Modified structure to support online backup.
**	 1-dec-1989 (rogerk)
**	    Added JSX_CKP_STALL flag.  This is used to facilitate online
**	    checkpoint testing by causing checkpoint to stall in the middle
**	    of the backup phase.
**	21-may-1990 (bryanp)
**	    Added JSX_CKP_SELECTED flag. This is used to record that the user
**	    requested a specific checkpoint. Also added the JSX_LAST_VALID_CKP
**	    flag. This flag is used to indicate that the
**	    user wishes to use the last valid checkpoint. The two flags are
**	    mutually exclusive. Finally, added jsx_ckp_number to record the
**	    specific checkpoint history number which the user selected.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    25-feb-1991 (rogerk)
**		Added alterdb operation - JSX_O_ALTER.
**		Added new jsp flags JSX_JDISABLE, JSX_NOLOGGING.
**		These were added as part of the Archiver Stability project.
**	05-feb-93 (jnash)
**	    Reduced logging project.  Add JSX_AUDIT_ALL.
**	21-apr-93 (jnash)
**	    Add support for auditing multiple tables and new ALTERDB options.
**	    Add JSX_TBL_LIST, JSX_FNAME_LIST, JSX_JNL_BLK_SIZE,
**	    JSX_JNL_NUM_BLOCKS, JSX_JNL_INIT_BLOCKS, jsx_status1,
**	    jsx_tbl_list, jsx_fname_cnt jsx_fname_list, jsx_max_jnl_blocks,
**	    jsx_jnl_block_size, jsx_init_jnl_blocks.  Remove unused
**	    jsx_audit_file and jsx_t_name.
**	    Also add JSX_NORFP_LOGGING to support rollforwarddb -nologging.
**	26-jul-93 (jnash)
**	    Add JSX_ATP_INCONS, JSX_ATP_WAIT, JSX_HELP.
**	20-sep-1993 (jnash)
**	    Delim ids
**	     - Add jsx_reg_case, jsx_delim_case, jsx_real_user_case,
**	       jsx_username_delim, jsx_aname_delim.
**	     - Make jsx_tbl_list into structure.
**	    ALTERDB -delete_oldest_ckp
**	     - Replace JSX_AUDIT_VERBOSE (not needed) with
**	       JSX_ALTDB_CKP_DELETE.
**	31-jan-1994 (jnash)
**	    Add JSX_FORCE.
**	21-feb-1994 (jnash)
**	    Add jsx_start_lsn, jsx_end_lsn, JSX_START_LSN, JSX_END_LSN.
**      14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**              Table Level Rollforwarddb:
**                  Added jsx_status1 for table level processing on
**                      -nosecondary_index
**                      -on_error_continue
**                      -online
**                      -status_count=
**                      -noblobs
**                      -force_logical_consistent
**                      -statistics
**                      -Default behaviour for rollforwarddb at db level
**	06-jan-1995 (shero03)
**	    Added Partial Checkpoint Support:
**		Added jsx_status1 for table level checkpoint
**	29-jun-1995 (thaju02)
**	    Added jsx_stall_time field and definition for JSX1_STALL_TIME
**	    for "ckpdb -max_stall_time=..." argument. 
**      20-july-1995 (thaju02)
**          bug #69981 - added jsx_ignored_errors flag.
**          jsx_ignored_errors = TRUE when -ignore argument specified
**          and errors have occurred during application of journal records.
**	11-sep-1995 (thaju02)
**	    Added JSX1_OUT_FILE.
**      13-sep-1995 (tchdi01)
**          Added a jsx_status2 and JSX2_PMODE for the production mode
**          project
** 	14-apr-1997 (wonst02)
** 	    Added JSX_READ_ONLY and JSX_READ_WRITE for readonly databases.
**	23-jun-1997 (shero03)
**	    Added JSX2_SWITCH_JOURNAL for journal switch project.
**	13-Nov-1997 (jenjo02)
**	    Added JSX_CKP_STALL_RW flag for online checkpoint.
**	15-Mar-1999 (jenjo02)
**	    Removed much-maligned "-s" option, JSX_CKP_STALL_RW define.
**      18-jan-2000 (gupsh01)
**          Added flags JSX2_INTERNAL_DATA and JX2_ABORT_TRANS for auditdb
**          switches -internal_data and -aborted_transactions.
**      26-jan-2000 (gupsh01)
**          Added flags JSX2_NO_HEADER and JX2_LIMIT_OUTPUT for auditdb
**          switches -no_header and -limit_output.
**      28-feb-2000 (gupsh01)
**          Added support for table names with format owner_name.table_name.
**          modified the DMF_JSX structure. Added tbl_owner field 
**	    JSX_ATP_TBL structure.                 
**	27-mar-2001 (somsa01)
**	    Added JSX2_EXPAND_LOBJS. This flag, when set, tells us that we
**	    need to not print out information on extension tables but
**	    expand a large object coupon into the actual blob.
**      13 -Apr-2004(nansa02)
**          Added  JSX2_ALTDB_CKP_INVALID flag for deletion of invalid checkpoints
**          and JSX2_ALTDB_UNICODE flag for converting non-Unicode enabled
**          database to Unicode enabled.             
**	21-feb-05 (inkdo01)
**	    Changed UNICODE to UNICODED (for NFD) and added JSX2_ALTDB_UNICODEC
**	    for NFC.
**	19-oct-06 (kibro01) bug 116436
**	    Change jsp_file_maint parameter to be sequence number rather than
**	    index into the jnl file array, and add a parameter to specify
**	    whether to fully delete (partial deletion leaves journal and
**	    checkpoint's logical existence)
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**	9-May-2008 (kibro01) b120373
**	    Added support for -keep=n checkpoints
**	11-Nov-2008 (jonj)
**	    SIR 120874: Add jsx_dberr.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add JSX2_ALTDB_EMVCC, JSX2_ALTDB_DMVCC.
[@history_template@]...
*/
typedef struct _DMF_JSX
{
    DMP_DCB		*jsx_next;		/*  Next db on work queue. */
    DMP_DCB		*jsx_prev;		/*  Last db on work queue. */
    i4		jsx_operation;		/*  The operation in progress. */
#define			    JSX_O_ATP		    1L
#define			    JSX_O_CPP		    2L
#define			    JSX_O_RFP		    3L
#define			    JSX_O_INF		    4L
#define			    JSX_O_DTP		    5L
#define			    JSX_O_ALTER		    6L
#define                     JSX_O_RELOC             7L
/* JSX_O_IN1 - DBALERT */
#define             	    JSX_O_IN1          	    8L 
#define                     JSX_O_FLOAD             9L

    i4		jsx_status;		/*  Various flags. */
#define			    JSX_VERBOSE		    0x0001L
#define                     JSX_SECURITY            0x0002L
#define			    JSX_DEVICE		    0x0004L
#define			    JSX_WAITDB		    0x0008L
#define			    JSX_AUDITFILE	    0x0010L
#define			    JSX_JON		    0x0020L
#define			    JSX_JOFF		    0x0040L
#define			    JSX_USECKP		    0x0080L
#define			    JSX_USEJNL		    0x0100L
#define			    JSX_DESTROY		    0x0200L
#define			    JSX_ANAME		    0x0400L
#define			    JSX_TNAME		    0x0800L
#define			    JSX_BTIME		    0x1000L
#define			    JSX_ETIME		    0x2000L
#define			    JSX_UNAME		    0x4000L
#define			    JSX_DRAINDB		    0x8000L
#define			    JSX_SYSCATS		    0x10000L
#define			    JSX_TRANID		    0x20000L
#define			    JSX_CKPXL		    0x40000L
#define			    JSX_TRACE		    0x80000L
#define			    JSX_OPER		    0x100000L
#define			    JSX_CKP_STALL	    0x400000L
#define			    JSX_CKP_SELECTED	    0x800000L
#define			    JSX_LAST_VALID_CKP	    0x1000000L
#define			    JSX_JDISABLE	    0x2000000L
#define			    JSX_NOLOGGING	    0x4000000L
#define			    JSX_AUDIT_ALL	    0x8000000L
#define			    JSX_ALTDB_CKP_DELETE    0x10000000L
#define			    JSX_NORFP_UNDO	    0x20000000L
#define			    JSX_TBL_LIST    	    0x40000000L
#define			    JSX_FNAME_LIST    	    0x80000000L
    i4		jsx_status1;		/*  Various flags, cont */
#define			    JSX_JNL_BLK_SIZE   	    0x00000001L
#define			    JSX_JNL_NUM_BLOCKS	    0x00000002L
#define			    JSX_JNL_INIT_BLOCKS	    0x00000004L
#define			    JSX_NORFP_LOGGING	    0x00000008L
#define			    JSX_ATP_INCONS 	    0x00000010L
#define			    JSX_ATP_WAIT	    0x00000020L
#define			    JSX_HELP	    	    0x00000040L
#define			    JSX_FORCE	    	    0x00000080L
#define			    JSX_START_LSN	    0x00000100L
#define			    JSX_END_LSN	    	    0x00000200L
#define                     JSX1_RECOVER_DB         0x00000400L
#define                     JSX1_NOSEC_IND          0x00000800L
#define                     JSX1_ON_ERR_CONT        0x00001000L
#define                     JSX1_ONLINE             0x00002000L
#define                     JSX1_F_LOGICAL_CONSIST  0x00004000L
#define                     JSX1_STAT_REC_COUNT     0x00008000L
#define                     JSX1_NOBLOBS            0x00010000L
#define                     JSX1_STATISTICS         0x00020000L
#define                     JSX_CKPT_RELOC          0x00040000L
#define                     JSX_DMP_RELOC           0x00080000L
#define                     JSX_WORK_RELOC          0x00100000L
#define                     JSX_JNL_RELOC           0x00200000L
#define                     JSX_DB_RELOC            0x00400000L
#define                     JSX_LOC_LIST            0x00800000L
#define                     JSX_NLOC_LIST           0x01000000L
#define                     JSX_TARGETDB            0x02000000L
#define                     JSX_RELOCATE            0x04000000L
#define                     JSX1_CKPT_DB            0x08000000L
#define                     JSX1_IGNORE_FLAG        0x10000000L
#define                     JSX1_STALL_TIME         0x20000000L
#define                     JSX1_OUT_FILE           0x40000000L
#define                     JSX1_CKPT_TABLE         0x80000000L

    i4             jsx_status2;            /*  various flags; cont. */
# define                    JSX2_PMODE              0x00000001L
# define                    JSX2_OCKP               0x00000002L
# define		    JSX2_READ_ONLY          0x00000004L
# define		    JSX2_READ_WRITE         0x00000008L
# define		    JSX2_SWITCH_JOURNAL     0x00000010L
# define		    JSX2_INTERNAL_DATA      0x00000020L
# define		    JSX2_ABORT_TRANS        0x00000040L
# define                    JSX2_NO_HEADER	    0x00000080L
# define                    JSX2_LIMIT_OUTPUT	    0x00000100L
# define                    JSX2_EXPAND_LOBJS	    0x00000200L
# define                    JSX2_ALTDB_CKP_INVALID  0x00000400L
# define                    JSX2_ALTDB_UNICODED     0x00000800L
# define                    JSX2_ALTDB_UNICODEC     0x00001000L
# define		    JSX2_INVALID_TBL_LIST   0x00002000L
# define		    JSX2_ON_ERR_PROMPT	    0x00004000L
# define		    JSX2_MUST_DISCONNECT_BM 0x00008000L
# define		    JSX2_SERVER_CONTROL	    0x00010000L
# define		    JSX2_INCR_JNL	    0x00020000L
# define		    JSX2_RFP_ROLLBACK	    0x00040000L
# define		    JSX2_ALTDB_KEEP_N	    0x00080000L
# define		    JSX2_ALTDB_DMVCC	    0x00100000L
# define		    JSX2_ALTDB_EMVCC	    0x00200000L
    DB_TAB_TIMESTAMP	jsx_bgn_time;		/*  Begin time. */
    DB_TAB_TIMESTAMP	jsx_end_time;		/*  End time. */
    i4		jsx_db_cnt;		/*  Count of db's passed. */
    i4		jsx_tx_total;		/*  Total transactions. */
    i4		jsx_tx_since;		/*  Transaction since last. */
    i4		jsx_reg_case;     	/*  Regular id case */
    i4		jsx_delim_case;     	/*  Delim id case */
    i4		jsx_real_user_case;     /*  Real user name case */
#define			    JSX_CASE_UPPER   	    1L
#define			    JSX_CASE_LOWER	    2L
#define			    JSX_CASE_MIXED	    3L
    DB_OWN_NAME		jsx_username;		/*  Username to use. */
    bool		jsx_username_delim;	/*  TRUE = delimited */
    DB_OWN_NAME		jsx_a_name;		/*  Name to audit. */
    bool		jsx_aname_delim;	/*  TRUE = delimited */
    i4		jsx_tbl_cnt;		/*  Number of tables. */
#define		            JSX_MAX_TBLS    64
    struct _JSX_ATP_TBL				/*  AUDITDB -table= names */
    {
 	DB_OWN_NAME             tbl_owner;      /*  Owner.table_name support */
	DB_TAB_NAME		tbl_name;	/*  Table name */
	bool			tbl_delim;	/*  TRUE = delimited */
    } jsx_tbl_list[JSX_MAX_TBLS];
    JSX_LOC             jsx_def_list[5];            /* data,jnl,ckpt */
						    /* work,dmp loc  */
#define                     JSX_DATALOC       0
#define                     JSX_JNLLOC        1
#define                     JSX_CKPTLOC       2
#define                     JSX_WORKLOC       3
#define                     JSX_DMPLOC        4
#define                     JSX_DEFMAX        5
#define JSX_MAX_LOC                          64
    JSX_LOC             jsx_loc_list[JSX_MAX_LOC];  /* old locations */
    JSX_LOC             jsx_nloc_list[JSX_MAX_LOC]; /* new locations */
    i4             jsx_loc_cnt;                /* location count*/
    DB_DB_NAME          jsx_newdbname;              /* new db name   */
    i4             jsx_nloc_cnt;               /* new location cnt*/
    i4		jsx_fname_cnt;	 	/*  Number of .trl files. */
    char		jsx_fname_list[JSX_MAX_TBLS][DI_PATH_MAX];
						/*  List of .trl files. */
    i4		jsx_dev_cnt;		/*  Number of devices. */
    char        jsx_ucollation[DB_MAXNAME + 4]; /*For Unicode*/
    struct _JSX_DEVICE
    {
	char		dev_name[DI_PATH_MAX];  /*  CPP or RFP device. */
        i4		l_device;		/*  Length of device name. */
        PID		dev_pid;		/*  Process using dev_name. */
        DB_STATUS	status;			/*  Status of process. */
    } jsx_device_list[JSX_MAX_LOC];

    DB_TRAN_ID		jsx_tran_id;		/*  Transaction id to ignore. */
    i4		jsx_tx_id;
    i4		jsx_db_id;
    i4		jsx_ckp_number;		/*  If the user requests a
						**  specific checkpoint history
						**  number be used for the
						**  roll-forward, that number
						**  is recorded here.
						*/
    LG_LSN		jsx_start_lsn;		/*  If the user requests a
						**  specific starting lsn,
						**  this is it. */
    LG_LSN		jsx_end_lsn;		/*  If the user requests a
						**  specific ending lsn,
						**  this is it. */
    i4		jsx_max_jnl_blocks;	/*  Alterdb -max_jnl_blocks
						**  parameter. */
    i4		jsx_jnl_block_size;	/*  Alterdb -jnl_block_size
						**  parameter. */
    i4		jsx_init_jnl_blocks;	/*  Alterdb -init_jnl_blocks
						**  parameter. */
    i4             jsx_rfp_status_rec_cnt;  /* rollforwarddb, how often
                                                ** to issue status messages
                                                */
    i4		jsx_stall_time;		/* ckpdb -timeout=*/
    bool		jsx_ignored_errors;     /* errors were '-ignored'
						** during processing of
						** journal records.
						*/
    bool	jsx_ignored_tbl_err;		/* errors were '-ignored'
						** during processing of
						** TABLE dmp/jnl records.
						*/
    bool	jsx_ignored_idx_err;		/* errors were '-ignored'
						** during processing of
						** INDEX dmp/jnl records.
						*/
    PID		jsx_pid;
    LK_LLID	jsx_lock_list;
    LK_LKID	jsx_lockid;		/* LK_LKID for LK_CKP_DB lock */
    LK_LKID	jsx_lockxid;		/* LK_LKID for LK_CKP_TXN lock */
    i4		jsx_log_id;
    DB_DB_NAME	jsx_db_name;
    DB_OWN_NAME	jsx_db_owner;
    DB_ERROR	jsx_dberr;		/* Common error source info */
    i4		jsx_node;		/* node number if cluster enabled */
    i4		jsx_ckp_node;		/* node number of dmfjsp ckpdb */
    i4		jsx_ckp_crash;		/* for testing csp-ckp error handling */
    i1		jsx_ckp_phase;
    i4		jsx_keep_ckps;		/* no of valid ckp's to keep */
#define		JSX_LGK_INFO_SIZE	64
    char	jsx_lgk_info[JSX_LGK_INFO_SIZE]; /* command info for lg/lk */
}   DMF_JSX;

/*}
** Name: TBLLST_CTX - Table List context
**
** Description:
**      This structure contains information about a table list file.
**
** History:
**	 6-feb-1996 (nick)
**	    Created.
*/
typedef struct _TBLLST_CTX
{
    DI_IO	dio;
    i4	seq;
    i4	blksz;
# define	TBLLST_BKZ	DM_PG_SIZE
    i4	blkno;
    i4	phys_length;
    DM_PATH	physical;
    DM_FILE	filename;
    char	tbuf[TBLLST_BKZ];
# define	TBLLST_MAX_TAB	(TBLLST_BKZ / sizeof(DB_TAB_NAME))
} TBLLST_CTX;

/* Function prototype definitions. */

FUNC_EXTERN DB_STATUS dmfalter(
    DMF_JSX	*jsx,
    DMP_DCB	*dcb);

FUNC_EXTERN DB_STATUS dmfatp(
    DMF_JSX	*jsx,
    DMP_DCB	*dcb);

FUNC_EXTERN DB_STATUS dmfcpp(
    DMF_JSX	*jsx,
    DMP_DCB	*dcb);

FUNC_EXTERN DB_STATUS dmfdtp(
    DMF_JSX	*jsx,
    DMP_DCB	*dcb);

FUNC_EXTERN DB_STATUS dmfinfo(
    DMF_JSX	*jsx,
    DMP_DCB	*dcb);

FUNC_EXTERN DB_STATUS dmffload(
    DMF_JSX	*jsx,
    DMP_DCB	*dcb,
    char	*owner,
    i4		buffsize);

FUNC_EXTERN DB_STATUS dmfrfp(
    DMF_JSX	*jsx,
    DMP_DCB	*dcb);

FUNC_EXTERN VOID dump_file(
    DM_FILE	*f);

FUNC_EXTERN VOID dump_cnf(
    DM0C_CNF	*cnf);

FUNC_EXTERN VOID dump_dsc(
    DM0C_DSC    *dsc);

FUNC_EXTERN VOID dump_loc_entry(
    DMP_LOC_ENTRY   *entry);

FUNC_EXTERN VOID dump_dcb(
    DMP_DCB    *dcb);

FUNC_EXTERN VOID dump_tcb(
    DMP_TCB    *tcb);

FUNC_EXTERN VOID dump_jnl(
    DM0C_CNF    *cnf);

FUNC_EXTERN VOID dump_dmp(
    DM0C_CNF    *cnf);

FUNC_EXTERN VOID dump_ext(
    DMP_EXT	*ext);

FUNC_EXTERN VOID dmfWriteMsgFcn(
    DB_ERROR	*dberr,
    i4		err_code,
    PTR		FileName,
    i4		LineNumber,
    i4		numparams,
    ...);

#ifdef HAS_VARIADIC_MACROS

/* Preferred macros, with DB_ERROR *dberr */
#define dmfWriteMsg(dberr,err_code,...) \
    dmfWriteMsgFcn(dberr,err_code,__FILE__,__LINE__,__VA_ARGS__)

#else

/* Variadic macros not supported */
#define dmfWriteMsg dmfWriteMsgNV

FUNC_EXTERN VOID dmfWriteMsgNV(
    DB_ERROR	*dberr,
    i4		err_code,
    i4		numparams,
    ...);

#endif /* HAS_VARIADIC_MACROS */

FUNC_EXTERN i4 dmf_put_line(
    PTR		arg,
    i4		length,
    char	*buffer);

FUNC_EXTERN VOID jsp_set_case(
    DMF_JSX	    *jsx,
    i4	    case_type,
    i4	    obj_len,
    char	    *obj,
    char	    *ret_obj);

FUNC_EXTERN DB_STATUS jsp_get_case(
    DMP_DCB 		*dcb,
    DMF_JSX 		*jsx);

FUNC_EXTERN DB_STATUS jsp_file_maint(
    DMF_JSX         *jsx,
    DMP_DCB         *dcb,
    DM0C_CNF        **config,
    i4              ckp_seq_no,
    bool            fully_delete);


    /* Abort transaction */
FUNC_EXTERN DB_STATUS dmf_jsp_abort_xact(
    DML_ODCB		*odcb,
    DMP_DCB		*dcb,
    DB_TRAN_ID		*tran_id,
    i4		log_id,
    i4		lock_id,
    i4		flag,
    i4		sp_id,
    LG_LSN		*savepoint_lsn,
    DB_ERROR		*dberr);

    /* Begin a transaction. */
FUNC_EXTERN DB_STATUS dmf_jsp_begin_xact(
    i4		mode,
    i4		flags,
    DMP_DCB		*dcb,
    i4		dcb_id,
    DB_OWN_NAME		*user_name,
    i4		related_lock,
    i4		*log_id,
    DB_TRAN_ID		*tran_id,
    i4		*lock_id,
    DB_ERROR		*dberr);

    /* Commit a transaction. */
FUNC_EXTERN DB_STATUS dmf_jsp_commit_xact(
    DB_TRAN_ID		*tran_id,
    i4		log_id,
    i4		lock_id,
    i4		flag,
    DB_TAB_TIMESTAMP    *ctime,
    DB_ERROR		*dberr);

FUNC_EXTERN DB_STATUS dmfreloc(
DMF_JSX         *jsx,
DMP_DCB         *dcb);

FUNC_EXTERN DB_STATUS tbllst_create(
DMF_JSX         *jsx,
TBLLST_CTX	*tblctx);

FUNC_EXTERN DB_STATUS tbllst_open(
DMF_JSX         *jsx,
TBLLST_CTX	*tblctx);

FUNC_EXTERN DB_STATUS tbllst_close(
DMF_JSX         *jsx,
TBLLST_CTX	*tblctx);

FUNC_EXTERN DB_STATUS tbllst_delete(
DMF_JSX         *jsx,
TBLLST_CTX	*tblctx);

FUNC_EXTERN DB_STATUS tbllst_read(
DMF_JSX         *jsx,
TBLLST_CTX	*tblctx);

FUNC_EXTERN DB_STATUS tbllst_write(
DMF_JSX         *jsx,
TBLLST_CTX	*tblctx);

