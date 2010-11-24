/*
**Copyright (c) 2004 Ingres Corporation
*/

#ifndef DMUCB_H_INCLUDED
#define DMUCB_H_INCLUDED

/**
** Name: DMUCB.H - Typedefs for DMF utilities support.
**
** Description:
** 
**      This file contains the control block used for all utility  operations.
**      Note, not all the fields of a control block need to be set for every 
**      operation.  Refer to the Interface Design Specification for 
**      inputs required for a specific operation.
**
** History:    
**    01-sep-85 (jennifer)
**        Created.
**    09-jun-86 (jennifer)
**          Modified DMUCB attr_array and key_array to DM_PTR instead
**          of DM_DATA.
**    14-jul-86 (jennifer)
**          Update to include standard header.
**    19-jul-86 (jennifer)
**          Added query id field to dmu control block for creating 
**          views.  
**    16-sep-86 (jennifer)
**          Added characteristic for modify to truncate.
**    19-nov-87 (jennifer)
**          Added support for multiple locations.
**    06-mar-89 (neil)
**          Added a DMU_F_RULE for rules.
**    17-apr-89 (mikem)
**	    Logical key development. Added DMU_F_SYS_MAINTAINED and DMU_F_WORM.
**    02-aug-899 (teg)
**	    Add support for Check Table, Patch Table operations.
**    15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateway - added DMU_GATEWAY attribute.
**	    Added gateway attribute fields to DMU_CB.
**    24-jan-1990 (fred)
**	    Added support for peripheral datatypes and extension tables.
**     2-mar-90 (andre)
**	    Define DMU_VGRANT_OK.
**    21-may-90 (linda)
**	    Remove DMU_F_EXTFMT flag value as valid value for attr_flags_mask;
**	    replaced with DMGW_F_EXTFMT in DMU_GWATTR_ENTRY's gwat_flags_mask
**	    instead.
**    24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**    29-apr-1991 (Derek)
**	    Added new items DMU_ALLOCATION,DMU_EXTEND.
**	10-jan-1991 (bryanp)
**	    Added new characteristic DMU_INDEX_COMP for specifying Btree index
**	    compression.
**	15-jan-1992 (bryanp)
**	    Renamed DMU_TEMP_CREATE to DMU_TEMP_TABLE for use with
**	    DROP and MODIFY on lightweight session tables. Added DMU_RECOVERY
**	    characteristic for specifying the NORECOVERY information for
**	    lightweight session tables.
**	29-Jun-92/8-Sep-92 (daveb)
**	    Define DMGW_IMA for the IMA gateway.  This mechanism is
**	    terrible.  The Gw id's should be pulled in from GWF rather
**	    than have their definitions duplicated here.
**	28-jul-92 (rickh)
**	    FIPS CONSTRAINTS improvements.
**	16-nov--1992 (fred)
**  	    Added DMU_EXT{TOO,ONLY}_MASK to dmu_flags_mask.  These
**          flags indicate that the extensions should {ONLY, ALSO}
**	    be relocated.
**	24-may-1993 (robf)
**	    Add dmu_sec_id for security label identifier in DMU_CB
**	30-mar-1993 (rmuth)
**	    Add new item DMU_ADD_EXTEND.
**	15-may-1993 (rmuth)
**	    Add new items DMU_READONLY, DMU_NOREADONLY and
**	    DMU_CONCURRENT_ACCESS.
**	26-july-1993 (rmuth)
**	    Remove DMU_NOREADONLY, use the ON/OFF field in DMU_READONLY.
**	7-jul-94 (robf)
**          Add DMU_INTERNAL_REQ, DMU_DB_LABEL
**	6-jan-95 (nanpr01)
**          Add DMU_PHYS_INCONSISTENT, DMU_LOG_INCONSISTENT, 
**	    DMU_TABLE_RECOVERY_DISALLOWED
**	    Added for partial backup & recovery Bug # 66002.
**	2-feb-95 (cohmi01)
**	    For RAW I/O, added dmu_rawextnm to DMU_CB. (extent names list)
**	28-apr-1995 (shero03)
**	    For RTREE added range 
**	06-mar-1996 (stial01 for bryanp)
**	    Add DMU_PAGE_SIZE characteristic.
**	14-may-1996 (shero03)
**	    Add DMU_C_HI for compression type
**      22-jul-1996 (ramra01 for bryanp)
**          Add Alter Table support: DMU_CASCADE, DMU_ALTER_TYPE,
**          DMU_C_DROP_ALTER, and DMU_C_ADD_ALTER.
**	13-aug-96 (somsa01)
**	    Added DMU_ETAB_JOURNAL for enabling jounaling at next checkpoint for
**	    etab tables.
**	21-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added DMU_TABLE_PRIORITY, DMU_TO_TABLE_PRIORITY.
**	12-mar-1999 (nanpr01)
**	    Get rid of the raw extents since only one table is supported per
**	    raw location.
**      12-may-2000 (stial01)
**          Added DMU_RETINTO for SIR 98092
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-Apr-2001 (horda03) Bug 104402
**          Ensure that Foreign Key constraint indices are created with
**          PERSISTENCE. This is different to Primary Key constraints as
**          the index need not be UNIQUE nor have a UNIQUE_SCOPE.
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      02-jan-2004 (stial01)
**          Added new DMU options for new WITH clauses
**          Removed DMU_ETAB_JOURNAL
**	23-Feb-2004 (schka24)
**	    More partition additions, for modify.
**      13-may-2004 (stial01)
**          Removed support for SET [NO]BLOBJOURNALING ON table.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**	15-feb-2010 (toumi01) SIR 122403
**	    Add support for column encryption.
**     22-apr-2010 (stial01)
**          Removed unreferenced DMU_EXTFMT_ENTRY
**	04-aug-2010 (miket) SIR 122403
**	    Change encryption activation terminology from
**	    enabled/disabled to unlock/locked.
**	12-Oct-2010 (kschendel) SIR 124544
**	    Turf out the dmu characteristics array and replace it with
**	    a structure that both parse and DMF can use.
**/

