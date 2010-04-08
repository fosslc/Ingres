/*
** Copyright (c) 2004, 2009 Ingres Corporataion
*/

/**
** Name: GCD.H - Usable structure definitions for GCA messages
**
** Description:
**	This file contains usable structure definitions for all GCA messages
**	and their components.  These structures match the naming of those
**	in GCA.H, but differ in two ways:
**
**	1)	All structures are prefixed with GCD_ rather than GCA_.
**
**	2)	Any elements that were placeholder arrays (array declared
**		as [1] even though the array may be of any length) are
**		in these structures declared as pointers.
**
**	These structures are understood by GCA_SEND and GCA_RECEIVE when
**	called with the gca_formatted flag.
**
**	The mapping between GCA message names and the names of the 
**	structures that describe the message contents is given here.
**
**		Message		#	Structure
**		--------------------------------------------
**		GCA_MD_ASSOC	(0x02)	GCD_SESSION_PARMS
**		GCA_ACCEPT	(0x03)	<none>
**		GCA_REJECT	(0x04)	GCD_ER_DATA
**		GCA_RELEASE	(0x05)	GCD_ER_DATA
**		GCA_Q_BTRAN	(0x06)	GCD_TX_DATA
**		GCA_S_BTRAN	(0x07)	GCD_RE_DATA
**		GCA_ABORT	(0x08)	<none>
**		GCA_SECURE	(0x09)	GCD_TX_DATA
**		GCA_DONE	(0x0A)	GCD_RE_DATA
**		GCA_REFUSE	(0x0B)	GCD_RE_DATA
**		GCA_COMMIT	(0x0C)	<none>
**		GCA_QUERY	(0x0D)	GCD_Q_DATA
**		GCA_DEFINE	(0x0E)	GCD_Q_DATA
**		GCA_INVOKE	(0x0F)	GCD_IV_DATA
**		GCA_FETCH	(0x10)	GCD_ID_DATA
**		GCA_DELETE	(0x11)	GCD_DL_DATA
**		GCA_CLOSE	(0x12)	GCD_ID_DATA
**		GCA_ATTENTION	(0x13)	GCD_AT_DATA
**		GCA_QC_ID	(0x14)	GCD_ID_DATA
**		GCA_TDESCR	(0x15)	GCD_TD_DATA
**		GCA_TUPLES	(0x16)	GCD_TU_DATA
**		GCA_C_INTO	(0x17)	GCD_CP_MAP
**		GCA_C_FROM	(0x18)	GCD_CP_MAP
**		GCA_CDATA	(0x19)	GCD_TU_DATA
**		GCA_ACK		(0x1A)	GCD_AK_DATA
**		GCA_RESPONSE	(0x1B)	GCD_RE_DATA
**		GCA_ERROR	(0x1C)	GCD_ER_DATA
**		GCA_TRACE	(0x1D)	GCD_TR_DATA
**		GCA_Q_ETRAN	(0x1E)	<none>
**		GCA_S_ETRAN	(0x1F)	GCD_RE_DATA
**		GCA_IACK	(0x20)	GCD_AK_DATA
**		GCA_NP_INTERRUPT(0x21)	GCD_AT_DATA
**		GCA_ROLLBACK	(0x22)	<none>
**		GCA_Q_STATUS	(0x23)	<none>
**		GCA_CREPROC	(0x24)	GCD_Q_DATA
**		GCA_DRPPROC	(0x25)	GCD_Q_DATA
**		GCA_INVPROC	(0x26)	GCD_IP_DATA
**		GCA_RETPROC	(0x27)	GCD_RP_DATA
**		GCA_EVENT	(0x28)	GCD_EV_DATA
**		GCA1_C_INTO	(0x29)	GCD1_CP_MAP
**		GCA1_C_FROM	(0x2A)	GCD1_CP_MAP
**		GCA1_DELETE	(0x2B)	GCD1_DL_DATA
**		GCA_XA_SECURE	(0x2C)	GCD_XA_TX_DATA
**		GCA1_INVPROC	(0x2D)	GCD1_IP_DATA
**		GCA_PROMPT	(0x2E)	GCD_PROMPT_DATA
**		GCA_PRREPLY	(0x2F)	GCD_PRREPLY_DATA
**		GCA1_FETCH	(0x30)	GCD1_FT_DATA
**		GCA2_FETCH	(0x31)	GCD2_FT_DATA
**		GCA2_INVPROC	(0x32)	GCD2_IP_DATA
**
**		GCN_NS_OPER	(0x42)	GCD_GCN_D_OPER
**		GCN_RESULT	(0x43)	GCD_GCN_OPER_DATA
**
**		GCA_XA_START	(0x5A)	GCD_XA_DATA
**		GCA_XA_END	(0x5B)	GCD_XA_DATA
**		GCA_XA_PREPARE	(0x5C)	GCD_XA_DATA
**		GCA_XA_COMMIT	(0x5D)	GCD_XA_DATA
**		GCA_XA_ROLLBACK	(0x5E)	GCD_XA_DATA
**
**		GCM_GET		(0x60)	GCD_GCM_QUERY_MSG
**		GCM_GETNEXT	(0x61)	GCD_GCM_QUERY_MSG
**		GCM_SET		(0x62)	GCD_GCM_QUERY_MSG
**		GCM_RESPONSE	(0x63)	GCD_GCM_QUERY_MSG
**		GCM_SET_TRAP	(0x64)	GCD_GCM_TRAP_MSG
**		GCM_TRAP_IND	(0x65)	GCD_GCM_TRAP_MSG
**		GCM_UNSET_TRAP	(0x66)	GCD_GCM_TRAP_MSG
**
** History: 
**      14-Mar-94 (seiwald)
**	    Created.
**	14-Jun-95 (gordy)
**	    Created structures for the current practice
**	    used for GCA_XA_SECURE data objects.
**	13-Sep-95 (gordy)
**	    Converted GCD_GCA prefix to GCD.
**	22-Feb-96 (gordy)
**	    Added GCD_FT_DATA, GCD_PROMPT_DATA, GCD_PRREPLY_DATA.
**	29-Feb-96 (gordy)
**	    Converted VAREXPAR arrays which occur at start of top level
**	    structures to VARIMPAR so that large messages may be easily
**	    split at element boundaries.  Created an new structure,
**	    GCD_TDESCR identical to original GCD_TD_DATA to be used in
**	    the copy map structures.  Removed GCD_GCN_NM_DATA which was
**	    simply a VAREXPAR and replaced its single usage by a VARIMPAR.
**	21-May-97 (gordy)
**	    New GCF security handling.  Added new data object GCN2_D_RESOLVED
**	    And add GCN1_D_RESOLVED to document historical changes in
**	    GCN_D_RESOLVED.
**	17-Oct-97 (rajus01)
**	    Added gcn_rmt_auth, gcn_rmt_emech, gcn_rmt_emode.
**	12-Jan-98 (gordy)
**	    Support added for direct GCA network connections.
**	31-Mar-98 (gordy)
**	    Make GCN2_D_RESOLVED extensible with variable array of values.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 6-Nov-01 (gordy)
**	    General cleanup as followup to removal of nat/longnat.
**	    Brought inline with changes in gca.h.  Removed internal 
**	    messages and structures.
**	22-Mar-02 (gordy)
**	    GCD_ID_DATA (message object) is same as GCD_ID (internal object).
**	30-Jun-06 (gordy)
**	    Extend XA support.
**	12-Mar-07 (gordy)
**	    Scrollable cursor support.
**	 1-Oct-09 (gordy)
**	    Unrestricted procedure names.
*/

