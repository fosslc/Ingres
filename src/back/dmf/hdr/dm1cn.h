
/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM1CN.H - V1 page and standard compression
**
** Description:
**      This header describes the visible standard accessors so that they
**	can also be used in other formats. These are the compression
**	accessors.
**
** History: 
**	16-jul-1992 (kwatts)
**	    MPF project. Created.
**	16-sep-1992 (jnash)
**	    Reduced logging project.
**	     -  Add FUNC_EXTERNs for dm1cn_format, dm1cn_load, dm1cn_isnew,
**		dm1cn_reclen, dm1cn_get_offset, used by dm1cs.c.  
**	     -  Add record_type and tid param's to dm1cn_uncompress, required
**		to support old style compression within dm1cn_uncompress().
**	22-oct-1992 (rickh)
**	    Replaced all references to DMP_ATTS with DB_ATTS.
**	03-nov-1992 (jnash))
**	    Add dm1cn_reallocate FUNC_EXTERN.
**	24-july-1997(angusm)
**	    Add dmpp_dput().
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument to dm1cn_format, dm1cn_reallocate,
**		dm1cn_reclen prototypes.
**	06-may-1996 (thaju02)
**	    Add page_size argument to dm1cn_isnew.
**	20-may-1996 (ramra01)
**	    Added new param to the load accessor
**      22-jul-1996 (ramra01 for bryanp)
**          Add row_version argument to dm1cn_uncompress.
**          Add current_table_version(tup_info) argument to dm1cn_load.
**	06-sep-1996 (canor01)
**	    Add buffer argument to dm1cn_uncompress prototype.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      05-oct-98 (stial01)
**          Updated dmpp_dput parameters, Added dmpp_put_hdr
**	13-aug-99 (stephenb)
**	    Alter def for dm1cn_uncomress.
**	05-dec-99 (nanpr01)
**	    Fixed up the prototypes for performance.
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
**      30-oct-2000 (stial01)
**          Changed prototype for dmpp_isnew page accessor
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      28-may-2003 (stial01)
**          Added line_start parameter to dmpp_allocate accessor
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      27-sep-2006 (stial01)
**          Fixed prototype for dm1cn_uncompress()
**	12-Apr-2008 (kschendel)
**	    Pass adf-cb to uncompress, not ptr.
**	23-Oct-2009 (kschendel) SIR 122739
**	    Call sequence changes for new rowaccessor scheme.
**	    Define a compression-control array to speed up standard
**	    (un)compression operations.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add lg_id parameter to
**	    dm1cn_delete, dm1cn_get, dm1cn_put prototypes.
**	9-Jul-2010 (kschendel) sir 123450
**	    Add BYTE column compression ops.
**      12-Jul-2010 (stial01) (SIR 121619 MVCC, B124076, B124077)
**          Prototype changes.
**/

/* Definition of compression / decompression control array.
** This array is computed when a row-accessor that uses standard compression
** is set up.  The idea is to allow [un]compress to process non-
** compressed attributes collectively, instead of moving them one at
** a time;  and also to select the [un]compression operation directly,
** instead of doing lots of if-testing and jumping around for rare
** cases (such as row versioning).
*/

struct _DM1CN_CMPCONTROL
{
    i1		opcode;		/* DM1CN_xxx Operation code, defined below */
    u_i1	flags;		/* Variable length / blob flags */
    u_i2	attno;		/* Attribute number (zero origin) if needed */
    u_i4	length;		/* Associated length, EXCLUDING any null
				** indicator */
};

typedef struct _DM1CN_CMPCONTROL DM1CN_CMPCONTROL;

