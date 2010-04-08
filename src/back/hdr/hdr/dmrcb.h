#ifndef INCLUDE_DMRCB_H
#define INCLUDE_DMRCB_H
/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMRCB.H - Typedefs for DMF record access support.
**
** Description:
**      This file contains the control block used for all record 
**      operations.  Note, not all the fields of a control block
**      need to be set for every operation.  Refer to the Interface
**      Design Specification for inputs required for a specific
**      operation.
**
** History:
**    01-sep-85 (jennifer)
**          Created.
**      12-jul-86 (jennifer)
**          Changed attribute array to DM_PTR from DM_DATA and added
**          flags for QEF support.
**      14-jul-86 (jennifer)
**          Update to include standard header.
**      04-sep-86 (jennifer)
**          Change DMR_RELOAD flag to DMR_END_LOAD flag
**          for the DMR_LOAD operation. 
**	10-sep-86 (jennifer)
**          Added entries for controlling load files.
**	12-mar-87 (rogerk)
**	    Added dmr_count, dmr_mdata fields to DMR_CB for multi-row interface.
**	14-sep-87 (rogerk)
**          Added DMR_HEAPSORT, DMR_EMPTY flags for dmr_cb for use in load.
**	25-sep-87 (rogerk)
**	    Added dmr_s_estimated_records field so that the optimizer can give
**	    estimate of how many rows it expects.
**	12-sep-88 (rogerk)
**	    Changed dmr_q_fcn and dmr_q_arg to be PTR's instead of i4s.
**	    This is to fix compile warnings in QEF.
**	12-feb-90 (rogerk)
**	    Added dmr_char_array for dmr_alter calls.
**	    Added dmr_sequence field for future use.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	17-dec-90 (jas)
**	    Smart Disk project integration:
**	    Add SDSCAN position type for requesting a Smart Disk scan,
**	    and fields to the DMR_CB for passing in a hardware-dependent
**	    encoded search condition for a Smart Disk scan.
**    12-feb-91 (mikem)
**	    Added two new fields dmr_val_logkey and dmr_logkey to the DMR_CB
**	    structure to support returning logical key values to the client
**	    which caused the insert of the tuple and thus caused them to be
**	    assigned.
**    23-jul-91 (markg)
**	    Added new DMR_F_USER_ERROR return status for qualification
**	    function in DMR_CB. (b38679)
**	04-dec-1992 (kwatts)
**	    Introduced DMR_SD_CB for Smart Disk scan initialisation info.
**   26-jul-1993 (rmuth)
**	    Add DMR_CHAR_ENTRIES flag to indicate that we should check to
**	    the char array for additional information on how to build
**	    the table.
**    9-aug-93 (robf)
**	  Add DMR_MINI_XACT flag to indicate operation should be 
**	  done in a mini-xact to prevent aborting later on. This applies
**	  to DMR_PUT only.
**   30-aug-1994 (cohmi01)
**          Added DMR_PREV - read btree backwards. For FASTPATH release. 
**	5-may-95 (stephenb/lewda02)
**	    Add DMR_LAST flag to position at end of btree.
**	17-may-95 (lewda02/shust01)
**	    Add support for control of locking.
**	22-may-95 (cohmi01)
**	    Added DMR_COUNT defines for agg optimization.
**	 7-jun-95 (shust01)
**	    Add support for getting/setting rcb rcb_lowtid, for repositioning
**          of current record pointer
**	    Also added support for setting rcb rcb_current_tid, for 
**	    repositioning of current tid, in case of get current after 
**	    rcb repositioning.
**	21-aug-1995 (cohmi01)
**	    Add new DMR_AGGREGATE request and related dmr_agg_xxxx flds for
**	    simple aggregate processing by  DMF. Supersedes DMR_COUNT.
**	22-apr-96 (inkdo01)
**	    Add Rtree probe opcodes.
**	30-jul-96 (nick)
**	    Add in another field to allow additional function results ; this
**	    was added specifically for 76243/78024 but can be used for 
**	    anything related to a particular dmf_call().
**	16-oct-96 (cohmi01)
**	    Add support for getting readlock=nolock status, for RAAT.
**	28-april-97(angusm)
**	    Add  DMR_XTABLE (cross-table update)
**	11-Feb-1997 (nanpr01)
**	    Bug 80636 : Causes random segv because in dmf the adf_cb is not
**	    initialized. I could initialize it to get rid of the segv, then
**	    the other problems such as if user has set II_DATE_FORMAT variable
**	    is not respected anymore because of arbitrary initialization.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add DMR_SORT flag to DMR_CB.
**      01-dec-97 (stial01)
**          Added DMR_SET_LOWKEY for RAAT access
**	19-dec-97 (inkdo01)
**	    Added DMR_SORT_NOCOPY, DMR_SORTGET for non-temp table sorting.
**      23-feb-98 (stial01)
**          Fixed cross integration of DM2R_XTABLE flag
**	23-jul-1998 (nanpr01)
**	    Added flag to enable positioning to read backwards.
**	29-jul-1998 (somsa01)
**	    Added code to prevent multiple inclusions of this header file.
**      16-nov-98 (stial01)
**          Added DMR_PKEY_PROJECTION
**      02-may-2000 (stial01)
**          Added DMR_RAAT
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**/