# ifndef __GCD_H__
# define __GCD_H__

/*
** Usable structure definitions for GCA messages and their components.
*/


/***************************** GCA data objects *****************************/

/*  
** GCD_NAME: Generalized name element 
*/
typedef struct _GCD_NAME GCD_NAME;

struct _GCD_NAME
{
    i4		gca_l_name;		/* Length of following array */
    char	*gca_name;		/* Variable length */
};

/*  
** GCD_ID: DBMS object ID 
*/
typedef struct _GCD_ID GCD_ID;

struct _GCD_ID
{
    i4		gca_index[ 2 ];
    char	gca_name[ GCA_MAXNAME ];
};

/*
** GCA_DATA_VALUE: Generalized data representation
*/
typedef struct _GCD_DATA_VALUE GCD_DATA_VALUE;

struct _GCD_DATA_VALUE
{
    i4		gca_type;
    i4		gca_precision;
    i4		gca_l_value;		/* Length of following array */
    char	*gca_value;		/* Variable length */
};

/*
** GCD_DBMS_VALUE: DBMS data representation
*/
typedef struct _GCD_DBMS_VALUE GCD_DBMS_VALUE;

struct _GCD_DBMS_VALUE
{
    i4		db_data;		/* --- unused --- */
    i4		db_length;		/* Length of data in bytes */
    i2		db_datatype;		/* GCA Data Type */
    i2		db_precision;		/* Hi: precision, Lo: scale */
};