/*
** Name: DMU_CHARACTERISTICS -- Table characteristics structure
**
** Description:
**	This structure is used to describe various table characteristics
**	to DMF:  structure, uniqueness, fillfactor, and so on.
**	The structure contains an "indicator" bitmap which shows
**	which characteristics are actually being passed, and a bunch
**	of values.
**
**	Indicator-only characteristics don't have an associated value;
**	all that's needed is the indicator.  A good example is DMU_TEMP_TABLE,
**	which says that the associated table is a temporary table.
**
**	Flag-valued characteristics are those which might be ON, OFF,
**	or not specified.  The associated flag is set (along with the
**	indicator, of course) if the passed value is ON.
**
**	Numeric-valued characteristics have some sort of associated
**	numeric value which is part of the structure.  Some numeric
**	characteristics take a set of predefined values (DMU_STRUCTURE),
**	and some take a more or less arbitrary value (FILLFACTOR or EXTEND).
**
**	The DMU_CHARACTERISTICS is not only used to pass values to DMF;
**	it's used by the parser to collect WITH-option values and
**	verify them.
**
**	As a historical note, DMU characteristics used to be passed
**	using a variable-length array.  Each array element had a type
**	and a value.  This form may have saved a bit of memory, but was
**	not especially convenient for parsing and checking.  Considerable
**	effort was being expended converting from one form to another.
**	The current DMU_CHARACTERISTICS structure isn't all that
**	large, and it's a lot easier to use by both parse and DMF.
**
** History:
**	12-Oct-2010 (kschendel)
**	    Ditch the dmu_char_array in favor of something that's
**	    easier for both the parser and DMF.
*/

/* The indicator array is a bit-map.  Define the indicator bits. */

enum dmu_charind_enum {

/* Indicator-only characteristics. */
	DMU_CASCADE,		/* CASCADE variation of ALTER TABLE */
	DMU_EXT_CREATE,		/* DMF internal: creating an etab */
	DMU_GATEWAY,		/* Doing a gateway operation */
	DMU_GLOBAL_INDEX,	/* Defining a global index (on a partitioned
				** base table */
	DMU_NOT_DROPPABLE,	/* Table/index not droppable */
	DMU_NOT_UNIQUE,		/* Table/index supports a non-unique
				** constraint.  This is *NOT* the opposite
				** of DMU_UNIQUE. */
	DMU_ROW_SEC_AUDIT,	/* Security-auditing on ROW */
	DMU_SUPPORTS_CONSTRAINT, /* Table/index supports a constraint */
	DMU_SYS_MAINTAINED,	/* Table contains a system_maintained column */
	DMU_SYSTEM_GENERATED,	/* System generated object (index, probably) */
	DMU_TEMP_TABLE,		/* Creating / modifying a temp table */
	DMU_UNIQUE,		/* UNIQUE key structure */
	DMU_VIEW_CREATE,	/* Creating a view */

/* Indicators with an associated flag value in dmu_char_flags */

	DMU_ACTION_ONOFF,	/* Distinguish e.g. modify to phys_consistent
				** from modify to phys_inconsistent */
	DMU_AUTOSTRUCT,		/* Auto constraint-index structuring; not
				** really a DMF flag, is for QEF, but
				** it's convenient to have it here. */
	DMU_CLUSTERED,		/* (notused at present) Clustered primary */
	DMU_CONCURRENT_ACCESS,	/* Concurrent index creation */
	DMU_CONCURRENT_UPDATES,	/* "online modify" */
	DMU_DUPLICATES,		/* Duplicate rows or not */
	DMU_GW_UPDT,		/* Gateway: [no]update */
	DMU_PERSISTS_OVER_MODIFIES,  /* [no]Persistence */
	DMU_RECOVERY,		/* [no]recovery */
	DMU_STATEMENT_LEVEL_UNIQUE,  /* unique_scope=statement (as opposed to
				** row.  flag set -> statement) */

/* Indicators for numeric value things */