/* Opcodes for standard compression/decompression control array:
** - COPYN just copies N bytes without conversion.  If there is no
**   null or row-version check needed, this may span multiple attributes.
**   (This is the only opcode that might span attributes;  everything
**   else operates on one specific attribute.)
**
** - type-specific opcodes specify (de)compression of that specific
**   type of attribute.
**
** - NULL CHECK applies to a nullable column.  All nulls (de)compress the
**   same, so if the value is NULL, we handle it and skip the (immediately
**   following) opcode for that column.  Obviously that following opcode
**   may only deal with one column, even if it's a "copy".
**
** - VERSION CHECK applies to a versioned column.  It applies row-versioning
**   checks, which are simple (use or don't use) for compression, but
**   can get fancy for decompression.  VERSION CHECK may end up skipping
**   over a following null check AND the following column-specific opcode.
*/

/*	notused			0	don't use */
#define DM1CN_OP_COPYN		1	/* Copy N bytes */
#define DM1CN_OP_CHR		2	/* C type: stored as ASCIZ */
#define DM1CN_OP_CHA		3	/* CHAR type: stored as varchar */
#define DM1CN_OP_VCH		4	/* VARCHAR, VARBYTE, or TEXT:
					** store actual length */
#define DM1CN_OP_NCHR		5	/* NCHAR: like char, for unicode */
#define DM1CN_OP_NVCHR		6	/* NVARCHAR: like varchar for unicode */
#define DM1CN_OP_BYTE		7	/* BYTE: like char, zeros not blanks */
#define DM1CN_OP_VLUDT		8	/* User defined, variable length */
#define DM1CN_OP_NULLCHK	9	/* Check for null, skip NI if null */
#define DM1CN_OP_VERSCHK	10	/* Check row-versioning, can skip any
					** following null-check and NI */

/* Compression control flags */

#define DM1CN_NULLABLE		0x01	/* Column is nullable
					** TRICKY, must be 1, nullable length
					** is op->length + flags&1 and no
					** branching needed.
					** The devil made me do it. */

#define DM1CN_PERIPHERAL	0x02	/* Column is a large-object column.
					** Used with NULLCHECK to indicate
					** that we still store the coupon */

#define DM1CN_DROPPED		0x04	/* Column has been dropped.
					** Used with VERSCHK to quickly skip
					** attributes no longer in the current
					** version of the row;  or to decide
					** whether an altered column was
					** ultimately dropped entirely or not.
					*/

#define DM1CN_ALTERED		0x08	/* Column has been altered, and not
					** entirely dropped later on.
					** Also used with VERSCHK to show that
					** an altered column is still part of
					** the current-version table row. */



/* V1 page level routines */

FUNC_EXTERN DB_STATUS   dm1cn_allocate(
			    DMPP_PAGE               *data,
			    DMP_RCB                 *r,
			    char		    *record,
			    i4                      record_size,
			    DM_TID                  *tid,
			    i4			    line_start);

FUNC_EXTERN bool	dm1cn_isempty(
			    DMPP_PAGE               *data,
			    DMP_RCB                 *r);

FUNC_EXTERN DB_STATUS	dm1cn_check_page(
			    DM1U_CB                 *dm1u,
			    DMPP_PAGE               *page,
			    i4                      page_size);

FUNC_EXTERN VOID	dm1cn_delete(
			    i4		    page_type,	
			    i4		    page_size,
			    DMPP_PAGE	    *page,
			    i4              update_mode,
			    i4         reclaim_space,
			    DB_TRAN_ID      *tran_id,
			    u_i2	    lg_id,
			    DM_TID          *tid,
			    i4              record_size);

FUNC_EXTERN VOID	dm1cn_format(
			    i4		    page_type,
			    i4		    page_size,
			    DMPP_PAGE       *page,
			    DM_PAGENO       pageno,
			    i4              stat,
			    i4		    fill_option);

FUNC_EXTERN DB_STATUS  dm1cn_get(
			    i4		    page_type,
			    i4		    page_size,
			    DMPP_PAGE	    *page,
			    DM_TID          *tid,
			    i4      *record_size,
			    char            **record,
			    i4		    *row_version,
			    u_i4	    *row_low_tran,
			    u_i2	    *row_lg_id,
			    DMPP_SEG_HDR    *seg_hdr);

