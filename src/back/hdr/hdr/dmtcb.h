#ifndef INCLUDE_DMTCB_H
#define INCLUDE_DMTCB_H
/* Copyright (c) 1995, 2005 Ingres Corporation
**
**
*/

/**
**
** Name:  DMTCB.H - Typedefs for DMF table access support.
**
** Description:
**
**      This file contains the two different control blocks used for all table
**      operations.  The control block for normal table operations is DMT_CB, 
**      and the control block  for the show operation is DMT_SHOW_CB.  Note, not
**      all fields of a control block need to be set for every operation.  Refer
**      to the Interface Design Specification for inputs required for a specific
**      operation.
**    
** History:
** 
**      01-sep-85 - (jennifer)
**          Created.
**      16-may-86 (jennifer)
**          Modified to include query statement id (dmt_sequence)for 
**          table open.
**      18-may-86 (jennifer)
**          Modified to include the DMT_C_QID_VIEW for a characteristics so
**          that dmt_alter can be called to set a query id for a view.
**      04-jun-86 (jennifer)
**          Modified dmtcb.h to use array of pointer for attr, key
**          and index arrays.
**      12-jul-86 (jennifer)
**          Added flag to indicate creating temporary for loading
**          sorted data.
**      14-jul-86 (jennifer)
**          Added flags and variables to DMTCB to handle sort cost
**          estimates and to indicate if a loaded temporary is to
**          use compressed sort files and eliminate duplicates.
**      14-jul-86 (jennifer)
**          Update to include standard header.
**      03-dec-86 (jennifer)
**          Fix proctection documentation and logic, expand to two
**          separate characteristics.
**	25-sep-87 (rogerk)
**	    Added dmt_io_blocking field which will be set to readahead blocking
**	    factor on calls to DMT_SORT_COST.
**      23-nov-87 (jennifer)
**          Added multiple locations per table support.
**	18-oct-88 (rogerk)
**	    Added dmt_cache_size field for the optimizer to use.
**	06-mar-89 (neil)
**	    For rules: added DMT_RULE for rules (and filled DMT_GATEWAY gap),
**	    added DMT_C_RULE, added DMT_F_RULE (for single column).
**	17-apr-89 (mikem)
**	    Logical key development.   Added DMT_F_WORM and 
**	    DMT_F_SYS_MAINTAINED.
**      18-JUL-89 (JENNIFER)
**          Added a flag to indicate that an internal request 
**          for statistical information is being requested.
**          This solves a B1 security problem where statistics
**          could be unpredicatable if B1 labels were checked.
**          For an internal request they aren't checked, for a 
**          normal query on the iihistogram and iistatistics 
**          table they are checked.
**	08-feb-1990 (fred)
**	    Added DMT_LO_TEMP for purposes of correctly naming the tables...
**	25-apr-90 (linda)
**	    Added tbl_relgwid and tbl_relgwother fields to DMT_TBL_ENTRY struct
**	    for support of non-SQL gateways.
**	14-jun-90 (linda,bryanp)
**	    Added new table access mode DMT_A_OPEN_NOACCESS for use (currently)
**	    by gateways.  Indicates that a table is to be opened but will not
**	    be accessed, only operation which will be performed is a close.
**	    Used to support remove table for gateways where foreign data file
**	    does not exist.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	10-dec-1990 (rogerk)
**	    Changed io_blocking factor to be a float.
**	17-dec-90 (jas)
**	    Smart Disk project integration:
**	    Added tbl_sdtype entry to DMT_TBL_ENTRY structure so that
**	    a table's Smart Disk type, if any, can be returned by
**	    dmt_show.
**	7-jan-91 (seputis)
**	    Added dmt_char_array to dmt_show_cb struct to support options to
**	    the dmt_show call.
**	 7-mar-1991 (bryanp)
**	    Added support for Btree index compression: DMT_INDEX_COMP bitflag.
**      09-sep-1991 (mikem)
**          bug #39632
**          DMT_OPEN now returns # of tuples and # of pages though added fields
**	    in the DMT_CB (dmt_record_count and dmt_page_count) following a 
**	    successful open.
**	15-nov-1991 (fred)
**	    Large object support.  Added DMT_F_PERIPHERAL to attribute
**	    description (DMT_ATTR_ENTRY), and added support for indicating
**	    whether tables contain large objects (DMT_TBL_ENTRY).
**	15-jan-1992 (bryanp)
**	    Temporary table enhancements: added DMT_SESSION_TEMP and
**	    DMT_RECOVERY flags.
**	    Made dmt_location into an array of locations, not just a single one.
**	    Also, added tbl_temporary field to the DMT_TBL_ENTRY.
**	    Added DMT_F_LEGAL_ATTR_BITS for temp table use.
**	    Added allocation and extend characteristics.
**	17-mar-92 (kwatts)
**	    Added tbl_comp_type and tbl_pg_type to DMT_TBL_ENTRY for VME
**	    MPF special.
**      29-August-1992 (rmuth)
**          File extend: add tbl_opage_count to DMT_TBL_ENTRY.
**	23-sep-92 (rickh)
**	    Fixed a nested comment that irritates the "standard=portable"
**	    compile time switch on VMS.
**	5-oct-1992 (ananth)
**	    Expose Relstat2 project.
**	3-oct-1992 (bryanp)
**	    Add missing semicolon.
**	9-oct-1992 (jnash)
**	    Reduced logging project. Add DMT_PG_SYSCAT.
**	28-oct-92 (rickh)
**	    FIPS CONSTRAINTS improvements.
**	30-November-1992 (rmuth)
**	     File Extend V. Add tbl_alloc_pg_count to the DMT_TBL_ENTRY
**	     structure.
**	10-dec-92 (rblumer)
**	    Added bit to for whether a table/index supports a UNIQUE constraint.
**	11-feb-93 (rickh)
**	    Added tbl_allocation and tbl_extend to DMT_TBL_ENTRY.
**	12-feb-93 (jhahn)
**	    Added support for statement level unique constraints. (FIPS)
**	15-may-1993 (rmuth)
**	    Add support for READONLY tables, add DMT_C_READONLY.
**	26-jul-1993 (rmuth)
**	    Add DMT_READONLY and DMT_CONCURRENT_ACCESS to the 
**	    tbl_2_status_mask of DMT_TBL_ENTRY.
**	7-oct-93 (rickh)
**	    Rearrange the table characteristics bits to be in the same order
**	    as the relstat2 bits in DMP.H.
**      20-Oct-1993 (fred)
**          Added support for DMT_C_HAS_EXTENSIONS.  This is used ONLY
**          when creating a session temporary which has large object
**          attributes. 
**	20-jun-1994 (jnash)
**	    Add DMT_A_SYNC_CLOSE table open access mode.
**      19-dec-94 (sarjo01) from 18-feb-94 (markm)
**          added the dmt_mustlock flag to make sure that we readlock even
**          if "nolocks" has been set when updating a relation. (58681)
**	09-jan-1995 (forky01)
**	    Add DMT_C_LOG_CONSISTENT, DMT_C_PHYS_CONSISTENT,
**	    DMT_C_TBL_RECOVERY_ALLOWED
**	17-may-1995 (lewda02/shust01)
**	    RAAT API Project:
**	    Add DMT_C_LOCKMODE to allow user control over locking.
**	    Add DMT_N to allow request of readnolock.
**	7-jun-1995 (lewda02/thaju02)
**	    RAAT API Project:
**	    Add DMT_M_MULTIBUF for (multiple) buffers of
**	    attribute and index information.
**      06-feb-96 (johnst)
**	    Bug 72205
**	    Add DMT_C_HICOMPRESS definition to match the new TCB_C_HICOMPRESS
**	    compression value from dmp.h. The Smart Disk code in opc/opcsd.c
**	    checks the DMT_C_* compression values when examining attributes.
**	06-mar-1996 (stial01 for bryanp)
**	    Add DMT_C_PAGE_SIZE characteristic for temp table page size.
**      06-mar-1996 (stial01)
**          Add tbl_pgsize for variable page size support
**          Maintain dmt_cache_size for each page size
**          Maintain dmt_io_blocking for each page size
**      22-jul-96 (ramra01)
**          Alter Table add/drop column att_intlid is fixed  
**          Add tbl_attr_high_colno for alter table, highest att_intl_id
**      22-nov-1996 (dilma04)
**          Row-Level Locking Project:
**            Add DMT_RIS and DMT_RIX.
**      12-feb-97 (dilma04)
**          Moved lockmode definitions for row locking to dm2t.h because 
**          they belong to lower layer. 
**	21-Mar-1997 (jenjo02)
**	    Table priority project.
**	    Added DMT_C_TABLE_PRIORITY
**	    Added tbl_cachepri to DMT_TBL_ENTRY.
**      28-may-97 (dilma04)
**          Re-added DMT_RIS and DMT_RIX for RAAT row locking support.
**      13-jun-97 (dilma04)
**          Add DMT_CONSTRAINT flag for dmt_flags_mask field of DMT_CB.
**      18-nov-97 (stial01)
**          Added DMT_C_EST_PAGES.
**      02-feb-98 (stial01)
**          Back out 18-nov-97 change.
**	29-jul-1998 (somsa01)
**	    Added code to prevent multiple inclusions of this header file.
**      23-Jul-98 (wanya01)
**          X-integreate change 432896(sarjo01)
**          Bug 67867: added DMT_C_SYS_MAINTAINED characteristic to support
**          logical keys in temp tables.
**	01-feb-1999 (somsa01)
**	    Added DMT_LO_LONG_TEMP to signify that ADP_POP_LONG_TEMP is set.
**	01-dec-2000 (thaju02)
**	    Added DMT_FORCE_NOLOCK. (cont. B103150)
**	15-feb-1999 (nanpr01)
**	    Support update mode locks.
**      18-nov-97 (stial01)
**          Added DMT_C_EST_PAGES, DMT_C_TOTAL_PAGES
**      27-mar-2000 (stial01)
**          Added DMT_BLOB_OPTIM (b101047)
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**      12-oct-2000 (stial01)
**          Added new fields in DMT_TBL_ENTRY to be used by optimizer (opf)
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-Apr-2001 (horda03) Bug 104402
**          Ensure that Foreign Key constraint indices are created with
**          PERSISTENCE. This is different to Primary Key constraints as
**          the index need not be UNIQUE nor have a UNIQUE_SCOPE.
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**	29-Dec-2003 (schka24)
**	    Add partitioning TCB flags and fields.
**      02-jan-2004 (stial01)
**          Added DMT_C_BLOBJOURNALED for SET [NO]BLOBJOURNALING on table
**	13-Jan-2004 (schka24)
**	    More partition stuff, physical partition array.
**	11-Mar-2004 (schka24)
**	    Get rid of DMT_LOC_ENTRY.
**      13-may-2004 (stial01)
**          Removed support for SET [NO]BLOBJOURNALING ON table.
**      07-Jun-2005 (inifa01) b111211 INGSRV2580
**          Verifydb reports error; S_DU1619_NO_VIEW iirelation indicates that
**          there is a view defined for table a (owner ingres), but none exists,
**          when view(s) on a table are dropped.
**          Were making DMT_ALTER call on DMT_C_VIEW, to turn on iirelation.relstat
**          TCB_VBASE bit, indicating view is defined over table(s).  TCB_VBASE
**          not tested anywhere in 2.6 and not turned off when view is dropped.
**          qeu_cview() call to dmt_alter() to set bit has been removed ,
**          removing references to TCB_VBASE, DMT_C_VIEWS & DMT_BASE_VIEW.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	6-Jul-2006 (kschendel)
**	    Comment update re db id's.
**      17-May-2007 (stial01)
**          Added DMT_LOCATOR_TEMP to be created with session lifetime
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      02-Dec-2009 (horda03) B103150
**          Add new flag DMT_NO_LOCK_WAIT.
**/