	DMU_DATAFILL,		/* FILLFACTOR= dmu_fillfac */
	DMU_IFILL,		/* NONLEAFFILL= dmu_nonleaff */
	DMU_LEAFFILL,		/* LEAFFILL= dmu_leaff */
	DMU_MINPAGES,		/* MINPAGES= dmu_minpgs */
	DMU_MAXPAGES,		/* MAXPAGES= dmu_maxpgs */
	DMU_ALLOCATION,		/* ALLOCATION= dmu_alloc */
	DMU_EXTEND,		/* EXTEND= dmu_extend */
	DMU_PAGE_SIZE,		/* PAGE_SIZE= dmu_page_size */
	DMU_TABLE_PRIORITY,	/* PRIORITY= dmu_cache_priority */
	DMU_BLOBEXTEND,		/* dmu_blobextend for blob etabs */
	DMU_DIMENSION,		/* RTREE dimensionality dmu_dimension */

/* Numeric things with specifically defined values */

	DMU_STRUCTURE,		/* STRUCTURE= dmu_struct (DB_xxx_STORE) */
	DMU_DCOMPRESSION,	/* Data compression, see below */
	DMU_KCOMPRESSION,	/* Key / index compression, see below */
	DMU_JOURNALED,		/* Journaling on or off (or later) */
	DMU_VACTION,		/* "Verify" action (patch, table_debug, etc) */
	DMU_VOPTION,		/* Verify sub-option (eg table_option) */

/* The last indicator that DMF cares / knows about. */

	DMU_CHARIND_LAST,	/* End of DMF-relevant indicators */

/* This is not the end of the bit-map!  We need to allow for more bits,
** because the parser has any amount of stuff that it wants to keep
** track of, such as location= and many others.
** We could define the parser indicators here too, but it seems a shame
** to make someone edit dmucb.h to make a parser change.  Instead, we'll
** simply make room for a couple dozen more indicators.  A quick
** check in the code will verify that there's enough extra bits.
**
** (I originally tried the alternative, which was to keep a separate
** and larger parser bit-map.  It works, but knowing which one to use
** when, and updating one from the other, was tedious and error prone.)
*/

	DMU_ALLIND_LAST = DMU_CHARIND_LAST+30	/* End of the bits */

};

typedef struct _DMU_CHARACTERISTICS
{

    char	dmu_indicators[(DMU_ALLIND_LAST+BITSPERBYTE) / BITSPERBYTE];

    i4		dmu_struct;		/* structure type */
    i4		dmu_fillfac;		/* fillfactor value */
    i4		dmu_leaff;		/* leaffill (indexfill) value */
    i4		dmu_nonleaff;		/* nonleaffill value */
    i4		dmu_minpgs;		/* minpages value */
    i4		dmu_maxpgs;		/* maxpages values */
    i4		dmu_alloc;		/* allocation value. */
    i4		dmu_extend;		/* extend value. */
    i4		dmu_page_size;		/* requested page size for table */
    i4		dmu_blobextend;		/* blob_extend value */
    i4		dmu_cache_priority;	/* priority= value */
    i4		dmu_dimension;		/* rtree dimension */
    i4		dmu_voption;		/* table_option= value */

/* dmu_voption values in addition to table_option values */
#define			    DMU_T_BITMAP	0x0001
#define			    DMU_T_LINK		0x0002
#define			    DMU_T_RECORD	0x0004
#define			    DMU_T_ATTRIBUTE	0x0008
#define			    DMU_T_PERIPHERAL	0x0010

    i2		dmu_vaction;		/* DMU_ACT_VERIFY sub-action */
#define			    DMU_V_VERIFY	1
#define			    DMU_V_REPAIR	2
#define			    DMU_V_PATCH		3
#define			    DMU_V_FPATCH	4
#define			    DMU_V_DEBUG		5
/* A modifier flag OR'ed to any of the above */
#define			    DMU_V_VERBOSE	8

    i2		dmu_journaled;		/* Journaled? */
#define	DMU_JOURNAL_OFF		0	/* Not journaled */
#define	DMU_JOURNAL_ON		1	/* Journaled now (db is journaled) */
#define	DMU_JOURNAL_LATER	2	/* Journaled later; db not journaled
					** *NOTE* If "later" makes it to DMF,
					** it means "ON".
					*/

    i2		dmu_dcompress;		/* Data compression */
    i2		dmu_kcompress;		/* Key compression */
#define	DMU_COMP_OFF	0		/* Explicitly no compression */
#define DMU_COMP_ON	1		/* "normal" compression on */
#define DMU_COMP_HI	2		/* Hidata compression on */
#define DMU_COMP_DFLT	3		/* Compression "on" but defaulted,
					** figure it out after doing storage
					** structure resolution.
					** *Note* DMF does not see "dflt",
					** it's for the parser.
					*/
    i4		dmu_flags;		/* On/Off flags */
#define DMU_FLAG_ACTON		0x01	/* General modify action ON (vs OFF)
					** This is used when the modify action
					** does NOT have an equivalent with-
					** option.  (else the with-option
					** flag is used instead.)
					*/
#define DMU_FLAG_AUTOSTRUCT	0x02	/* AUTOSTRUCT option */
#define DMU_FLAG_CLUSTERED	0x04	/* Clustered option */
#define DMU_FLAG_CONCUR_A	0x08	/* CONCURRENT_ACCESS option */
#define DMU_FLAG_CONCUR_U	0x10	/* CONCURRENT_UPDATES option */
#define DMU_FLAG_DUPS		0x20	/* DUPLICATES option */
#define DMU_FLAG_EXTONLY	0x40	/* EXTENSIONS_ONLY option */
#define DMU_FLAG_RECOVERY	0x80	/* RECOVERY option */
#define DMU_FLAG_PERSISTENCE	0x100	/* PERSISTENCE option */
#define DMU_FLAG_UPDATE		0x200	/* (gw)UPDATE option (register) */
#define DMU_FLAG_UNIQUE_STMT	0x400	/* UNIQUE_SCOPE stmt (not row) */

} DMU_CHARACTERISTICS;


