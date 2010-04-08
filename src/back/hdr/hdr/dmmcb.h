/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:  DMMCB.H - Typedefs for DMF maintenance support.
**
** Description:
**      This file contains the control block used for all maintenance
**      operations.  Note, not all the fields of a control block
**      need to be set for every operation.  Refer to the Interface
**      Design Specification for inputs required for a specific
**      operation.
**
** History:
**      20-mar-89 (Derek)
**          Created for ORANGE.
**	28-mar-89 (teg)
**	    added fields that purgedb needs.
**	11-feb-92 (wolf) 
**	    Cosmetic change--<LF> to <FF> to suppress annoying Piccolo dif
**	02-nov-92 (jrb)
**	    Changed DMM_CB for multi-location sorts.
**	14-dec-92 (jrb)
**	    Removed DMM_NODIR_FLAG.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
** 	02-Apr-2001 (jenjo02
**	    Added loc_raw_start to DMM_LOC_LIST.
**	11-May-2001 (jenjo02)
**	    Added loc_raw_pct to DMM_LOC_LIST.
**	23-May-2001 (jenjo02)
**	    Added DMM_CHECK_LOC_AREA flag to DMM_CB.
**	15-Oct-2001 (jenjo02)
**	    Added DMM_MAKE_LOC_AREA, DMM_EXTEND_LOC_AREA 
**	    flags to DMM_CB.
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**      06-jun-2008 (stial01)
**          Changed DMM_UPDATE_CONFIG to use new dmm_upd_map (120475)
**/

/*}
** Name:  DMM_CB - DMF control call control block.
**
** Description:
**      This typedef defines the control block required for 
**      the following DMF control operations:
**
**      DMM_ALTER_DB   - Alter database configuration file.
**	DMM_CREATE_DB  - Create core database.
**	DMM_DESTROY_DB - Destroy database.
**	DMM_LIST       - List files/directories subordinate to specified dir
**      DMM_DROP_FILE  - Destroy specified file without logging/locking support
**	DMM_CONFIG     - Read or update fields in the config file
**	DMM_ADD_LOC    - Extend a database to a location.
**
**      Note:  Included structures are defined in DMF.H if 
**      they start with DM_, otherwise they are define as 
**      global DBMS types.
**
** History:
**      20-mar-89 (Derek)
**          Created for ORANGE.
**	28-mar-89 (teg)
**	    added fields that purgedb needs.
**	25-jan-91 (teresa)
**	    added the following bitmaps to dmm_flags_mask:
**		DMM_UNKNOWN_DBA, DMM_PRIV_50DBS, DMM_VERBOSE
**	02-Oct-92 (teresa)
**	    added the following bitmaps to dmm_flags_mask:
**		DMM_1GET_STATUS, DMM_2GET_CMPTLVL, DMM_3GET_CMPTMIN,
**		DMM_4GET_ACCESS, DMM_5GET_SERVICE, DMM_UPDATE_CONFIG.
**	    Also added the following fields:
**		dmm_status,  dmm_cmptlvl, dmm_dbcmptminor
**	20-feb-90 (jrb)
**	    Multi-Location Sorts Project.
**	    o Added DMM_L_AWRK as new dmm_loc_type.  This is an auxiliary work
**	      location (as opposed to a normal work location DMM_L_WRK which
**	      behaves slightly differently).
**	    o Added dmm_alter_op so that dmm_alter() could be used for changing
**	      config file extents as well as table id, access, and service.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	24-mar-2001 (stephenb)
**	    Add dmm_utable_name.
**	23-May-2001 (jenjo02)
**	    Added DMM_CHECK_LOC_AREA flag to DMM_CB.
**	15-Oct-2001 (jenjo02)
**	    Added DMM_MAKE_LOC_AREA, DMM_EXTEND_LOC_AREA 
**	    flags to DMM_CB.
**	29-Dec-2004 (schka24)
**	    Add place for updating config dsc_c_version.
**	6-Jul-2006 (kschendel)
**	    Doc update.
*/
typedef struct _DMM_CB
{
    PTR             q_next;
    PTR             q_prev;
    SIZE_TYPE       length;                 /* Length of control block. */
    i2		    type;                   /* Control block type. */
#define                 DMM_MAINT_CB      11L
    i2              s_reserved;
    PTR             l_reserved;
    PTR             owner;
    i4         	    ascii_id;             
#define                 DMM_ASCII_ID        CV_C_CONST_MACRO('#', 'D', 'M', 'M')
    DB_ERROR        error;                  /* Common DMF error block. */
    PTR		    dmm_tran_id;	    /* Transaction Identifier. */
    i4         	    dmm_flags_mask;         /* Modifier to operation. */
#define		DMM_NOWAIT	    0x01L
#define		DMM_FILELIST	    0x02L	/* request DIlistfile() */
#define		DMM_DIRLIST	    0x04L	/* request DIlistdir()  */
#define		DMM_DEL_FILE        0x08L	/* request DIdelete() */
#define		DMM_UNKNOWN_DBA	    0x10L	/* iicodemap is not available
						** to translate ucode to uname*/
#define		DMM_PRIV_50DBS	    0x20L	/* have finddbs force all
						** unconverted v5 dbs it finds
						** to private */
#define		DMM_VERBOSE	    0x40L	/* verbose flag, ie, give more
						** info as you work.  Only
						** implemented for DMM_FINDDBS
						*/
#define		DMM_1GET_STATUS	    0x100L	/* read cnf->dsc_status */
#define		DMM_2GET_CMPTLVL    0x200L	/* read cnf->dsc_dbcmptlvl */
#define		DMM_3GET_CMPTMIN    0x400L	/* read cnf->dsc_1dbcmptminor */
#define		DMM_4GET_ACCESS	    0x800L	/* read cnf->dsc_dbaccess */
#define		DMM_5GET_SERVICE    0x1000L	/* read cnf->dsc_dbservice */
#define		DMM_UPDATE_CONFIG   0x2000L	/* update config file */
#define		DMM_EXTEND_LOC_AREA 0x10000L	/* extend location to db */
#define		DMM_CHECK_LOC_AREA  0x20000L	/* check loc area existence */
#define		DMM_MAKE_LOC_AREA   0x40000L	/* make location area dirs */


    i4              dmm_upd_map;            /* config fields to be updated */
#define	    DMM_UPD_STATUS    0x00001 /* update dsc_status in config */
#define	    DMM_UPD_CMPTLVL   0x00002 /* update dsc_cmptlvl in config */
#define	    DMM_UPD_CMPTMINOR 0x00004 /* update dsc_cmptminor in config */
#define	    DMM_UPD_ACCESS    0x00010  /* update dsc_dbaccess in config */
#define	    DMM_UPD_SERVICE   0x00020  /* update dbservice in config */
#define	    DMM_UPD_CVERSION  0x00040  /* update dsc_c_version in config */

    DB_DB_NAME      dmm_db_name;            /* Database name. */
    DM_DATA         dmm_db_location;        /* Pointer to area containing
                                            ** database location. */
    DB_LOC_NAME	    dmm_location_name;	    /* Logical location name. */
    DM_PTR	    dmm_loc_list;	    /* List of initial locations. */
    i4	    	    dmm_db_id;		    /* Database identifier. */
					/* Note: this is the infodb db id,
					** NOT the "dmf db id" pointer cookie.
					** Sometimes called "unique db id".
					*/
    i4	    	    dmm_db_service;	    /* Database service. */
    i4	    	    dmm_db_access;	    /* Database access. */
    i4	    	    dmm_status;		    /* new config file status word */
    u_i4	    dmm_cmptlvl;	    /* new config file dbcmptlvl */
    i4	    	    dmm_dbcmptminor;	    /* new config file 1dbcmptminor */
    i4		    dmm_cversion;	    /* new config dsc_c_version */
    i4	    	    dmm_tbl_id;		    /* New next table id. */
    i4	    	    dmm_loc_type;	    /* Type of location. */
#define			DMM_L_DATA		1
#define			DMM_L_JNL		2
#define			DMM_L_CKP		3
#define			DMM_L_DMP		4
#define			DMM_L_WRK		5
#define			DMM_L_AWRK		6
#define			DMM_L_RAW		0x0100 /* ...and it's RAW */
    DB_OWN_NAME	    dmm_owner;		    /* owner of purge table */
    DB_TAB_NAME	    dmm_table_name;	    /* name of purge table */
    DB_TAB_NAME	    dmm_utable_name;	    /* unicode collation table */   
    DB_ATT_NAME	    dmm_att_name;	    /* name of attribute to list into */
    DM_DATA	    dmm_filename;	    /* name of file to DIdelete */
    i4	    	    dmm_count;		    /* number of tuples added to user
					    ** table during DIlistxxx() */
    i4	    	    dmm_alter_op;	    /* DMM_ALTER operation desired */
#define			DMM_EXT_ALTER	0   /* Change bits in extent entry */
#define			DMM_TAS_ALTER	1   /* Change table id, access, and
					    ** service fields in config file
					    */
}   DMM_CB;

/*}
** Name:  DMM_LOC_LIST - List of default locations for createdb.
**
** Description:
**      This typedef defines the structure used to pass the set of
**	default location for createdb.
** History:
**      20-mar-89 (Derek)
**          Created for ORANGE.
**	11-mar-99 (nanpr01)
**	    Support Raw location.
** 	02-Apr-2001 (jenjo02
**	    Added loc_raw_start to DMM_LOC_LIST.
**	11-May-2001 (jenjo02)
**	    Added loc_raw_pct to DMM_LOC_LIST.
*/
typedef struct
{
    i4	    	    loc_type;		    /* Same as dmm_loc_type. */
    DB_LOC_NAME	    loc_name;		    /* Logical name for location. */
    i4	    	    loc_l_area;		    /* Length of area string. */
    char	    loc_area[DB_AREA_MAX];  /* Actual area string. */
    i4	    	    loc_raw_pct;	    /* Pct of raw area to alloc */
    i4		    loc_raw_start;	    /* Starting rawblock */
    i4	    	    loc_raw_blocks;	    /* Number of blocks alloc'd */
}   DMM_LOC_LIST;