/*}
** Name: DMT_ATT_ENTRY - Structure for table attribute show information.
**
** Description:
**      This typedef defines the stucture needed for table attribute 
**      information.  If attribute information is requested in the table 
**      show call, the DMT_ATT_ENTRY structure is returned in the 
**      buffer pointed to by DMT_SHOW_CB.**
**      Note:  Included structures are defined in DMF.H if they start with
**      DM_, otherwise they are defined as global DBMS types.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
**      26-feb-87 (jennifer)
**          Added ndefault flag.
**	06-mar-89 (neil)
**          Rules: added rule flag.
**	28-oct-92 (rickh)
**	    Added default id and default pointer for FIPS CONSTRAINTS.
**	22-jul-96 (ramra01)
**	    Alter Table add/drop column att_intlid is fixed  
**	9-dec-04 (inkdo01)
**	    Added att_collID for column level collation.
**	29-Sept-09 (troal01)
**	    Added att_geomtype and att_srid.
**	07-Dec-09 (troal01)
**		Moved DMT_F_SEC_LBL and DMT_F_TPROCPARM defines here after
**		removing DMT_ATTR_ENTRY.
*/
typedef struct
{
    DB_ATT_NAME     att_name;              /* Name of the attribute. */
    i4	    att_number;		   /* Attribute number. */
    i4	    att_offset;		   /* Offset to attribute. */
    i4         att_type;              /* Data type for attribute. */
    i4         att_width;		   /* Storage size of attribute. */
    i4         att_prec;              /* Attribute precision. */
    i4         att_flags;             /* set to DMT_F_NDEFAULT if 
                                           ** attribute is non-nullable and
                                           ** non-defaultable. This can also
					   ** be set to DMT_F_RULE if rules
					   ** are applied.
					   */
#define                 DMT_F_SEC_LBL      0x4000 /* This attribute is the
                                                   ** security label for the
                                                   ** record.
                                                   */
#define                 DMT_F_TPROCPARM    0x8000 /* This attribute is actually
                                                   ** a table procedure parameter
                                                   ** (flag used ONLY in
                                                   ** DMT_ATT_ENTRYs) */
    i4	    att_key_seq_number;	   /* Sequence number of attribute in primary key. */
    DB_DEF_ID	    att_defaultID;	   /* default id */
    PTR		    att_default;	   /* points at storage for the default,
					   ** null if def tuple not read yet */
    i4	    att_intlid;		   /* internal attribute id not altered
					   ** by any drop column ajustments that
					   ** effect att_number.
					   */
    i4		att_collID;		/* collation ID */
    i4		att_srid;		/* Spatial reference ID */
    i2		att_geomtype;	/* Geometry data type code */
}   DMT_ATT_ENTRY;

