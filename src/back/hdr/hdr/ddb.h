/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DDB.H - Types common to all components of STAR.
**
** Description:
**      Types common to and used by all STAR components:
**
**	    1)  This file depends on dbms.h and hence compat.h.
**	    2)  All type names start with the prefix "DD_".
**	    3)  All field names start with the prefix "dd_".
**
**
** History: $Log-for RCS$
**      24-apr-88 (carl)
**          written
**      14-jan-89 (carl)
**	    changed dd_t9_ldb_p of DD_2LDB_TAB_INFO to use DD_1LDB_INFO,
**	    changed dd_p1_dba_b of DD_0LDB_PLUS to dd_p1_character
**      22-jan-89 (carl)
**	    added dd_c8_owner_name and defines to DD_CAPS
**      20-feb-89 (carl)
**	    added DD_MODIFY for modifying LDB association parameters,
**	    deleted unused DD_SET_DESC
**      07-apr-89 (georgeg)
**	    added DD_COM_FLAGS. 
**      09-apr-89 (carl)
**	    added DD_201CAP_DIFF_ARCH, a define for dd_c2_ldb_caps of DD_CAPS
**      21-oct-89 (carl)
**	    added DD_DX_ID for 2PC
**      06-jun-90 (carl)
**	    added DD_01FLAG_1PC for DD_LDB_DESC for 2PC
**      23-jun-90 (carl)
**	    added DD_03FLAG_SLAVE2PC for DD_LDB_DESC for SLAVE 2PC capability
**      23-sep-90 (carl)
**	    extended DD_0LDB_PLUS to include new define DD_3CHR_SYS_NAME for
**	    dd_p1_character, and 2 new fields dd_p5_usr_name and dd_p6_sys_name
**	25-sep-90 (fpang)
**	    Fixed syntax errors for DD_NODENAMES, and DD_CLUSTER_INFO. 
**	12-aug-91 (fpang)
**	    Added dd_co32_gw_parms, dd_co33_len_gw_parms and dd_co31_timezone 
**	    to DD_COM_FLAGS to support GCA_GW_PARMS and GCA_TIMEZONE.
**      14-jul-92 (fpang)
**          Fixed compiler warnings, DD_PACKET.
**      23-sep-92 (stevet)
**	    Added new field dd_co34_tz_name to support new timezone 
**	    message GCA_TIMEZONE_NAME
**	04-dec-92 (tam)
**	    Defined DD_REGPROC_DESC to support registered dbproc in star.
**	13-jan-1993 (fpang)
**	    Added DD_04FLAG_TMPTBL flag for DD_LDB_DESC.dd_l6_flags for
**	    temp table support.
**	01-mar-93 (barbara)
**	    Delimited id support.  Added new fields to DD_CAPS structure.
**      12-aug-1993 (stevet)
**          Added dd_co35_strtrunc to support GCA_STRTRUNC.
**	16-sep-1993 (fpang)
**	    To support the -G (GCA_GRPID) and -R (GCA_APLID) flags, added 
**	    dd_co36_grpid and dd_co37_aplid to DD_COM_FLAGS.
**	11-oct-1993 (fpang)
**	    Added dd_co38_decformat to DD_COM_FLAGS to support -kf 
**	    (GCA_DECFLOAT).
**	    Fixes B55510 (Star server doesn't recognize -kf flag).
**	22-nov-93 (robf)
**          Add  dd_co40_usrpwd to DD_COM_FLAGS to support  GCA_RUPASS
**	13-nov-95 (nick)
**	    Add dd_co39_year_cutoff to DD_COM_FLAGS to support GCA_YEAR_CUTOFF.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-jan-2002 (toumi01)
**	    Replace DD_300_MAXCOLUMN with DD_MAXCOLUMN (1024).
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/


#ifdef	    DDB_INCLUDED

#else
 

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct	_DD_PACKET  DD_PACKET;


/*
**  Forward and/or External function references.
*/


/*
**  Defines of STAR system constants
*/

#define	    DD_1_IS_CDB_ID	    1
#define	    DD_25_DATE_SIZE	    25
#define	    DD_256_MAXDBNAME	    256
#define	    DD_MAXCOLUMN	    1024


/*}
** Name: DD_ACCESS - Access status of a DB.
**
** Description:
**	Codes denoting access status of a DDB as provided by the STATUS
**  field of IIDATABASE in IIDBDB.
**
**
** History:
**      24-apr-88 (carl)
**          written
*/

typedef i4  DD_ACCESS;

#define     DD_1ACC_GLOBAL	(DD_ACCESS) 0x0001  /* DB is globally 
						    ** accessible */
#define     DD_2ACC_DESTROYDB	(DD_ACCESS) 0x0004  /* DB is in process of
						    ** being destroyed */
#define     DD_3ACC_CREATEDB	(DD_ACCESS) 0x0008  /* DB is in process of
						    ** being created */
#define     DD_4ACC_OPERATIVE	(DD_ACCESS) 0x0010  /* DB is operational */
#define     DD_5ACC_CONVERTING	(DD_ACCESS) 0x0020  /* DB has not been
						    ** successfully converted 
						    ** yet */

/*}
** Name: DD_DBSERVICE - Services provided by a DB.
**
** Description:
**	Codes denoting services of a DB as provided by the DBSERVICE
**  field of IIDATABASE in IIDBDB.
**
**
**
** History:
**      24-apr-88 (carl)
**          written
*/

typedef i4  DD_DBSERVICE;

#define     DD_1DBS_SER_DDB	(DD_DBSERVICE) 0x0001	/* DB is a distributed 
							** database */
#define     DD_2DBS_SER_CDB	(DD_DBSERVICE) 0x0002	/* DB is a coordinator
							** database */

/*}
** Name: DD_QMODE - DD query modes.
**
** Description:
**	The possible query modes.
**
**
** History:
**      24-apr-88 (carl)
**          written
*/

typedef i4  DD_QMODE;

#define		DD_0MODE_NONE	    (DD_QMODE) 0
					/* not a query */
#define         DD_1MODE_READ	    (DD_QMODE) 1
					/* a read query */
#define		DD_2MODE_UPDATE	    (DD_QMODE) 2
					/* an update query */


/*}
** Name: DD_OBJ_TYPE - Object type.
**
** Description:
**	The possible object types.
**
**
** History:
**      24-apr-88 (carl)
**          written
**	11-nov-92 (teresa)
**	    added DD_5OBJ_REG_PROC for registered procedures, also
**	    added DD_6OBJ_PROCEDURE for distributred procedures (not used yet)
*/

typedef i4  DD_OBJ_TYPE;

#define		DD_0OBJ_NONE	    (DD_OBJ_TYPE) 0
					/* no type */
#define         DD_1OBJ_LINK	    (DD_OBJ_TYPE) 1
					/* link */
#define		DD_2OBJ_TABLE	    (DD_OBJ_TYPE) 2
					/* table */
#define		DD_3OBJ_VIEW	    (DD_OBJ_TYPE) 3
					/* view */
#define		DD_4OBJ_INDEX	    (DD_OBJ_TYPE) 4
					/* index */
#define		DD_5OBJ_REG_PROC    (DD_OBJ_TYPE) 5
					/* registered procedure */
#define		DD_6OBJ_PROCEDURE   (DD_OBJ_TYPE) 6
					/* distributed procedure */

/*}
** Name: DD_TV_TYPE - Tree representation type.
**
** Description:
**	Codes denoting the types of tree representation.
**
**
** History:
**      07-nov-88 (carl)
**          written
*/

typedef i2  DD_TV_TYPE;

#define     DD_0TV_UNKNOWN	(DD_TV_TYPE)	0   /* unknown representation */
#define     DD_1TV_VAX		(DD_TV_TYPE)	1   /* VAX binary */



/*}
** Name: DD_DX_ID - A distributed transaction id.
**
** Description:
**      This construct contains 2 4-byte integers derived from the system time
**	obtained via TMnow().
**
** History:
**      21-oct-89 (carl)
**          written
*/

typedef struct	_DD_DX_ID
{
    i4	dd_dx_id1;			/* 1st part of id */
    i4	dd_dx_id2;			/* 2nd part of id */
    char	dd_dx_name[DB_DB_MAXNAME];	/* 3rd part of id */
}   DD_DX_ID;



/*}
** Name: DD_NAME - A name construct.
**
** Description:
**      This construct contains exactly DB_MAXNAME characters, 
**	including trailing blanks.
**
** History:
**      24-apr-88 (carl)
**          written
*/

typedef char            DD_NAME[DB_MAXNAME];

typedef char            DD_DB_NAME[DB_DB_MAXNAME];
typedef char            DD_LOC_NAME[DB_LOC_MAXNAME];
typedef char            DD_TAB_NAME[DB_TAB_MAXNAME];
typedef char            DD_OWN_NAME[DB_OWN_MAXNAME];
typedef char            DD_ATT_NAME[DB_ATT_MAXNAME];
typedef char            DD_DBP_NAME[DB_DBP_MAXNAME];
typedef char            DD_OBJ_NAME[DB_OBJ_MAXNAME];
typedef char            DD_NODE_NAME[DB_NODE_MAXNAME];

/* type value, not a name */
typedef char            DD_TYPE_VAL[DB_TYPE_MAXLEN];


/*}
** Name: DD_COLUMN_DESC - A column descriptor.
**
** Description:
**      This structure contains the name of a column and its ordinal within 
**  its table.
**
** History:
**      24-apr-88 (carl)
**          written
*/

typedef struct	_DD_COLUMN_DESC
{
    DD_ATT_NAME	dd_c1_col_name;			/* name of column */
    i4		dd_c2_col_ord;			/* ordinal of column */
}   DD_COLUMN_DESC;


/*}
** Name: DD_DATE - A 25-character array.
**
** Description:
**      This structure defines a 25-character date type.
**
** History:
**      24-apr-88 (carl)
**          written
*/

typedef char	DD_DATE[DD_25_DATE_SIZE];


/*}
** Name: DD_CAPS - Capability structure for LDB.
**
** Description:
**      This structure contains existence information about each known 
**  and relevant capability for an LDB/Gateway.  Note that AUTOCOMMIT 
**  is a STAR function and hence it is not kept for an LDB.
**
**
** History:
**      24-apr-88 (carl)
**          written
**      22-jan-89 (carl)
**	    added dd_c8_owner_name and defines 
**      09-apr-89 (carl)
**	    added DD_201CAP_DIFF_ARCH, a define for dd_c2_ldb_caps of DD_CAPS
**      13-apr-89 (mings)
**	    added DD_7CAP_USE_PHYSICAL_SOURCE for iiphysical_tables
**	01-mar-93 (barbara)
**	    Delimited id support.  Added new fields.
**	02-sep-93 (barbara)
**	    1. Added new field for holding DB_REAL_USER_CASE capability.
**	    2. Added new bit DD_8CAP_DELIMITED_ID to dd_c2_ldb_caps field.
**	       This bit is set if the LDB supports delimited identifiers.
**	06-mar-96 (nanpr01)
**	    Added new define for new sql level.
**	28-Jan-2009 (kibro01) b121521
**	    Add 910 level to determine ingres date capability
*/

typedef struct _DD_CAPS
{
    i4	dd_c1_ldb_caps;		    /* boolean-type LDB caps */
    
#define	    DD_1CAP_DISTRIBUTED	    0x0001L /* ON if the LDB is managed by STAR,
					    ** OFF otherwise */
#define	    DD_2CAP_INGRES	    0x0002L /* ON if (full) INGRES is supported,
					    ** including such things as TIDS,
					    ** OFF otherwise */
#define	    DD_3CAP_SAVEPOINTS	    0x0004L /* ON if SAVEPOINTS are supported,
					    ** OFF otherwise */
#define	    DD_4CAP_SLAVE2PC	    0x0008L /* ON if SLAVE2PC is supported,
					    ** OFF otherwise */
#define	    DD_5CAP_TIDS	    0x0010L /* ON if TIDS are supported,
					    ** OFF otherwise */
#define	    DD_6CAP_UNIQUE_KEY_REQ  0x0020L /* ON if all LDB tables require a
					    ** unique key, OFF otherwise */
#define	    DD_7CAP_USE_PHYSICAL_SOURCE 0x0040L 
					    /* ON if PHYSICAL_SOURCE capability
					    ** is 'P' which means physical
					    ** information is contained only
					    ** in the LDB's iiphysical_tables */
#define	    DD_8CAP_DELIMITED_IDS   0x0080L /* ON if the LDB supports delimited
					    ** ids, i.e. the DB_DELIMITED_CASE
					    ** capability is present */

    i4	dd_c2_ldb_caps;		    /* boolean-type LDB caps continued,
					    ** (reserved for future use) */
#define	    DD_0CAP_PRE_602_ING_SQL 0x0001L /* ON if INGRES/SQL_LEVEL is
					    ** 600 or 601 */
#define	    DD_201CAP_DIFF_ARCH	    0x0002L /* ON if the LDB has a machine 
					    ** architecture different from
					    ** Titan's */

    i4	dd_c3_comsql_lvl;	    /* level of COMMON/SQL supported,
					    ** 0 if not supported */
    i4	dd_c4_ingsql_lvl;	    /* level of INGRES/SQL supported,
					    ** 0 if not supported */
#define     DD_605CAP_LEVEL     605         /*  Ingres 65 level */
#define     DD_606CAP_LEVEL     606         /*  Ingres 66 level - Var Page Sz */
#define     DD_910CAP_LEVEL     910         /*  Ingres 910 level - ingresdate */

    i4	dd_c5_ingquel_lvl;	    /* level of INGRES/QUEL supported,
					    ** 0 if not supported */
    i4		dd_c6_name_case;	    /* regular (undelimited) object
					    ** name case support: one
					    ** of the following defines */

#define	    DD_0CASE_LOWER	0	    /* names are lower-cased */
#define	    DD_1CASE_MIXED	1	    /* names may be mixed case */ 
#define	    DD_2CASE_UPPER	2	    /* names are upper-cased */

    char 	dd_c7_dbms_type[DB_TYPE_MAXLEN]; /* a DBMS name */
    i4		dd_c8_owner_name;	    /* owner name support: one of
					    ** the following defines */

#define	    DD_0OWN_NO		0	    /* <owner>.<table> NOT allowed */ 
#define	    DD_1OWN_YES		1	    /* <owner>.<table> allowed */ 
#define	    DD_2OWN_QUOTED	2	    /* '<owner>'.<table> for QUEL or
					    ** "<owner>".<table> for SQL 
					    ** allowed */ 

    i4		dd_c9_opensql_level;	    /* level of OPEN/SQL supported */
    i4		dd_c10_delim_name_case;	    /* delimited object name case
					    ** support: one
					    ** of the above defines for
					    ** dd_c6_name_case */
    i4		dd_c11_real_user_case;	    /* real user name case support:
					    ** one of the above defines for
					    ** dd_c6_name_case */
}   DD_CAPS;


/*}
** Name: DD_256C - A 256-character construct.
**
** Description:
**      This construct contains exactly 256 characters, including trailing 
**  blanks.
**
** History:
**      24-apr-88 (carl)
**          written
*/

typedef char            DD_256C[DD_256_MAXDBNAME];	/* 256 chars */


/*}
** Name: DD_PACKET - A query text or message.
**
** Description:
**      This structure defines a query text or temporary table consisting of 
**  one or more (sub)packets.  If dd_p2_pkt_p is NULL, a temporary table name
**  is specified and it is picked from a temporary table name array to be
**  constructed by QEF.
**
** History:
**      24-apr-88 (carl)
**          written
**      01-dec-88 (carl)
**          added new field dd_p4_slot
**      14-jul-92 (fpang)
**          Removed typedef, it's forward declared above.
*/

struct _DD_PACKET
{
    i4	     dd_p1_len;	    /* length of text or temporary table name */
    char	    *dd_p2_pkt_p;   /* ptr to packet or subpacket, NULL ==>
				    ** a temporary table name takes the place
				    ** of text */
    DD_PACKET	    *dd_p3_nxt_p;   /* ptr to next subpacket, NULL if none */
    i4		     dd_p4_slot;    /* temporary table id >= 0, set to -1 or
				    ** DD_NIL_SLOT if not applicable */
#define	DD_NIL_SLOT	-1

}   ;


/*}
** Name: DD_QEF_LEVEL - QEF DDB level of capability.
**
** Description:
**	Describes DDB level of capability of QEF.
**
** History:
**      24-apr-88 (carl)
**          written
*/

typedef i4  DD_QEF_LEVEL;

#define		DD_0QEF_DDB	(DD_QEF_LEVEL) 0   /* no DDB capability */
#define         DD_1QEF_DDB	(DD_QEF_LEVEL) 1   /* Titan DDB capability */


/*}
** Name: DD_MODIFY - Structure for modifying LDB association parameters
**
** Description:
**      Descriptor for passing information from SCF thru QEF to RQF for
**  the purpose of modifying LDB association parameters
**
**
** History:
**      20-feb-89 (carl)
**	    written
*/

#define	DD_0MOD_NONE		(i4) 0	    /* none */
#define	DD_1MOD_ON		(i4) 1	    /* + */
#define	DD_2MOD_OFF		(i4) 2	    /* - */

typedef struct _DD_MODIFY
{
    i4	     dd_m1_u_flag;		    /* +/- U or none, i.e. one of
					    ** DD_0MOD_NONE, DD_1MOD_ON or
					    ** DD_2MOD_OFF */
    i4	     dd_m2_y_flag;		    /* +/- Y or none, i.e. one of
					    ** DD_0MOD_NONE, DD_1MOD_ON or
					    ** DD_2MOD_OF */
    i4	     dd_m3_appval;		    /* application value */
    
}   DD_MODIFY;



/*}
** Name: DD_COM_FLAGS - Structure to transfer command flags to the LDB.
**
** Description:
**      Descriptor for passing information from SCF thru QEF to RQF for
**  the purpose of modifying LDB association parameters during session 
**  startup.
**
**
** History:
**      07-apr-89 (georgeg)
**	    written
**	12-aug-91 (fpang)
**	    Added dd_co32_gw_parms, dd_co33_len_gw_parms and dd_co31_timezone 
**	    to support GCA_GW_PARMS and GCA_TIMEZONE.
**      12-aug-1993 (stevet)
**          Added dd_co35_strtrunc_opt to support GCA_STRTRUNC_OPT.
**	16-sep-1993 (fpang)
**	    Added dd_co36_grpid and dd_co37_aplid  to support -G (GCA_GRPID) 
**	    and -R (GCA_APLID) :
**	11-oct-1993 (fpang)
**	    Added dd_co38_decformat to support -kf (GCA_DECFLOAT).
**	    Fixes B55510 (Star server doesn't recognize -kf flag).
**	22-nov-93 (robf)
**          Add  dd_co40_usrpwd to DD_COM_FLAGS to support GCA_RUPASS
**	13-nov-95 (nick)
**	    Add dd_co39_year_cutoff to support GCA_YEAR_CUTOFF.
**	12-apr-04 (inkdo01)
**	    Add dd_co41_i8width for BIGINT support.
**	03-aug-2007 (gupsh01)
**	    Added dd_co42_date_alias for date_alias support.
**	24-aug-2007 (toumi01) bug 119015
**	    Correct dd_co42_date_alias from i4 to char.
*/
typedef struct _DD_COM_FLAGS
{
    i4	    dd_co0_active_flags;		/* no of flags received/ GCA_MD_ASSOC */
    bool    dd_co1_exlusive;			/* GCA_AXCLUSIVE */
    i4	    dd_co2_application;			/* GCA_APPLICATION */
    i4	    dd_co3_qlanguage;			/* GCA_QLANGUAGE */
    i4	    dd_co4_cwidth;			/* GCA_CWIDTH */
    i4	    dd_co5_twidth;			/* GCA_TWIDTH */
    i4	    dd_co6_i1width;			/* GCA_I1WIDTH */
    i4	    dd_co7_i2width;			/* GCA_I2WIDTH */
    i4	    dd_co8_i4width;			/* GCA_I4WIDTH */
    i4	    dd_co9_f4width;			/* GCA_F4WIDTH */
    i4	    dd_co10_f4precision;		/* GCA_F4PRECISION */
    i4	    dd_co11_f8width;			/* GCA_F8WIDTH */
    i4	    dd_co12_f8precision;		/* GCA_F8PRECISION */
    i4	    dd_co13_nlanguage;			/* GCA_NLANGUAGE */
    i4	    dd_co14_mprec;			/* GCA_MPREC */
    i4	    dd_co15_mlort;			/* GCA_MLORT */
    i4	    dd_co16_date_frmt;			/* GCA_DATE_FORMAT */
    char    dd_co17_idx_struct[32];		/* GCA_IDX_STRUCT */
    u_i2    dd_co18_len_idx;			/* length of previous */
    char    dd_co19_res_struct[32];		/* GCA_RES_STRUCT */
    u_i2    dd_co20_len_res;			/* length of previous */
    char    dd_co21_euser[DB_OWN_MAXNAME];		/* GCA_EUSER */
    u_i2    dd_co22_len_euser;			/* length of previous */
    char    dd_co23_mathex[4];			/* GCA_MATHEX */
    char    dd_co24_f4style[4];			/* GCA_F4STYLE */
    char    dd_co25_f8style[4];			/* GCA_F8STYLE */
    char    dd_co26_msign[DB_MAXMONY+1];	/* GCA_MSIGN */
    i4	    dd_co27_decimal;			/* GCA_DECIMAL */
    i4	    dd_co28_xupsys;			/* GCA_XUPSYS */
    i4	    dd_co29_supsys;			/* GCA_SUPSYS */
    i4	    dd_co30_wait_lock;			/* GCA_WAIT_LOCK */
    i4	    dd_co31_timezone;			/* GCA_TIMEZONE */
    PTR	    dd_co32_gw_parms;			/* GCA_GW_PARMS */
    i4	    dd_co33_len_gw_parms;		/* length of previous */
    char    dd_co34_tz_name[32];		/* GCA_TIMEZONE_NAME */
    char    dd_co35_strtrunc[4];                /* GCA_STRTRUNC  */
    char    dd_co36_grpid[ DB_OWN_MAXNAME  	/* GCA_GRPID	 */
			  +sizeof(DB_PASSWORD)	/* Password	 */
			  +2 ];			/* '/' and EOS   */
    char    dd_co37_aplid[ DB_OWN_MAXNAME 	/* GCA_APLID	 */
			  +sizeof(DB_PASSWORD)	/* Password	 */
			  +2 ];			/* '/' and EOS   */
    char    dd_co38_decformat[4];		/* GCA_DECFORMAT */
    i4	    dd_co39_year_cutoff;		/* GCA_YEAR_CUTOFF */
    char    dd_co40_usrpwd[sizeof(DB_PASSWORD)+2];/* GCA_RUPASS */
    i4	    dd_co41_i8width;			/* GCA_I8WIDTH */
    char    dd_co42_date_alias[DB_TYPE_MAXLEN];	/* GCA_DATE_ALIAS */
} DD_COM_FLAGS;

/*}
** Name: DD_USR_DESC - DDB user descriptor 
**
** Description:
**      This structure defines a user's name and password at the DDB level.
**
** History:
**      24-apr-88 (carl)
**          written
*/

typedef struct _DD_USR_DESC
{
    DD_OWN_NAME	dd_u1_usrname;		    /* user name */
    bool	dd_u2_usepw_b;		    /* TRUE if password is
					    ** known, FALSE otherwise */
    DD_256C	dd_u3_passwd;		    /* user password */
    i4		dd_u4_status;		    /* from column STATUS of
					    ** IIUSER, 0 if unknown */
#define		DD_0STATUS_UNKNOWN	(i4) 0

}   DD_USR_DESC;

/*}
** Name: DD_NODENAMES - NODE NAMES
**
** Description:
**      This structure contains a linked list of null terminated node names.
**	These node names are read from II_CONFIG:cluster.cnf at server
**	startup by RDF.
**
** History:
**      24-sep-90 (teresa)
**	    created for TITAN.
*/
typedef struct _DD_NODENAMES
{
    struct _DD_NODENAMES  *dd_nextnode;   /* ptr to next node name */
    DD_NODE_NAME          dd_nodename;   /* null terminated node name*/
} DD_NODENAMES;

/*}
** Name: DD_CLUSTER_INFO - describes the nodes in the cluster.
**
** Description:
**      This structure contains the number of nodes in the cluster
**	and a pointer to a linked list of null terminated node names.
**	These node names are read from II_CONFIG:cluster.cnf at server
**	startup by RDF, and the count is set to the number of nodes names 
**	in the linked list.
**
** History:
**      24-sep-90 (teresa)
**	    created for TITAN.
*/
typedef struct _DD_CLUSTER_INFO
{
    i4              dd_node_count;  /* # nodes in this installation */
    DD_NODENAMES   *dd_nodenames;   /* linked list of null terminated
				    ** node names.  There will be
				    ** dd_node_count names in the list
				    */
} DD_CLUSTER_INFO;

/*}
** Name: DD_NETCOST - list of network link costs
**
** Description:
**      This structure is used to describe the contents of the ii_netcost 
**      table to the optimizer.
[@comment_line@]...
**
** History:
**      14-apr-89 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
typedef struct _DD_NETCOST
{
    DD_NODE_NAME    net_source;		/* source node name */
    DD_NODE_NAME    net_dest;           /* destination node name */
    f8              net_cost;           /* cost of moving one byte
					** from source to destination
					** as a fraction of 1 DIO */
    f8              net_exp1;           /* expansion */
    f8              net_exp2;		/* expansion */
} DD_NETCOST;

