/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM2R.H - Definition for the record level services.
**
** Description:
**      This file contains the types and constants used by the record processing
**	services of DMF.
**
** History:
**      17-dec-1985 (derek)
**          Created new for Jupiter.
**	25-sep-89 (paul)
**	    Add new flag to indicate that dm2r_replace should return the
**	    new TID after a replace operation.
**      29-apr-91 (Derek)
**          Add new copy feature flag: DM2R_EMPTY.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    17-dec-1990 (jas)
**		Add new Position flag DM2R_SDSCAN, to request a Smart Disk scan.
**	    22-mar-1991 (bryanp)
**		B36630: Added FUNC_EXTERN for dm2r_set_rcb_btbi_info().
**     7-July-1992 (rmuth)
**	    Prototyping DMF
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed obsolete rcb_bt_bi_logging and rcb_merge_logging fields.
**	    Deleted dm2r_set_rcb_btbi_info routine.
**	26-July-1993 (rmuth)
**	    Allow the caller of dm2r_load to pass information on how to
**	    to build the table. This is passed in the following structure
**	    DM2R_BUILD_TBL_INFO.
**      30-Aug-1994 (cohmi01)
**          Add DM2R_GETPREV to read btree backwards. For FASTPATH release.
**      19-apr-1995 (stial01)
**          Added DM2R_LAST to perform btree select last for MANMANX.
**	22-May-1995 (cohmi01)
**	    Added dm2r_count() prototype and defines for agg optimization.
**	10-aug-1995 (sarjo01)
**	    Added DM2R_L_FLOAD flag for fastload. 
**	2-july-1997(angusm)
**	bug 79623: cross-table updates, ambiguous replace: add DM2R_XTABLE,
**      14-may-97 (dilma04)
**          Replaced decimal constants in get flags with hex numbers,
**          because they are used as bit flags.  
**	19-dec-97 (inkdo01)
**	    Added DM2R_L_SORTNC flag for sorts whose results are NOT copied
**	    to a temp table.
**      23-feb-98 (stial01)
**          Fixed cross integration of DM2R_XTABLE flag.
**	23-jul-1998 (nanpr01)
**	    Added flag to position at the end of qualification range.
**      16-nov-98 (stial01)
**          Added DM2R_PKEY_PROJ
**      05-may-1999 (nanpr01)
**          Changed prototype for dm2r_return_rcb
**      05-may-1999 (nanpr01,stial01)
**          Added DM2R_DUP_CONTINUE
**      23-sep-1999 (stial01)
**          Added DM2R_HAVE_DATA, DM2R_IN_PLACE (STARTRAK 7391263,2)
**	20-oct-1999 (nanpr01)
**	    Made all the DM2R_??? flag bit mask flag and added DM2R_NOREADAHEAD
**	    flag for read-ahead override.
**      02-may-2000 (stial01)
**          Added DM2R_RAAT
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**      1-Jul-2000 (wanfr01)
**        Bug 101521, INGSRV 1226 - Added DM2R_RAAT flag to detect 
**        if this query came from RAAT
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2r? functions converted to DB_ERROR *
**          
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-apr-2003 (stial01)
**          Added prototypes for dmf projection functions (b110061)
**	22-Jan-2004 (jenjo02)
**	    Added DM2R_SI_AGENT, DM2R_MPUT, DM2R_MREPLACE,
**	    DM2R_MDELETE flags, function prototypes for
**	    dm2r_si_put(), dm2r_si_delete(), dm2r_si_replace(),
**	    for parallel secondary index update Agents.
**	11-Apr-2008 (kschendel)
**	    Remove qual func stuff from dm2r-position call.
**	    It's a little more complex now and is setup into the rcb
**	    directly.
**	23-Mar-2010 (kschendel) SIR 123448
**	    Define no-parallel-sort flag for load.
*/


/*
**  Position flags.
*/

#define			DM2R_ALL		0x00000001L
#define			DM2R_QUAL		0x00000002L
#define			DM2R_REPOSITION		0x00000004L
#define                 DM2R_LAST       	0x00000010L
#define                 DM2R_ENDQUAL    	0x00000020L
#define                 DM2R_NOREADAHEAD    	0x00000040L

/*
**  Get flags.
*/

#define                 DM2R_COUNT_KEYRANGE 0x0001L
#define                 DM2R_COUNT_KEYVAL   0x0002L
#define                 DM2R_COUNT_TABLE    0x0004L
#define                 DM2R_GETPREV        0x0008L
#define			DM2R_RETURNTID	    0x0010L
#define			DM2R_NOLOG	    0x0020L
#define			DM2R_DUPLICATES	    0x0040L
#define			DM2R_BYTID	    0x0080L
#define			DM2R_GETNEXT	    0x0100L
#define			DM2R_BYPOSITION	    0x0200L 
#define			DM2R_NODUPLICATES   0x0400L
#define                 DM2R_XTABLE         0x0800L
#define                 DM2R_PKEY_PROJ      0x1000L
#define                 DM2R_DUP_CONTINUE   0x2000L
#define                 DM2R_HAVE_DATA      0x4000L
#define                 DM2R_IN_PLACE       0x8000L
#define                 DM2R_RAAT          0x10000L
/* Secondary index updates are done using dmrAgents */
#define                 DM2R_SI_AGENT      0x20000L
#define                 DM2R_MPUT          0x40000L
#define                 DM2R_MREPLACE      0x80000L
#define                 DM2R_MDELETE       0x100000L
#define			DM2R_REDO_SF	   0x200000L