/*}
** Name:  DMT_ATTR_ENTRY - Structure to identify attributes for table create.
**
** Description:
**      This typedef defines the structured used in a DMT_CREATE_TEMP 
**      operation to describe the attributes of the table to be created.
**
**      Note:  Included structures are defined in DMF.H if they start with
**      DM_, otherwise they are defined as global DBMS types.
** 
** History:
**      01-sep-85 (jennifer)
**          Created.
**	06-mar-89 (neil)
**          Rules: Added DMT_F_RULE for a rule on a column.
**	17-apr-89 (mikem)
**	    Logical key development.   Added DMT_F_WORM and 
**	    DMT_F_SYS_MAINTAINED.
**	15-nov-1991 (fred)
**	    Large object support.  Added DMT_F_PERIPHERAL to
**	    indicate that a column contains a large object (i.e. a peripheral
**	    datatype.
**	15-jan-1992 (bryanp)
**	    Added DMT_F_LEGAL_ATTR_BITS for temp table use.
**	28-oct-92 (rickh)
**	    Added default id for FIPS CONSTRAINTS.  Also, changed DMT_F_NULL
**	    to DMT_F_KNOWN_NOT_NULLABLE.
**	12-feb-93 (teresa)
**	    make attr_default a ptr.
**	15-jun-93 (robf)
**	    Add DMT_F_SEC_LBL and DMT_F_SEC_KEY
**	01-mar-94 (andre)
**	    Fix for bug 60198:
**	    changed DMT_F_LEGAL_ATTR_BITS from 0x01FA to 0x01FB to allow for
**	    temp tables to be created with NOT DEFAULT attributes 
**	9-dec-04 (inkdo01)
**	    Added attr_collID for column level collation.
**	17-april-2008 (dougi)
**	    Added DMT_F_TPROCPARM for procedure parameter "columns" (only used
**	    in DMT_ATT_ENTRY).
**	6-june-2008 (dougi)
**	    Added flags for identity columns.
**	07-Dec-2009 (troal01)
**	    Merged with DMF_ATTR_ENTRY
*/

/*
 * Removed.
 */