/*}
** Name: DD_NETLIST - linked list of DD_NETCOST tuples
**
** Description:
**      This is an element of an ordered list of node names which 
**      describe the ii_netcost table, it should be ordered by
**      net_source, net_dest, nodes.
**
** History:
**      14-apr-89 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _DD_NETLIST
{
    struct _DD_NETLIST     *net_next;	/* next element or NULL */

    DD_NETCOST      netcost;            /* contents of ii_netcost tuple */
} DD_NETLIST;

/*}
** Name: DD_COSTS - relative power of individual nodes
**
** Description:
**      This structure describes a tuple from the ii_cpucost relation 
**      which contains the relative power of CPU's available to star 
**
** History:
**      14-apr-89 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _DD_COSTS
{
    DD_NODE_NAME    cpu_name;		/* node name of CPU */
    f8              cpu_power;          /* as a relative multiple where
					** 1 is the default */
    f8              dio_cost;           /* relative multiple of I/O speed
					** where 1.0 is default */
    f8              create_cost;        /* number of DIO's required to
					** create a temporary table where
					** 16.0 is the default */
    i4		    page_size;		/* number of bytes read in a
					** single disk i/o, default is
					** 2008 */
    i4		    block_read;		/* given a scan, how many pages
					** are read in a block read 
					** default is 8 */
    i4		    sort_size;		/* number of bytes used for an
					** in memory sort, as opposed
					** to a disk based sort, the
					** default is 16K */
    i4		    cache_size;		/* number of pages available
					** per thread in the memory
					** manager cache */
    f8		    cpu_exp0;		/* expansion init to 0.0 */
    f8              cpu_exp1;           /* expansion init to 0.0 */
    f8              cpu_exp2;           /* expansion init to 0.0 */
} DD_COSTS;

