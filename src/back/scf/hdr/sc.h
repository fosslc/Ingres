/*
** Copyright (c) 1986, 2005 Ingres Corporation
**
**
*/

/**
** Name: SC.H - Overall high level definitions and forward struct references
**
** Description:
**      This file contains some high level definitions used throughout SCF
**      and forward references to typedef's as well.
**
** History:
**      26-Jun-1986 (fred)
**          Created on Jupiter.
**	19-Jan-1986 (rogerk)
**	    Added two new copy processing states - SCS_CP_GETSTAT and 
**	    SCS_CP_ERROR.
**      23-Mar-1989 (fred)
**          Added error define's for server initialization errors
**	12-may-1989 (anton)
**	    Added SC_COLLATION and SC_LANGUAGE
**	22-jun-89 (jrb)
**	    Added E_SC0312_SCA_NO_LICENSE.
**	19-oct-89 (paul)
**	    Add error messages for event handling
**      30-nov-89 (fred)
**	    Added E_SC0255_INCOMPLETE_SEND
**	09-oct-90 (ralph)
**	    6.3->6.5 merge
**	10-dec-90 (neil)	(6.5: 19-oct-89)
**	    Alerters: Add error messages for event handling
**	28-feb-91 (rogerk)
**	    Added entry points for dbmsinfo SC_ONERROR_STATE and 
**	    SC_SVPT_ONERROR to return information about the session's
**	    on_error setting.
**	23-jun-1992 (fpang)
**	    Sybil merge.
**	    Start of merged STAR comments: 
**              08-oct-1990 (georgeg)
**                  Added error messages from jpt sc023a, sc023b.
**              22-apr-91 (szeng)
**                  Removed duplicate #define E_SC0240_DBMS_TASK_INITIATE.
**	    End of STAR comments.
**	22-sep-1992 (bryanp)
**	    Added new LG/LK error messages SC031D ... SC0326.
**	7-oct-1992 (bryanp)
**	    Added new LG/LK error message SC0327.
**	26-nov-1992 (markg)
**	    Added new audit error messages SC0329, SC032A.
**	19-jan-1993 (bryanp)
**	    Add CPTIMER error messages.
**	30-mar-1993 (robf)
**	    Add secure startup error messages
**	07-may-1993 (anitap) 
**	    Add #define for SC_ROWCNT_INFO
**      16-Aug-1993 (fred)
**          Adde new #define SCS_IERROR for sscb_state.  This is used 
**          to notify the user that an error was detected during input 
**          processing without terminating the connection.
**	28-Jul-1993 (daveb) 58936
**	    remove unprototyped extern of sc0e_put.  It's in sc0e.h now.
**	28-Oct-1993 (daveb) 58733
**	    Add E_SC0345_SERVER_CLOSED.
**	5-nov-93 (robf)
**          Add SCS_SPASSWORD/SCS_RPASSWORD
**	21-jan-94 (iyer)
**	    Add SC_TRAN_ID in support of dbmsinfo('db_tran_id').
**	    Add SC_CLUSTER_NODE in support of dbmsinfo('db_cluster_node').
**	14-feb-94 (rblumer)
**	    Added E_SC026B_INVALID_CURSOR_MODE.
**	24-apr-95 (cohmi01)
**	    Add defines for iomaster server errors.
**	22-may-96 (stephenb)
**	    Add E_SC037B and E_SC037C to support DBMS replication.
**	10-oct-1996 (canor01)
**	    Change E_SC037B to E_SC037D to match erscf.h.
**      01-aug-1996 (nanpr01 for ICL keving)
**          Added E_SC037F_LK_CALLBACK_EXIT and
**          E_SC037E_LK_CALLBACK_ADD.
**	15-Nov-1996 (jenjo02)
**	    Added E_SC0371_DEADLOCK_THREAD_ADD and
**	          E_SC0372_DEADLOCK_THREAD_EXIT
**	01-Apr-1998 (jenjo02)
**	    SCD_MCB renamed to SCS_MCB.
**	06-may-1998 (canor01)
**	    Add E_SC0382_LICENSE_THREAD_ADD.
**	25-jun-98 (stephenb)
**	    add SCS_CTHREAD opcode and E_SC0384_CUT_INIT error.
**      15-Feb-1999 (fanra01)
**          Add E_SC0384_CLEANUP_THREAD_ADD and E_SC0385_CLEANUP_THREAD_EXIT.
**      25-Feb-1999 (fanra01)
**          Add E_SC0386_LOGTRC_THREAD_ADD and E_SC0387_LOGTRC_THREAD_EXIT.
**	09-mar-1999 (mcgem01)
**	    Add E_SC0386_NO_LICENSE.
**      27-May-1999 (hanal04)
**          Added E_SC0387, E_SC0388, E_SC0389. b97083.
**	24-mar-2001 (stephenb)
**	    Add define for SC_UCOLLATION.
**      22-Nov-2002 (hanal04) Bug 107159 INGSRV 1696
**          Added E_SC0390.
**      19-jan-2004 (horda03) Bug 111047
**          Added E_SC0394. E_SC0392 already in use
**          - Manual changem, E_SC0394 already in use, so using E_SC039A
**	30-Aug-2004 (sheco02) 
**	    Cross-integrating change 466017 to main and bump up the E_SC0390 to
**	    E_SC0393.
**	31-Aug-2004 (schka24)
**	    should be SC0395.  (actually, usage should be looking in erscf.h)
**	09-may-2005 (devjo01)
**	    Add E_SC0396_DIST_DEADLOCK_THR_ADD and
**	    E_SC0397_DIST_DEADLOCK_THR_EXIT
**	21-jun-2005 (devjo01)
**	    Add E_SC0398_GLC_SUPPORT_THR_ADD and
**	    E_SC0399_GLC_SUPPORT_THR_EXIT.
**	17-may-2007 (dougi)
**	    Add SC_CHARSET for new dbmsinfo() option.
**	25-Feb-2008 (kiria01) b119976
**	    Added E_SC039B_VAR_LEN_INCORRECT
**	7-march-2008 (dougi) SIR 119414
**	    Added SC_CACHEDYN for new dbmsinfo() option.
**	26-Mar-2010 (kschendel) SIR 123485
**	    Delete some unused states (trying to reduce confusion in scs).
**/