/*}
** Name: DMT_CB - DMF table access call control block.
**
** Description:
**      This typedef defines the control block required for
**      the following DMF table operations:
**
**      DMT_ALTER - Alter a system catalog record
**      DMT_CLOSE - Close a table opened by DMT_OPEN
**      DMT_CREATE_TEMP - Create temporary table
**      DMT_DELETE_TEMP - Delete a temporary table
**      DMT_SHOW - Describe a table, it's attributes and indices
**      DMT_OPEN - Open a table
**      DMT_SORT_COST - Estimates the cost of a sort.
**
**      Note:  Included structures are defined in DMF.H if they start with
**      DM_, otherwise they are defined as global DBMS types.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
**      16-may-86 (jennifer)
**          Modified to include query statement id (dmt_sequence) 
**          for table open.
**      12-jul-86 (jennifer)
**          Added flag to indicate creating temporary for loading
**          sorted data.
**      14-jul-86 (jennifer)
**          Added flags and variables to DMTCB to handle sort cost
**          estimates and to indicate if a loaded temporary is to
**          use compressed sort files and eliminate duplicates.
**      14-jul-86 (jennifer)
**          Update to include standard header.
**	25-sep-87 (rogerk)
**	    Added dmt_io_blocking field which will be set to readahead blocking
**	    factor on calls to DMT_SORT_COST.
**	18-oct-88 (rogerk)
**	    Added dmt_cache_size field for the optimizer to use.
**      18-JUL-89 (JENNIFER)
**          Added a flag to indicate that an internal request 
**          for statistical information is being requested.
**          This solves a B1 security problem where statistics
**          could be unpredicatable if B1 labels were checked.
**          For an internal request they aren't checked, for a 
**          normal query on the iihistogram and iistatistics 
**          table they are checked.
**	08-feb-1990 (fred)
**	    Added DMT_LO_TEMP for purposes of correctly naming the tables...
**	14-jun-90 (andre)
**	    defined a new mask over dmt_flags_mask which will be used when
**	    calling dmt_alter() to change table's timestamp.
**	10-dec-1990 (rogerk)
**	    Changed io_blocking factor to be a float.
**      09-sep-1991 (mikem)
**          bug #39632
**          DMT_OPEN now returns # of tuples (dmt_record_count) and # of pages
**	    (dmt_page_count) a table in the DMT_CB following a successful open. 
**
**          Prior to this change QEF, while validating DBP QEP's would first
**          call DMT_SHOW for this information and subsequently call DMT_OPEN.
**          This extra call was expensive on platforms like the RS6000 or
**          DECSTATION where cross process semaphores must be implemented as
**          system calls.  This change allows QEF (qeq_validate()) to eliminate
**          the extra DMT_SHOW call.
**	15-jan-1992 (bryanp)
**	    Temporary table enhancements: added DMT_SESSION_TEMP and
**	    DMT_RECOVERY flags.
**	    Made dmt_location into an array of locations, not just a single one.
**	15-sep-93 (robf)
**	    Add DMT_UPD_ROW_SECLABEL flag for DMT_OPEN.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	20-jun-1994 (jnash)
**	    Add DMT_A_SYNC_CLOSE table open access mode.
**      19-dec-94 (sarjo01) from 18-feb-94 (markm)
**          added the dmt_mustlock flag to make sure that we readlock even
**          if "nolocks" has been set when updating a relation. (58681)
**	17-may-1995 (lewda02/shust01)
**	    RAAT API Project:
**	    Add DMT_N to allow request of readnolock.
**      06-mar-1996 (stial01)
**          Maintain dmt_cache_size for each page size
**          Maintain dmt_io_blocking for each page size
**      22-nov-1996 (dilma04)
**          Row-Level Locking Project:
**            Add DMT_RIS and DMT_RIX.
**      12-feb-97 (dilma04)
**          Moved lockmode definitions for row locking to dm2t.h because
**          they belong to lower layer.
**      28-may-97 (dilma04)
**          Re-added DMT_RIS and DMT_RIX for RAAT row locking support.
**      13-jun-97 (dilma04)
**          Add DMT_CONSTRAINT flag for dmt_flags_mask field.
**	01-feb-1999 (somsa01)
**	    Added DMT_LO_LONG_TEMP to signify that ADP_POP_LONG_TEMP is set.
**	01-dec-2000 (thaju02)
**	    Added DMT_FORCE_NOLOCK. (cont. B103150)
**	13-Jan-2004 (schka24)
**	    Add partitioning info.
**	10-May-2004 (schka24)
**	    Remove blob-optim flag, long-temp flag.
**	22-jun-2004 (thaju02)
**	    Added DMT_LK_ONLN_NEEDED. (B112536)
**	19-Sep-2006 (jonj)
**	    Add dmt_unique_dbid for DMT_INVALIDATE_TCB calls from RDF.
**	7-Aug-2008 (kibro01) b120592
**	    Add flag to mark that we're in an online modify, which changes
**	    the rules within creation of temporary tables
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: DMT_MVCC lock mode to designate
**	    MVCC lock protocols, DMT_CURSOR flag 
**	26-Feb-2010 (jonj)
**	    Add dmt_cursid.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Replace DMT_MVCC lock mode with DMT_MIS, DMT_MIX.
**	17-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Add dmt_crib_ptr.
**	24-Mar-2010 (jonj)
**	    SIR 121619 MVCC blobs: Added DMT_CRIBPTR
*/
typedef struct _DMT_CB
{
    PTR             q_next;
    PTR             q_prev;
    SIZE_TYPE       length;                 /* Length of control block. */
    i2		    type;                   /* Control block type. */
#define                 DMT_TABLE_CB        9
    i2              s_reserved;
    PTR             l_reserved;
    PTR             owner;
    i4         	    ascii_id;             
#define                 DMT_ASCII_ID        CV_C_CONST_MACRO('#', 'D', 'M', 'T')
    DB_ERROR        error;                  /* Common DMF error block. */
    PTR             dmt_db_id;             /* DMF database id for session. */
					/* Really an ODCB pointer, ie the
					** db id is the same for all threads
					** of a session.
					*/
    PTR 	    dmt_tran_id;           /* Transaction identifier. */
    PTR 	    dmt_record_access_id;  /* Identifies record stream. */
    i4         	    dmt_flags_mask;         /* Modifier to operations. */
#define                 DMT_NOWAIT          0x01L
                                            /* Don't want to wait for 
                                            ** lock on open. */
#define                 DMT_FORCE_CLOSE     0x02L
                                            /* This is only used internally
                                            ** and tells DMF to get rid
                                            ** of the TCB cache entry for
                                            ** this table. */
#define                 DMT_LOAD            0x04L
                                            /* DMT_LOAD only valid on 
                                            ** creation of temporary file
                                            ** which will be used for sorted
                                            ** temporary data. */
#define                 DMT_DBMS_REQUEST    0x08L
					    /* This is an internal DBMS
	                                    ** request. */
#define                 DMT_SHOW_STAT       0x010L
					    /* This is an internal DBMS
	                                    ** request for statistics. */
#define			DMT_TIMESTAMP_ONLY  0x20L
					    /*
					    ** This mask will be used when
					    ** calling dmt_alter() to change
					    ** table's timestamp
					    */
#define                 DMT_SESSION_TEMP    0x40L
					    /* This is a session-specific
					    ** user-visible temporary table. */
#define			DMT_RECOVERY	    0x0080L
					    /* This temporary table should log
					    ** its updates and perform recovery
					    */
#define			DMT_LO_TEMP	    0x0100L
					    /* Request for large object temp.
					    ** These are session-lived, but
					    ** don't get update-tracked like
					    ** session temps do.
					    */
#define                 DMT_CONSTRAINT      0x0200L
                                            /* The operation on this table is 
                                            ** associated with a constraint.
                                            */
#define			DMT_LO_HOLDING_TEMP 0x0400L
					    /* (use with DMT_LO_TEMP) Table is
					    ** a blob holding temp and should
					    ** be flagged as such within DMF;
					    ** table will be deleted upon
					    ** calling the "cleanup blob
					    ** objects" call.
					    */
#define			DMT_LK_ONLN_NEEDED  0x0800L
#define			DMT_LOCATOR_TEMP    0x1000L
					    /* Contains dmf locators */
#define			DMT_FORCE_NOLOCK    0x2000L
#define			DMT_ONLINE_MODIFY   0x4000L /* From online modify */
#define			DMT_NO_LOCK_WAIT    0x8000L /* Don't wait for a busy lock */
#define			DMT_CURSOR	    0x10000L /* Table is opened for a cursor */
#define			DMT_CRIBPTR	    0x20000L
					    /* Use MVCC lock level and dmt_cribptr,
					    ** used by blob etabs
					    */

    i4		    dmt_unique_dbid;	    /* Unique DBid from rdr_unique_dbid */
    DB_TAB_ID       dmt_id;                 /* Internal table identifier */
    DB_TAB_NAME     dmt_table;              /* Table name. */
    DB_OWN_NAME     dmt_owner;              /* Owner of table. */
    i4	    	    dmt_sequence;	    /* Sequence number used to relate
					    ** a group of open tables and 
                                            ** indices to the statement they
                                            ** are associated with.
					    */
    i4         	    dmt_lock_mode;          /* Table lock mode. */
#define                 DMT_X               LK_X 
					    /* Exclusive Table lock. */
#define			DMT_U		    LK_U
					    /* Update mode page locks */
#define			DMT_SIX		    LK_SIX
					    /* Shared Table lock with 
                                            ** intent to update. */
#define                 DMT_S               LK_S
					    /* Shared Table lock. */
#define                 DMT_IX              LK_IX
					    /* Exclusive Page lock. */
#define                 DMT_IS              LK_IS
					    /* Shared Page lock. */
#define			DMT_N		    LK_N
                                            /* No read lock protocol */
#define                 DMT_RIS            -1L
                                            /* Shared row lock */
#define                 DMT_RIX            -2L
                                            /* Exclusive row lock */
#define                 DMT_MIS            -3L
					    /* MVCC table lock IS */
#define                 DMT_MIX            -4L
					    /* MVCC table lock IX */

    i4         	    dmt_access_mode;        /* Table access mode. */
#define                 DMT_A_READ          1L
#define                 DMT_A_WRITE         2L
#define                 DMT_A_RONLY         3L
#define			DMT_A_OPEN_NOACCESS 4L
#define			DMT_A_SYNC_CLOSE    5L
    i4         	    dmt_update_mode;        /* Table update mode
                                            ** of the user query (statement).
                                            ** This is needed for each open 
                                            ** associated with the statement. */
#define                 DMT_U_DEFERRED      1L
#define                 DMT_U_DIRECT        2L
    bool            dmt_mustlock;           /* flag indicating query is an
                                            ** update - disables noreadlock */

    DB_TAB_TIMESTAMP
		    dmt_timestamp;	    /* Date of that structural change.*/
    DM_PTR          dmt_attr_array;         /* Create table attribute 
                                            ** infomation for temporary
                                            ** tables. */
    DM_PTR          dmt_key_array;	    /* Create table attribute 
                                            ** infomation for temporary
                                            ** tables. */
    
    DM_DATA         dmt_location;           /* Pointer to an array 
                                            ** of table locations for temporary
                                            ** tables. This is an array up to 
                                            ** DM_LOC_MAX. */
    DM_DATA         dmt_char_array; 	    /* Table characteristics information */
    DM_DATA	    dmt_phypart_array;	    /* Physical partition info array
					    ** (array of DMT_PHYS_PART)
					    */
    i2		    dmt_nparts;		    /* Number of physical partitions */
    i2		    dmt_ndims;		    /* Number of partitioning
					    ** dimensions (relnpartlevels)
					    */
    f4         	    dmt_s_estimated_records;
                                            /* Estimate of number of records
                                            ** to be sorted. */
    i4         	    dmt_s_width;            /* Width of records to sort. */
    f4         	    dmt_s_io_cost;          /* Estimate of i/o cost, returned.*/
    f4         	    dmt_s_cpu_cost;         /* Estimate of cpu cost, returned.*/
    double          dmt_io_blocking[DB_MAX_CACHE]; 
					     /* Readahead blocking factors */
    i4	    	    dmt_cache_size[DB_MAX_CACHE]; 
					    /* Size of Buffer Manager Caches */
    i4	    	    dmt_page_count;	    /* # pgs in relation, returned. */
    i4	    	    dmt_record_count;	    /* # tups in relation, returned. */
    PTR		    dmt_cursid;		    /* For MVCC, abstraction of a
    					    ** cursor id provided by QEF
					    */
    PTR		    dmt_crib_ptr;	    /* Base table's LG_CRIB passed when
    					    ** opening an etab.
					    */
}   DMT_CB;