/*}
** Name: DD_NODELIST - list of cost tuples
**
** Description:
**      This is an element of an ordered list of node names which 
**      describe the ii_cpucost table, it should be ordered by cpu_name. 
[@comment_line@]...
**
** History:
**      14-apr-89 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _DD_NODELIST
{
    struct _DD_NODELIST	*node_next;	/* next element or NULL */
    DD_COSTS		nodecosts;	/* contents of ii_cpucost tuple */
} DD_NODELIST;

/*}
** Name: DD_LDB_DESC - Descriptor of an LDB.
**
** Description:
**      Descriptor containing access information of an LDB.
**
** History:
**      24-apr-88 (carl)
**          written
**	13-jan-1993 (fpang)
**	    Added DD_04FLAG_TMPTBL flag for DD_LDB_DESC.dd_l6_flags for
**	    temp table support.
**	28-Jan-2009 (kibro01) b121521
**	    Added DD_05FLAG_INGRESDATE for ingresdate support.
*/

typedef struct _DD_LDB_DESC
{
    bool	    dd_l1_ingres_b;	/* TRUE if LDB access requires the 
					** $ingres status, FALSE otherwise */
    DD_NODE_NAME    dd_l2_node_name;	/* node name */
    DD_256C	    dd_l3_ldb_name;	/* LDB name */
    char	    dd_l4_dbms_name[DB_TYPE_MAXLEN];/* dbms name, e.g. INGRES */
    i4	    dd_l5_ldb_id;	/* assigned LDB id, DD_0_IS_UNASSIGNED
					*/
#define	    DD_0_IS_UNASSIGNED	    0

    i4	    dd_l6_flags;	/* flags */

#define	    DD_00FLAG_NIL    0x0000	
#define	    DD_01FLAG_1PC    0x0001	/* ON if this CDB has no 2PC catalogs,
					** otherwise, used between QEF and 
					** TPF */
#define	    DD_02FLAG_2PC    0x0002	/* ON if this is a CDB association 
					** for 2PC catalogs, OFF otherwise,
					** used between TPF and RQF */
#define	    DD_03FLAG_SLAVE2PC   0x0004	/* ON if this LDB supports the SLAVE
					** 2PC protocol, OFF otherwise,
					** used between QEF and TPF */
#define	    DD_04FLAG_TMPTBL	 0x0008	/* On if LDB supports temp tables. */
#define	    DD_05FLAG_INGRESDATE 0x0010	/* On LDB supports "ingresdate". */
}   DD_LDB_DESC;