/*
** GCD_COL_ATT: Column Attribute
*/
typedef struct _GCD_COL_ATT GCD_COL_ATT;

struct _GCD_COL_ATT
{
    GCD_DBMS_VALUE	gca_attdbv;
    i4			gca_l_attname;	/* Length of following array */
    char		*gca_attname;	/* Variable length */
};

/*
** GCD_CPRDD: Copy row domain descriptor
*/
typedef struct _GCD_CPRDD GCD_CPRDD;

struct _GCD_CPRDD
{
    char	gca_domname_cp[ GCA_MAXNAME ];	/* domain name */
    i4		gca_type_cp;		/* row type */
    i4		gca_length_cp;		/* row length */
    i4		gca_user_delim_cp;	/* TRUE if domain delimiter */
    i4		gca_l_delim_cp;		/* Number of delimiters */
    char 	gca_delim_cp[ 2 ];	/* delimiter(s) */
    i4		gca_tupmap_cp;		/* column for this domain */
    i4		gca_cvid_cp;		/* ADF function instance id */
    i4		gca_cvlen_cp;		/* 'convert to' value length */
    i4		gca_withnull_cp;	/* 'with null' clause given */
    i4		gca_nullen_cp;		/* Length of following array */
    char	*gca_nuldata_cp;	/* Variable length */
};

/*
** GCD1_CPRDD: Copy row domain descriptor (GCA_PROTOCOL_LEVEL_50)
*/
typedef struct _GCD1_CPRDD GCD1_CPRDD;

struct _GCD1_CPRDD
{
    char		gca_domname_cp[ GCA_MAXNAME ];	/* domain name */
    i4			gca_type_cp;		/* row type */
    i4			gca_precision_cp;	/* row precision */
    i4			gca_length_cp;		/* row length */
    i4			gca_user_delim_cp;	/* TRUE if domain delimiter */
    i4			gca_l_delim_cp;		/* Number of delimiters */
    char 		gca_delim_cp[ 2 ];	/* delimiter(s) */
    i4			gca_tupmap_cp;		/* column for this domain */
    i4			gca_cvid_cp;		/* ADF function instance id */
    i4			gca_cvprec_cp;		/* "convert to" value prec */
    i4			gca_cvlen_cp;		/* "convert to" value length */
    i4			gca_withnull_cp;	/* with null clause given */
    i4			gca_l_nullvalue_cp;	/* Length of following array */
    GCD_DATA_VALUE	*gca_nullvalue_cp;	/* Variable length */
};

/*
** GCD_E_ELEMENT: Error data element
*/
typedef struct _GCD_E_ELEMENT GCD_E_ELEMENT;

struct _GCD_E_ELEMENT
{
    i4			gca_id_error;
    i4			gca_id_server;
    i4			gca_server_type;
    i4			gca_severity;
    i4			gca_local_error;
    i4			gca_l_error_parm;	/* Length of following array */
    GCD_DATA_VALUE	*gca_error_parm;	/* Variable length */
};

/*
** GCD_P_PARAM: Database procedure parameter
*/
typedef struct _GCD_P_PARAM GCD_P_PARAM;

struct _GCD_P_PARAM
{
    GCD_NAME		gca_parname;
    i4			gca_parm_mask;
    GCD_DATA_VALUE	gca_parvalue;
};