/*}
** Name:  DMU_CB - DMF utility call control block.
**
** Description:
**      This typedef defines the control block required for
**      the following DMF utility operations:
**      
**      DMU_CREATE_TABLE - Create a database table.
**      DMU_DESTROY_TABLE - Destroy a database table.
**      DMU_INDEX_TABLE - Create an Index on a database table.
**      DMU_MODIFY_TABLE - Modify the structure of a database table.
**      DMU_RELOCATE_TABLE - Move a database table to another location.
**      DMU_ALTER - Modify configuration variables.
**      DMU_SHOW - Show the configuration variables.
**
**      Note:  Included structures are defined in DMF.H if they 
**      start with DM_, otherwise they are defined as global
**      DBMS types.
**
** History:
**    01-sep-85 (jennifer) 
**          Created.
**    09-jun-86 (jennifer)
**          Modified attr_array and key_array to DM_PTR instead
**          of DM_DATA.
**    14-jul-86 (jennifer)
**          Update to include standard header.
**    19-jul-86 (jennifer)
**          Added query id field to dmu control block for creating 
**          views.  
**    08-aug-89 (teg)
**	    add 4 new bit masks to dmu_flags_mask for table check/patch.
**    15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateway - Added gateway attribute fields
**	    (dmu_gwattr_array, dmu_gwchar_array) to DMU_CB.
**    02-mar-90 (andre)
**	    define DMU_VGRANT_OK. This bit will be set when a view is being
**	    created and it has been determined that the owner will be allowed to
**	    grant permit on that view to other users.
**	29-Jun-92/8-sep-92 (daveb)
**	    Define DMGW_IMA for the IMA gateway.  This mechanism is
**	    terrible.  The Gw id's should be pulled in from GWF rather
**	    than have their definitions duplicated here.
**	11-aug-92 (pholman)
**	    Add DMGW_SEC_AUD for the C2 security auditing gateway.
**	16-nov--1992 (fred)
**  	    Added DMU_EXT{TOO,ONLY}_MASK to dmu_flags_mask.  These
**          flags indicate that the extensions should {ONLY, ALSO}
**	    be relocated.
**	24-may-1993 (robf)
**	    Add dmu_sec_id for security label identifier 
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	7-jul-94 (robf)
**          Add DMU_INTERNAL_REQ, DMU_DB_LABEL
**	4-Feb-2004 (jenjo02/schka24)
**	    Add partition definition information so that create, modify
**	    can do the correct partitiony things.
**	23-Feb-2004 (schka24)
**	    More partition stuff, physical partition characteristics array.
**	10-Mar-2004 (schka24)
**	    To get repartitioning modify more->fewer partitions working,
**	    need another no-file-exists flag for the post-drop.
**	30-apr-2004 (thaju02)
**	    Added DMU_PIND_CHAINED, persistent index DMU_CBs linked to modify DMU_CB. 
**	25-May-2004 (jenjo02)
**	    Fixed botched, duplicate dmu_flags_mask defines.
**	08-jul-2004 (thaju02)
**	    Online Modify - added indxcb_online_fhdr_pageno.
**	    (B112610)
**	11-mar-2005 (thaju02)
**	    Online modify: removed indxcb_online_fhdr_pageno. 
**	    added dmu_on_{relfhdr, relpages, relprim, relmain}
**	20-mar-2006 (toumi01)
**	    Add DMU_NODEPENDENCY_CHECK to flag that constraint dependency
**	    check for the modify command has been overridden by the user.
**	6-Jul-2006 (kschendel)
**	    Comment update.
**	18-Mar-2010 (gupsh01) SIR 123444
**	    Added dmu_newtab_name to support Alter table rename operation.
*/

/* MODIFY and ALTER TABLE have numerous variations.  This "action code"
** defined exactly what sort of operation is going on.  (One note of caution:
** MODIFY to RELOCATE is not at present listed here, because it's
** handled as a separate statement with a separate dmf-call.  It may
** be desirable to fix that at some point.)
** Many if not all of these actions require as a minimum that certain
** indicators and values be set in the DMU_CHARACTERISTICS section.
*/