/*}
** Name: DD_0LDB_PLUS - DBA information and capabilities of an LDB.
**
** Description:
**      Descriptor containing the DBA information and encoded capabilities of 
**  an LDB.  The information of such an LDB descriptor is filled in by QEF 
**  by querying the LDB at the request of RDF.
**
** History:
**      24-apr-88 (carl)
**          written
**      14-jan-89 (carl)
**	    changed dd_p1_dba_b to dd_p1_character
**      23-sep-90 (carl)
**	    extended DD_0LDB_PLUS to include new define DD_3CHR_SYS_NAME for
**	    dd_p1_character, and 2 new fields dd_p5_usr_name and dd_p6_sys_name
**	06-mar-96 (nanpr01)
**	    Added the fields to store max tuple length, pagesize and diff
**	    page size and its tuple size that is available in the installation.
**	    Though this is really a installation specific information, however
**	    the way star works and the way new var. page size project has
**	    defined the buffer manager, we have to get this info, each time
**	    we come across a new node.
*/

typedef struct _DD_0LDB_PLUS
{
    i4	dd_p1_character;	/* characteristics of LDB */

#define	DD_1CHR_DBA_NAME	0x0001L	/* ON if notion of DBA name is
					** supported, OFF otherwise */
#define	DD_2CHR_USR_NAME	0x0002L	/* ON if notion of user name is
					** supported, OFF otherwise */
#define DD_3CHR_SYS_NAME	0x0004L /* ON if system name exists, OFF 
					** otherwise */
  
    DD_OWN_NAME	dd_p2_dba_name;		/* DBA name, valid if such notion
					** is supported */
    DD_CAPS	dd_p3_ldb_caps;		/* encoded LDB capabilities */
    DD_DB_NAME	dd_p4_ldb_alias;	/* same as dd_l3_ldb_name in the
					** dd_i1_ldb_desc portion if the LDB
					** name has no more than DB_DB_MAXNAME 
					** characters, otherwise a derived
					** DB_DB_MAXNAME character alias */
    DD_OWN_NAME dd_p5_usr_name;		/* from the LDB's 
					** iidbconstants.user_name */
    DD_OWN_NAME dd_p6_sys_name;	        /* from LDB's iidbconstants.system_name 
					** if DD_3CHR_SYS_NAME above is ON,
					** blank-filled otherwise */
    i4     dd_p7_maxtup;		/* LDB's max tuple size   */ 
    i4     dd_p8_maxpage;		/* LDB's max page  size   */ 
    i4     dd_p9_pagesize[DB_MAX_CACHE];		
					/* LDB's available page sizes */ 
    i4     dd_p10_tupsize[DB_MAX_CACHE];		
					/* LDB's available tuple sizes */ 
}   DD_0LDB_PLUS;