/*}
** Name: DMR_ATTR_ENTRY - Structure for qualifying attributes.
**
** Description:
**      This typedef defines the structure for each attribute array
**      entry provided as input in the record operation control 
**      block.  The array is used to describe all the attribute information
**      needed to qualify a record for retrieval.  Each entry contains the
**      ordinal number of an attribute, a pointer to an area containing the 
**      low value associated with this attribute, and a pointer to an area
**      containing the high value associated with this attribute.  If the
**      low pointer is null, then the minimum value is considered to be 
**      negative infinity.  If the high value is null, then the maximum
**      value is considered to be infinity.
**
**      Note:  Included structures are defined in DMF.H if they start
**      with DM_, otherwise they are defined as global DBMS types.
**
** History:
**    01-sep-85 (jennifer)
**          Created.
**    10-sep-86 (jennifer)
**          Added entries for controlling load files.
*/
typedef struct _DMR_ATTR_ENTRY
{
    i4         attr_number;            /* Ordinal number of attribute. */
    i4         attr_operator;          /* Comparison operator for this value */
#define                 DMR_OP_EQ           1L
#define                 DMR_OP_LTE	    2L
#define                 DMR_OP_GTE          3L
#define                 DMR_OP_INTERSECTS   4L
#define                 DMR_OP_OVERLAY	    5L
#define                 DMR_OP_INSIDE	    6L
#define                 DMR_OP_CONTAINS     7L
    char            *attr_value;            /* Value for comparison. */
}   DMR_ATTR_ENTRY;

/*}
** Name:  DMR_CHAR_ENTRY - Structure for setting characteristics.
**
** Description:
**      This typedef defines the structure needed to define the access
**      characteristics that should be used on a table.
**
** History:
**       7-feb-90 (rogerk)
**          Created for dmr_alter calls.
**    7-jan-94 (robf)
**        Add DMR_UPD_ROW_SECLABEL characteristic 
**	17-may-95 (lewda02/shust01)
**	    Add support for control of locking.
**	 7-jun-95 (shust01)
**	    Add support for getting/setting rcb rcb_lowtid, for repositioning
**          of current record pointer
**	16-oct-96 (cohmi01)
**	    Add support for getting readlock=nolock status, for RAAT.
*/
typedef struct _DMR_CHAR_ENTRY
{
    i4         char_id;                /* Characteristic identifer. */
#define                 DMR_SEQUENCE_NUMBER 1L
#define			DMR_LOCK_MODE	    2L
#define			DMR_GET_RCB_LOWTID  3L
#define			DMR_SET_RCB_LOWTID  4L
#define			DMR_SET_CURRENTTID  5L
#define			DMR_GET_DIRTYREAD   6L
#define                 DMR_SET_LOWKEY      7L
    i4         char_value;             /* Value of characteristic. */
#define                 DMR_C_ON            1L
#define                 DMR_C_OFF           0L
}   DMR_CHAR_ENTRY;