/*
**  Forward and/or External function references.
*/


/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _SCD_CRB SCD_CRB;
typedef	struct _SCV_DBCB SCV_DBCB;
typedef	struct _SCV_LOC	SCV_LOC;
typedef struct _SCS_MCB	SCS_MCB;
typedef struct _SCD_SCB	SCD_SCB;


/*
**  Defines of other constants.
*/

/*
**      Definitions of operation codes used throughout the sequencer
**      and dispatcher.
**	These are found in scb->scb_sscb.sscb_state.
*/
#define                 SCS_INITIATE    0x1	/* time to start up a session */
#define                 SCS_INPUT       0x2	/* need input from the user */
#define                 SCS_CONTINUE    0x3	/* call back scs -- more data */
#define                 SCS_TERMINATE   0x4	/* session is ending */
#define			SCS_CP_INTO	0x5	/* a copy into */
#define			SCS_CP_FROM	0x6	/* A copy from */
#define			SCS_CP_ERROR	0x7	/* COPY FROM is waiting for
						** FE to notice error attn,
						** meanwhile we flush data
						*/
#define			SCS_ENDRET	0x8	/* end retrieve operation */
#define			SCS_INTERRUPT	0x9	/* interrupt the query */
#define			SCS_ADDPROT	0xA	/* add sys update capability */
#define			SCS_DELPROT	0xB	/*
						** delete syscat update
						** capability 
						*/
#define			SCS_PROCESS	0xC	    /*
						    ** now run the query
						    ** This value is an error if
						    ** ever set outside the
						    ** sequencer 
						    */
#define                 SCS_ACCEPT      0xD	/* Accept connection and ask for
						** initiation parameters
						*/
#define                 SCS_SPECIAL     0xE	/* Non-user DBMS thread - run
						** specialized processing
						** depending on thread type
						*/
#define			SCS_DCONNECT	0xF	/* Direct Connect mode.
						** Distributed server only.
						** User is connected directly
						** to the LDB.
						*/
#define                 SCS_IERROR      0x10    /* Error found in */
						/* processing input */
						/* blocks. */

#define			SCS_SPASSWORD	0x11	/* Send password prompt */

#define			SCS_RPASSWORD	0x12	/* Recieve password reply */
#define			SCS_CTHREAD	0x13	/* Coordinating thread sent an
						** event */
/*
**  Definitions of error states for building response blocks
*/

#define			SCS_OK		0x0
#define			SCS_ERROR	0x1

/*
**  Definitions for sizes of communication storage areas
*/

#define                 SCS_TEXT_DEFAULT 256
#define                 SCS_DATA_DEFAULT 16