FUNC_EXTERN i4          dm1cn_get_offset(
			    i4		    page_type,
			    i4		    page_size,
			    DMPP_PAGE       *page,
			    i4             item);

FUNC_EXTERN i4		dm1cn_getfree(
			    DMPP_PAGE       *page,
			    i4              page_size);

FUNC_EXTERN i4 	dm1cn_isnew(
			    DMP_RCB	    *r,
			    DMPP_PAGE       *page,
			    i4             line_num);

FUNC_EXTERN i4		dm1cn_ixsearch(
			    i4		    page_type,
			    i4		    page_size,
			    DMPP_PAGE       *page,
			    DB_ATTS         **keyatts,
			    i4              keys_given,
			    char            *key,
			    bool            partialkey,
			    i4              direction,
			    i4              relcmptlvl,
			    ADF_CB          *adf_cb);

FUNC_EXTERN DB_STATUS    dm1cn_load(
			    i4		    page_type,
			    i4		    page_size,
			    DMPP_PAGE       *data,
			    char            *record,
			    i4         record_size,
			    i4         record_type,
			    i4         fill,
			    DM_TID          *tid,
			    u_i2            current_table_version,
			    DMPP_SEG_HDR    *seg_hdr);

FUNC_EXTERN VOID dm1cn_put(
			    i4		    page_type,
			    i4		    page_size,
			    DMPP_PAGE	    *page,
			    i4      update_mode,
			    DB_TRAN_ID      *tran_id,
			    u_i2	    lg_id,
			    DM_TID          *tid,
			    i4              record_size,
			    char            *record,
			    u_i2            current_table_version,
			    DMPP_SEG_HDR    *seg_hdr);

FUNC_EXTERN DB_STATUS	dm1cn_reallocate(
			    i4		page_type,
			    i4		page_size,
			    DMPP_PAGE	*page,
			    i4		line_num,
			    i4		reclen);

FUNC_EXTERN VOID         dm1cn_reclen(
			    i4		page_type,
			    i4		page_size,
			    DMPP_PAGE	*page,
			    i4         lineno,
			    i4         *record_size);

FUNC_EXTERN i4		 dm1cn_tuplecount(
			    DMPP_PAGE       *page,
			    i4              page_size);
                        
FUNC_EXTERN i4	         dm1cn_dput(
			    DMP_RCB	*rcb,
			    DMPP_PAGE	*page, 
			    DM_TID	*tid,
			    i4		*err_code);


/* Row-level access routines, for Standard compression */
FUNC_EXTERN DB_STATUS	dm1cn_compress(
			DMP_ROWACCESS	*rac,
			char       	*rec,
			i4		rec_size,
			char       	*crec,
			i4		*crec_size);

FUNC_EXTERN DB_STATUS	dm1cn_uncompress(
			DMP_ROWACCESS	*rac,
			char		*src,
			char		*dst,
			i4		dst_maxlen,
			i4		*dst_size,
			char		**buffer,
			i4		row_version,
			ADF_CB		*rcb_adf_cb);

FUNC_EXTERN i4		dm1cn_compexpand(
			i4		compression_type,
			DB_ATTS		**atts,
			i4		att_cnt);

FUNC_EXTERN DB_STATUS	dm1cn_cmpcontrol_setup(
			DMP_ROWACCESS	*rac);

FUNC_EXTERN i4		dm1cn_cmpcontrol_size(
			i4		att_count,
			i4		relversion);

FUNC_EXTERN DB_STATUS	dm1cn_clean(
			    DMP_RCB	*r,
			    DMPP_PAGE	*page,
			    i4		*avail_space);

FUNC_EXTERN DB_STATUS defer_add_new(
			DMP_RCB *rcb,
			DM_TID  *put_tid,
			i4	page_updated,
			i4	*err_code);