enum dmu_action_enum {
	DMU_ACT_NONE = 0,	/* Not set yet */

	/* MODIFY actions */

	DMU_ACT_STORAGE,	/* MODIFY to storage-structure, including
				** MODIFY to RECONSTRUCT */
	DMU_ACT_ADDEXTEND,	/* MODIFY to add_extend */
	DMU_ACT_ENCRYPT,	/* MODIFY to encrypt */
	DMU_ACT_LOG_CONSISTENT,	/* MODIFY to log_[in]consistent */
	DMU_ACT_MERGE,		/* MODIFY to merge */
	DMU_ACT_PERSISTENCE,	/* MODIFY to [no]persistence */
	DMU_ACT_PHYS_CONSISTENT, /* MODIFY to phys_[in]consistent */
	DMU_ACT_PRIORITY,	/* MODIFY to priority=n */
	DMU_ACT_READONLY,	/* MODIFY to [no]readonly */
	DMU_ACT_REORG,		/* MODIFY to reorganize */
	DMU_ACT_TABLE_RECOVERY,	/* MODIFY to table_recovery_[dis]allowed */
	DMU_ACT_TRUNC,		/* MODIFY to truncated */
	DMU_ACT_USCOPE,		/* MODIFY to unique_scope=[row | statement] */
	DMU_ACT_VERIFY,		/* MODIFY to patch / verify / table_debug */

	/* ALTER TABLE actions */

	DMU_ALT_ADDCOL,		/* Add column */
	DMU_ALT_DROPCOL,	/* Drop column */
	DMU_ALT_ALTERCOL,	/* Alter column */
	DMU_ALT_TBL_RENAME,	/* Rename table */
	DMU_ALT_COL_RENAME,	/* Rename column */

	/* Fake action for RELOCATE.  This never appears (at present)
	** as a dmu_action, but we define it so that callers can treat
	** DMU_RELOCATE_TABLE (a qeu_d_op) as if it were a MODIFY action
	** for comparison purposes.
	*/
	DMU_ACT_RELOC
};