/*
** }
** Name:  DMT_CHAR_ENTRY - Structure for setting characteristics.
**
** Description:
**      This typedef defines the structure needed to define the 
**      characteristics that a table should adopt when it is opened.
**
**      Note:  Included structures are defined in DMF.H if they start with
**      DM_, otherwise they are defined as global DBMS types.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
**      18-may-86 (jennifer)
**          Modified to include the DMT_C_QID_VIEW for a characteristics so
**          that dmt_alter can be called to set a query id for a view.
**	06-mar-89 (neil)
**          Rules: Added DMT_C_RULE to turn rules on/off.
**	31-jan-90 (teg)
**	    Added DMT_C_COMMENT and DMT_C_SYNONYM to request that DMF set
**	    iirelation.relstat fields for possible comment text existing and
**	    for possible table name synonyms.
**	11-feb-1992 (bryanp)
**	    Added allocation and extend characteristics.
**	15-may-1993 (rmuth)
**	    Add support for READONLY tables, add DMT_C_READONLY.
**      20-Oct-1993 (fred)
**          Added support for DMT_C_HAS_EXTENSIONS.  This is used ONLY
**          when creating a session temporary which has large object
**          attributes. 
**	8-dec-93 (robf)
**          Add support for tables with alarms, DMT_C_ALARM.
**	09-jan-1995 (forky01)
**	    Add DMT_C_LOG_CONSISTENT, DMT_C_PHYS_CONSISTENT,
**	    DMT_C_TBL_RECOVERY_ALLOWED
**	17-may-1995 (lewda02/shust01)
**	    RAAT API Project:
**	    Add DMT_C_LOCKMODE to allow user control.
**	06-mar-1996 (stial01 for bryanp)
**	    Add DMT_C_PAGE_SIZE characteristic for temp table page size.
**	21-Mar-1997 (jenjo02)
**	    Table priority project.
**	    Added DMT_C_TABLE_PRIORITY
**	23-nov-2006 (dougi)
**	    Add DMT_C_VIEWS back into the fold.
*/
typedef struct _DMT_CHAR_ENTRY
{
    i4         char_id;                /* Characteristic identifer. */
#define                 DMT_C_TIMEOUT_LOCK  1L
#define                 DMT_C_PG_LOCKS_MAX  2L
#define                 DMT_C_JOURNALED     3L
#define                 DMT_C_PERMITS       4L
#define                 DMT_C_INTEGRITIES   5L
#define			DMT_C_VIEWS	    6L
#define                 DMT_C_ZOPTSTATS     7L
					    /* The protection values 
                                            ** DMT_C_ALLPRO and DMT_C_RETPRO
					    ** are used together to indicate
                                            ** type of protection that exists
                                            ** for the table.  These variables
                                            ** use negative logic to indicate
                                            ** existence of special protections
					    ** as follows:
                                            ** If both are on then no special
                                            ** proctections exist.  If both
                                            ** are off, then all to all and
                                            ** ret to all are both given.  
                                            ** If DMT_C_ALLPRO is set on and
                                            ** DMT_C_RETPRO is set off then
					    ** retrieve to all is given.  If
                                            ** DMT_C_RETALL is set on and
                                            ** DMT_C_ALLPRO is set off then
                                            ** all to all is given. */
#define                 DMT_C_ALLPRO        8L
		                            /* If set on then protection
                                            ** of all to all not given, if
                                            ** set off, then all to all is 
                                            ** given. */
#define                 DMT_C_PROTECT       8L
#define                 DMT_C_STRUCTURE     9L
#define			DMT_C_SAVEDATE	    10L
#define                 DMT_C_HQID_VIEW     11L
#define                 DMT_C_LQID_VIEW     12L
#define                 DMT_C_RETPRO       13L
		                            /* If set on then protection
                                            ** of ret to all not given, if
                                            ** set off, then ret to all is 
                                            ** given. */
#define                 DMT_C_DUPLICATES    14L
		                            /* If set on then duplicates
                                            ** are allowed, if set off 
                                            ** duplicates are not allowed.
                                            ** Note duplicate checking
                                            ** is never performed on a heap. 
                                            ** This is here for completeness
                                            ** since temporaries will 
                                            ** be other data structures
                                            ** in the future. */
#define                 DMT_C_RULE	    15L /* Turn rules on/off */
#define			DMT_C_COMMENT	    16L /* operate on iirelation.relstat
						** bit TCB_COMMENT (DMT_COMMENT)
						*/
#define			DMT_C_SYNONYM	    17L /* operate on iirelation.relstat
						** bit TCB_SYNONYM (DMT_SYNONYM)
						*/
#define			DMT_C_ALLOCATION    18L /* initial allocation for
						** temp tables.
						*/
#define			DMT_C_EXTEND	    19L /* File extend amount for
						** temp tables.
						*/
#define			DMT_C_READONLY	    20L 
						/* Indicate that a tables
						** [no]readonly status is
						** being changed
						*/
#define                 DMT_C_HAS_EXTENSIONS 21L
    	    	    	    	    	    	/* 
    	    	    	    	    	    	** Indicates that
    	    	    	    	    	    	** table has
    	    	    	    	    	    	** extensions -- i.e.
    	    	    	    	    	    	** has columns that
    	    	    	    	    	    	** are large.
						*/
#define			DMT_C_PAGE_SIZE	    22L
						/* Specifies requested page
						** size for a temporary table.
						*/
#define			DMT_C_TABLE_PRIORITY 23L
						/* Specifies Table priority */
#define			DMT_C_ALARM	    25L
						/*
						** Indicate table alarm status
						** is being changed
						*/
#define			DMT_C_LOG_CONSISTENT 26L
						/*
						** Indicate that a tables
						** logical consistent status
						** is being changed
						*/
#define			DMT_C_PHYS_CONSISTENT 27L
						/*
						** Indicate that a tables
						** physical consistent status
						** is being changed
						*/
#define			DMT_C_TBL_RECOVERY_ALLOWED 28L
						/*
						** Indicate that a tables
						** recovery allowed status
						** is being changed
						*/
#define			DMT_C_LOCKMODE	      29L
						/*
						** If this is set at
						** table open time then
						** use supplied lock mode.
						*/
#define                 DMT_C_SYS_MAINTAINED  30L
                                                /* Set if temp table being
                                                ** created has logical keys
                                                */
#define                 DMT_C_EST_PAGES       31L
						/* Set if QEF sending
						** estimate pages on open
						*/
#define                 DMT_C_TOTAL_PAGES     32L
						/* Set if QEF sending
						** total pages at optimization
						*/
#define			DMT_C_PAGE_TYPE	    33L
						/* Specifies requested page
						** type for a temporary table.
						*/
    i4         char_value;             /* Value of characteristic. */
#define                 DMT_C_ON            1L
#define                 DMT_C_OFF           0L
}   DMT_CHAR_ENTRY;

/*}
** Name: DMT_IDX_ENTRY - Structure for table index information.
**
** Description:
**      This typedef defines the structure needed for the index information.
**      If index information is requested in the table show call,
**      a DMT_IDX_ENTRY structure is returned in the buffer pointed to by
**      DMT_SHW_CB.dmt_index_array.
**
**      Note:  Included structures are defined in DMF.H if they start with
**      DM_, otherwise they are defined as global DBMS types.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
**	12-feb-93 (jhahn)
**	    Added support for statement level unique constraints. (FIPS)
**      15-feb-95 (inkdo01)
**          Added flag for inconsistent index (b66907)
**      06-mar-96 (nanpr01)
**          Add idx_pgsize for variable page size support
**	30-May-2006 (jenjo02)
**	    DB_MAXKEYS replaced with DB_MAXIXATTS.
*/
typedef struct _DMT_IDX_ENTRY
{
    DB_TAB_NAME     idx_name;               /* Internal name of index. */    
    DB_TAB_ID       idx_id;                 /* Internal table identifier */
    i4         	    idx_attr_id[DB_MAXIXATTS];/* Ordinal number of attribute. */
    i4         	    idx_array_count;        /* Number of attributes in index. */
    i4	    	    idx_key_count;	    /* Number of keys in index. */
    i4         	    idx_dpage_count;        /* Not used, returned as zero */
    i4         	    idx_ipage_count;        /* Not used, returned as zero */
    i4         	    idx_status;
#define		DMT_I_STATEMENT_LEVEL_UNIQUE	0x01L /* Does this index support
						      ** a statement level
						      ** unique index index
						      */
#define DMT_I_INCONSISTENT            0x02L /* Index is inconsistent with
                                            ** corresponding base table */
#define DMT_I_PERSISTS_OVER_MODIFIES  0x04L /* Index has "persistence"
					    ** attribute (to support
					    ** ANSI constraint). */
    i4         	    idx_storage_type;       /* Storage structure. */
    i4              idx_pgsize;             /* index page size    */
}   DMT_IDX_ENTRY;