/*
** GCD_TUP_DESCR: Tuple descriptor
*/
typedef struct _GCD_TUP_DESCR GCD_TUP_DESCR;

struct _GCD_TUP_DESCR
{
    i4		gca_tsize;		/* Size of the tuple */
    i4		gca_result_modifier;
    i4		gca_id_tdscr;		/* ID of this descriptor */
    i4		gca_l_col_desc;		/* Length of following array */
    GCD_COL_ATT	*gca_col_desc;		/* Variable length */
};

/*
** GCD_USER_DATA: User data
*/
typedef struct _GCD_USER_DATA GCD_USER_DATA;

struct _GCD_USER_DATA
{
    i4			gca_p_index;
    GCD_DATA_VALUE	gca_p_value;
};

/*
** GCD_XA_DIS_TRAN_ID: XA distributed transaction ID
*/
typedef struct _GCD_XA_DIS_TRAN_ID GCD_XA_DIS_TRAN_ID;

struct _GCD_XA_DIS_TRAN_ID
{
    i4		formatID;
    i4		gtrid_length;
    i4		bqual_length;
    char	data[ GCA_XA_XID_MAX ];
    i4		branch_seqnum;
    i4		branch_flag;
};


/**************************** GCA message objects ****************************/

/*
** GCD_AK_DATA: Acknowledgement Data
*/
typedef struct _GCD_AK_DATA GCD_AK_DATA;

struct _GCD_AK_DATA
{
    i4		gca_ak_data;
};

/*
** GCD_AT_DATA: Interrupt Data
*/
typedef struct _GCD_AT_DATA GCD_AT_DATA;

struct _GCD_AT_DATA
{
    i4		gca_itype;
};

/*
** GCD_CP_MAP: Copy Map
*/
typedef struct _GCD_CP_MAP GCD_CP_MAP;

struct _GCD_CP_MAP
{
    i4			gca_status_cp;		/* status bits */
    i4			gca_direction_cp;	/* INTO or FROM */
    i4			gca_maxerrs_cp;
    i4			gca_l_fname_cp;		/* length of following array */
    char		*gca_fname_cp;		/* Variable length */
    i4			gca_l_logname_cp;	/* length of following array */
    char		*gca_logname_cp;	/* Variable length */
    GCD_TUP_DESCR	gca_tup_desc_cp;	/* describes tuple */
    i4			gca_l_row_desc_cp;	/* length of following array */
    GCD_CPRDD		gca_row_desc_cp[1];	/* Variable length */
};

/*
** GCD1_CP_MAP: Copy Map (GCA_PROTOCOL_LEVEL_50)
*/
typedef struct _GCD1_CP_MAP GCD1_CP_MAP;

struct _GCD1_CP_MAP
{
    i4			gca_status_cp;		/* Status bits */
    i4			gca_direction_cp;	/* INTO or FROM */
    i4			gca_maxerrs_cp;
    i4			gca_l_fname_cp;		/* Length of following array */
    char		*gca_fname_cp;		/* Variable length */
    i4			gca_l_logname_cp;	/* Length of following array */
    char		*gca_logname_cp;	/* Variable length */
    GCD_TUP_DESCR	gca_tup_desc_cp;	/* Describes tuple */
    i4			gca_l_row_desc_cp;	/* Length of following array */
    GCD1_CPRDD		gca_row_desc_cp[1];	/* Variable length */
};

/*
** GCD_DL_DATA: Cursor Delete
*/
typedef struct _GCD_DL_DATA GCD_DL_DATA;

struct _GCD_DL_DATA
{
    GCD_ID 	gca_cursor_id;
    GCD_NAME	gca_table_name;
};

/*
** GCD1_DL_DATA: Cursor Delete (GCA_PROTOCOL_LEVEL_60)
*/
typedef struct _GCD1_DL_DATA GCD1_DL_DATA;

struct _GCD1_DL_DATA
{
    GCD_ID 	gca_cursor_id;
    GCD_NAME	gca_owner_name;
    GCD_NAME	gca_table_name;
};