typedef struct _DMU_CB
{
    PTR		q_next;
    PTR		q_prev;
    SIZE_TYPE	length;                 /* Length of control block. */
    i2		type;                   /* Control block type. */
#define	DMU_UTILITY_CB      2
    i2		s_reserved;
    PTR		l_reserved;
    PTR		owner;
    i4		ascii_id;
#define	DMU_ASCII_ID        CV_C_CONST_MACRO('#', 'D', 'M', 'U')
    DB_ERROR	error;                  /* Common DMF error block. */
    PTR		dmu_tran_id;            /* Transaction ID. */
    enum dmu_action_enum dmu_action;	/* MODIFY or ALTER action code */
    i4		dmu_flags_mask;         /* Modifier to operation. */
#define			DMU_EXTONLY_MASK    0x0001L
#define                 DMU_EXTTOO_MASK     0x0002L
#define			DMU_PARTITION	    0x0004L  /* Create a partition */

/* DMU_MASTER_OP is set when a modify operation on a partitioned table
** was syntactically specified for the whole table.  DMF uses this to
** update master info, e.g. the master location.
** This flag is needed because there are constructs where DMF cannot
** otherwise tell whether the modify is against the entire table
** (and master should be updated) or against some partitions only (master
** should not be updated).
** The flag has no meaning if the table in dmu_tbl_id is not a partitioned
** master table.
*/
#define			DMU_MASTER_OP	    0x0008L

/* DMU_DROP_NOFILE is set when a repartitioning modify is dropping
** extra unused partitions.  The partition catalog stuff needs to be
** dropped, but the modify process has already renamed the partition's
** physical files (to drop-temps).  So the flag says "skip the actual
** file rename stuff".  (We can't just do this with partitions all the
** time, because when we get around to MODIFY .. DROP PARTITION, that
** will be an actual full drop.)
*/
#define			DMU_DROP_NOFILE	    0x0010L  /* DROP: no disk file */

#define			DMU_ONLINE_START    0x0020L
#define			DMU_ONLINE_END      0x0040L
#define			DMU_INTERNAL_REQ    0x0100L
#define                 DMU_DB_LABEL        0x0200L
#define                 DMU_RETINTO         0x0400L
#define			DMU_NO_PAR_INDEX    0x0800L
#define			DMU_VGRANT_OK	    0x1000L
#define			DMU_IGNORE_DUPKEY   0x2000L
#define			DMU_PIND_CHAINED    0x4000L
#define			DMU_NODEPENDENCY_CHECK	0x10000L

    PTR 	    dmu_db_id;              /* DMF database identifier. */
					/* Really an ODCB pointer, so it's the
					** same for all threads of a session.
					*/
    DB_TAB_NAME     dmu_table_name;         /* Database table name. */
    DB_TAB_ID       dmu_tbl_id;             /* Database table identifier. */
    DB_QRY_ID       dmu_qry_id;             /* View Query identifier. */
    i4	    dmu_tup_cnt;	    /* Returned tuple counts. */
    DB_OWN_NAME     dmu_owner;              /* Database table owner. */
    DB_TAB_NAME     dmu_index_name;         /* Database index name. */
    DB_TAB_ID       dmu_idx_id;             /* Database index identifier */
    DB_TAB_NAME	    dmu_online_name;	    /* generic, used by online modify
					    ** to hold index name temporarily
					    */
    DB_TAB_ID	    dmu_dupchk_tabid;	    /* online modify - phantom 
					    ** duplicate key checking tabid
					    */
    DM_DATA         dmu_location;           /* Pointer to an array 
                                            ** of table locations.
                                            ** For all commands 
                                            ** this is an array up to 
                                            ** DM_LOC_MAX.  For relocate
					    ** it is used for the new
                                            ** location list. */
    DM_DATA         dmu_olocation;          /* Pointer to array of 
                                            ** table locations.
                                            ** For all commands 
                                            ** this is an array up to 
                                            ** DM_LOC_MAX.  For relocate
					    ** it is used for the old
                                            ** location list. */
    DM_PTR          dmu_key_array;          /* Array of key attributes. */
    DM_PTR          dmu_attr_array;         /* Array of attributes. */
    DM_DATA         dmu_conf_array;         /* Array of configuration
                                            ** variables. */
    DMU_CHARACTERISTICS dmu_chars;	    /* Table characteristics */
    DB_TAB_NAME     dmu_newtab_name;	    /* New table name for Rename
					    ** operation. */
    i4         dmu_gw_id;              /* Gateway id */
#define	    DMGW_NONE       0
#define	    DMGW_VSAM       1
#define	    DMGW_RMS        2
#define	    DMGW_IMS        3
#define	    DMGW_IDMSX      4
					    /* Gateway id's 5-6 unused. */

#define     DMGW_SEC_AUD    6		    /* C2 security Auditing   */
#define	    DMGW_IMA	    7		    /* IMA gateway */
#define	    DMGW_LG_TEST    8		    /* Internal Test Gateways */
#define	    DMGW_DMF_TEST   9

#define	    DMGW_MAX_GW_ID  9		    /* for now... (linda) */

    DM_PTR	    dmu_gwattr_array;	    /* Gateway column attributes. */
    DM_DATA	    dmu_gwchar_array;	    /* Gateway table characteristics */
    i4	    dmu_gwrowcount;	    /* Gateway row count */
#define	DMGW_NO_ROW_ENTRY	(-1)
    i4		    dmu_range_cnt;	    /* # of range entries */
    f8		    *dmu_range;		    /* Range entries for rtree */

    u_i4            dmu_on_relfhdr;         /* $online idx relfhdr */
    i4              dmu_on_reltups;         /* $online idx reltups */
    i4              dmu_on_relpages;        /* $online idx relpages */
    i4              dmu_on_relprim;         /* $online idx relprim */
    i4              dmu_on_relmain;         /* $online idx relmain */

    /* Pointer to partition definition if the DMU operation is to result
    ** in partitioning.  (This is the NEW partitioning setup if modify.)
    ** The size-in-bytes of the partition definition is not used by DMU,
    ** but is stored here as a convenience for query compilation.
    */
    DB_PART_DEF	    *dmu_part_def;	    /* Pointer to partition def */
    i4		    dmu_partdef_size;	    /* Size in bytes of part def */

    /* Create and drop of partitions are not symmetrical.  For CREATE, the
    ** caller fills in dmu_part_def, and for a partition sets DMU_PARTITION
    ** and fills in this dmu_partno.  For DROP, the internals of dmu work
    ** a bit differently, and the caller is to simply pass the partition
    ** table ID.  (The difference arises because CREATE has to operate on
    ** each partition specifically, while DROP of the master drops everything.
    ** Individual partition DROP is a modify side-effect.)
    */
    u_i2	    dmu_partno;		    /* Partition number to create */

    /* Avoid the need for an extra "show" in qeu to see if the table
    ** in question is partitioned (most aren't).  This field is really
    ** for QEF, not for DMF, as DMF will typically have the actual table
    ** info around.  There's no other convenient way to pass this number
    ** to QEF though.  (The QEU_CB is used for a zillion other completely
    ** unrelated things, and the info placed there would be too
    ** unreliable/misleading.)
    */
    u_i2	    dmu_nphys_parts;	    /* Number of physical partitions
					    ** currently in the table.
					    ** zero = not partitioned
					    */

    /* The physical partition characteristics array is passed when dmu
    ** needs to know something specific about individual partitions, such
    ** as their locations, or which partitions to operate on.
    ** The ppchar array is always passed for repartitioning modifies.
    ** It's optional for partition-by-partition modifies, such as
    ** "modify pmast to btree on k";  if the ppchar array is omitted
    ** when it's optional, that means "just do the operation on every
    ** partition identically".
    ** This points to an array of DMU_PHYPART_CHAR.
    */
    DM_DATA	    dmu_ppchar_array;	    /* Physical partition info array */

    /* Encryption values: AES bit count and user-supplied key (which is
    ** circularly added to a 16/24/32 byte AES key for 128/192/256 bit
    ** encryption).
    */ 
    u_i2	    dmu_enc_flags;	    /* encryption flags .. */
#define			DMU_ENCRYPTED	0x0001L /* .. must match relencflags! */
#define			DMU_AES128	0x0002L
#define			DMU_AES192	0x0004L
#define			DMU_AES256	0x0008L
    u_i2	    dmu_enc_flags2;	    /* local to DMU_CB */
#define			DMU_AESKEY	0x0001L /* user supplied AESKEY= */
#define			DMU_NEWPASS	0x0002L /* new passphrase */
#define			DMU_NULLPASS	0x0004L /* NULL passphrase */
    u_i2	    dmu_enc_aeskeylen;
    u_char	    dmu_enc_aeskey[AES_256_BYTES];
    u_char	    dmu_enc_passphrase[AES_256_BYTES];
    /* For CREATE TABLE the encryption key bits is specified, but for
    ** MODIFY we don't know when we are parsing what we'll find when we
    ** go to unlock or modify the encryption. Bummer. Just prepare 128,
    ** 192, and 256 bit versions, computationally cheaper than looking
    ** up the catalog info. Sure is ugly, though! :-(
    */
    u_char	    dmu_enc_old128pass[AES_128_BYTES];
    u_char	    dmu_enc_old192pass[AES_192_BYTES];
    u_char	    dmu_enc_old256pass[AES_256_BYTES];
    u_char	    dmu_enc_new128pass[AES_128_BYTES];
    u_char	    dmu_enc_new192pass[AES_192_BYTES];
    u_char	    dmu_enc_new256pass[AES_256_BYTES];

}   DMU_CB;

