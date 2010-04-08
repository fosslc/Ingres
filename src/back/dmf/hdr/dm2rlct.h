/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM2RLCT.H - Definition for the load context.
**
** Description:
**      This file contains the struct needed for handling
**	load files in DMF.
**
** History: $Log-for RCS$
**      04-sep-86 (jennifer)
**          Created for Jupiter.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**/

/*
**  Forward and/or External function references.
*/
typedef struct _DM2R_L_CONTEXT DM2R_L_CONTEXT;

/*}
** Name: DM2R_L_CONTEXT - Structure for loading sort tables.
**
** Description:
**      This typedef defines the structure used for the context
**      required when loading a table with sorted records.
**      This structure points to the control block needed by
**      the build routines, and the sort control block used
**      by the sorter.  It also contains enough room for 
**      a record which is fetched from the sorter and put
**      to the table after all records have been given to the
**      sorter.
**
** History:
**    04-sep-1986 (jennifer)
**          Created for Jupiter.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	22-oct-92 (rickh)
**	    Replaced all references to DM_CMP_LIST with DB_CMP_LIST.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	    In this instance the names of the reserved elements were altered
**          to be consistent with their type.
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**	11-Aug-2004 (schka24)
**	    Build data cleanup: remove some unused stuff.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define LCT_CB as DM_LCT_CB.
*/
struct _DM2R_L_CONTEXT
{
    DM2R_L_CONTEXT  *lct_next;             /* Standard Header */
    DM2R_L_CONTEXT  *lct_prev;
    SIZE_TYPE       lct_length;
    i2              lct_type;
#define                  LCT_CB             DM_LCT_CB
    i2              lct_s_reserved;
    PTR             lct_l_reserved;
    PTR             lct_owner;
    i4              lct_ascii_id;
#define                  LCT_ASCII_ID       CV_C_CONST_MACRO('L', 'C', 'T', ' ')
    DMS_SRT_CB	    *lct_srt;               /* Pointer to sort control
                                            ** block. */
    DM2U_M_CONTEXT  lct_mct;                /* Context needed for building
                                            ** a table. */
    DB_CMP_LIST     *lct_cmp_list;          /* Pointer to sort compare
                                            ** list which is varaible 
					    ** lenght depending on number
                                            ** of keys. */
    i4         lct_c_count;            /* Count of number of entries in
                                            ** compare list. */
    i4         lct_width;              /* Width of record to sort. */
    i4         lct_flags;              /* Load flags specified at
					    ** begin LOAD time. */
    i4         lct_tupadds;            /* Number records loaded */
    char            *lct_record;            /* Pointer to record area. */
};