/*
** GCD_ER_DATA: Standard Error Representation
*/
typedef struct _GCD_ER_DATA GCD_ER_DATA;

struct _GCD_ER_DATA
{
    i4			gca_l_e_element;	/* Length of following array */
    GCD_E_ELEMENT	gca_e_element[1];	/* Variable length */
};

/*
** GCD_EV_DATA: Event Notification Data
*/
typedef struct _GCD_EV_DATA GCD_EV_DATA;

struct _GCD_EV_DATA
{
    GCD_NAME		gca_evname;
    GCD_NAME		gca_evowner;
    GCD_NAME		gca_evdb;
    GCD_DATA_VALUE	gca_evtime;
    i4			gca_l_evvalue;		/* Length of following array */
    GCD_DATA_VALUE	gca_evvalue[1];		/* Variable length */
};

/*
** GCD1_FT_DATA: Cursor Fetch
*/
typedef struct _GCD1_FT_DATA GCD1_FT_DATA;

struct _GCD1_FT_DATA
{
    GCD_ID	gca_cursor_id;
    i4		gca_rowcount;
};

/*
** GCD2_FT_DATA: Cursor Fetch
*/
typedef struct _GCD2_FT_DATA GCD2_FT_DATA;

struct _GCD2_FT_DATA
{
    GCD_ID	gca_cursor_id;
    i4		gca_rowcount;
    u_i4	gca_anchor;
    i4		gca_offset;
};

/*
** GCD_ID_DATA: Query and Cursor Identifier
*/
typedef GCD_ID GCD_ID_DATA;


/*
** GCD_IP_DATA: Stored Procedure Invocation Data
*/
typedef struct _GCD_IP_DATA GCD_IP_DATA;

struct _GCD_IP_DATA
{
    GCD_ID	gca_id_proc;
    i4		gca_proc_mask;
    GCD_P_PARAM	gca_param[1];		/* Variable length */
};

/*
** GCD1_IP_DATA: Stored Procedure Invocation Data (GCA_PROTOCOL_LEVEL_60)
*/
typedef struct _GCD1_IP_DATA GCD1_IP_DATA;

struct _GCD1_IP_DATA
{
    GCD_ID	gca_id_proc;
    GCD_NAME    gca_proc_own;
    i4		gca_proc_mask;
    GCD_P_PARAM	gca_param[1];		/* Variable length */
};

/*
** GCD2_IP_DATA: Stored Procedure Invocation Data (GCA_PROTOCOL_LEVEL_68)
*/
typedef struct _GCD2_IP_DATA GCD2_IP_DATA;

struct _GCD2_IP_DATA
{
    GCD_NAME	gca_proc_name;
    GCD_NAME	gca_proc_own;
    i4		gca_proc_mask;
    GCD_P_PARAM	gca_param[1];		/* Variable length */
};

/*
** GCD_IV_DATA: Repeat Query Invocation Data
*/
typedef struct _GCD_IV_DATA GCD_IV_DATA;

struct _GCD_IV_DATA
{
    GCD_ID		gca_qid;
    GCD_DATA_VALUE	gca_qparm[1];		/* VARIMPAR */
}; 

/*
** GCD_PROMPT_DATA: Prompt Data
*/
typedef struct _GCD_PROMPT_DATA GCD_PROMPT_DATA;

struct _GCD_PROMPT_DATA
{
    i4			gca_prflags;	/* Prompt flags */
    i4			gca_prtimeout;	/* Timeout in seconds */
    i4			gca_prmaxlen;	/* Maximum length of reply */
    i4			gca_l_prmesg;	/* Length of following array */
    GCD_DATA_VALUE	*gca_prmesg;	/* Variable length */
};

/*
** GCD_PRREPLY_DATA: Reply to Prompt
*/
typedef struct _GCD_PRREPLY_DATA GCD_PRREPLY_DATA;

struct _GCD_PRREPLY_DATA
{
    i4		gca_prflags;	/* Response flags */
    i4		gca_l_prvalue;	/* Number of following values */
    GCD_DATA_VALUE	*gca_prvalue;	/* Reply */
};