/*}
** Name: DMT_KEY_ENTRY - Structure for table key information.
**
** Description:
**      This typedef defines the stucture needed for table key
**      information.  If your are creating a temporary which will be sorted
**      then the key information must be provided.
**
** History:
**      24-apr-86 (jennifer)
**          Created.
*/
typedef struct
{
    DB_ATT_NAME     key_attr_name;	    /* Name of the attribute. */
    i4	    key_order;              /* Ordereing of key values. */
#define                 DMT_ASCENDING       1L
#define                 DMT_DESCENDING      2L
}   DMT_KEY_ENTRY;

/*}
** Name: DMT_PHYS_PART - DMF physical partition descriptor
**
** Description:
**	A partitioned table needs some way to cross-reference to the
**	actual partitions.  DMF reads and writes rows to the individual
**	partition tables, while logically the user only sees the
**	partitioned master.
**
**	When a partitioned table is opened or described (with dmt-show),
**	an array of DMF_PHYS_PART entries is constructed.  This array
**	is indexed by the physical partition number, and contains that
**	partition's table-ID as well as row and page counts.
**
**	DMF actually stores this array as part of the master TCB.
**	The row and page counts are updated whenever the partition's
**	TCB row and page counts are updated.  (Partitions are never
**	opened without having the master open, so the partition can
**	count on finding the physical partition array around.)
**
**	Fields other than the page/row counts are strictly for the use
**	of DMF.  The PP array entry is (still) relatively small, so it
**	doesn't really make sense to separate out the DMF-only
**	fields.  This may change if it gets much bigger.
**
** History:
**	13-Jan-2004 (schka24)
**	    New for the partitioned tables project.
*/

typedef struct
{
    DB_TAB_ID	pp_tabid;		/* Physical partition table ID */
    i4		pp_reltups;		/* Physical partition row-count */
    i4		pp_relpages;		/* Physical partition page count */
    /* A TCB pointer is convenient within DMF.  Obviously other facilities
    ** won't see it.  The pointer may be NULL if the partition hasn't been
    ** opened or accessed yet.
    */
    struct _DMP_TCB *pp_tcb;		/* Physical partition TCB */
    /* To open a partition, we don't want to re-read the iirelation row
    ** by table ID, because the iirelation key is just the base ID.
    ** There are potentially *many* rows with that base ID ...
    ** Instead, in a hack to be credited entirely to JonJ :-), we'll store
    ** the iirelation row tid.  It takes a sysmod to change iirelation,
    ** so this is actually somewhat safe...
    */
    DB_TID	pp_iirel_tid;		/* Tid of partition iirelation row */
} DMT_PHYS_PART;

/*}
** Name: DMT_COLCOMPARE - column comparison statistics descriptor
**
** Description:
**	Column comparison statistics (proportions of rows lt, eq, gt when
**	2 columns are compared) are kept in an array by table, c/w count
**	of entries.
**
** History:
**	5-jan-06 (dougi)
**	    Created.
*/

typedef struct
{
    i4		col_comp_cnt;		/* count of entries in descr array */
    DB_COLCOMPARE col_comp_array[1];	/* descriptor array */
} DMT_COLCOMPARE;

/*}
** Name: DMT_SHW_CB - DMF describe table call control block.
**
** Description:
**      This typedef defines the control block required for the DMT_SHOW
**      operation to return information about a table, it's attributes and
**      it's indices.  The table, attribute and index information is returned
**      in four different structures: DMT_TBL_ENTRY, DMT_ATT_ENTRY,
**      DMT_IDX_ENTRY and DB_LOC_NAME if multiple locations associated
**      with the table..
**
**      Note:  Included structures are defined in DMF.H if they start with
**      DM_, otherwise they are defined as global DBMS types.
**
**	In order to avoid server deadlock, any DMT_SHW_CB operation can specify
**	a timeout condition in which a timeout error code will be returned if
**	the timeout is reached.  The timeout applies to the table contol lock
**	aquired during the show operation.  It will be overridden by any SET
**	LOCK MODE operation which the user has explicitly specified on the
**	table being shown.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
**      14-jul-86 (jennifer)
**          Update to include standard header.
**      23-nov-87 (jennifer)
**          Added multiple locations per table support.
**	7-jan-91 (seputis)
**	    Added dmt_char_array to support show call options.  The only
**	    currently supported option is TIMEOUT.
**     13-sep-93 (smc)
**          Made dmt_session_id a CS_SID.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**      7-jun-1995 (lewda02/thaju02)
**          RAAT API Project:
**          Add DMT_M_MULTIBUF for (multiple) buffers of
**          attribute and index information.
**	13-Jan-2004 (schka24)
**	    Pass back PP array.  Add updated-counts request.
**	11-Mar-2004 (schka24)
**	    Comment update re locations.
*/
typedef struct
{
    PTR             q_next;
    PTR             q_prev;
    SIZE_TYPE       length;                 /* Length of control block. */
    i2		    type;                   /* Control block type. */
#define                 DMT_SH_CB           10
    i2              s_reserved;
    PTR             l_reserved;
    PTR             owner;
    i4         ascii_id;             
#define                 DMTS_ASCII_ID        CV_C_CONST_MACRO('D', 'M', 'T', 'S')
    DB_ERROR        error;              /* Common DMF error block. */
    PTR		    dmt_db_id;          /* DMF database id for session. */
					/* Really an ODCB pointer, ie the
					** db id is the same for all threads
					** of a session.
					*/
    PTR		    dmt_record_access_id;   /* Optional record stream ID. */
    CS_SID    	    dmt_session_id;	/* Session identifier. */
    i4         dmt_flags_mask;		/* Modifier to operation. */
#define	DMT_M_TABLE         0x001L	/* Want the Table information. */
#define	DMT_M_ATTR          0x002L	/* Want the Attribute information. */
#define	DMT_M_INDEX         0x004L	/* Want the Index information. */
#define DMT_M_PHYPART	    0x008L	/* Return the phypart array */
#define	DMT_M_NAME	    0x010L	/* Doing a show by name. */
#define	DMT_M_ACCESS_ID	    0x020L	/* Doing a show by record access id. */
#define	DMT_M_LOC           0x040L	/* Want the extra locations information */
#define	DMT_M_MULTIBUF      0x080L	/* Doing a show into multiple buffers */
#define DMT_M_COUNT_UPD	    0x100L	/* Update row, page counts */
    DB_TAB_ID	    dmt_tab_id;		/* Table to show by table identifier */
    DB_TAB_NAME     dmt_name;           /* Table name. */
    DB_OWN_NAME     dmt_owner;          /* Table owner. */
    DM_DATA         dmt_table;          /* Table information. */
    DM_PTR          dmt_attr_array;     /* Attribute info (DMT_ATT_ENTRY) */
    DM_PTR          dmt_index_array;    /* Index information. */
    DM_PTR          dmt_loc_array;      /* Location info (DB_LOC_NAME) */
    DM_DATA         dmt_char_array;     /* Show characteristics options. */
    DM_DATA	    dmt_phypart_array;	/* Physical partition info array
					** (array of DMT_PHYS_PART)
					*/
    /* The row/page counts that follow are returned when DMT_M_COUNT_UPD
    ** is requested.  This is a relatively fast mechanism for callers to
    ** get current row-counts without asking for a full table or attribute
    ** update.  For partitioned tables, these are the master counts;  ask
    ** for the PP array to get current partition counts.
    */
    i4		    dmt_reltups;	/* Latest value of # of rows */
    i4		    dmt_relpages;	/* Latest value of # of pages */
}   DMT_SHW_CB;