/*}
** Name:  DMR_CB - DMF record call control block.
**
** Description:
**      This typedef defines the control block required for the 
**      following DMF record operations:
**
**	DMR_ALTER - Alter record access characteristics.
**      DMR_DELETE - Delete a database table record.
**      DMR_GET - Get a database table record.
**	DMR_LOAD - Load rows into a regular or temporary table.
**      DMR_POSITION - Set scan criteria for a database table.
**      DMR_PUT - Put(insert) a database table record.
**      DMR_REPLACE - Replace(update) a database table record.
**	DMR_AGGREGATE - Execute simple aggregates, scanning any needed data.
**
**      The position table operation sets the starting and ending
**      record positions for a scan.  These positions are based on
**      input from the caller.  The entire table will be scanned
**      (i.e. the starting position is the beginning of the table and
**      the end position is the end of the table) if the flag indicating
**      all is set.  If the input flag indicates TID, then the starting and
**      ending postion will be set so that only the record specified by TID is 
**      available for selection.  In all other cases the positions
**      are calculated from the values provided in the attribute description
**      area of the call control block.  The positioning calculation
**      is as follows:
**
**      START POSITION:
**          The attribute values associated with keys are searched to find the
**          the key with the minimum value.  Once this is determined the
**          starting position is set to the first record having this key
**          value.  
**
**      END POSITION:
**          The attribute values associated with keys are searched to find the
**          key with the maximum value.  Once this is determined the ending 
**          position is set to the last record having this key value.
**
**      The positioning operation does not actually determine if there is a 
**      record which meets all the restrictions for all key and non key 
**      attributes.  If there is no record qualifying, this error will be 
**      returned on the get operation.
**
**      Note:  Included structures are defined in DMF.H if they start with
**      DM_, otherwise they are defined as global DBMS types.
**
** History:
**     01-sep-85 (jennifer)
**          Created.
**      12-jul-86 (jennifer)
**          Changed sttribute array to DM_PTR from DM_DATA and added new
**          flags for QEF support.
**      14-jul-86 (jennifer)
**          Update to include standard header.
**      04-sep-86 (jennifer)
**          Change DMR_RELOAD flag to DMR_END_LOAD flag
**          for the DMR_LOAD operation. 
**      10-sep-86 (jennifer)
**          Added entries for controlling load files.
**	12-mar-87 (rogerk)
**	    Added dmr_count, dmr_mdata fields to DMR_CB for multi-row interface.
**	14-sep-87 (rogerk)
**          Added DMR_HEAPSORT, DMR_EMPTY flags for load
**	25-sep-87 (rogerk)
**	    Added dmr_s_estimated_records field.  This enables the optimizer to
**	    give an estimate of the number of tuples to be added in a LOAD
**	    sequence.  The sorter can then allocate large enough sort buffers to
**	    cut down on IO.
**	12-sep-88 (rogerk)
**	    Changed dmr_q_fcn and dmr_q_arg to be PTR's instead of i4s.
**	    This is to fix compile warnings in QEF.
**	12-feb-90 (rogerk)
**	    Added dmr_char_array for dmr_alter calls.
**	    Added dmr_sequence field for future use.
**	17-dec-90 (jas)
**	    Smart Disk project integration:
**	    Add SDSCAN position type for requesting a Smart Disk scan,
**	    and fields to the DMR_CB for passing in a hardware-dependent
**	    encoded search condition for a Smart Disk scan.
**	12-feb-1991 (mikem)
**	    Added two new fields dmr_val_logkey and dmr_logkey to the DMR_CB
**	    structure to support returning logical key values to the client
**	    which caused the insert of the tuple and thus caused them to be
**	    assigned.
**    23-jul-91 (markg)
**	    Added new DMR_F_USER_ERROR return status for qualification
**	    function. Also included some explanatory comments about the 
**	    actions performed for each qualification function return 
**	    status. (b38679)
**	04-dec-1992 (kwatts)
**	    Replaced two Smart Disk fields with a single pointer dmr_sd_cb
**	    to a Smart Disk control block structure.
**   26-jul-1993 (rmuth)
**	    Add DMR_CHAR_ENTRIES flag to indicate that we should check to
**	    the char array for additional information on how to build
**	    the table.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	21-aug-1995 (cohmi01)
**	    Add new DMR_AGGREGATE request and related dmr_agg_xxxx flds for
**	    simple aggregate processing by  DMF.
**	30-jul-96 (nick)
**	    Add dmr_res_data to permit context specific additional results.
**	11-Feb-1997 (nanpr01)
**	    Bug 80636 : Causes random segv because in dmf the adf_cb is not
**	    initialized. I could initialize it to get rid of the segv, then
**	    the other problems such as if user has set II_DATE_FORMAT variable
**	    is not respected anymore because of arbitrary initialization.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add DMR_SORT flag.
**	23-jul-1998 (nanpr01)
**	    Added flag to enable positioning to read backwards.
**	01-oct-1998 (nanpr01)
**	    added attset to pass which attributes has been changed.
**      05-may-1999 (nanpr01,stial01)
**          Added DMR_DUP_ROLLBACK, set if duplicate will result in rollback
**          This allows DMF to dupcheck in indices after base table is updated.
**      1-Jul-2000 (wanfr01)
**        Bug 101521, INGSRV 1226 -  Add DMR_RAAT flag 
**	17-Dec-2003 (jenjo02)
**	    Added DMR_AGENT, DMR_AGENT_CALLBACK, DMR_NO_SIAGENTS flags
**	    for Parallel/Partitioning project.
**	22-Feb-2008 (kschendel) SIR 122513
**	    Add back-pointer to VALID struct to eliminate issues re searching
**	    for the master's valid list entry when opening partitions.
**	11-Apr-2008 (kschendel)
**	    Expand qual-function mechanism to be more directly suitable to
**	    calling ade-execute-cx, which is the normal case.
**      01-Dec-2008 (coomi01) b121301
**          Add bitmask for context from DSH.
**	23-Mar-2010 (kschendel) SIR 123448
**	    Define no-parallel-sort flag for load.
*/
typedef struct _DMR_CB 
{
    PTR             q_next;
    PTR             q_prev;
    SIZE_TYPE       length;                 /* Length of control block. */
    i2		    type;                   /* Control block type. */
#define                 DMR_RECORD_CB       5
    i2              s_reserved;
    PTR             l_reserved;
    PTR             owner;
    i4         ascii_id;             
#define                 DMR_ASCII_ID        CV_C_CONST_MACRO('#', 'D', 'M', 'R')
    DB_ERROR        error;                  /* Common DMF error block. */
    i4         dmr_flags_mask;         /* Modifier to operation. */
#define                 DMR_NODUPLICATES    0x01
			                    /* DMR_NODUPLICATES is used on first
                                            ** DMR_LOAD call to indicate
                                            ** if the sort should eliminate
                                            ** duplicates. It can be used
                                            ** on get other operation to
                                            ** indicate duplicates are not
                                            ** allowed. */
#define                 DMR_NEXT            0x02
#define                 DMR_CURRENT_POS     0x04
#define                 DMR_BY_TID          0x08
                                            /* Above three items describe
                                            ** type of get operation. */
#define                 DMR_ENDLOAD         0x10     
					    /* DMR_END_LOAD is only used
                                            ** on internal temporary tables
                                            ** used for loading sorted data. 
                                            */

#define			DMR_NOPARALLEL	    0x20
					    /* LOAD: asks for non-parallel
					    ** sort, e.g. for partitioned
					    ** bulk-load which could spawn
					    ** too many threads.
					    */

#define                 DMR_NOSORT          0x40
					    /* DMR_NOSORT is used on first
					    ** DMR_LOAD call to indicate that
					    ** no sorting is necessary -
					    ** either because the records are
					    ** already in sorted order, or
					    ** the table type is HEAP. */
#define                 DMR_TABLE_EXISTS    0x80
					    /* DMR_TABLE_EXISTS is used on first
					    ** DMR_LOAD call to indicate that
					    ** the table being loaded is a
					    ** already existing permanent
					    ** table. */
#define                 DMR_SKIP_DUPKEY     0x100
					    /* DMR_SKIP_DUPKEY is used on first
					    ** DMR_LOAD call if caller wants
					    ** to skip over duplicate keys
					    ** rather than generate an error. */
#define                 DMR_HEAPSORT        0x200
					    /* DMR_HEAPSORT is used on first
					    ** DMR_LOAD call if caller wants to
					    ** sort the rows to be added to the
					    ** heap table. */
#define                 DMR_EMPTY           0x400
					    /* DMR_EMPTY is used on first
					    ** DMR_LOAD call if caller is sure
					    ** that the table is empty and wants
					    ** to avoid scanning the table to
					    ** make sure.
					    */
/* FIXME get rid of dmr-char-entries flag, it exists only because too many
** places were lazy about initing a dmr-cb and the contents of char_array
** is untrustworthy.  (fix qef)
*/
#define			DMR_CHAR_ENTRIES    0x800
					    /* DMR_CHAR_ENTRY used to indicate
					    ** that there may be information
					    ** in dmr_char_array on how to
					    ** build the table
					    */
#define                 DMR_PREV            0x1000
					    /* DMR_PREV is opposite of DMR_NEXT.
					    ** Only valid for BTREE. Ignores
					    ** 'limit key' (as of 8/94) thus it
					    ** reads back till start of table.
					    */
#define			DMR_SORT_NOCOPY	    0x2000
					    /* DMR_SORT_NOCOPY indicates that
					    ** the results of the sort should 
					    ** NOT be copied to the temp table -
					    ** they will be read directly from
					    ** the sort buffers.
					    */
#define			DMR_SORTGET	    0x4000
					    /* DMR_SORTGET indicates a DMR_GET
					    ** request which should be 
					    ** satisfied by the sorter, and NOT
					    ** by reading from the sort temp
					    ** table.
					    */
#define		        DMR_MINI_XACT 	    0x10000
					    /*
					    ** DMR_MINI_XACT is used to indicate
					    ** that the record operation should
					    ** be done in a mini transaction,
					    ** typically to prevent user abort
					    ** later. Only used for DMR_PUT 
					    ** currently
					    */
#define                 DMR_SORT            0x20000
                                            /* This flag is used when getting
                                            ** tuples to append them to a
                                            ** sorter. 
                                            */
#define			DMR_XTABLE	    0x40000
					    /*
					    ** cross-table update
					    */
#define                 DMR_PKEY_PROJECTION 0x80000
#define                 DMR_DUP_ROLLBACK    0x100000
#define                 DMR_RAAT            0x200000
#define                 DMR_AGENT           0x400000
					    /* Use dmrAgent thread to
					    ** execute DMR request
					    */
#define                 DMR_AGENT_CALLBACK  0x800000
					    /* dmrAgent callback to  
					    ** dmr? function
					    ** (set internally)
					    */
#define                 DMR_NO_SIAGENTS     0x1000000
					    /* Do not use siAgents to
					    ** update indexes.
					    */
    i4         dmr_position_type;       /* Type of positioning data. */
#define                 DMR_ALL             1L
#define                 DMR_QUAL            2L
#define                 DMR_REPOSITION      3L
                                            /* Reposition using previous
                                            ** qualification. */
#define                 DMR_TID             4L
#define			DMR_LAST	    6L /* last record in btree */
#define			DMR_ENDQUAL	    7L /* position at the end of qual */
    PTR		     dmr_access_id;         /* DMF access ID which is returned
                                            ** from an open table operation
                                            ** and must be provided on each 
                                            ** subsequent associated record 
                                            ** operation. */
    i4         	    dmr_tid;                /* On input this is the TID
                                            ** (internal record identifier)
                                            ** used for positioning.  On output
                                            ** it identifies the record 
                                            ** returned. */
    i4	    	    dmr_count;		    /* Number of records in dmr_mdata
					    ** list. */
    DM_MDATA	    *dmr_mdata;		    /* List of DM_MDATA structs - each
					    ** pointing to one data record. */
    DM_DATA         dmr_data;               /* Single record data area. */
    DM_PTR          dmr_attr_desc;          /* Pointer to an array of pts to
                                            ** attributes used to qualify
                                            ** which records will be 
                                            ** retrieved. */

    /* The qualification mechanism is optimized for calling a CX to be
    ** run against the given row.  As such, we pass:
    ** - where to put the current row address;
    ** - the function to call, and a parameter to pass to it;
    ** - where to look for the qualification result, an i4.
    ** If the function doesn't return E_DB_OK, we have an error situation.
    ** If the qualification result isn't ADE_TRUE, the row does not qualify.
    **
    ** Thus:
    **  *dmr_q_rowaddr = this row addr;
    **  status = (*dmr_q_fcn)(ADF_SCB, dmr_q_arg);
    **  if (status != E_DB_OK)
    **      some error occurred;
    **  if (*dmr_q_retval == ADE_TRUE)
    **      row qualifies, break;
    **  else get next row to qualify.
    **
    ** For the usual QEF-orig row qualification, the row address is stashed
    ** in either the global basearray or the CX base array, the qfunction
    ** is ade_execute_cx itself, the parameter is an ADE_EXCB, and the
    ** result is the excb_value.
    **
    ** For special cases, such as are common in qeu, the row address and
    ** function result are usually if not always a little structure (on
    ** the calling routine's stack);  the struct can hold additional
    ** params, whatever is needed, and the qual function is some sort of
    ** special purpose comparer.
    */

    PTR		    *dmr_q_rowaddr;	    /* Address of row to be qualified
					    ** is stashed here before calling
					    ** the qual function. */

    DB_STATUS	    (*dmr_q_fcn)(void *,void *);
					    /* Function to call to qualify
					    ** some particular row. */

    void	    *dmr_q_arg;		    /* Argument to qualifcation function. */
    i4		    *dmr_q_retval;	    /* Qual func sets *retval to
					    ** ADE_TRUE if row qualifies */
    PTR		    dmr_qef_adf_cb;         /* ptr to QEF ADF_CB passed
					    ** to DMF for update after qual
					    ** function is run.  Update warning
					    ** info in this ADF_CB if there
					    ** were any non-issued arithmetic
					    ** warnings during qualification.
					    */

    /* Back-pointer to QEF VALID struct that was used to open this (master)
    ** dmr-cb.  This is purely for the convenience of QEF, and is not used
    ** at all by DMF.  This will only be filled in for regular tables opened
    ** by regular validation-open calls in QEF.
    ** It's rather unfortunate that this has to be in the DMR_CB, but
    ** QEF has no other decent way of tracking it short of much more
    ** complicated new machinery.
    */
    struct _QEF_VALID *dmr_qef_valid;

    i4	    dmr_s_estimated_records;/* Optimizers estimate of how many
					    ** rows will be added during load
					    ** sequence.
					    */
    i4	    dmr_sequence;	    /* statement seq. # - not used */
    i4 	    dmr_val_logkey;    	    /* If either DMR_TABKEY or 
					    ** DMR_OBJKEY is asserted then there
					    ** is a valid logical key stored in
					    ** dmr_logkey.
					    */
#define			DMR_TABKEY	    0x01L
					    /* If asserted then a table_key was
					    ** assigned by the last insert.
					    */
#define			DMR_OBJKEY	    0x02L
					    /* If asserted then an object_key 
					    ** was assigned by the last insert.
					    */
    DB_OBJ_LOGKEY_INTERNAL 
		    dmr_logkey;	    	    /* logical key last assigned - to
					    ** be used to pass this info up
					    ** through the interfaces and 
					    ** eventually to the client which
					    ** caused the insert.
					    */
    DM_DATA         dmr_char_array;         /* Record access characteristics */
    i4 	    dmr_agg_flags;	    /* bits indicating what types
					    ** of aggregates are present in
					    ** a DMR_AGGREGATE request. If no
					    ** bits are on, any combination
					    ** of aggs may be present, else
					    ** just the ones indicated are.
					    */
#define DMR_AGLIM_COUNT       0x0001          /* COUNT aggregate requested  */
#define DMR_AGLIM_MAX         0x0002          /* MAX aggregate requested    */
#define DMR_AGLIM_MIN         0x0004          /* MIN aggregate requested    */
    PTR		    dmr_agg_excb;	    /* ADE_EXCB to use when executing
					    ** a DMR_AGGREGATE request.
					    */
    PTR		    dmr_res_data;	    /* ptr to additional result block */
    PTR		    dmr_attset;		    /* Ptr to an array indicating
					    ** which attribute has been 
					    ** updated
					    */
    BITFLD          dmr_dsh_isdbp:1;        /* Reflect QEQP_ISDBP Flag      */
    BITFLD          dmr_bits_free:31;       /* reserved for future use      */

}   DMR_CB;

#endif /* INCLUDE_DMRCB_H */