/*
** GCD_Q_DATA: Query Data
*/
typedef struct _GCD_Q_DATA GCD_Q_DATA;

struct _GCD_Q_DATA
{
    i4		gca_language_id;
    i4		gca_query_modifier;
    GCD_DATA_VALUE	gca_qdata[1];		/* VARIMPAR */
};

/*
** GCD_RE_DATA: Response Data
*/
typedef struct _GCD_RE_DATA GCD_RE_DATA;

struct _GCD_RE_DATA
{
    i4		gca_rid;
    i4		gca_rqstatus;
    i4		gca_rowcount;
    i4		gca_rhistamp;
    i4		gca_rlostamp;
    i4		gca_cost;
    i4		gca_errd0;
    i4		gca_errd1;
    i4		gca_errd4;
    i4		gca_errd5;
    char 	gca_logkey[ 16 ];
};

/*
** GCD_RP_DATA: Stored Procedure Respone Completion
*/
typedef struct _GCD_RP_DATA GCD_RP_DATA;

struct _GCD_RP_DATA
{
    GCD_ID	gca_id_proc;
    i4		gca_retstat;
};

/*
** GCD_SESSION_PARMS: Association Parameters
*/
typedef struct _GCD_SESSION_PARMS GCD_SESSION_PARMS;

struct _GCD_SESSION_PARMS
{
    i4			gca_l_user_data;	/* Length of following array */
    GCD_USER_DATA	gca_user_data[1];	/* Variable length */
};

/*
** GCD_TD_DATA:
**
** GCA typedefs this message object to be the same
** as GCA_TUP_DESCR.  For GCD, different structures
** are used.  GCD_TUP_DESCR must be fixed length
** since it is contained in other structures.  This
** object (as a top-level structure) may be variable
** length, so the column descriptor array is kept
** in-line.
*/
typedef struct _GCD_TD_DATA GCD_TD_DATA;

struct _GCD_TD_DATA
{
    i4			gca_tsize;		/* Size of the tuple */
    i4			gca_result_modifier;
    i4			gca_id_tdscr;		/* ID of this descriptor */
    i4			gca_l_col_desc;		/* Length of following array */
    GCD_COL_ATT		gca_col_desc[1];	/* Variable length */
};

/*
** GCD_TR_DATA: DBMS Trace Information
*/
typedef struct _GCD_TR_DATA GCD_TR_DATA;

struct _GCD_TR_DATA
{
    char	gca_tr_data[1];		/* Variable length */
};

/*
** GCD_TU_DATA: Tuple Data
*/
typedef struct _GCD_TU_DATA GCD_TU_DATA;

struct _GCD_TU_DATA
{
    char	gca_tu_data[1];		/* Variable length */
};

/*
** GCD_TX_DATA: Transaction Data
*/
typedef struct _GCD_TX_DATA GCD_TX_DATA;

struct _GCD_TX_DATA
{
    GCD_ID	gca_tx_id;
    i4		gca_tx_type;
};

/*
** GCD_XA_TX_DATA: XA Transaction Data
*/
typedef struct _GCD_XA_TX_DATA GCD_XA_TX_DATA;

struct _GCD_XA_TX_DATA
{
    i4			gca_xa_type;		/* Ignored - set to 0 */
    i4			gca_xa_precision;	/* Ignored - set to 0 */
    i4			gca_xa_l_value;		/* Length of following */
    GCD_XA_DIS_TRAN_ID	gca_xa_id;
};


/*
** GCD_XA_DATA: XA Transaction Data
*/
typedef struct _GCD_XA_DATA GCD_XA_DATA;

struct _GCD_XA_DATA
{
    u_i4		gca_xa_flags;
    GCD_XA_DIS_TRAN_ID	gca_xa_xid;
};


/***************************** GCN data objects *****************************/

/*
** GCD_GCN_VAL: General GCN value
*/
typedef struct _GCD_GCN_VAL GCD_GCN_VAL;

struct _GCD_GCN_VAL
{
    i4		gcn_l_item;		/* Length of following array */
    char	*gcn_value;		/* Variable length */
};

