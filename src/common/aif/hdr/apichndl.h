/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/*
** Name: apichndl.h
**
** Description:
**	Defines the connection handle and associated information.
**
** History:
**      10-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	14-Nov-94 (gordy)
**	    Generalized the processing of GCA_MD_ASSOC.
**	18-Nov-94 (gordy)
**	    Made IIAPI_CP_RESTART a local connection parameter.
**	20-Dec-94 (gordy)
**	    Cleaned up common handle processing.
**	10-Jan-95 (gordy)
**	    Added IIapi_getConnIdHndl().
**	19-Jan-95 (gordy)
**	    Added internal parameter IIAPI_CP_TIMEZONE_OFFSET.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	10-May-95 (gordy)
**	    Changed return value of IIapi_setConnParm().  Made
**	    IIAPI_CP_EFFECTIVE_USER visible to the application.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	14-Jun-95 (gordy)
**	    Transaction IDs are passed formatted into character strings.
**	28-Jul-95 (gordy)
**	    Used fixed length types.
**	14-Sep-95 (gordy)
**	    GCD_GCA_ prefix changed to GCD_.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	 8-Mar-96 (gordy)
**	    Added ch_msg_continued to connection handle to track messages
**	    split across buffers.
**	22-Apr-96 (gordy)
**	    Convert ch_msg_continued to ch_flags and add COPY interrupt.
**	12-Jul-96 (gordy)
**	    Enhanced event support with IIapi_getEvent().
**	 9-Sep-96 (gordy)
**	    Added IIAPI_SP_YEAR_CUTOFF.
**	 2-Oct-96 (gordy)
**	    Added connection type, dropped values stored in connection
**	    parms, simplified function callbacks.
**	31-Jan-97 (gordy)
**	    Added het-net descriptor connection flag for COPY FROM.
**	13-Mar-97 (gordy)
**	    Added back some dropped values (target,username,password)
**	    for Name Server connections which needs manipulate the
**	    values or reference them after the connection is made.
**	04-Sep-98 (rajus01)
**	    Added internal parameter IIAPI_CP_SL_TYPE.
**	06-Oct-98 (rajus01)
**	    Added support for GCA_PROMPT. Changed sp_sl_type to be II_LONG.
**	19-Aug-2002 (wansh01)     
**	    Added support for login type local as a connection
**          parameter. 
**	15-Mar-04 (gordy)
**	    Added IIAPI_SP_I8WIDTH.
**      11-Jan-2005 (wansh01)
**          INGNET 161 bug # 113737
**          A new field ch_target_display is added to the connHndl.
**          This contains target info with supressed login info(user,pwd).
**	 8-Aug-07 (gordy)
**	    Added IIAPI_SP_DATE_ALIAS.
**	25-Mar-10 (gordy)
**	    Added message buffer queues to the connection handle.
**	26-May-10 (gordy)
**	    Write DBMS trace message to log.
*/

# ifndef __APICHNDL_H__
# define __APICHNDL_H__

typedef struct _IIAPI_CONNHNDL	IIAPI_CONNHNDL;

# include <apienhnd.h>
# include <apiihndl.h>