/*}
** Name:  DMU_CONF_ENTRY - Description of configuration variables.
**
** Description:
**      This structure is used to describe the configuration variables 
**      that can be displayed or changed in a show or alter operation.
**      The DMU_CB.dmu_conf_array field contains a pointer to an array
**      containing entries of this type.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
*/
typedef struct _DMU_CONF_ENTRY
{
    i4         conf_id;                 /* Configuration identifier. */
#define                 DMU_QUESTIONABLE    1L
    i4         conf_type;               /* Describes the nature of the 
                                            ** operation.  */
#define                 DMU_PERMANENT       1L
#define                 DMU_DYNAMIC         2L
    i4         conf_value;             /* Value for integer configuration 
                                            ** variable. */  
    DM_DATA         conf_ascii_value;       /* Value for ascii configuration 
                                            ** variable. */
}   DMU_CONF_ENTRY;

/*}
** Name:  DMU_KEY_ENTRY - Description of key for a database table.
**
** Description:
**      This structure is used to describe the key attributes of
**      the table being created, modified or indexed.
**      The DMU_CB.dmu_key_array field contains a pointer to an array 
**      containing entries of this type.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
*/
typedef struct _DMU_KEY_ENTRY
{
    DB_ATT_NAME     key_attr_name;          /* Name of table attribute. */
    i4         key_order;              /* Sort order of attribute. */
#define                 DMU_ASCENDING       1L
#define                 DMU_DESCENDING      2L
}   DMU_KEY_ENTRY;

/*
** Name:  DMU_GATTR_ENTRY- Description of gateway attribute array able.
**
** Description:
**
** History:
**  18-may-90 (linda)
**	Define valid flag values.  Before we were using the attr_flags_mask
**	field, and DMU_F_*** flag values; but we can use our own struct and put
**	these values into the extended attribute catalog instead -- which is
**	good, as the iiattribute flag field is getting low on bits.
*/
#define   DMU_MAXSIZE_EXTFMT     80

typedef struct _DMU_GWATTR_ENTRY
{
    i4         gwat_id;                /* Characteristic id */
    i4         gwat_type;              /* define the type */
    char            gwat_value[256];
    i4         gwat_size;              /* Size of attribute. */
    i4         gwat_precision;         /* Precision of GATTR */
    i4         gwat_flags_mask;        /* Used to indicate flag */
    i4	    gwat_xsize;		    /* size of extended fmt. entry */
    char	    gwat_xbuffer[DMU_MAXSIZE_EXTFMT];
}   DMU_GWATTR_ENTRY;

/* Define valid flag values for gwat_flags_mask field. */
#define	    DMGW_F_EXTFMT   0x01
#define	    DMGW_F_VARIANT  0x02 
 
/*
** Name:  DMU_from_path_entry - Description of gateway from, path
**        array table
**
** Description:
**
** History:
*/
#define   DMU_MAXSIZE_FROM       256
#define   DMU_MAXSIZE_PATH       256
 
typedef struct _DMU_FROM_PATH_ENTRY
{
    char            from_buffer[DMU_MAXSIZE_FROM];
    i4         from_size;
    char            path_buffer[DMU_MAXSIZE_PATH];
    i4         path_size;
}   DMU_FROM_PATH_ENTRY;