/*
** GCD_GCN_REQ_TUPLE: GCN tuple format
*/
typedef struct _GCD_GCN_REQ_TUPLE GCD_GCN_REQ_TUPLE;

struct _GCD_GCN_REQ_TUPLE
{
    GCD_GCN_VAL	gcn_type;
    GCD_GCN_VAL	gcn_uid;
    GCD_GCN_VAL	gcn_obj;
    GCD_GCN_VAL	gcn_val;
};

/**************************** GCN message objects ****************************/

/*
** GCD_GCN_D_OPER: GCN Operation Requests
*/
typedef struct _GCD_GCN_D_OPER GCD_GCN_D_OPER;

struct _GCD_GCN_D_OPER
{
    i4			gcn_flag;
    i4			gcn_opcode;
    GCD_GCN_VAL		gcn_install;
    i4			gcn_tup_cnt;		/* Length of following array */
    GCD_GCN_REQ_TUPLE	gcn_tuple[1];		/* Variable length */
};

/*
** GCD_GCN_OPER_DATA: GCN Operation Results
*/
typedef struct _GCD_GCN_OPER_DATA GCD_GCN_OPER_DATA;

struct _GCD_GCN_OPER_DATA
{
    i4			gcn_op;
    i4			gcn_tup_cnt;		/* Length of following array */
    GCD_GCN_REQ_TUPLE	gcn_tuple[1];		/* Variable length */
};

/***************************** GCM data objects *****************************/

/*
** GCD_GCM_IDENTIFIER: GCM object identifier
*/
typedef struct _GCD_GCM_IDENTIFIER GCD_GCM_IDENTIFIER;

struct _GCD_GCM_IDENTIFIER
{
    i4		l_object_class;		/* Length of following array */
    char	*object_class;		/* Variable length */
    i4		l_object_instance;	/* Length of following array */
    char	*object_instance;	/* Variable length */
};

/*
** GCD_GCM_VALUE: GCM object value
*/
typedef struct _GCD_GCM_VALUE GCD_GCM_VALUE;

struct _GCD_GCM_VALUE
{
    i4		l_object_value;		/* Length of following array */
    char	*object_value;		/* Variable length */
};

/*
** GCD_GCM_VAR
*/
typedef struct _GCD_GCM_VAR GCD_GCM_VAR;

struct _GCD_GCM_VAR
{
    GCD_GCM_IDENTIFIER	var_name;
    GCD_GCM_VALUE	var_value;
    i4			var_perms;
};

/*
** GCD_GCM_MSG_HDR: GCM message header
*/
typedef struct _GCD_GCM_MSG_HDR GCD_GCM_MSG_HDR;

struct _GCD_GCM_MSG_HDR
{
    i4		error_status;
    i4		error_index;
    i4		future[2];
    i4		client_perms;
    i4		row_count;
};

/*
** GCD_GCM_TRAP_HDR: GCM trap header
*/
typedef struct _GCD_GCM_TRAP_HDR GCD_GCM_TRAP_HDR;

struct _GCD_GCM_TRAP_HDR
{
    i4		trap_reason;
    i4		trap_handle;
    i4		mon_handle;
    i4		l_trap_address;		/* Length of following array */
    char	*trap_address;		/* Variable length */
};

/**************************** GCM message objects ****************************/

/*
** GCD_GCM_QUERY_DATA: GCM Query data
*/
typedef struct _GCD_GCM_QUERY_DATA GCD_GCM_QUERY_DATA;

struct _GCD_GCM_QUERY_DATA
{
    GCD_GCM_MSG_HDR	msg_hdr;
    i4			element_count;		/* Length of following array */
    GCD_GCM_VAR		var_binding[1];		/* Variable length */
};

/*
** GCD_GCM_TRAP_DATA: GCM Trap data
*/
typedef struct _GCD_GCM_TRAP_DATA GCD_GCM_TRAP_DATA;

struct _GCD_GCM_TRAP_DATA
{
    GCD_GCM_MSG_HDR	msg_hdr;
    GCD_GCM_TRAP_HDR	trap_hdr;
    i4			element_count;		/* Length of following array */
    GCD_GCM_VAR		var_binding[1];		/* Variable length */
};

#endif