/*
** Name: IIAPI_SESSIONPARM
**
** Description:
**	API structure defining the GCA session parameters which may
**	be passed in a GCA_MD_ASSOC message.  This structure contains
**	all the GCA parameters in the same order as defined by GCA.
**	some parameters may not be used by the actual API implementation.
**      All parameters within this structure must be prefixed
**      with sp_.
**
**      sp_mask1
**          mask for first 32 parameters to indicate if they have been set.
**      sp_exclusive
**          lock the DB exclusively
**	sp_version
**	    GCA version - unused!
**      sp_application
**          -A option
**	sp_qlanguage
**	    query language - QUEL or SQL
**      sp_cwidth
**          width of c columns
**      sp_twidth
**          width of text columns
**      sp_i1width
**          width of integer 1
**      sp_i2width
**          width of integer 2
**      sp_i4width
**          width of integer 4
**	sp_i8width
**	    width of integer 8 (sp_mask2)
**      sp_f4width
**          width of float 4
**      sp_f4precision
**          precision of float 4
**      sp_f8width
**          width of float 8
**      sp_f8precision
**          precision of float 8
**      sp_svtype
**          type of server function desired.
**      sp_nlanguage
**          native language for errors
**      sp_mprec
**          money precision
**      sp_mlort
**          leading or trailing money sign
**      sp_date_format
**          ingres internal date format
**	sp_timezone
**	    timezone offset
**      sp_timezone_name
**          name of timezone
**	sp_dbname
**	    name of database
**      sp_idx_struct
**          default secondary index structure.
**      sp_res_struct
**          default structure for result table.
**	sp_euser
**	    effective user name (-u flag)
**      sp_mathex
**          treatment of math exception
**      sp_f4style
**          floating point 4 format style.
**      sp_f8style
**          floating point 8 format style.
**      sp_msign
**          money sign
**      sp_decimal
**          decimal character
**      sp_aplid
**          application ID or password
**      sp_grpid
**          group ID
**      sp_decfloat
**          treatment of numeric literals
**      sp_mask2
**          mask for second 32 parameters to indicate if they have been set.
**	sp_rupass
**	    real user password (-P flag) - unused!
**      sp_misc_parm_count
**          number of miscellaneous parameters - not an actual GCA parameter
**      sp_misc_parm
**          miscellaneous parameters (multiple allowed, count given above)
**	sp_strtrunc
**	    string truncation
**      sp_xupsys
**          update system catalog, exclusive lock.
**      sp_supsys
**          update system catalog: shared lock.
**      sp_wait_lock
**          wait on DB.
**      sp_gw_parm_count
**          number of gateway parameters - not an acutal GCA parameter
**      sp_gw_parm
**          gateway parmeters (multiple allowed, count given above)
**      sp_restart
**          transaction restart
**	sp_xa_restart
**	    XA transaction restart 
**
** History:
**      06-sep-93 (ctham)
**          creation
**	14-Nov-94 (gordy)
**	    Converted to internal structure, added all GCA parameters.
**	19-Jan-95 (gordy)
**	    Added internal parameter IIAPI_CP_TIMEZONE_OFFSET.
**	14-Jun-95 (gordy)
**	    Transaction IDs are passed formatted into character strings.
**	 9-Sep-96 (gordy)
**	    Added IIAPI_SP_YEAR_CUTOFF.
**	26-Aug-98 (rajus01)
**	    Added IIAPI_SP_SL_TYPE.
**	04-Sep-98 (rajus01)
**	    Added internal parameter IIAPI_CP_SL_TYPE.
**	15-Mar-04 (gordy)
**	    Added IIAPI_SP_I8WIDTH (sp_mask2) and sp_i8width.
**	 8-Aug-07 (gordy)
**	    Added IIAPI_SP_DATE_ALIAS and sp_date_alias;
*/

