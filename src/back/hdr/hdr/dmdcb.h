/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMDCB.H - Typedefs for DMF debug support.
**
** Description:
**    This file contains the control block used for all debug/trace operations.
**    Note, not all the fields of a control block need to be set for every operation.
**    Refer to the Interface Design Specification for 
**    inputs required for a specific operation.
**
** History:    
**    01-sep-85 (j. lyerla) 
**        Created.
**      14-jul-86 (jennifer)
**          Update to include standard header.
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
** Name:    DMD_CB - DMF debug call control block.
**
** Description:
**    This typedef defines the control block required for
**    the following DMF debug and tracing operations:
**
**    DMD_ALTER_CB - Alter a DMF internal Control Block.
**    DMD_CANCEL_TRACE - Cancel a DMF trace.
**    DMD_DISPLAY_TRACE - Display results of DMF trace.
**    DMD_SNAP_CB - Dump the contents of all internal DMF
**        Control Block.
**    DMD_RELEASE_CB - Free an internal DMF control Block.
**    DMD_SET_TRACE - Start a DMF trace.
**    DMD_SHOW_CB - Dump the contents of a specified type 
**        of internal Control Block.
**
**     Note: Included structures are defined in DMF.H if
**     they start with DM_, otherwise they are defined as 
**     global DBMS types.
**
** History:
**    01-sep-85 (jennifer) 
**        Created.
**      14-jul-86 (jennifer)
**          Update to include standard header.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
typedef struct _DMD_CB 
{
    PTR             q_next;
    PTR             q_prev;
    SIZE_TYPE       length;                 /* Length of control block. */
    i2		    type;                   /* Control block type. */
#define                 DMD_DEBUG_CB        1
    i2              s_reserved;
    PTR             l_reserved;
    PTR             owner;
    i4         ascii_id;             
#define                 DMD_ASCII_ID        CV_C_CONST_MACRO('#', 'D', 'M', 'D')
    DB_ERROR        error;                  /* Common DMF error block. */ 
    i4         dmd_flags_mask;         /* Modifier to operation. */
#define                 DMD_ALL             0x01L
#define                 DMD_DUMP            0x02L
#define                 DMD_SEARCH          0x04L
    i4         dmd_cb_type;            /* Internal control block
                                            ** to dump. */
    i4         dmd_context;            /* For dumping multiple control 
                                            ** blocks, contains the scan 
                                            ** criteria. */
    char            *dmd_cb_address;        /* Address of control block 
                                            ** dumped. */
    DM_DATA         dmd_data;               /* Control block data area. */
    i4         dmd_trace_point;        /* Trace identifier. */
    i4         dmd_iterate_value;      /* Trace iteration control. */
    i4         dmd_value;              /* Trace point value. */
}   DMD_CB;