/*
** Name: DMU_PHYPART_CHAR -- Physical partition characteristics entry.
**
** Description:
**	DMU modify-like operations on multiple partitions need info
**	about the physical partitions involved.  The DMU_CB contains
**	a pointer (DM_DATA) to an array of DMU_PHYPART_CHAR, one entry
**	per physical partition.  Each entry holds:
**
**	oloc_count - The number of current (old) locations for the
**		partition.  This saves DMF a little bit of work since
**		it needs to know this to allocate memory.  (We could save
**		DMF even more work by passing the actual old locations,
**		but the format requirements are somewhat inconvenient.
**		So just the count is passed.)
**		* Note * This might be zero == unknown.
**	flags - Some flags.  Currently defined flags are:
**		DMU_PPCF_NEW_TABID - this entry describes a brand
**		new partition table, or at least one which is going to
**		be assigned a new table ID.  The caller (QEF) will pre-
**		create the new table and post-drop any unused ones.
**		DMU_PPCF_IGNORE_ME - this entry should be skipped.
**		Used when a partition-by-partition operation does not
**		apply to all physical partitions (eg modify pmaster
**		partition logpart to truncated).  Note that if the
**		operation is specific to ONE physical partition, QEF
**		will simply indicate that physical partition tabid in
**		the DMU_CB, instead of passing the master and a whole
**		slew of ignore-me's.
**	part_tabid - The table ID of the physical partition, passed in
**		case it's different from the old partition table or
**		perhaps an entirely new partition.
**	loc_array - The (new) location information for the physical
**		partition, if different from where the partition
**		currently resides.
**	char_array - The (new) characteristics for the physical partition.
**	key_array - The (new) key columns for the physical partition.
**
**	For this release (Feb '04), char_array and key_array are not used.
**
**	A ppchar array is not necessarily always present just because
**	the modify is dealing with a partitioned table.  Situations where
**	no ppchar array is present are:
**	- A non-repartitioning modify on one single physical partition.
**	  (The parse resolves this such that the modify dmu_tbl_id is
**	  adjusted to be that of the actual partition.)
**	- A non-repartitioning modify on the entire (master) table, with
**	  no location= specification.  Since no partitions can be changing
**	  locations, no ppchar array is needed.
**
**	In any situation where the modify preparation in QEU decides to
**	look at the partitions with DMT_SHOW, we'll build a ppchar array.
**	We can save DMF some work by feeding it current (old) location
**	counts.
**
**	Basically DMF needs to be prepared for a ppchar array on any
**	modify of a partitioned table, and should not attempt to draw any
**	fancy conclusions from the presence or absence of the array.
**
**	The parser only builds a ppchar array if it needs to do so because
**	of a table PARTITION logpart specification in the query.  (QEU
**	depends on this.)  QEU will build a ppchar array if the modify is
**	a repartitioning one, or if there is any question about changing
**	locations.
**
**	Some other observations:
**
**	The (new) location in any given ppchar entry is left empty
**	if the partition is already where it belongs;  UNLESS the
**	ppchar entry is describing a new partition.
**
**	New partitions are pre-created by QEU in the location where they
**	belong.  But DMF has no way to know where that is.  (New partitions
**	can't be looked up or queries until they become part of the
**	table, at the end of the modify.)  So we always pass the new
**	location info for newly created partitions.
**
**	Repartitioning modifys are indicated by the presence of a
**	non-null dmu_part_def, NOT by the presence or absence of a
**	ppchar array.  If the new scheme has more partitions than the
**	old, partition tables for the extra partitions will be pre-created
**	before DMF sees the modify.  If the new scheme has fewer partitions,
**	after DMF is done with the modify, QEF will drop the extra ones. 
**
**	For the special case of un-partitioning a table, there's no
**	ppchar_array;  DMF must re-use the old master table ID for the "new"
**	unpartitioned version, and if the location changes too then the
**	new location is in the dmu_cb dmu_location (just like it would be
**	for an ordinary nonpartitioned modify).
**
** History:
**	23-Feb-2004 (schka24)
**	    Need a way to feed partitioned/partitioning modify info
**	    about specific partitions.
**	11-Mar-2004 (schka24)
**	    Remove (unneeded) partno, change to old-location-count,
**	    update comments.
*/

typedef struct _DMU_PHYPART_CHAR
{
	u_i2		oloc_count;	/* Number of current (old) locations */
	u_i2		flags;		/* Flags for this partition */

#define DMU_PPCF_NEW_TABID	0x0001	/* Tabid is new/different */
#define DMU_PPCF_IGNORE_ME	0x0002	/* Skip this physical partition */

	DB_TAB_ID	part_tabid;	/* (new) partition table ID */
	DM_DATA		loc_array;	/* (new) partition locations */
	DM_DATA		char_array;	/* Currently notused */
	DM_PTR		key_array;	/* Currently notused */
} DMU_PHYPART_CHAR;

#endif /* double-include protection */