typedef struct _IIAPI_SESSIONPARM
{
    II_ULONG	sp_mask1;
# define IIAPI_SP_EXCLUSIVE		0x00000001
# define IIAPI_SP_VERSION		0x00000002
# define IIAPI_SP_APPLICATION		0x00000004
# define IIAPI_SP_QLANGUAGE		0x00000008
# define IIAPI_SP_CWIDTH		0x00000010
# define IIAPI_SP_TWIDTH		0x00000020
# define IIAPI_SP_I1WIDTH		0x00000040
# define IIAPI_SP_I2WIDTH		0x00000080
# define IIAPI_SP_I4WIDTH		0x00000100
# define IIAPI_SP_I8WIDTH		0x00001000	/* sp_mask2 */
# define IIAPI_SP_F4WIDTH		0x00000200
# define IIAPI_SP_F4PRECISION		0x00000400
# define IIAPI_SP_F8WIDTH		0x00000800
# define IIAPI_SP_F8PRECISION		0x00001000
# define IIAPI_SP_SVTYPE		0x00002000
# define IIAPI_SP_NLANGUAGE		0x00004000
# define IIAPI_SP_MPREC			0x00008000
# define IIAPI_SP_MLORT			0x00010000
# define IIAPI_SP_DATE_FORMAT		0x00020000
# define IIAPI_SP_TIMEZONE		0x00040000
# define IIAPI_SP_TIMEZONE_NAME		0x00080000
# define IIAPI_SP_DBNAME		0x00100000
# define IIAPI_SP_IDX_STRUCT		0x00200000
# define IIAPI_SP_RES_STRUCT		0x00400000
# define IIAPI_SP_EUSER			0x00800000
# define IIAPI_SP_MATHEX		0x01000000
# define IIAPI_SP_F4STYLE		0x02000000
# define IIAPI_SP_F8STYLE		0x04000000
# define IIAPI_SP_MSIGN			0x08000000
# define IIAPI_SP_DECIMAL		0x10000000
# define IIAPI_SP_APLID			0x20000000
# define IIAPI_SP_GRPID			0x40000000
# define IIAPI_SP_DECFLOAT		0x80000000

    II_LONG	sp_exclusive;
    II_LONG	sp_version;		/* !unused! */
    II_LONG	sp_application;
    II_LONG	sp_qlanguage;
    II_LONG	sp_cwidth;
    II_LONG	sp_twidth;
    II_LONG	sp_i1width;
    II_LONG	sp_i2width;
    II_LONG	sp_i4width;
    II_LONG	sp_i8width;
    II_LONG	sp_f4width;
    II_LONG	sp_f4precision;
    II_LONG	sp_f8width;
    II_LONG	sp_f8precision;
    II_LONG	sp_svtype;
    II_LONG	sp_nlanguage;
    II_LONG	sp_mprec;
    II_LONG	sp_mlort;
    II_LONG	sp_date_format;
    II_LONG	sp_timezone;
    II_CHAR	*sp_timezone_name;
    II_CHAR	*sp_dbname;
    II_CHAR	*sp_idx_struct;
    II_CHAR	*sp_res_struct;
    II_CHAR	*sp_euser;
    II_CHAR	*sp_mathex;
    II_CHAR	*sp_f4style;
    II_CHAR	*sp_f8style;
    II_CHAR	*sp_msign;
    II_CHAR	*sp_decimal;
    II_CHAR	*sp_aplid;
    II_CHAR	*sp_grpid;
    II_CHAR	*sp_decfloat;

    II_ULONG	sp_mask2;
# define IIAPI_SP_RUPASS		0x00000001
# define IIAPI_SP_MISC_PARM		0x00000002
# define IIAPI_SP_STRTRUNC		0x00000004
# define IIAPI_SP_XUPSYS		0x00000008
# define IIAPI_SP_SUPSYS		0x00000010
# define IIAPI_SP_WAIT_LOCK		0x00000020
# define IIAPI_SP_GW_PARM		0x00000040
# define IIAPI_SP_RESTART		0x00000080
# define IIAPI_SP_XA_RESTART		0x00000100
# define IIAPI_SP_YEAR_CUTOFF		0x00000200
# define IIAPI_SP_SL_TYPE		0x00000400
# define IIAPI_SP_CAN_PROMPT		0x00000800
/*	 IIAPI_SP_I8WIDTH		0x00001000	See above. */
# define IIAPI_SP_DATE_ALIAS		0x00002000

    II_CHAR		*sp_rupass;
    II_LONG		sp_misc_parm_count;	/* non-GCA count for next */
    II_CHAR		**sp_misc_parm;
    II_CHAR		*sp_strtrunc;
    II_LONG		sp_xupsys;
    II_LONG		sp_supsys;
    II_LONG		sp_wait_lock;
    II_LONG		sp_gw_parm_count;	/* non-GCA count for next */
    II_CHAR		**sp_gw_parm;
    II_CHAR		*sp_restart;
    II_CHAR		*sp_xa_restart;
    II_LONG		sp_year_cutoff;
    II_LONG		sp_sl_type;
    II_LONG		sp_can_prompt;
    II_CHAR		*sp_date_alias;
    
} IIAPI_SESSIONPARM;

/*
** The following symbols are additional internal
** connection parameters which may be passed to
** IIapi_setConnParm() in addition to those which
** are defined in iiapi.h for IIapi_setConnectParam().
** All internal parameters must have negative values.
*/
# define IIAPI_CP_DATABASE		(-1)
# define IIAPI_CP_QUERY_LANGUAGE	(-2)
# define IIAPI_CP_RESTART		(-3)
# define IIAPI_CP_TIMEZONE_OFFSET	(-4)
# define IIAPI_CP_SL_TYPE		(-5)
# define IIAPI_CP_CAN_PROMPT		(-6)