/*}
** Name: DD_1LDB_INFO - Descriptor, DBA information and capabilities of an LDB.
**
** Description:
**      Descriptor containing detailed information of an LDB.
**
** History:
**      24-apr-88 (carl)
**          written
*/

typedef struct _DD_1LDB_INFO
{
    DD_LDB_DESC	    dd_i1_ldb_desc;	/* identity information */
    DD_0LDB_PLUS    dd_i2_ldb_plus;	/* DBA information and capabilities */
}   DD_1LDB_INFO;



/*}
** Name: DD_2LDB_TAB_INFO - LDB table information.
**
** Description:
**      This structure defines the complete information of an LDB table
**  It includes the schema of the iidd_ddb_tabinfo catalog and an LDB
**  descriptor pointer.
**
**
** History:
**      24-apr-88 (carl)
**          written
**      14-jan-89 (carl)
**	    changed dd_t9_ldb_p to use DD_1LDB_INFO
*/

typedef struct _DD_2LDB_TAB_INFO
{
    DD_TAB_NAME	    dd_t1_tab_name;	/* the table name */
    DD_OWN_NAME	    dd_t2_tab_owner;	/* the table owner */
    DD_OBJ_TYPE	    dd_t3_tab_type;	/* DD_2OBJ_TABLE, DD_3OBJ_VIEW, or 
					** DD_4OBJ_INDEX */
    DD_DATE	    dd_t4_cre_date;	/* create date */
    DD_DATE	    dd_t5_alt_date;	/* alter date */
    bool	    dd_t6_mapped_b;	/* TRUE if column name mapping is 
					** required, FALSE otherwise */
    i4		    dd_t7_col_cnt;	/* number of columns in table */
    DD_COLUMN_DESC  **dd_t8_cols_pp;	/* ptr to array of pointers to LDB 
					** columns, NULL if no column mapping */
    DD_1LDB_INFO    *dd_t9_ldb_p;	/* ptr to LDB info structure */
}   DD_2LDB_TAB_INFO;


