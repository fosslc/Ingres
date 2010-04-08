/*
**Copyright (c) 2004 Ingres Corporation
**
** History
**	oct-92 (robf)
**	  Created
**   15-sep-93 (smc)
**       Made prototype of gwsxa_scf_session pass CS_SID.
**    1-jul-93 (robf)
**	  Added object label attribute
**    23-dec-93 (robf)
**        Added querytext_sequence info
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    04-dec-2008 (coomi01) b121323
**          Add prototype for gwsxa_rightTrimBlank()
*/

# define	SXA_CURRENT_SAL  "current"

# define 	SXA_CUR_SAL_LEN	 7	/* Characters in SXA_CURRENT_SAL */

/*
**	Maximum number of columns that can be registered through the
**	gateway. This corrosponds to the actual data which might possibly
**	be registered.
**/
# define	SXA_MAX_COLUMNS	14
/*
**	Maximum length of an attribute name [Will need to be increased if
**	we ever go to longer names]
*/
# define SXA_ATTR_NAME_LEN 32

/*
**	Maximum length of an audit file name
*/
# define SXA_MAX_AUDIT_FILE_NAME	256

/*
**	Maximum size of  a privilege mask
*/
# define SXA_MAX_PRIV_LEN	32

/*
** Name: GWSXA_XREL - SXA Extended relation entry
*/
typedef struct _GWSXA_XREL {
	DB_TAB_ID tab_id;
	char	audit_file[SXA_MAX_AUDIT_FILE_NAME];
	i4	regtime;
	i4	flags;
} GWSXA_XREL;

/*
** Name: GWSXA_XATT - SXA Extended attribute entry
*/
typedef struct _GWSXA_XATT {
	DB_TAB_ID xatt_tab_id;
	i2	attid;
	char    attname[SXA_ATTR_NAME_LEN];
	i2	audid;
	char	audname[SXA_ATTR_NAME_LEN];
} GWSXA_XATT;

/*
** Name: GWSXA_RSB  - SXA gateway internal RSB.
**
** Description:
**	This structure is the SXA gateway's private RSB.
**
** History:
**	14-sep-92 (robf)
**	    Created.
*/
typedef struct _GWSXA_RSB
{
	i4	reltid;
	i4	reltidx;
	i4	regtime;
	i4	tups_read;	/* Number of tuples read so far */
	i4	flags;
# define SXA_FLAG_INVALID  	0x0
# define SXA_FLAG_VALID	  	0x1
# define SXA_FLAG_OPEN	  	0x2
# define SXA_FLAG_NODB_REG 	0x4
# define SXA_FLAG_CURRENT  	0x8
# define SXA_FLAG_POSTID   	0x10
# define SXA_FLAG_OBJLABEL_REG  0x20
# define SXA_FLAG_DETAIL_REG    0x40
	PTR	sxf_access_id;		/* cookie passed into SXF */
	DB_NAME	database;		/* CURRENT database name */
} GWSXA_RSB;
/*
**	Builtins for gwsxa_error type
**	This is a bit-map
**/
# define SXA_ERR_INTERNAL 1	/* Internal Error */
# define SXA_ERR_USER	 2	/* User Error */
# define SXA_ERR_LOG	 4	/* Send errors to log as well as user */
/*
**	Size of various relations
*/
# define SXA_XREL_SIZE	sizeof(GWSXA_XREL)
# define SXA_XATTR_SIZE 	sizeof(GWSXA_XATT)
# define SXA_XNDX_SIZE	0	

typedef i4 SXA_ATT_ID;
# define SXA_ATT_INVALID 	-1	/* Invalid atttribute id */
# define	SXA_ATT_AUDITTIME 	0
# define	SXA_ATT_USER_NAME 	1
# define	SXA_ATT_REAL_NAME 	2
# define	SXA_ATT_USERPRIVILEGES	3
# define	SXA_ATT_OBJPRIVILEGES	4
# define	SXA_ATT_DATABASE	5
# define	SXA_ATT_AUDITSTATUS	6
# define	SXA_ATT_AUDITEVENT	7
# define	SXA_ATT_OBJECTTYPE	8
# define	SXA_ATT_OBJECTNAME	9
# define	SXA_ATT_DESCRIPTION	10
# define	SXA_ATT_OBJECTOWNER	11
# define 	SXA_ATT_DETAILINFO     12
# define        SXA_ATT_DETAILNUM      13
# define	SXA_ATT_OBJLABEL       14
# define	SXA_ATT_SESSID	       15
# define        SXA_ATT_QRYTEXTSEQ     16
/*
**	Message Cache structure defintions
**
**	These are used when converting from message ids to external
**	descriptions
*/
# define SXA_MAX_DESC_LEN 128	/* Max length of a description */