/*}
** Name: DMT_TBL_ENTRY - Structure returning table information  for DMT_SHOW.
**
** Description:
**      This typedef defines the structure needed to describe a table.
**      If table information is requested in the table describe call, the
**      DMT_TBL_ENTRY structure is returned in the buffer pointed to by
**      DMT_SHOW_CB.dmt_table.
**
**      Note:  Included structures are defined in DMF.H if they start with
**      DM_, otherwise they are defined as global DBMS types.
**
** History:
**      01-sep-85 (jennifer)
**          Created.
**      23-nov-87 (jennifer)
**          Added multiple locations per table support.
**	06-mar-89 (neil)
**          Added DMT_RULE for rules (and filled DMT_GATEWAY gap).
**	31-jan-90 (teg)
**	    added DMT_COMMENT for table/column comments and DMT_SYNONYM for
**	    synonym support.  These correspond to iirelation.relstat bits
**	    TCB_COMMENT and TCB_SYNONYM.
**	02-mar-90 (andre)
**	    defined DMT_VGRANT_OK to correspond to TCB_VGRANT_OK.
**	25-apr-90 (linda)
**	    Added tbl_relgwid and tbl_relgwother fields for support of non-SQL
**	    gateways.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.  Moved over DMT_SYS_MAINTAINED and
**	    DMT_GW_NOUPDT relstat values from 6.3 code.  Had to change value
**	    for DMT_SECURE relstat value because it clashed with GW_NOUPDT.
**	7-mar-1991 (bryanp)
**	    Added support for Btree index compression: DMT_INDEX_COMP bitflag.
**	27-sep-1991 (bryanp)
**	    Temporary table enhancements: added tbl_temporary field to the
**	    DMT_TBL_ENTRY.
**	15-nov-1991 (fred)
**	    Added DMT_TEXTENTSION and DMT_HAS_EXTENSION bits for peripheral
**	    datatype support.
**
**	    Moved these into an ifdef so they aren't used.  reintegrate after a
**	    relstat2 solution is done... 
**	13-mar-92 (kwatts)
**	    Added tbl_comp_type and tbl_pg_type for MPF.
**      29-August-1992 (rmuth)
**          File extend: add tbl_opage_count to DMT_TBL_ENTRY.
**	5-oct-1992 (ananth)
**	    Expose Relstat2 project.
**	9-oct-1992 (jnash)
**	    Reduced logging project. Add DMT_PG_SYSCAT.
**	28-oct-92 (rickh)
**	    FIPS CONSTRAINTS:  New table status word.  Indicates whether
**	    table: is droppable, supports a statement level unique index,
**	    persists over modifies, and/or is system generated.
**	30-November-1992 (rmuth)
**	     File Extend V. Add tbl_alloc_pg_count to the DMT_TBL_ENTRY
**	     structure.
**	10-dec-92 (rblumer)
**	    Added bit to for whether a table/index supports a UNIQUE constraint.
**	11-feb-93 (rickh)
**	    Added tbl_allocation and tbl_extend to DMT_TBL_ENTRY.
**	5-jul-93 (robf)
**	    Added tbl_secid and DMT_ROW_LABEL/AUDIT attributes 
**	26-jul-1993 (rmuth)
**	    Add DMT_READONLY and DMT_CONCURRENT_ACCESS to the 
**	    tbl_2_status_mask of DMT_TBL_ENTRY.
**	7-oct-93 (rickh)
**	    Rearrange the table characteristics bits to be in the same order
**	    as the relstat2 bits in DMP.H.
**	8-oct-93 (rblumer)
**	    Fix a couple unclosed comments.
**	07-jan-94 (andre)
**	    defined DMT_JON over tbl_status_mask to correspond to TCB_JON
**	16-feb-94 (robf)
**          Add DMT_ALARM
**      15-feb-95 (inkdo01)
**          Added flags for logical/physical inconsistencies (b66907)
**      06-mar-96 (stial01)
**          Add tbl_pgsize for variable page size support
**	14 jun-1996 (shero03)
**	    Added Rtree dimension, hilbertsize, and range
**	22-jul-96 (ramra01)
**	    Add tbl_attr_high_colno for alter table, highest att_intl_id
**	21-Mar-1997 (jenjo02)
**	    Table priority project.
**	    Added tbl_cachepri to DMT_TBL_ENTRY.
**      17-Apr-2001 (horda03) Bug 104402
**          Added DMT_NOT_UNIQUE.
**	29-Dec-2004 (schka24)
**	    Add partitioning stuff.
**	05-May-2006 (jenjo02)
**	    Add DMT_CLUSTERED to match TCB_CLUSTERED relstat bit.
**	23-nov-2006 (dougi)
**	    Return 0x40 bit to DMT_BASE_VIEW.
**	26-aug-2008 (dougi)
**	    Add TPROC to storage types - for completeness.
*/
typedef struct _DMT_TBL_ENTRY
{
    DB_TAB_ID	    tbl_id;		    /* Table identifier. */
    DB_TAB_NAME     tbl_name;               /* Table name. */
    DB_OWN_NAME     tbl_owner;              /* Table owner. */
    i4         tbl_loc_count;          /* Count of locations. */
    DB_LOC_NAME     tbl_location;           /* First Table location name. */
    DB_LOC_NAME	    tbl_filename;	    /* Table file name. */
    i4         tbl_attr_count;         /* Table attribute count. */
    i4         tbl_index_count;        /* Count of indexes on table. */
    i4         tbl_width;              /* Table record width. */
    i4         tbl_storage_type;       /* Table storage structure. */
#define                DMT_HEAP_TYPE        3L
#define                DMT_ISAM_TYPE        5L
#define                DMT_HASH_TYPE        7L
#define                DMT_BTREE_TYPE       11L
#define                DMT_RTREE_TYPE       13L
#define		       DMT_TPROC_TYPE	    17L
    i4        tbl_status_mask;         /* Table status. */
				/* *Note* Flags are same as TCB_xxxx
				** flags in dmf!hdr!dmp.h
				*/
#define                DMT_CATALOG          0x000001L
#define                DMT_RD_ONLY          0x000002L
#define                DMT_PROTECTION       0x000004L
#define                DMT_INTEGRITIES      0x000008L
#define                DMT_CONCURRENCY      0x000010L
#define                DMT_VIEW             0x000020L
#define                DMT_CLUSTERED        0x000000L
					    /* Clustered primary Btree */
#define		       DMT_BASE_VIEW	    0x000040L
#define                DMT_IDX              0x000080L
#define                DMT_BINARY           0x000100L
#define                DMT_COMPRESSED       0x000200L
#define		       DMT_INDEX_COMP	    0x000400L
					    /* DMT_INDEX_COMP is on if the
					    ** table or index uses index
					    ** compression. Currently, only
					    ** Btrees support this.
					    */
#define		       DMT_IS_PARTITIONED   0x000800L
					    /* Table is partitioned */
#define                DMT_ALL_PROT         0x001000L
                                            /* If this bit is set in
                                            ** tbl_status_mask then
					    ** you do not have all to all
                                            ** protection, if it is off
                                            ** then you do have all to all
                                            ** protection. */
#define                DMT_RETRIEVE_PRO     0x002000L
                                            /* If this bit is set in
                                            ** tbl_status_mask then
					    ** you do not have ret to all
                                            ** protection, if it is off
                                            ** then you do have ret to all
                                            ** protection. */
#define                DMT_EXTENDED_CAT     0x004000L
#define		       DMT_JON		    0x008000L
#define                DMT_UNIQUEKEYS       0x010000L
#define                DMT_IDXD             0x020000L
#define                DMT_JNL              0x040000L
#define                DMT_ZOPTSTATS        0x100000L
#define                DMT_DUPLICATES       0x200000L
#define                DMT_MULTIPLE_LOC     0x400000L
#define                DMT_GATEWAY     	    0x800000L
#define                DMT_RULE	     	  0x01000000L	/* Rules apply to tbl */
#define		       DMT_SYS_MAINTAINED 0x02000000L
#define		       DMT_GW_NOUPDT	  0x04000000L
#define                DMT_COMMENT        0x08000000L	/* comments on tbl or 
							** a col in the tbl */
#define		       DMT_SYNONYM       0x10000000L	/* synonyms on tbl */
#define		       DMT_VGRANT_OK	 0x20000000L
#define                DMT_SECURE        0x40000000L
					    /*
					    ** owner may grant permits on this
					    ** view
					    */
    i4        tbl_record_count;        /* Table estimated record count. */
    i4        tbl_page_count;          /* Estimated page count in table. */
    i4        tbl_dpage_count;         /* Estimated data page count 
                                            ** in table. */
    i4        tbl_opage_count;         /* Estimated overflow page count
                                            ** in table */
    i4        tbl_ipage_count;         /* Estimated index page count, for
					** btree this includes leaf pages */
    i4	      tbl_lpage_count;         /* Estimated leaf page count */
    i4        tbl_alloc_pg_count;      /* Estimated of the total number
					    ** of pages allocated to the
					    ** table
					     */
    i4	      tbl_tperpage;            /* Estimated tuples per page */
    i4	      tbl_kperpage;            /* Estimated keys per page */
    i4	      tbl_kperleaf;	       /* Estimated keys per leaf */
    i4	      tbl_ixlevels;            /* Estimated number of index levels */
    i4	    tbl_expiration;          /* Table expiration date. */
    DB_TAB_TIMESTAMP
                   tbl_date_modified;       /* Table last modified date. */
    i4        
		   tbl_create_time;         /* Time table created. */
    DB_QRY_ID      tbl_qryid;               /* Associated query id of query
                                            ** text for views. */
    i4        tbl_struct_mod_date;     /* Date structure was modified. */
    i4        tbl_i_fill_factor;       /* Fill factor for index pages. */
    i4	   tbl_d_fill_factor;       /* Fill factor for data pages. */
    i4        tbl_l_fill_factor;       /* Fill factor for BTREE leaf
                                            ** pages. */
    i4        tbl_min_page;            /* Minimum number of pages 
                                            ** specified for modify. */
    i4        tbl_max_page;            /* Maximum number of pages 
                                            ** specified for modify. */
    i4	   tbl_relgwid;		    /* gateway id */
    i4	   tbl_relgwother;	    /* other, gateway-specific info. */
    i4	    tbl_temporary;	    /* non-zero means that this table
					    ** is a temporary table */
    i4		   tbl_comp_type;	    /* Compression scheme */
#define		       DMT_C_NONE	  0L
#define		       DMT_C_STANDARD	  1L
#define		       DMT_C_OLD	  2L
#define		       DMT_C_HICOMPRESS   7L
    i4		   tbl_pg_type;		    /* The page type for this table */
#define		       DMT_PG_STANDARD	  0L
#define		       DMT_PG_SYSCAT	  2L
    i4	   tbl_2_status_mask;
				/* *Note* unlike tbl_status_mask, these
				** flags are NOT always in the same place
				** as the corresponding relstat2 bits!
				*/
#define		       DMT_TEXTENSION	    0x00000001L
					    /*
					    ** Set at table creation time if
					    ** this table is an extension of
					    ** another table -- currently for
					    ** support of peripheral attributes.
					    */
#define		       DMT_HAS_EXTENSIONS   0x00000002L
					    /*
					    ** Set at table creation time if
					    ** this table has any extensions
					    */
#define DMT_PERSISTS_OVER_MODIFIES  0x00000004L /* persists across modifies*/
#define DMT_SUPPORTS_CONSTRAINT	    0x00000008L /* this table/index directly
						** supports a UNIQUE constraint
						** and certain characteristics
						** cannot be changed
						*/
#define DMT_SYSTEM_GENERATED	    0x00000010L /* created by the system */
#define DMT_NOT_DROPPABLE	    0x00000020L /* not user-droppable */
#define DMT_STATEMENT_LEVEL_UNIQUE  0x00000040L /* check uniqueness at stmt end */
#define DMT_READONLY		    0x00000080L /* Readonly Table */	
#define DMT_CONCURRENT_ACCESS       0x00000100L
					/*
					** Used by an Index, to indicate that
					** it is being created in concurrent
					** access mode
					*/
#define DMT_NOT_UNIQUE		    0x00000200L /* Non-unique constraint */
#define DMT_ROW_SEC_AUDIT	    0x00002000L /* Table has row security
						** auditing
						*/
#define DMT_ALARM                   0x00004000L /* Table may have security
						** alarms.
						*/
#define DMT_PARTITION		    0x00008000L	/* Table is a physical
						** partition, not a master
						*/
#define DMT_LOGICAL_INCONSISTENT    0x00010000L /* Table has log. incons.
                                                */
#define DMT_PHYSICAL_INCONSISTENT    0x00020000L /* Table has phy. incons.
                                                */
#define DMT_INCONSIST (DMT_LOGICAL_INCONSISTENT | DMT_PHYSICAL_INCONSISTENT)
#define DMT_GLOBAL_INDEX	    0x00040000L	/* Global secondary index */
    i4             tbl_allocation;	     /* number of pages to initially
					     ** allocate to the table */
    i4             tbl_extend;               /* number of pages to add when
					     ** extending the table */
    i4             tbl_pgsize;               /* table page size      */
    i4	   tbl_attr_high_colno;	     /* highest internal column id
					     ** obtained from att_intl_id
					     */

    i2		   tbl_nparts;		     /* Number of physical partitions */
    i2		   tbl_ndims;		     /* Number of partitioning
					     ** dimensions (relnpartlevels)
					     */

    i2		   tbl_dimension;	     /* RTree dimension		*/
    i2		   tbl_hilbertsize;	     /* RTree Hilbertsize	*/
    i2		   tbl_rangesize;	     /* RTree Range Size	*/
    i2		   tbl_cachepri;	     /* Cache priority		*/
    DB_DT_ID	   tbl_rangedt;		     /* RTree Range Data type	*/
    f8		   tbl_range[DB_MAXRTREE_DIM * 2]; /* RTree range */
}   DMT_TBL_ENTRY;
#endif /* INCLUDE_DMTCB_H */