/*
**  Load type.
*/

#define			DM2R_L_BEGIN 	    01L
#define			DM2R_L_END	    02L
#define			DM2R_L_ADD	    03L
#define                 DM2R_L_FLOAD        64L
/*
**  Load flags.
*/
/* unused		unused		    0x01 */
#define                 DM2R_ELIMINATE_DUPS 0x02
#define                 DM2R_NOSORT         0x04
#define                 DM2R_TABLE_EXISTS   0x010
#define                 DM2R_DUPKEY_ERR     0x020
#define                 DM2R_EMPTY          0x040
#define			DM2R_SORTNC	    0x080
#define			DM2R_NOPARALLEL	    0x100

/*}
** Name: DM2R_KEY_DESC - Structure for qualifying attributes.
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
** History:
**    15-jan-1985 (derek)
**          Created for Jupiter.
*/
typedef struct _DM2R_KEY_DESC
{
    i4         attr_number;            /* Ordinal number of attribute. */
    i4         attr_operator;          /* Comparison operator for 
                                            ** this value */
#define                 DM2R_EQ             1L
#define                 DM2R_LTE	    2L
#define                 DM2R_GTE            3L
#define			DM2R_LT	            4L
#define			DM2R_GT	            5L
    char            *attr_value;            /* Value for comparison. */
}   DM2R_KEY_DESC;

/*}
** Name: DM2R_BUILD_TBL_INFO - Holds information on how to build table.
**
** Description:
**	Holds information on how to build the table.
**	
** History:
**    26-July-1993 (rmuth)
**	    Created.
*/
typedef struct _DM2R_BUILD_TBL_INFO
{
    i4	    dm2r_fillfactor;	/* Fillfactor for table */
    i4	    dm2r_leaffill;	/* Fillfactor for BTREE leafs */
    i4	    dm2r_nonleaffill;   /* Fillfactor for BTREE idx pages */
    i4	    dm2r_allocation;	/* Allocation size for table */
    i4	    dm2r_extend;	/* Extend size for table */
    i4	    dm2r_hash_buckets;  /* HASH buckets */
}   DM2R_BUILD_TBL_INFO;

FUNC_EXTERN DB_STATUS	dm2r_rcb_allocate(
				DMP_TCB         *tcb,
				DMP_RCB         *base_rcb,
				DB_TRAN_ID      *tran_id,
				i4         lock_id,
				i4         log_id,
				DMP_RCB         **rcb,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm2r_release_rcb(
				DMP_RCB         **rcb,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm2r_return_rcb(
				DMP_RCB		**rcb,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm2r_delete(
				DMP_RCB         *rcb,
				DM_TID          *tid,
				i4         flag,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm2r_si_delete(
				DMP_RCB		*ir,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm2r_get(
				DMP_RCB         *rcb,
				DM_TID          *tid,
				i4         opflag,
				char            *record,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm2r_position(
				DMP_RCB         *rcb,
				i4         flag,
				DM2R_KEY_DESC   *key_desc,
				i4         key_count,
				DM_TID          *tid,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm2r_put(
				DMP_RCB         *rcb,
				i4         flag,
				char            *record,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm2r_si_put(
				DMP_RCB		*ir,
				char            *record,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm2r_replace(
				DMP_RCB         *rcb,
				DM_TID          *tid,
				i4         flag,
				char            *record,
				char		*attset,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm2r_si_replace(
				DMP_RCB		*ir,
				char            *record,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm2r_unfix_pages(
				DMP_RCB		*rcb,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm2r_load(
				DMP_RCB         *rcb,
				DMP_TCB         *tcb,
				i4         type,
				i4         flag,
				i4         *count,
				DM_MDATA        *record_list,
				i4         est_rows,
				DM2R_BUILD_TBL_INFO *build_tbl_info,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS 	dm2r_count(
    				DMP_RCB     *rcb,
				DM_TID      *tid,
	    			i4     opflag,
				char        *dummy,
				DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS	dm2r_init_projection(
				DMP_RCB	    *rcb,
				DB_ERROR	*dberr );


FUNC_EXTERN DB_STATUS	dm2r_init_proj_cols(
				DMP_RCB	    *rcb,
				i4	    proj_col_map[],
				DB_ERROR	*dberr );