typedef struct _SXA_MSG_DESC {
	i4	msgid;
	char    db_data[SXA_MAX_DESC_LEN+1];	
	i4	db_length;
	struct  _SXA_MSG_DESC *left,*right;
} SXA_MSG_DESC;

/*
**	Conversion errors. These really should be in gwf.h but are
**	defined locally by the RMS gateway, so don't want to duplicate.
**
*/

#ifndef E_GWF_MASK
# define		E_GWF_MASK			(DB_GWF_ID << 16)
#endif
#ifndef E_GW7000_INTEGER_OVF
# define		E_GW7000_INTEGER_OVF		(E_GWF_MASK + 0x7000)
# define		E_GW7001_FLOAT_OVF		(E_GWF_MASK + 0x7001)
# define		E_GW7002_FLOAT_UND		(E_GWF_MASK + 0x7002)
#endif
/*
**	Function prototypes
*/
DB_STATUS gwsxa_term	(GWX_RCB *gwx_rcb);
DB_STATUS gwsxa_tabf	(GWX_RCB *gwx_rcb);
DB_STATUS gwsxa_idxf	(GWX_RCB *gwx_rcb);
DB_STATUS gwsxa_open	(GWX_RCB *gwx_rcb);
DB_STATUS gwsxa_close	(GWX_RCB *gwx_rcb);
DB_STATUS gwsxa_position(GWX_RCB *gwx_rcb);
DB_STATUS gwsxa_get	(GWX_RCB *gwx_rcb);
DB_STATUS gwsxa_put	(GWX_RCB *gwx_rcb);
DB_STATUS gwsxa_replace	(GWX_RCB *gwx_rcb);
DB_STATUS gwsxa_delete	(GWX_RCB *gwx_rcb);
DB_STATUS gwsxa_begin	(GWX_RCB *gwx_rcb);
DB_STATUS gwsxa_commit	(GWX_RCB *gwx_rcb);
DB_STATUS gwsxa_abort	(GWX_RCB *gwx_rcb);
DB_STATUS gwsxa_info	(GWX_RCB *gwx_rcb);
DB_STATUS gwsxa_init	(GWX_RCB *gwx_rcb);

VOID 	  gwsxa_incr(CS_SEMAPHORE *sem, i4 *counterp );
VOID      gwsxa_error ();

DB_STATUS	gwsxa_priv_user(i4);
DB_STATUS 	gwsxachkrcb( GWX_RCB *gwx_rcb, char *routine);
SXA_ATT_ID  	gwsxa_find_attbyname(char *attname);
char*		gwsxa_find_attbyid(SXA_ATT_ID attid);
DB_STATUS	gwsxa_scf_session(CS_SID *where);
VOID		gwsxa_zapblank(char *str,i4 maxlen);
VOID		gwsxa_rightTrimBlank(char *str,i4 maxlen);
DB_STATUS	gwsxa_sxf_to_ingres(GWX_RCB *gwx_rcb,SXF_AUDIT_REC*sxf_record);
DB_STATUS 	gwsxa_msgid_to_desc(i4 msgid, DB_DATA_VALUE *desc);
DB_STATUS	gwsxa_find_msgid(i4 msgid,DB_DATA_VALUE *desc) ;

/*
**	Structure to hold SXA gateway statistics 
*/
typedef struct {
	i4 sxa_num_register;
	i4 sxa_num_reg_fail;
	i4 sxa_num_open;
	i4 sxa_num_close;
	i4 sxa_num_get;
	i4 sxa_num_open_fail;
	i4 sxa_num_close_fail;
	i4 sxa_num_get_fail;
	i4 sxa_num_update_tries;
	i4 sxa_num_position_keyed;
	i4 sxa_num_reg_index_tries;
	i4 sxa_num_msgid;
	i4 sxa_num_msgid_fail;
	i4 sxa_num_msgid_lookup;
	i4 sxa_num_tid_get;
	i4 sxa_num_tid_scanstart;
} GWSXASTATS;