/*
**  Special information requests from ADF
*/
#define			SC_XACT_INFO	    0x1000
#define			SC_AUTO_INFO	    0x1001
#define			SC_ET_SECONDS	    0x1002
#define			SC_QLANG	    0x1003
#define			SC_OPENDB_COUNT	    0x1004
#define			SC_DB_COUNT	    0x1005
#define			SC_SERVER_CLASS     0x1006
#define			SC_MAJOR_LEVEL	    0x1007
#define			SC_MINOR_LEVEL	    0x1008
#define			SC_COLLATION	    0x1009
#define			SC_LANGUAGE	    0x100A
#define			SC_ONERROR_STATE    0x100B
#define			SC_SVPT_ONERROR	    0x100C
#define			SC_ROWCNT_INFO	    0x100D	
#define			SC_TRAN_ID	    0x100E
#define			SC_CLUSTER_NODE	    0x100F
#define			SC_UCOLLATION	    0x1010
#define			SC_CHARSET	    0x1011
#define			SC_DB_CACHEDYN	    0x1012

/*
**  Definitions of internal SCF errors
*/
#define                 E_SC022C_PREMATURE_MSG_END	    (E_SC_MASK + 0x22CL)
#define			E_SC022D_1ST_BLOCK_NOT_MD_ASSOC     (E_SC_MASK + 0x22DL)
#define			E_SC022E_WRONG_BLOCK_TYPE	    (E_SC_MASK + 0x22EL)
#define			E_SC022F_BAD_GCA_READ		    (E_SC_MASK + 0x22FL)
#define			E_SC0230_INVALID_CHG_PROT_LEN	    (E_SC_MASK + 0x230L)
#define			E_SC0231_INVALID_CHG_PROT_CODE	    (E_SC_MASK + 0x231L)
#define			E_SC0232_INVALID_STARTUP_CODE	    (E_SC_MASK + 0x232L)
#define			E_SC0233_SESSION_START		    (E_SC_MASK + 0x233L)
#define			E_SC0234_SESSION_END		    (E_SC_MASK + 0x234L)
#define                 E_SC0235_AVGROWS		    (E_SC_MASK + 0x235L)
#define                 E_SC0236_FAST_COMMIT_INVALID	    (E_SC_MASK + 0x236L)
#define                 E_SC0237_FAST_COMMIT_ADD	    (E_SC_MASK + 0x237L)
#define                 E_SC0238_FAST_COMMIT_EXIT	    (E_SC_MASK + 0x238L)
#define                 E_SC0239_DBMS_TASK_ERROR	    (E_SC_MASK + 0x239L)
#define			E_SC023A_GCA_LISTEN_FAIL       	    (E_SC_MASK + 0x23AL)
#define			E_SC023B_INCONSISTENT_MSG_QUEUE	    (E_SC_MASK + 0x23BL)
#define                 E_SC0240_DBMS_TASK_INITIATE	    (E_SC_MASK + 0x240L)
#define                 E_SC0241_VITAL_TASK_FAILURE	    (E_SC_MASK + 0x241L)
#define                 E_SC0242_ALTER_MAX_SESSIONS	    (E_SC_MASK + 0x242L)
#define                 E_SC0243_WRITE_BEHIND_ADD	    (E_SC_MASK + 0x243L)
#define                 E_SC0244_WRITE_BEHIND_EXIT	    (E_SC_MASK + 0x244L)
#define			E_SC0245_SERVER_INIT_ADD	    (E_SC_MASK + 0x245L)
#define			E_SC0246_SCA_SHOW_ERROR		    (E_SC_MASK + 0x246L)
#define			E_SC0247_SCA_OPEN_ERROR		    (E_SC_MASK + 0x247L)
#define			E_SC0248_SCA_POSITION_ERROR	    (E_SC_MASK + 0x248L)
#define			E_SC0249_SCA_READ_ERROR		    (E_SC_MASK + 0x249L)
#define			E_SC024A_SCA_CLOSE_ERROR	    (E_SC_MASK + 0x24AL)
#define			E_SC024B_SCA_ALLOCATE		    (E_SC_MASK + 0x24BL)
#define			E_SC024C_SCA_DEALLOCATE		    (E_SC_MASK + 0x24CL)
#define			E_SC024D_SCA_ADDING		    (E_SC_MASK + 0x24DL)
#define			E_SC024E_SCA_NOT_ADDED		    (E_SC_MASK + 0x24EL)
#define			E_SC024F_SCA_IMP_ARGUMENTS	    (E_SC_MASK + 0x24FL)
#define			E_SC0250_SCE_EVENT_THREAD_ADD	    (E_SC_MASK + 0x250L)
#define			E_SC0255_INCOMPLETE_SEND	    (E_SC_MASK + 0x255L)
#define                 E_SC0256_SORT_THREAD_EXIT	    (E_SC_MASK + 0x256L)
#define			E_SC0263_SCA_RISK_CONSISTENCY	    (E_SC_MASK + 0x263L)
#define			E_SC0264_SCA_ILLEGAL_MAJOR	    (E_SC_MASK + 0x264L)
#define			E_SC0265_SCA_STATE		    (E_SC_MASK + 0x265L)
#define			E_SC0266_SCA_USER_SHUTDOWN	    (E_SC_MASK + 0x266L)
#define			E_SC0267_SCA_ID_UNKNOWN		    (E_SC_MASK + 0x267L)
#define			E_SC0268_SCA_RISK_INVOKED	    (E_SC_MASK + 0x268L)
#define			E_SC0269_SCA_INCOMPATIBLE	    (E_SC_MASK + 0x269L)
#define			E_SC026A_SCA_REGISTER_ERROR	    (E_SC_MASK + 0x26AL)
#define			E_SC026B_INVALID_CURSOR_MODE	    (E_SC_MASK + 0x26BL)
#define			E_SC0303_SCA_CREATE		    (E_SC_MASK + 0x303L)
#define			E_SC0304_SCA_MODIFY		    (E_SC_MASK + 0x304L)
#define			E_SC0305_SCA_QTXT_CREATE	    (E_SC_MASK + 0x305L)
#define			E_SC0306_SCA_DBP_CREATE		    (E_SC_MASK + 0x306L)
#define			E_SC0307_SCA_CONVERTING		    (E_SC_MASK + 0x307L)
#define			E_SC0308_SCA_CONVERTED		    (E_SC_MASK + 0x308L)
#define			E_SC0309_SCA_NOT_CONVERTED	    (E_SC_MASK + 0x309L)
#define			E_SC030A_SCA_ALTER		    (E_SC_MASK + 0x30AL)
#define			E_SC030B_SCA_BEGIN_XACT		    (E_SC_MASK + 0x30BL)
#define			E_SC030C_SCA_COMMIT		    (E_SC_MASK + 0x30CL)
#define			E_SC030D_SCA_ABORT		    (E_SC_MASK + 0x30DL)
#define			E_SC030E_SCA_ADD_DB		    (E_SC_MASK + 0x30EL)
#define			E_SC030F_SCA_OPEN_DB		    (E_SC_MASK + 0x30FL)
#define			E_SC0310_SCA_CLOSE_DB		    (E_SC_MASK + 0x310L)
#define			E_SC0311_SCA_DELETE_DB		    (E_SC_MASK + 0x311L)
#define			E_SC0312_SCA_NO_LICENSE		    (E_SC_MASK + 0x312L)
#define                 E_SC0318_SCA_USER_TRACE             (E_SC_MASK + 0x318L)
#define			E_SC031A_LO_PARAM_ERROR		    (E_SC_MASK + 0x31AL)
#define			E_SC031D_RECOVERY_ADD		    (E_SC_MASK + 0x31DL)
#define			E_SC031E_RECOVERY_EXIT		    (E_SC_MASK + 0x31EL)
#define			E_SC031F_LOGWRITER_ADD		    (E_SC_MASK + 0x31FL)
#define			E_SC0320_LOGWRITER_EXIT		    (E_SC_MASK + 0x320L)
#define			E_SC0321_CHECK_DEAD_ADD		    (E_SC_MASK + 0x321L)
#define			E_SC0322_CHECK_DEAD_EXIT	    (E_SC_MASK + 0x322L)
#define			E_SC0323_GROUP_COMMIT_ADD	    (E_SC_MASK + 0x323L)
#define			E_SC0324_GROUP_COMMIT_EXIT	    (E_SC_MASK + 0x324L)
#define			E_SC0325_FORCE_ABORT_ADD	    (E_SC_MASK + 0x325L)
#define			E_SC0326_FORCE_ABORT_EXIT	    (E_SC_MASK + 0x326L)
#define			E_SC0327_RECOVERY_SERVER_NOCNCT	    (E_SC_MASK + 0x327L)
#define			E_SC0329_AUDIT_THREAD_ADD	    (E_SC_MASK + 0x329L)
#define			E_SC032A_AUDIT_THREAD_EXIT	    (E_SC_MASK + 0x32AL)
#define			E_SC032F_CP_TIMER_ADD		    (E_SC_MASK + 0x32FL)
#define			E_SC0330_CP_TIMER_EXIT		    (E_SC_MASK + 0x330L)
#define			E_SC0345_SERVER_CLOSED		    (E_SC_MASK + 0x345L)
#define                 E_SC0332_B1_NEEDS_C2		    (E_SC_MASK + 0X332L)
#define                 E_SC0333_B1_NO_SECLABELS	    (E_SC_MASK + 0X333L)
#define                 E_SC0334_BAD_SECURE_LEVEL	    (E_SC_MASK + 0X334L)
#define                 E_SC0335_NO_PROC_SEC_LABEL	    (E_SC_MASK + 0X335L)
#define                 E_SC0336_BAD_SESS_SEC_LABEL	    (E_SC_MASK + 0X336L)
#define			E_SC0337_SEC_LABEL_TOO_LOW	    (E_SC_MASK + 0x337L)
#define			E_SC0338_SEC_LABEL_TOO_HIGH	    (E_SC_MASK + 0x338L)
#define			E_SC0339_SECURE_INVALID_PARM	    (E_SC_MASK + 0x339L)
#define			E_SC033A_DB_SEC_LABEL	    	    (E_SC_MASK + 0x33AL)
#define			E_SC033B_SET_LABEL_INVLD	    (E_SC_MASK + 0x33BL)
#define			E_SC033C_SET_SESSION_ERR	    (E_SC_MASK + 0x33CL)
#define			E_SC033E_SET_SESSION_PRIORITY	    (E_SC_MASK + 0x33EL)
#define			E_SC033F_INVLD_PRIORITY	            (E_SC_MASK + 0x33FL)
#define			E_SC0340_TERM_THREAD_ADD            (E_SC_MASK + 0x340L)
#define			E_SC0341_TERM_THREAD_EXIT     	    (E_SC_MASK + 0x341L)
#define			E_SC0342_IDLE_LIMIT_INVALID	    (E_SC_MASK + 0x342L)
#define			E_SC0343_CONNECT_LIMIT_INVALID	    (E_SC_MASK + 0x343L)
#define			I_SC0344_SESSION_DISCONNECT	    (E_SC_MASK + 0x344L)
#define			E_SC0349_GET_DB_ALARMS	            (E_SC_MASK + 0x349L)
#define			E_SC034A_FIRE_DB_ALARMS	            (E_SC_MASK + 0x34AL)
#define			E_SC034B_ALARM_QRY_IIEVENT	    (E_SC_MASK + 0x34BL)
#define			E_SC034C_GET_SESSION_ROLE	    (E_SC_MASK + 0x34CL)
#define			E_SC034D_ROLE_NOT_AUTHORIZED	    (E_SC_MASK + 0x34DL)
#define			E_SC034E_PRIV_PROPAGATE_ERROR	    (E_SC_MASK + 0x34EL)
#define			E_SC034F_NO_FLAG_PRIV	            (E_SC_MASK + 0x34FL)
#define			E_SC0350_B1_NOT_LICENCED	    (E_SC_MASK + 0x350L)
#define			E_SC0351_IIROLE_GET_ERROR	    (E_SC_MASK + 0x351L)
#define			E_SC0352_IIROLE_ERROR	    	    (E_SC_MASK + 0x352L)
#define			E_SC0353_SESSION_ROLE_ERR   	    (E_SC_MASK + 0x353L)
#define			E_SC0354_EXTPWD_GCA_COMPLETION	    (E_SC_MASK + 0x354L)
#define			E_SC0355_EXTPWD_GCA_NOPEER	    (E_SC_MASK + 0x355L)
#define			E_SC0356_EXTPWD_GCA_CSSUSPEND	    (E_SC_MASK + 0x356L)
#define			E_SC0357_EXTPWD_GCA_FASTSELECT	    (E_SC_MASK + 0x357L)
#define			E_SC0358_EXTPWD_GCA_FORMAT	    (E_SC_MASK + 0x358L)
#define			E_SC0359_EXTPWD_GCM_ERROR	    (E_SC_MASK + 0x359L)
#define			E_SC035A_EXTPWD_NO_AUTH_MECH        (E_SC_MASK + 0x35AL)
#define			E_SC035B_ROLE_CHECK_ERROR           (E_SC_MASK + 0x35BL)
#define			E_SC035C_USER_CHECK_EXT_PWD         (E_SC_MASK + 0x35CL)
#define			E_SC035D_SET_ROLE_IN_XACT           (E_SC_MASK + 0x35DL)
#define			E_SC035E_LOAD_ROLE_DBPRIV           (E_SC_MASK + 0x35EL)
#define			E_SC035F_SET_ROLE_NODBACCESS        (E_SC_MASK + 0x35FL)
#define			E_SC0360_SEC_WRITER_THREAD_ADD      (E_SC_MASK + 0x360L)
#define			E_SC0361_SEC_WRITER_THREAD_EXIT     (E_SC_MASK + 0x361L)
#define			E_SC0362_SECLABEL_PARAM_SLTYPE	    (E_SC_MASK + 0x362L)
#define			E_SC0364_READ_AHEAD_EXIT   	    (E_SC_MASK + 0x364L)
#define			E_SC0365_IOMASTER_OPENDB    	    (E_SC_MASK + 0x365L)
#define			E_SC0366_WRITE_ALONG_EXIT   	    (E_SC_MASK + 0x366L)
#define			E_SC0367_WRITE_ALONG_ADD    	    (E_SC_MASK + 0x367L)
#define			E_SC0368_READ_AHEAD_ADD    	    (E_SC_MASK + 0x368L)
#define			E_SC0369_IOMASTER_BADCPU   	    (E_SC_MASK + 0x369L)
#define			E_SC0370_IOMASTER_SERVER_NOCNCT	    (E_SC_MASK + 0x370L)
#define			E_SC0371_DEADLOCK_THREAD_ADD	    (E_SC_MASK + 0x371L)
#define			E_SC0372_DEADLOCK_THREAD_EXIT	    (E_SC_MASK + 0x372L)
#define			E_SC037C_REP_QMAN_EXIT		    (E_SC_MASK + 0x37CL)
#define			E_SC037D_REP_QMAN_ADD		    (E_SC_MASK + 0x37DL)
#define                 E_SC037E_LK_CALLBACK_THREAD_ADD     (E_SC_MASK + 0x37EL)
#define                 E_SC037F_LK_CALLBACK_THREAD_EXIT    (E_SC_MASK + 0x37FL)
#define                 E_SC0382_LICENSE_THREAD_ADD         (E_SC_MASK + 0x382L)
#define                 E_SC0383_LICENSE_THREAD_EXIT        (E_SC_MASK + 0x383L)
#define                 E_SC0384_CLEANUP_THREAD_ADD         (E_SC_MASK + 0x384L)
#define                 E_SC0385_CLEANUP_THREAD_EXIT        (E_SC_MASK + 0x385L)
#define                 E_SC0386_LOGTRC_THREAD_ADD          (E_SC_MASK + 0x386L)
#define                 E_SC0387_LOGTRC_THREAD_EXIT         (E_SC_MASK + 0x387L)
#define                 E_SC0388_NO_LICENSE        	    (E_SC_MASK + 0x388L)
#define			E_SC0389_LOCK_CREATE                (E_SC_MASK + 0x389L)
#define			E_SC0390_BAD_LOCK_REQUEST           (E_SC_MASK + 0x390L)
#define			E_SC0391_BAD_LOCK_RELEASE           (E_SC_MASK + 0x391L)
#define			E_SC0392_CUT_INIT		    (E_SC_MASK + 0x392L)
#define			E_SC0393_RAAT_NOT_SUPPORTED	    (E_SC_MASK + 0x393L)
#define			E_SC0394_CLUSTER_NOT_SUPPORTED	    (E_SC_MASK + 0x394L)
#define                 E_SC0395_RDF_DDB_FAILURE            (E_SC_MASK + 0x395L)
#define			E_SC0396_DIST_DEADLOCK_THR_ADD	    (E_SC_MASK + 0x396L)
#define			E_SC0397_DIST_DEADLOCK_THR_EXIT	    (E_SC_MASK + 0x397L)
#define			E_SC0398_GLC_SUPPORT_THR_ADD	    (E_SC_MASK + 0x398L)
#define			E_SC0399_GLC_SUPPORT_THR_EXIT	    (E_SC_MASK + 0x399L)
#define                 E_SC039A_SET_SESSION_READWRITE_ERR  (E_SC_MASK + 0x39AL)
#define			E_SC039B_VAR_LEN_INCORRECT	    (E_SC_MASK + 0x39BL)