/*}
** Name: DD_OBJ_DESC - Object descriptor.
**
** Description:
**      This structure defines the description of an object at the STAR level.
**
** History:
**      24-apr-88 (carl)
**          written
*/

typedef struct _DD_OBJ_DESC
{
    DD_OBJ_NAME		dd_o1_objname;	/* object name */
    DD_OWN_NAME		dd_o2_objowner;	/* object owner */
    DB_TAB_ID		dd_o3_objid;	/* object id */
    DB_QRY_ID		dd_o4_qryid;	/* query id if view */
    DD_DATE		dd_o5_cre_date;	/* create date */
    DD_OBJ_TYPE		dd_o6_objtype;	/* object type, DD_0OBJ_LINK, 
					** DD_1OBJ_TABLE or DD_2OBJ_VIEW */    
    DD_DATE		dd_o7_alt_date;	/* alter date */
    bool		dd_o8_sysobj_b;	/* TRUE if a system object, FALSE 
					** if user */
    DD_2LDB_TAB_INFO	dd_o9_tab_info;	/* LDB table name, valid if link or 
					** table */
}   DD_OBJ_DESC;


/*}
** Name: DD_DDB_DESC - Descriptor of a DDB.
**
** Description:
**      Descriptor containing general information of a DDB, initialized by SCF
**  for DDB.
**
**
** History:
**      24-apr-88 (carl)
**          written
*/