/*
** Name: IIAPI_CONNHNDL
**
** Description:
**      This data type defines the connection handle.
**
** History:
**      10-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	14-Nov-94 (gordy)
**	    Generalized the processing of GCA_MD_ASSOC.
**	20-Dec-94 (gordy)
**	    Cleaned up common handle processing.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	28-Jul-95 (gordy)
**	    Used fixed length types.
**	22-Apr-96 (gordy)
**	    Convert ch_msg_continued to ch_flags and add COPY interrupt.
**	12-Jul-96 (gordy)
**	    Enhanced event support with IIapi_getEvent().
**	 2-Oct-96 (gordy)
**	    Added connection type, dropped values stored in connection
**	    parms, simplified function callbacks.
**	31-Jan-97 (gordy)
**	    Added het-net descriptor connection flag for COPY FROM.
**	13-Mar-97 (gordy)
**	    Added back some dropped values (target,username,password)
**	    for Name Server connections which needs manipulate the
**	    values or reference them after the connection is made.
**	14-Oct-98 (rajus01)
**	    Added ch_prmptParm.
**	19-Aug-2002 (wansh01)     
**	    Added IIAPI_CH_LT_LOCAL ch_flags.
**      11-Jan-2005 (wansh01)
**          INGNET 161 bug # 113737
**          A new field ch_target_display is added to the connHndl.
**          This contains target info with supressed login info(user,pwd).
**	25-Mar-10 (gordy)
**	    Added message buffer queues.
**	26-May-10 (gordy)
*8	    Added flag to indicate processing of GCA_TRACE message group.
*/ 

struct _IIAPI_CONNHNDL
{
    IIAPI_HNDL	ch_header;
    
    /*
    ** Connection specific parameters
    */
    IIAPI_ENVHNDL	*ch_envHndl;		/* Parent */
    II_UINT2		ch_type;		/* Connection type */
    char		*ch_target;
    char		*ch_username;
    char		*ch_password;

    II_LONG		ch_connID;		/* returned from GCA_REQUEST */
    II_LONG		ch_sizeAdvise;		/* returned from GCA_REQUEST */
    II_LONG		ch_partnerProtocol;	/* returned from GCA_REQUEST */
    IIAPI_SESSIONPARM	*ch_sessionParm;	/* GCA_MD_ASSOC parm pending */
    II_LONG		ch_cursorID;		/* Cursor ID counter */
    II_BOOL		ch_flags;		/* Connection Flags */

#define IIAPI_CH_GCA_EXP_ACTIVE		0x0001	/* GCA Exp. channel active */
#define IIAPI_CH_INTERRUPT		0x0002	/* COPY Interrupt */
#define IIAPI_CH_MSG_CONTINUED		0x0004	/* GCA Message Continued */
#define IIAPI_CH_DESCR_REQ		0x0008	/* GCA Het-Net descriptors */
#define IIAPI_CH_LT_LOCAL 		0x0010	/* login type is local */
#define	IIAPI_CH_TRACE_ACTIVE		0x0020	/* Processing GCA_TRACE */

    /*
    ** Object queues
    */
    QUEUE		ch_idHndlList;
    QUEUE		ch_tranHndlList;
    QUEUE		ch_dbevHndlList;
    QUEUE		ch_dbevCBList;
    QUEUE		ch_msgQueue;
    QUEUE		ch_rcvQueue;
    
    /*
    ** API Function call info.
    */
    II_BOOL		ch_callback;
    IIAPI_GENPARM	*ch_parm;

    /*
    ** Prompt parameter Block
    */
    II_PTR      ch_prmptParm; 

    /*
    ** target display info 
    */
    char	*ch_target_display;

};


/*
** Some connection types need to manipulate the
** server target info.  The following defines
** the additional amount of space to be allocated
** to hold the target in the connection handle.
*/

# define IIAPI_CONN_EXTRA	16


/*
** Global functions.
*/

II_EXTERN IIAPI_CONNHNDL 	*IIapi_createConnHndl(IIAPI_ENVHNDL *envHndl);

II_EXTERN II_VOID		IIapi_deleteConnHndl(IIAPI_CONNHNDL *connHndl);

II_EXTERN II_BOOL		IIapi_isConnHndl( IIAPI_CONNHNDL *connHndl );

II_EXTERN IIAPI_CONNHNDL	*IIapi_getConnHndl( IIAPI_HNDL *handle );

II_EXTERN IIAPI_STATUS		IIapi_setConnParm( IIAPI_CONNHNDL *connHndl,
						   II_LONG parmID, 
						   II_PTR parmValue );

II_EXTERN II_VOID		IIapi_clearConnParm( IIAPI_CONNHNDL *connHndl );

II_EXTERN IIAPI_IDHNDL		*IIapi_getConnIdHndl( IIAPI_CONNHNDL *connHndl,
						      II_LONG type,
						      GCA_ID *gcaID );


# endif /* __APICHNDL_H__ */