typedef struct _DD_DDB_DESC
{
    DD_DB_NAME	     dd_d1_ddb_name;	/* DDB name */
    DD_USR_DESC	     dd_d2_dba_desc;	/* DBA desc */
    DD_1LDB_INFO     dd_d3_cdb_info;	/* CDB info */
    DD_1LDB_INFO    *dd_d4_iidbdb_p;	/* information used for accessing 
					** IIDBDB */
    DD_ACCESS	     dd_d5_access;	/* embedded access information for */
    DD_DBSERVICE     dd_d6_dbservice;	/* embedded DB service information */
    i4	     dd_d7_uniq_id;	/* CDB id unique in iidbdb */
}   DD_DDB_DESC;


/*
** Name : DD_REGPROC_DESC - Register dbp descriptor.
**
** Description:
**      This strcuture defines the description of a registerd procedure
**      object for STAR.
**
** History:
**      04-dec-92 (tam)
**          written
**	14-dec-92 (teresa)
**	    make LDB descriptor a ptr.
*/

typedef struct _DD_REGPROC_DESC
{
    DB_CURSOR_ID    dd_p1_local_proc_id;    /* name,id LDBMS uses */
    DD_OWN_NAME     dd_p2_ldbproc_owner;    /* owner of LDB proc */
    DB_CURSOR_ID    dd_p3_regproc_id;       /* name,id star uses */
    DD_OWN_NAME     dd_p4_regproc_owner;    /* owner of reg'd proc*/
    DD_LDB_DESC     *dd_p5_ldbdesc;          /* LDB descriptor */
}   DD_REGPROC_DESC;


#endif	    
