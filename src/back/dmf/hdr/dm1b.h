/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM1B.H - Types, Constants and Macros for BTREEs.
**
** Description:
**      This file contains all the BTREE specific typedefs,
**      constants, and macros.
**
** History:
**      17-oct-85 (Jennifer)
**          Converted for Jupiter.
**	17-dec-1990 (bryanp)
**	    Added DM1B_INDEX_COMPRESSION macro to support index compression.
**	07-jul-92 (jrb)
**	    Prototype DMF.
**	16-jul-1992 (kwatts)
**	    MPF project changes: brought in the dmpp_ macros, amended the
**	    BTREE page definition and the derivation of some defines.
**	07-oct-1992 (jnash)
**	    6.5 Recovery Project
**	      - In _DM1B_INDEX, substitute page_checksum for page_version.
**	22-oct-1992 (rickh)
**	    Replaced all references to DMP_ATTS with DB_ATTS.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project (phase 2):
**	      - Added btree index page field "bt_split_lsn".
**	      - Changed prototype arguments to dm1b_replace, dm1b_delete,
**		dm1bxinsert, dm1bxdelete, dm1bxsearch.
**	      - Added prototypes for new btree routines.
**	      - Removed unused DM1B_PCHAINED page type.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	10-aug-1995 (cohmi01)
**	    Add prototype for dm1b_build_dplist() used by prefetch.
**	21-aug-1995 (cohmi01)
**	    Prototype for dm1b_count().
**	06-mar-1996 (stial01 for bryanp)
**	    DM1B_FREE now takes the page size as an argument.
**      06-mar-1996 (stial01)
**          Added page_size to prototype for dm1bxinsert()
**	06-may-1996 (thaju02 & nanpr01)
**	    New Page Format Project.
**	      - Added DM1B_V2_INDEX structure.
**	      - Renamed and typedef'd DM1B_INDEX structure to DM1B_V1_INDEX.
**	      - Created DM1B_INDEX union.
**	      - Created macros to access page fields.
**	      - Modified macros dm1bkido_macro, dm1bkidoff_macro, 
**		dm1bkeyof_macro, dm1bkidget_macro, dm1bkidput_macro,
**		dm1bkeyget_macro, dm1bkeyput_macro, dm1bkbget_macro, 
**		dm1bkbput_macro due to DM1B_BT_SEQUENCE_MACRO.
**	      - Modified dmpp_get_offset_macro, dmpp_put_offset_macro,
**		dmpp_test_new_macro, dmpp_set_new_macro, dmpp_clear_new_macro
**		for implementation of tuple header.
**	      - Modified dm1bkido_macro, dm1bkidoff_macro to account for
**		sizeof(DMPP_TUPLE_HDR).
**	      - Changed the DM1B_VPT_OVER to be a macro instead of a constant.
**	      - Added dm1b_entry_macro. Modified dm1bkido_macro,
**		dm1bkidoff_macro to account for tuple header in leaf pages
**		only.
**	      - Modified dm1bxformat function prototype.
**      20-jun-1996 (thaju02)
**          Modified offset macros, flags macros due to DM_V2_LINEID
**          structure change.  Typo corrected in DM1B_GET_PAGE_LOG_ADDR_MACRO.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**            - Added bt_clean_count to btree (leaf) pages
**            - Added macros to get/incr bt_clean_count on leaf page
**            - Added DM1B_POSITION_INFO
**            - Added macros: dm1b_save_position and dm1b_restore_position
**            - Added macros to test,set,clear DM_FREE_SPACE in linetab flags 
** 	      - Clear bt_clean_count in DM1B_SET_PAGE_LOG_ADDR_MACRO
**            - Added dm1b_incr_page_clean_count_macro.  
**            - Added prototypes for: dm1b_position(), dm1bxclean()
**            - Changed prototype for dm1b_rcb_update() Added lsn,clean_counts
**            - Changed prototype for dm1b_bybid_get()
**            - Added prototype for dm1b_rcb_update()
**      25-nov-96 (stial01)
**          Added low_lsn and low_clean_count to DM1B_POSITION_INFO
**	04-feb-1997 (nanpr01)
**	    Changed the dm1bkido_macro & dm1bkidoff_macro to DMMP_INDEX 
**	    so that the tuple headers are considered for non-index pages.
**      27-feb-1997 (stial01)
**          Changed prototype for dm1bxdelete()
**      07-apr-1997 (stial01)
**          Added key to prototype for dm1bxchain()
**          Added prototype for dm1b_compare_range, dm1b_compare_key
**      21-apr-97 (stial01)
**          Changed prototype for dm1b_dupcheck, dm1b_bybid_get,
**          Added prototype for dm1bxreserve.
**      21-May-97 (stial01)
**          Removed prototypes for static routines dm1b_dupcheck,dm1badupcheck
**          Added buf_locked to prototype for dm1bxchain, dm1bxsplit.
**          Changed prototype for dm1bxclean.
**      12-jun-97 (stial01)
**          Added bt_attcnt to btree page header
**          Added macros to set,get key description on leaf page
**          dm1bxformat() Added attr,key args to prototype 
**      29-jul-97 (stial01)
**          Changed prototype for dm1b_put, dm1b_delete, dm1b_rcb_update
**          Added prototype for dm1b_reposition
**          Removed dm1b_reclaim_space_macro.
**	09-feb-98 (kinte01)
**	    For DM1B_GET_PAGE_LOG_ADDR_MACRO change to if-else format so it
**	    will compile with the DEC C 5.6 compiler on VAX/VMS. 
**	14-Apr-1998 (fucch01)
**	    ifdef'd for sgi_us5 to use straight dest = src structure 
**	    assignment instead of the STRUCT_ASSIGN_MACRO, which i had to
**	    change previously (from a straight assignment) to an MEcopy to
**	    avoid a SIGBUS error that was occurring when certain data was
**	    present inside the structures involved.
**      07-jul-98 (stial01)
**          Removed unecessary btree position macros.
**	01-oct-1998 (nanpr01)
**	    Update performance improvement.
**      09-feb-1999 (stial01)
**          Added relcmptlvl to dm1b_kperpage prototype
**      12-apr-1999 (stial01)
**          Different att/key info for LEAF vs. INDEX pages
**          Removed keyatts from dm1bxsearch, dm1bxsplit prototypes.
**          Added prototype for dm1b_build_index_key()
**	22-apr-1999 (nanpr01)
**	    Correct the DM1B_INIT_PAGE_STAT macro.
**	10-may-1999 (walro03)
**	    Remove obsolete version string i39_vme.
**	11-Nov-1999 (hanch04)
**	    Broke macros into V1 and V2 to check page size only once.
**      12-nov-1999 (stial01)
**          Changed prototype
**	21-nov-1999 (nanpr01)
**	    Code optimization/reorganization based on quantify profiler output.
**      27-jan-2000 (stial01)
**          Added DM1B_CC_TIDEOF_LEAF, DM1B_CC_REPOSITION
**      24-Jul-2001 (horda03)
**          DM1B_MAXBID is the maximum number of keys that can be
**          stored on a Btree Index or Leaf page. It was originally
**          set to low, now using the same algorithm to cap the
**          number of Keys when the btree is created.
**          (105302)
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      30-oct-2000 (stial01)
**          Changed prototype for dm1b_allocate
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-dec-2000 (toumi01)
**	    correct typo
**	    < dm1bvpt_kidget_vpt_macro
**	    > dm1bvpt_kidget_macro
**      01-feb-2001 (stial01)
**          Arg/macro changes for Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**	31-Dec-2003 (jenjo02)
**	    Changes for Partitioned Table project, TID8's.
**	    Deprecated DM1B_TID_S; size of appropriate TID
**	    now stored on page in bt_tid_size.
**	    Modified dm1bvpt_kido_macro to extract TID
**	    size from page rather than use the constant
**	    DM1B_TID_S.
**	    Added DM1B_VPT_GET_BT_TIDSZ_MACRO,
**		  DM1B_VTP_SET_BT_TIDSZ_MACRO.
**	    Modified dm1bvpt_kidget_macro to return the
**	    TID's partition number, dm1bvpt_kidput_macro
**	    to store TID8s when appropriate.
**	    dm1bxformat() is now passed the TID size,
**	    dm1bxinsert() is now passed a partition number,
**	    dm1bxsearch() now returns a partition number,
**	    dm1b_kperpage() must now be passed a TID size.
**      02-jan-2004 (stial01)
**          Changed prototype for dm1bxinsert
**	29-Jan-2004 (hanje04)
**	    Added extra parens. to dm1bvpt_kidget_macro, and 
**	    dm1bvpt_kidput_macro definitions to resolve compile errors
**	    on Linux.
**	02-Feb-2004 (jenjo02)
**	    Corrected tid_i4 assignment in dm1bvpt_kidget_macro,
**	    cast correctly for LINUX.
**	14-apr-2004 (stial01)
** 	    Increase btree key size, varies by pagetype, pagesize (SIR 108315)
**	25-Apr-2006 (jenjo02)
**	    Add Clustered parm to dm1bxformat(), dm1b_kperpage() prototypes.
**	    Upped DM1B_KEYLENGTH from 2000 to 8192, add prototype
**	    for dm1b_build_ixkey().
**	05-Jun-2006 (jenjo02)
**	    Add char *record parm to dm1bxsearch() prototype.
**	07-Jun-2006 (jenjo02)
**	    Reinstate DM1B_KEYLENGTH to 2000 while a permanent fix is
**	    being worked on. 8192 blows the stack sometimes.
**	13-Jun-2006 (jenjo02)
**	    Ok: DM1B_KEYLENGTH (2048) is max length of key-only attributes.
**	        DM1B_MAXLEAFLEN (32684) is max length of a Leaf entry.
**	        DM1B_MAXSTACKKEY (2048) is the biggest leaf we can hold on
**	    				the stack without more memory.
**	    Renamed DM1B_VPT_MAXKLEN_MACRO to DM1B_VPT_MAXLEAFLEN_MACRO
**	    and changed it to compute the maximum Leaf length based on
**	    supplied variables.
**	    Added function prototypes for dm1b_AllocKeyBuf(),
**	    dm1b_DeallocKeyBuf().
**	    Deleted DM1B_MAXKEY_MACRO as it served no useful purpose.
**      20-oct-2006 (stial01)
**          Disable clustered index (DM1B_VPT_MAXKLEN_MACRO,DM1B_MAXKEY_MACRO)
**          Limit DM1B_KEYLENGTH,DM1B_MAXSTACKKEY,DM1B_MAXLEAFLEN
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2a_?, dm1?_count functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1b_? functions converted to DB_ERROR *
**      10-sep-2009 (stial01)
**          DM1B_DUPS_ON_OVFL_MACRO true if btree can have overflow pages
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**      14-oct-2009 (stial01)
**          Move btree page types into DMPP_PAGE, update page macros 
**	23-Oct-2009 (kschendel) SIR 122739
**	    Update prototype for dm1bxinsert.
**      04-dec-2009 (stial01)
**          remove DM1B_DUPS_ON_OVFL_MACRO (replaced by tcb_dups_onf_ovfl)
**          Added mode param for dm1bxsplit prototype
**      11-jan-2010 (stial01)
**          Define macros for setting and checking position information in rcb
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**/


/*
**  Forward and/or External typedef/struct references.
*/

typedef union _DM1B_INDEX DM1B_INDEX;

/* BTREE general symbolic constant definitions. */

# define DM1B_ABSMAX	1000L	          /* Absolute limit on 
                                          ** tries in merging. */
# define DM1B_BARRED	327L             

# define DM1B_CC_INVALID     ((u_i4) 0xffffffff)
# define DM1B_CC_REPOSITION  ((u_i4) 0xfffffffe)

/* This value was obtained from the following algorithm.
** Calculate maximum keys that can fit onto a page.  This is done as 
** follows:
** maxkeys = (DM_PG_SIZE - DM1B_VPT_OVER) / 
**               (sizeof(DM_LINEID  ) + SIZEOF(DM_TID ) )
** Calculate number of addresses that can be held in a block.
** maxaddr = DM1B_MAXBID.
** Take the minimum of these two.  This is always maxkeys assuming
** a zero length key.
** Now subtract out the line table offset.
** DM1B_BARRED = maxkeys - DM1B_OFFSEQ + 2 .
*/

# define DM1B_FALSE        0L           /* Not Ok. */

/*
** DM1B_FREE was defined as a fixed value of 1960L. Since the free space in a
** page is page format dependant, this needs to be calculated based on the
** overhead for the page (DM1B_VPT_OVER) which becomes the fixed value.
*/
# define DM1B_V1_OVER  84L
# define DM1B_V2_OVER  112L

#define DM1B_VPT_OVER(page_type) \
     (page_type != DM_COMPAT_PGTYPE ? DM1B_V2_OVER : DM1B_V1_OVER) 

#define DM1B_VPT_FREE(page_type, page_size)		\
      (page_size - DM1B_VPT_OVER(page_type) - 	\
      (2 * DM_VPT_SIZEOF_LINEID_MACRO(page_type))) 

# define DM1B_HEADER_PAGE  1            /* Page number of header page. */

# define DM1B_LEFT	0L	        /* Used to determine directions of 
                                        ** insertion. Place at left of given
                                        ** position. */

# define DM1B_LRANGE	-2L             /* Offset in line table where low 
                                        ** key value on page is kept. */

# define DM1B_NOTSAME   1L              /* Arguments to compare are 
                                        ** not equal. */

# define DM1B_OFFSEQ	2L             /* Offset in line table where
                                        ** real tid,key pairs are kept. */

# define DM1B_OVFJOIN   1L             /* Join an overflow page to leaf page. */

# define DM1B_PINDEX	0L              /* Index page indicator. */
# define DM1B_PLEAF	1L              /* Leaf page indicator. */
# define DM1B_PDATA	2L              /* Data page indicator. */
# define DM1B_POVFLO	3L              /* Overflow page indicator. */
# define DM1B_PSPRIG	5L	        /* Parent of leaf page */

# define DM1B_RIGHT	1L	        /* Used to determine direction of 
                                        ** insertion. Place at right of given
                                        ** position. */

# define DM1B_RRANGE	-1L             /* Offset in line table where high
                                        ** key value on page is kept. */

# define DM1B_ROOT_PAGE  0              /* Page number of root page. */

# define DM1B_SAME	0		/* Arguments to compare are 
                                        ** equal. */ 

# define DM1B_SIBJOIN   2L              /* Doing a sibling join. */

# define DM1B_SMASK     0xF0E0          /* Isolate BTREE status bits. 
                                        ** for data, leaf and free. */  

/*
** Don't use this anymore. TID size may be 4 or 8 on Global
** index LEAF pages, 4 on all other page types, and is now
** stored on the page.
** I left it here because RTREEs don't yet support Global 
** indexes and use DM1B_TID_S all over the place.
*/
# define DM1B_TID_S	sizeof(DM_TID ) /* Size of TID entry for BTREE 
                                        ** record which is (TID,KEY). */

# define DM1B_TRUE      1L              /* Join occurred. */

# define DM1B_XTENT	30L		/* Number of pages allocated when a 
                                        ** BTREE file is out of space. */


/* More BTREE general definitions, these are dependent on previous ones. */

# define DM1B_MAXBID       (DM_TIDEOF + 1) - DM1B_OFFSEQ


/*
**
** Name : _DM1B_ATTR - BTREE attribute description
**
** Description : 
**
** History :
**      12-jun-97 (stial01)
**          Created to make undo/redo btree leaf page updates more efficient.
**          Attribute information is kept on the page to avoid the logical 
**          table open in dmve_bid_check.
**	13-dec-04 (inkdo01)
**	    Added union to prec and overlaid collID.
*/
typedef struct _DM1B_ATTR
{
    i2              type;               /* Type of attribute. */
    i2              len;                /* Length of attribute. */
    union {
	i2              prec;           /* Precision of attribute. */
	i2		collID;		/* collation ID (for char cols) */
    } u;
    i2              key;                /* Indicates part of key. */ 
} DM1B_ATTR;

/*
** DM1B_MAXSTACKKEY is the max size of stack leaf entry buffers.
** DM1B_MAXLEAFLEN  defines the maximum size of a leaf entry
**		    and includes key and non-key attributes.
** DM1B_KEYLENGTH   is the maximum size of only key attributes,
** 		    non-key attributes excluded.
*/
#define DM1B_COMPAT_KEYLENGTH	440L
#define DM1B_KEYLENGTH		2000L 
#define DM1B_MAXSTACKKEY	2000L 
#define DM1B_MAXLEAFLEN		2000L 

#define DM1B_RESERVED_BYTES(page_type)			\
 ( (page_type == DM_COMPAT_PGTYPE) ? 180: 0)
 
#define DM1B_ATTDESC_BYTES(page_type, leafatts)		\
 ( (page_type == DM_COMPAT_PGTYPE) ? 0 : leafatts * sizeof(DM1B_ATTR))

/*
** Macro to check the real maximum btree key length
** Note the max on 2kV1 btree pages is 440 because we reserved 180 bytes.
** Note the max on 2k V2,V3,V4 (which has larger page and key overheads)
** is still >= 440 ONLY because we DON'T reserve 180 bytes.
** However, if the key is made up of many columns, you may not fit
** a 440 byte key... due to DM1B_ATTDESC_BYTES overhead 
*/
#define DM1B_VPT_MAXKLEN_MACRO(pgtype, pgsize, leafatts)		\
 ( (( pgsize -								\
 (DM1B_VPT_OVER(pgtype) + DM1B_ATTDESC_BYTES(pgtype, leafatts) + 	\
 DM1B_RESERVED_BYTES(pgtype)) )  / (2 + DM1B_OFFSEQ) ) ) -		\
 (DM_VPT_SIZEOF_LINEID_MACRO(pgtype) + DM1B_TID_S +			\
 DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(pgtype))

/*
** We don't want to document different maximum key lengths for
** different page types.... so to keep it simple we are defining the
** maximum less than what it could be... If you need a larger key size
** you must go to a larger page size.
** 
** Note: There will be no problem starting server with different max_btree_key
** (Changing max_btree_key does not change keys per index/leaf for
** existing btree tables/indexes)
*/
#define DM1B_MAXKEY_MACRO(page_size)					\
 (page_size == 2048 ? DM1B_COMPAT_KEYLENGTH: page_size == 4096 ? 500 :	\
  page_size == 8192 ? 1000: DM1B_KEYLENGTH)
/*
** Macro to check the real maximum btree leaf size
** which is dependent on page type, page size, the number
** of attribute descriptors stored on the page, and
** the length of key-only attributes. Knowing the length
** of key-only attributes lets us factor out the size
** of the two range entries from the page size right off
** the top (as of Ingres2007, we no longer store non-key
** attributes in Range entries).
** 
** Note that what we're really computing here is the
** maximum length of a Leaf entry, not a "key", and the
** computation insists on room for at least 2 non-range
** Leaf entries.
**
** DM1B_MAXLEAFLEN should be defined as something equal
** or less than the largest number produced by this
** macro, currently 32684 bytes (64k V4 Clustered with
** a 4-byte key).
*/
#define DM1B_VPT_MAXLEAFLEN_MACRO(pgtype, pgsize, leafatts, RangeKeyLen)	\
 ( (( pgsize -								\
 (DM1B_VPT_OVER(pgtype) + DM1B_ATTDESC_BYTES(pgtype, leafatts) + 	\
 DM1B_RESERVED_BYTES(pgtype) +						\
 DM1B_OFFSEQ * (RangeKeyLen + DM_VPT_SIZEOF_LINEID_MACRO(pgtype) + 	\
	  DM1B_TID_S + DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(pgtype)) ) )	\
   / 2 ) ) -								\
 (DM_VPT_SIZEOF_LINEID_MACRO(pgtype) + DM1B_TID_S +			\
 DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(pgtype))

/*
**  Macros for getting info in and out of index buffers.
*/
/* Generic page macros. These are used in access to BTREE pages only. */

#define dmpp_V1_get_offset_macro(line_tab, index)        		\
	(((DM_V1_LINEID *)line_tab)[index] & 0x7fff)

#define dmpp_V2_get_offset_macro(line_tab, index)        		\
	(((DM_V2_LINEID *)line_tab)[(index)] & DM_OFFSET_MASK)

#define dmpp_vpt_get_offset_macro(page_type, line_tab, index)        	\
	(page_type != DM_COMPAT_PGTYPE ?                         	\
	 dmpp_V2_get_offset_macro(line_tab, index) :			\
	 dmpp_V1_get_offset_macro(line_tab, index))

#define dmpp_vpt_put_offset_macro(page_type, line_tab, index, value) 	\
    {									\
        if (page_type != DM_COMPAT_PGTYPE)                       	\
            ((DM_V2_LINEID *)line_tab)[index] = 			\
		(((DM_V2_LINEID *)line_tab)[index] & DM_FLAGS_MASK |	\
		((value) & DM_OFFSET_MASK));				\
        else                                                    	\
            (((DM_V1_LINEID *)line_tab)[index] =			\
                (((DM_V1_LINEID   *)line_tab)[index] & 0x8000) |	\
                (value & 0x7fff));					\
    }
 
#define dmpp_vpt_clear_flags_macro(page_type, line_tab, index)		\
    {									\
	if (page_type != DM_COMPAT_PGTYPE)                       	\
            (((DM_V2_LINEID *)line_tab)[index] =			\
		(((DM_V2_LINEID *)line_tab)[index] & DM_OFFSET_MASK));	\
	else                                                    	\
            (((DM_V1_LINEID *)line_tab)[index] =                	\
                (((DM_V1_LINEID   *)line_tab)[index] & 0x07fff));	\
    }

#define dmpp_vpt_test_new_macro(page_type, line_tab, index)   	\
        (page_type != DM_COMPAT_PGTYPE ?				\
        ((DM_V2_LINEID *)line_tab)[index] & DM_REC_NEW :	\
        (((DM_V1_LINEID   *)line_tab)[index] & 0x8000))
 
#define dmpp_vpt_set_new_macro(page_type, line_tab, index)    	\
    {								\
        if (page_type != DM_COMPAT_PGTYPE)                       \
            ((DM_V2_LINEID *)line_tab)[index] |= DM_REC_NEW;	\
        else                                                    \
            ((DM_V1_LINEID   *)line_tab)[index] |= 0x8000;	\
    }
 
#define dmpp_vpt_clear_new_macro(page_type, line_tab, index)  	\
    {								\
        if (page_type != DM_COMPAT_PGTYPE)                       \
            ((DM_V2_LINEID *)line_tab)[index] &= ~DM_REC_NEW; 	\
        else                                                    \
            ((DM_V1_LINEID *)line_tab)[index] &= ~0x8000;	\
    }

#define dmpp_vpt_test_free_macro(page_type, line_tab, index)   	\
        (page_type != DM_COMPAT_PGTYPE ?				\
        ((DM_V2_LINEID *)line_tab)[index] & DM_FREE_SPACE : \
        0)
 
#define dmpp_vpt_set_free_macro(page_type, line_tab, index)    	\
    {								\
        if (page_type != DM_COMPAT_PGTYPE)                       \
            ((DM_V2_LINEID *)line_tab)[index] |= DM_FREE_SPACE;	\
    }
 
#define dmpp_vpt_clear_free_macro(page_type, line_tab, index)  	\
    {								\
        if (page_type != DM_COMPAT_PGTYPE)                       \
            ((DM_V2_LINEID *)line_tab)[index] &= ~DM_FREE_SPACE; \
    }

/*
**  dm1b_vpt_entry_macro(buffer, i, page_type) -- Get pointer to the i'th
**  child of index page. 
**  Inputs :    (DM1B_INDEX *)buffer - pointer to index page.
**              (i4)i - index into bt_sequence array.
**              (i4)page_type - page type.
**
**  Returns : (char *)- pointer to the i'th child of index page.
**			(points to beginning of tuple hdr, key, tid record)
*/
# define dm1b_vpt_entry_macro(page_type, buffer, i)                         \
	(page_type != DM_COMPAT_PGTYPE ?					 \
         ((char *)(buffer) + dmpp_V2_get_offset_macro(                   \
            DM1B_V2_BT_SEQUENCE_MACRO(buffer), i + DM1B_OFFSEQ)) :  	 \
         ((char *)(buffer) + dmpp_V1_get_offset_macro(                   \
            DM1B_V1_BT_SEQUENCE_MACRO(buffer), i + DM1B_OFFSEQ))) 

/*
**  dm1bvpt_kido_macro  -- Get pointer to the i'th child's key,tid pair 
**  of index page.
**  Offset by DM1B_OFFSEQ to account for high and low range values.
**  Prior to New Page Format Project, macro was defined as:
**	# define dm1bkido_macro(buffer, i)	\
**         ((char *)(buffer) +			\
**         dmpp_get_offset_macro((buffer)->bt_sequence, i + DM1B_OFFSEQ))
**
**  Inputs : 	(DM1B_INDEX *)buffer - pointer to index page.
**		(i4)i - index into bt_sequence array.
**		(i4)page_type - page_type.
**
**  Returns : (char *)- pointer to the i'th child of index page key,tid pair.
**
**  NOTE:	V1 pages do not have tuple headers, V2 pages have variable
**		size tuple headers
*/
# define dm1bvpt_kido_macro(page_type, buffer, i)			\
	(page_type != DM_COMPAT_PGTYPE ?					\
	((DM1B_V2_GET_PAGE_STAT_MACRO(buffer) & DMPP_INDEX ?		\
	((char *)(buffer) + dmpp_V2_get_offset_macro(			\
	    DM1B_V2_BT_SEQUENCE_MACRO(buffer), i + DM1B_OFFSEQ))  :	\
	((char *)(buffer) + dmpp_V2_get_offset_macro(			\
	    DM1B_V2_BT_SEQUENCE_MACRO(buffer), i + DM1B_OFFSEQ) +	\
	    DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(page_type)))) :		\
	((DM1B_V1_GET_PAGE_STAT_MACRO(buffer) & DMPP_INDEX ?		\
	((char *)(buffer) + dmpp_V1_get_offset_macro(			\
	    DM1B_V1_BT_SEQUENCE_MACRO(buffer), i + DM1B_OFFSEQ))  :	\
	((char *)(buffer) + dmpp_V1_get_offset_macro(			\
	    DM1B_V1_BT_SEQUENCE_MACRO(buffer), i + DM1B_OFFSEQ) ))))

/*
**  Name : dm1bvpt_kidoff_macro()
**
**  Description : Gets offset to the i'th child of index page.
**      Offset by DM1B_OFFSEQ to account for high and low range values.
**      Prior to New Page Format Project, macro was defined as:
**      # define dm1bkidoff_macro(buffer, i)	\
**          (dmpp_get_offset_macro((buffer)->bt_sequence, i + DM1B_OFFSEQ))
**
**  Inputs :    (DM1B_INDEX *)buffer - pointer to index page.
**              (i4)i - index into bt_sequence array.
**              (i4)page_type - page type.
**
**  Returns : (DM_LINEID)offset - offset into page of the i'th child.
**
**  NOTE:	V1 pages do not have tuple headers, V2 pages have variable
**		size tuple headers
*/
# define dm1bvpt_kidoff_macro(page_type, buffer, i)			\
	(page_type != DM_COMPAT_PGTYPE ?					\
    (DM1B_V2_GET_PAGE_STAT_MACRO(buffer) & DMPP_INDEX ?			\
	(dmpp_V2_get_offset_macro(DM1B_V2_BT_SEQUENCE_MACRO(buffer),	\
		i + DM1B_OFFSEQ)) :					\
	(dmpp_V2_get_offset_macro(DM1B_V2_BT_SEQUENCE_MACRO(buffer),	\
	    i + DM1B_OFFSEQ) + DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(page_type) )) :\
    (DM1B_V1_GET_PAGE_STAT_MACRO(buffer) & DMPP_INDEX ?			\
	(dmpp_V1_get_offset_macro(DM1B_V1_BT_SEQUENCE_MACRO(buffer),	\
		i + DM1B_OFFSEQ)) :					\
	(dmpp_V1_get_offset_macro(DM1B_V1_BT_SEQUENCE_MACRO(buffer),	\
		i + DM1B_OFFSEQ) )))

/*
**  dm1bvpt_keyof_macro -- Get position of i'th key in a page.
**  Offset by tid size since key is second value in record. 
*/

# define dm1bvpt_keyof_macro(page_type, buffer, child)	\
	dm1bvpt_kido_macro(page_type, buffer, child) + \
	DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, buffer)

/*
**  dm1bvpt_kidget_macro -- Get tid of i'th child of index page.
**
**  Inputs :    (i4)page_type - page type.
**              (DM1B_INDEX *)buffer - pointer to index page.
**              (i4)child - index into bt_sequence array.
**  Outputs:    (DM_TID*)tid4 - the returned DM_TID
**		(i4*)partno - the returned partition number
**			      (i4*)NULL if done't care
*/

# ifdef BYTE_ALIGN
# define dm1bvpt_kidget_macro(page_type, buffer, child, tid4, partno) \
    {								\
	char	*ctid = dm1bvpt_kido_macro(page_type, buffer, child); \
	if (DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, buffer) == sizeof(DM_TID)) \
	{							\
	    MEcopy(ctid, sizeof(DM_TID), (char*)tid4);		\
	    if ( partno )					\
		*partno = 0;					\
	}							\
	else							\
	{							\
	    DM_TID8	tid8;					\
	    MEcopy(ctid, sizeof(DM_TID8), (char*)&tid8);	\
	    ((DM_TID*)tid4)->tid_i4 = tid8.tid_i4.tid;		\
	    if ( partno )					\
		*partno = tid8.tid.tid_partno;			\
	}							\
    }
# else
# define  dm1bvpt_kidget_macro(page_type, buffer, child, tid4, partno)  \
    {								\
	char	*ctid = dm1bvpt_kido_macro(page_type, buffer, child); \
	if (DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, buffer) == sizeof(DM_TID)) \
	{							\
	    ((DM_TID*)tid4)->tid_i4 = ((DM_TID*)ctid)->tid_i4;	\
	    if ( partno )					\
		*partno = 0;					\
	}							\
	else    						\
	{							\
	    ((DM_TID*)tid4)->tid_i4 = ((DM_TID8*)ctid)->tid_i4.tid;\
	    if ( partno )					\
		*partno = ((DM_TID8*)ctid)->tid.tid_partno;	\
	}							\
    }
# endif

/*
**  dm1bvpt_kidput_macro -- Replace tid for i'th child in buffer.
**
**  Inputs :    (i4)page_type - page type.
**              (DM1B_INDEX *)buffer - pointer to index page.
**              (i4)child - index into bt_sequence array.
**              (DM_TID*)tid4 - the DM_TID
**		(i4)partno - the partition number value
*/

# ifdef BYTE_ALIGN
# define    dm1bvpt_kidput_macro(page_type, buffer, child, tid4, partno) \
    {								\
	char	*ctid = dm1bvpt_kido_macro(page_type, buffer, child); \
	if (DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, buffer) == sizeof(DM_TID)) \
	    MEcopy((char*)tid4, sizeof(DM_TID), ctid);		\
	else    						\
	{							\
	    DM_TID8	tid8;					\
	    tid8.tid_i4.tid = ((DM_TID*)tid4)->tid_i4;		\
	    tid8.tid_i4.tpf = 0;				\
	    tid8.tid.tid_partno = partno;			\
	    MEcopy((char*)&tid8, sizeof(DM_TID8), ctid); 	\
	}							\
    }
# else
# define    dm1bvpt_kidput_macro(page_type, buffer, child, tid4, partno) \
    {								\
	char	*ctid = dm1bvpt_kido_macro(page_type, buffer, child); \
	if (DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, buffer) == sizeof(DM_TID)) \
	    ((DM_TID*)ctid)->tid_i4 = ((DM_TID*)tid4)->tid_i4;	\
	else    						\
	{							\
	    ((DM_TID8*)ctid)->tid_i4.tid = ((DM_TID*)tid4)->tid_i4;\
	    ((DM_TID8*)ctid)->tid_i4.tpf = 0;			\
	    ((DM_TID8*)ctid)->tid.tid_partno = partno;		\
	}							\
    }
# endif


/*
** DM1B_INDEX_COMPRESSION
**
** Description:
**	This macro extracts the index compression information from the
**	table information pointed to by the provided RCB.
**
*/
#define DM1B_INDEX_COMPRESSION(r) \
	    ((r->rcb_tcb_ptr->tcb_rel.relstat &  TCB_INDEX_COMP) ? \
		DM1CX_COMPRESSED : DM1CX_UNCOMPRESSED)


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DB_STATUS dm1b_allocate(
			DMP_RCB	*rcb,
			DMP_PINFO   *leafPinfo,
			DMP_PINFO   *dataPinfo,
			DM_TID      *bid,
			DM_TID      *tid,
			char        *record,
			char	    *crecord,
			i4     need,
			char        *key,
			i4     dupflag,
			DB_ERROR    *dberr);

FUNC_EXTERN DB_STATUS dm1b_bybid_get(
			DMP_RCB         *rcb,
			i4          operation,
			DB_ERROR    *dberr);

FUNC_EXTERN DB_STATUS dm1b_delete(
			DMP_RCB        *rcb,
			DM_TID         *bid,
			DM_TID         *tid,
			i4        opflag,
			DB_ERROR    *dberr);

FUNC_EXTERN DB_STATUS dm1b_get(
			DMP_RCB         *rcb,
			DM_TID          *tid,
			char		*record,
			i4		opflag,
			DB_ERROR    *dberr);

FUNC_EXTERN DB_STATUS dm1b_put(
			DMP_RCB	    *rcb,
			DMP_PINFO   *leafPinfo,
			DMP_PINFO   *dataPinfo,
			DM_TID     *tid,
			DM_TID     *bid,
			char        *record,
			char        *key,
			i4     record_size,
			i4     opflag,
			DB_ERROR    *dberr);

FUNC_EXTERN DB_STATUS dm1b_replace(
			DMP_RCB         *rcb,
			DM_TID          *bid,
			DM_TID          *tid,
			char            *record,
			i4         record_size,
			DMP_PINFO	*newleafPinfo,
			DMP_PINFO	*newdataPinfo,
			DM_TID          *newbid,
			DM_TID          *newtid,
			char            *nrec,
			i4         nrec_size,
			char            *newkey,
			i4		inplace_update,
			i4		delta_start,
			i4		delta_end,
			DB_ERROR    *dberr);

FUNC_EXTERN DB_STATUS dm1b_rcb_update(
			DMP_RCB        *rcb,
			DM_TID         *bid_input,
			DM_TID         *newbid_input,
			i4        split_pos,
			i4        opflag,
			i4        del_reclaim_space,
			DM_TID         *tid,
			DM_TID         *newtid,
			char           *newkey,
			DB_ERROR    *dberr);

FUNC_EXTERN DB_STATUS dm1b_search(
			DMP_RCB            *rcb,
			char		   *key,
			i4            keys_given,
			i4            mode,
			i4            direction,
			DMP_PINFO         *leafPinfo,
			DM_TID            *bid,
			DM_TID            *tid,
			DB_ERROR    *dberr);

#define dm1b_badtid(rcb,bid,tid,key) \
    dm1b_badtid_fcn(rcb,bid,tid,key,__FILE__,__LINE__)

FUNC_EXTERN VOID dm1b_badtid_fcn(
			DMP_RCB		*rcb,
			DM_TID		*bid,
			DM_TID		*tid,
			char		*key,
			PTR		FileName,
			i4		LineNumber);

FUNC_EXTERN DB_STATUS dm1bxchain(
			DMP_RCB             *rcb,
			DMP_PINFO	    *leafPinfo,
			DM_TID		    *bid,
			i4		    start,
			char                *key,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1bxdelete(
			DMP_RCB		    *rcb,
			DMP_PINFO	    *bufferPinfo,
			DM_LINE_IDX	    child,
			i4		    log_updates,
			i4		    mutex_required,
			i4             *reclaim_space,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1bxdupkey(
			DMP_RCB		   *rcb,
			DMP_PINFO	    *leafPinfo,
			char               dupspace[],
			DB_STATUS          *state,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1bxformat(
			i4		    page_type,
			i4		    page_size,
			DMP_RCB             *rcb,
			DMP_TCB             *tcb,
			DMPP_PAGE          *buffer,
			i4             kperpage,
			i4             klen,
			DB_ATTS             **atts_array, 
			i4             atts_count,
			DB_ATTS             **key_array,
			i4             key_count,
			DM_PAGENO           page,
			i4		    indexpagetype,
			i4		    compression_type,
			DMPP_ACC_PLV	    *loc_plv,
			i2		    tidsize,
			i4		    Clustered,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1bxinsert(
			DMP_RCB		    *rcb,
			DMP_TCB		    *tcb,
			i4		    page_type,
			i4		    page_size,
			DMP_ROWACCESS	    *rac,
			i4		    klen,
			DMP_PINFO	    *bufferPinfo,
			char		    *key,
			DM_TID		    *tid,
			i4		    partno,
			DM_LINE_IDX	    child,
			i4		    log_updates,
			i4		    mutex_required,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1bxjoin(
			DMP_RCB          *rcb,
			DMP_PINFO	*parentPinfo,
			i4		pos,
			DMP_PINFO      *currentPinfo,
			DMP_PINFO      *siblingPinfo,
			i4		mode,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1bxnewroot(
			DMP_RCB	    *rcb,
			DMP_PINFO   *rootPinfo,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1bxsearch(
			DMP_RCB             *rcb,
			DMPP_PAGE	    *page,
			i4		    mode,
			i4		    direction,
			char		    *key,
			i4		    keyno,
			DM_TID		    *tid,
			i4		    *partno,
			DM_LINE_IDX	    *pos,
			char		    *record,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1bxsplit(
			DMP_RCB     *rcb,
			DMP_PINFO   *currentPinfo,
			i4     pos,
			DMP_PINFO   *parentPinfo,
			char        *key,
			i4	    keyno,
			i4          mode,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1bmmerge(
			DMP_RCB       *rcb,
			i4	      index_fill,
			i4       leaf_fill,
			DB_ERROR      *dberr);

FUNC_EXTERN DB_STATUS dm1bxreplace(
			DMP_RCB		*rcb,
			DMP_TCB		*tcb,
			DB_ATTS		**atts_array,
			i4		atts_count,
			i4		compression_type,
			i4		key_len,
			char            *key,
			DM_TID		*datatid,
			DMPP_PAGE      *old_buffer,
			DM_LINE_IDX	old_child,
			DMPP_PAGE      *new_buffer,
			DM_LINE_IDX     new_child,
			i4		log_updates,
			i4		mutex_required,
			DB_ERROR    	    *dberr);


FUNC_EXTERN DB_STATUS dm1b_find_ovfl_space(
			DMP_RCB		*rcb,
			DMP_PINFO	*leafPinfo,
			char		*leafkey,
			i4		space_required,
			DM_TID		*bid,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1bxovfl_alloc(
			DMP_RCB		*rcb, 
			DMP_PINFO	*leafPinfo,
			char		*key,
			DM_TID		*bid,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1bxfree_ovfl(
			DMP_RCB		*rcb,
			i4		duplicate_keylen,
			char		*duplicate_key,
			DMP_PINFO	*ovflPinfo,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1bxclean(
			DMP_RCB             *rcb,
			DMP_PINFO           *leafPinfo,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1bxreserve(
			DMP_RCB         *rcb,
			DMP_PINFO       *leafPinfo,
			DM_TID          *bid,
			char            *key,
			bool            allocate,
			bool            mutex_required,
			DB_ERROR    	    *dberr);

FUNC_EXTERN i4  dm1b_build_dplist(
			DMP_PFREQ       *pfreq,
			DM_PAGENO       datapages[]);

FUNC_EXTERN DB_STATUS dm1b_count(
    			DMP_RCB         *rcb,
    			i4         *countptr,
			i4         direction,
			void            *record,
    			DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS dm1b_position(
			DMP_RCB     *rcb,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1b_reposition(
			DMP_RCB     *rcb,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1b_compare_key(
			DMP_RCB            *rcb,
			DMPP_PAGE         *leaf,
			i4             lineno,
			char               *key,
			i4             keys_given,
			i4                *compare,
			DB_ERROR    	    *dberr);

FUNC_EXTERN DB_STATUS dm1b_compare_range(
			DMP_RCB	*rcb,
			DMPP_PAGE  *leafpage,
			char        *key,
			bool        *correct_leaf,
			DB_ERROR    	    *dberr);

FUNC_EXTERN i4    dm1b_kperpage(
			i4 page_type,
			i4 page_size,
			i4      relcmptlvl,
			i4 atts_cnt,
			i4 key_len,
			i4 pagetype,
			i2 tidsize,
			i4 Clustered);

FUNC_EXTERN VOID  dm1b_build_key(
			i4        keycnt,
			DB_ATTS **srckeys,
			char     *srcbuf,
			DB_ATTS **dstkeys,
			char     *dstbuf);

FUNC_EXTERN VOID  dm1b_build_ixkey(
			i4        keycnt,
			DB_ATTS **srckeys,
			char     *srcbuf,
			DB_ATTS **dstkeys,
			char     *dstbuf);

FUNC_EXTERN DB_STATUS dm1b_dupcheck(
        DMP_RCB *rcb,
        DMP_PINFO   *leafPinfo,
        DM_TID  *bid,
        char        *record,
        char        *leafkey,
        bool        *requalify_leaf,
        bool        *redo_dupcheck,
        DB_ERROR    *dberr);

FUNC_EXTERN DB_STATUS	dm1b_AllocKeyBuf(
			i4	LeafLen,
			char	*StackLeaf,
			char	**LeafBuf,
			char	**AllocBuf,
			DB_ERROR *dberr);

FUNC_EXTERN VOID	dm1b_DeallocKeyBuf(
			char	**AllocBuf,
			char	**LeafBuf);

FUNC_EXTERN VOID	dm1b_position_save_fcn(
			DMP_RCB *r,
			DMPP_PAGE *b,
			DM_TID *bidp,
			i4 pop,
			i4 line);

/*
**  Macros for accessing index page fields.
*/
#define DM1B_VPT_GET_BT_ATTCNT(page_type, pgptr)                   \
 (page_type != DM_COMPAT_PGTYPE ? (pgptr)->dmpp_v2_index.bt_attcnt :0)

#define DM1B_VPT_SET_BT_ATTCNT(page_type, btree_type, pgptr, value)        \
     {								    \
        if (page_type != DM_COMPAT_PGTYPE)                    \
	{							    \
	    if (btree_type == DM1B_PLEAF)				    \
        	(pgptr)->dmpp_v2_index.bt_attcnt = value;	    \
	    else						    \
		(pgptr)->dmpp_v2_index.bt_attcnt = 0;            \
	}							    \
     }

#define DM1B_VPT_GET_ATTSIZE_MACRO(page_type, pgptr)                     \
	((page_type != DM_COMPAT_PGTYPE) ?                     \
	(pgptr)->dmpp_v2_index.bt_attcnt * sizeof(DM1B_ATTR) : 0)

#define DM1B_VPT_GET_ATTR(page_type, page_size, pgptr, keyno, attdesc)   \
{                                                                   \
    MEcopy( (PTR)(((char *)pgptr + page_size - (keyno * sizeof(DM1B_ATTR)))),\
    sizeof(DM1B_ATTR), (PTR)attdesc);                               \
}

#define DM1B_VPT_SET_ATTR(page_type, page_size, pgptr, keyno, attdesc)     \
{                                                                   \
    MEcopy((PTR)attdesc, sizeof(DM1B_ATTR),                         \
    (PTR)( ( (char *)pgptr + page_size - (keyno * sizeof(DM1B_ATTR)) ))); \
}

#define DM1B_V1_GET_PAGE_STAT_MACRO(pgptr)			\
	(pgptr)->dmpp_v1_index.page_stat

#define DM1B_V2_GET_PAGE_STAT_MACRO(pgptr)			\
	(pgptr)->dmpp_v2_index.page_stat 

#define DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, pgptr)		\
	((page_type != DM_COMPAT_PGTYPE) ?			\
	 DM1B_V2_GET_PAGE_STAT_MACRO(pgptr) :			\
	 DM1B_V1_GET_PAGE_STAT_MACRO(pgptr))

#define DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, pgptr, value)	\
     {								\
	if (page_type != DM_COMPAT_PGTYPE)			\
          (pgptr)->dmpp_v2_index.page_stat |= value;		\
        else 							\
	  (pgptr)->dmpp_v1_index.page_stat |= value;		\
     }
 
#define DM1B_VPT_INIT_PAGE_STAT_MACRO(page_type, pgptr, value)      	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                       \
          (pgptr)->dmpp_v2_index.page_stat = value;          \
        else                                                    \
          (pgptr)->dmpp_v1_index.page_stat = value;          \
     }

#define DM1B_VPT_CLR_PAGE_STAT_MACRO(page_type, pgptr, value)	\
     {								\
	if (page_type != DM_COMPAT_PGTYPE)			\
          (pgptr)->dmpp_v2_index.page_stat &= ~value;	\
        else 							\
	  (pgptr)->dmpp_v1_index.page_stat &= ~value;	\
     } 

#define DM1B_VPT_GET_PAGE_MAIN_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ? 			\
	(pgptr)->dmpp_v2_index.page_main :			\
        (pgptr)->dmpp_v1_index.page_main)
 
#define DM1B_VPT_SET_PAGE_MAIN_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
          (pgptr)->dmpp_v2_index.page_main = value;		\
        else 							\
	  (pgptr)->dmpp_v1_index.page_main = value;		\
     }
 
#define DM1B_VPT_ADDR_PAGE_MAIN_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ? 			\
	&(pgptr)->dmpp_v2_index.page_main :			\
        &(pgptr)->dmpp_v1_index.page_main)
 
#define DM1B_V1_GET_PAGE_OVFL_MACRO(pgptr)			\
        (pgptr)->dmpp_v1_index.page_ovfl

#define DM1B_V2_GET_PAGE_OVFL_MACRO(pgptr)			\
	(pgptr)->dmpp_v2_index.page_ovfl

#define DM1B_VPT_GET_PAGE_OVFL_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ? 			\
	 DM1B_V2_GET_PAGE_OVFL_MACRO(pgptr) :			\
	 DM1B_V1_GET_PAGE_OVFL_MACRO(pgptr))
 
#define DM1B_VPT_SET_PAGE_OVFL_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
          (pgptr)->dmpp_v2_index.page_ovfl = value;		\
        else 							\
	  (pgptr)->dmpp_v1_index.page_ovfl = value;		\
     }
 
#define DM1B_VPT_ADDR_PAGE_OVFL_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ? 			\
	&(pgptr)->dmpp_v2_index.page_ovfl :			\
        &(pgptr)->dmpp_v1_index.page_ovfl)
 
#define DM1B_V1_GET_PAGE_PAGE_MACRO(pgptr)			\
        (pgptr)->dmpp_v1_index.page_page

#define DM1B_V2_GET_PAGE_PAGE_MACRO(pgptr)			\
	(pgptr)->dmpp_v2_index.page_page

#define DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ? 			\
	 DM1B_V2_GET_PAGE_PAGE_MACRO(pgptr) :			\
	 DM1B_V1_GET_PAGE_PAGE_MACRO(pgptr))
 
#define DM1B_VPT_SET_PAGE_PAGE_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
          (pgptr)->dmpp_v2_index.page_page = value;		\
        else 							\
	  (pgptr)->dmpp_v1_index.page_page = value;		\
     }
 
#define DM1B_VPT_ADDR_PAGE_PAGE_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ? 			\
	&(pgptr)->dmpp_v2_index.page_page :			\
        &(pgptr)->dmpp_v1_index.page_page)
 
#define DM1B_VPT_SET_PAGE_SZTYPE_MACRO(page_type, page_size, pgptr)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
          (pgptr)->dmpp_v2_index.page_sztype = page_size + page_type;\
     }
 
#define DM1B_V1_GET_PAGE_SEQNO_MACRO(pgptr)			\
        (pgptr)->dmpp_v1_index.page_seq_number

#define DM1B_V2_GET_PAGE_SEQNO_MACRO(pgptr)			\
        (pgptr)->dmpp_v2_index.page_seq_number

#define DM1B_VPT_GET_PAGE_SEQNO_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ?				\
	 DM1B_V2_GET_PAGE_SEQNO_MACRO(pgptr) :			\
	 DM1B_V1_GET_PAGE_SEQNO_MACRO(pgptr))
 
#define DM1B_VPT_SET_PAGE_SEQNO_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
            (pgptr)->dmpp_v2_index.page_seq_number = value;	\
        else 							\
	    (pgptr)->dmpp_v1_index.page_seq_number = value;	\
     }

#define DM1B_VPT_SET_PAGE_LG_ID_MACRO(page_type, pgptr, lg_id) 	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
          ((DM1B_V2_INDEX *)pgptr)->page_lg_id = (u_i2)lg_id;	\
     }

#define DM1B_VPT_GET_PAGE_CHKSUM_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ? 			\
        (pgptr)->dmpp_v2_index.page_checksum :		\
        (pgptr)->dmpp_v1_index.page_checksum)
 
#define DM1B_VPT_SET_PAGE_CHKSUM_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
          (pgptr)->dmpp_v2_index.page_checksum = value;	\
        else 							\
	  (pgptr)->dmpp_v1_index.page_checksum = value;	\
     }
 
#define DM1B_VPT_GET_PAGE_TRAN_ID_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ?				\
        (pgptr)->dmpp_v2_index.page_tran_id :		\
        (pgptr)->dmpp_v1_index.page_tran_id)
 
#ifdef sgi_us5
#define DM1B_VPT_SET_PAGE_TRAN_ID_MACRO(page_type, pgptr, tran_id)   \
     {                                                          \
        if (page_type != DM_COMPAT_PGTYPE)                       \
          (pgptr)->dmpp_v2_index.page_tran_id = tran_id;     \
        else                                                    \
          (pgptr)->dmpp_v1_index.page_tran_id = tran_id;     \
     }
#else
#define DM1B_VPT_SET_PAGE_TRAN_ID_MACRO(page_type, pgptr, tran_id)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
          STRUCT_ASSIGN_MACRO(tran_id, (pgptr)->dmpp_v2_index.page_tran_id);\
        else							\
          STRUCT_ASSIGN_MACRO(tran_id, (pgptr)->dmpp_v1_index.page_tran_id);\
     } 
#endif /* sgi_us5 */

#define DM1B_V1_GET_TRAN_ID_LOW_MACRO(pgptr)			\
        (pgptr)->dmpp_v1_index.page_tran_id.db_low_tran

#define DM1B_V2_GET_TRAN_ID_LOW_MACRO(pgptr)			\
        (pgptr)->dmpp_v2_index.page_tran_id.db_low_tran

#define DM1B_VPT_GET_TRAN_ID_LOW_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ?				\
	 DM1B_V2_GET_TRAN_ID_LOW_MACRO(pgptr) :			\
	 DM1B_V1_GET_TRAN_ID_LOW_MACRO(pgptr))
 
#define DM1B_V1_GET_TRAN_ID_HIGH_MACRO(pgptr)			\
        (pgptr)->dmpp_v1_index.page_tran_id.db_high_tran

#define DM1B_V2_GET_TRAN_ID_HIGH_MACRO(pgptr)			\
        (pgptr)->dmpp_v2_index.page_tran_id.db_high_tran 

#define DM1B_VPT_GET_TRAN_ID_HIGH_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ?				\
         DM1B_V2_GET_TRAN_ID_HIGH_MACRO(pgptr) :		\
	 DM1B_V1_GET_TRAN_ID_HIGH_MACRO(pgptr))

#define DM1B_VPT_GET_PAGE_LOG_ADDR_MACRO(page_type, pgptr, log_addr)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
        STRUCT_ASSIGN_MACRO((pgptr)->dmpp_v2_index.page_log_addr,log_addr); \
        else							\
        STRUCT_ASSIGN_MACRO((pgptr)->dmpp_v1_index.page_log_addr,log_addr);\
     }

#define DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, pgptr, log_addr)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
        {							\
        STRUCT_ASSIGN_MACRO(log_addr,(pgptr)->dmpp_v2_index.page_log_addr); \
	(pgptr)->dmpp_v2_index.bt_clean_count = 0;           \
        }							\
        else							\
        STRUCT_ASSIGN_MACRO(log_addr, (pgptr)->dmpp_v1_index.page_log_addr);\
     }
 
#define DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, pgptr)            \
        (page_type != DM_COMPAT_PGTYPE ?				\
        (pgptr)->dmpp_v2_index.page_log_addr.lsn_low :	\
        (pgptr)->dmpp_v1_index.page_log_addr.lsn_low)
 
#define DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, pgptr)           \
        (page_type != DM_COMPAT_PGTYPE ?				\
        (pgptr)->dmpp_v2_index.page_log_addr.lsn_high :	\
        (pgptr)->dmpp_v1_index.page_log_addr.lsn_high)

#define DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ?				\
        (LG_LSN *)&((pgptr)->dmpp_v2_index.page_log_addr) :	\
        (LG_LSN *)&((pgptr)->dmpp_v1_index.page_log_addr))

#define DM1B_VPT_GET_BT_CLEAN_COUNT_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ? 			\
	(pgptr)->dmpp_v2_index.bt_clean_count :		\
	DM1B_CC_INVALID)

#define DM1B_VPT_INCR_BT_CLEAN_COUNT_MACRO(page_type, pgptr)       \
        if (page_type != DM_COMPAT_PGTYPE)                       \
	(pgptr)->dmpp_v2_index.bt_clean_count++;
 
#define DM1B_V1_GET_BT_NEXTPAGE_MACRO(pgptr)			\
        (pgptr)->dmpp_v1_index.bt_nextpage

#define DM1B_V2_GET_BT_NEXTPAGE_MACRO(pgptr)			\
        (pgptr)->dmpp_v2_index.bt_nextpage

#define DM1B_VPT_GET_BT_NEXTPAGE_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ?				\
	 DM1B_V2_GET_BT_NEXTPAGE_MACRO(pgptr) :			\
	 DM1B_V1_GET_BT_NEXTPAGE_MACRO(pgptr))
 
#define DM1B_VPT_SET_BT_NEXTPAGE_MACRO(page_type, pgptr, value)	\
     {								\
	if (page_type != DM_COMPAT_PGTYPE) 			\
          (pgptr)->dmpp_v2_index.bt_nextpage = value;	\
	else							\
          (pgptr)->dmpp_v1_index.bt_nextpage = value;	\
     }

#define DM1B_VPT_GET_BT_SPLIT_LSN_MACRO(page_type, pgptr, split_lsn)	\
        (page_type != DM_COMPAT_PGTYPE ?				\
        STRUCT_ASSIGN_MACRO((pgptr)->dmpp_v2_index.bt_split_lsn,split_lsn): \
        STRUCT_ASSIGN_MACRO((pgptr)->dmpp_v1_index.bt_split_lsn, split_lsn))
 
#define DM1B_VPT_GET_SPLIT_LSN_LOW_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ?				\
        (pgptr)->dmpp_v2_index.bt_split_lsn.lsn_low :        \
        (pgptr)->dmpp_v1_index.bt_split_lsn.lsn_low)
 
#define DM1B_VPT_GET_SPLIT_LSN_HIGH_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ?				\
        (pgptr)->dmpp_v2_index.bt_split_lsn.lsn_high :       \
        (pgptr)->dmpp_v1_index.bt_split_lsn.lsn_high)

#define DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(page_type, pgptr, split_lsn)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
        STRUCT_ASSIGN_MACRO(split_lsn,(pgptr)->dmpp_v2_index.bt_split_lsn);\
        else								\
        STRUCT_ASSIGN_MACRO(split_lsn,(pgptr)->dmpp_v1_index.bt_split_lsn);\
     }
 
#define DM1B_V1_ADDR_BT_SPLIT_LSN_MACRO(pgptr)			\
        (LG_LSN *)&((pgptr)->dmpp_v1_index.bt_split_lsn)

#define DM1B_V2_ADDR_BT_SPLIT_LSN_MACRO(pgptr)			\
        (LG_LSN *)&((pgptr)->dmpp_v2_index.bt_split_lsn) 

#define DM1B_VPT_ADDR_BT_SPLIT_LSN_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ?				\
	 DM1B_V2_ADDR_BT_SPLIT_LSN_MACRO(pgptr) :		\
	 DM1B_V1_ADDR_BT_SPLIT_LSN_MACRO(pgptr))
 
#define DM1B_V1_GET_BT_DATA_MACRO(pgptr)			\
        (pgptr)->dmpp_v1_index.bt_data

#define DM1B_V2_GET_BT_DATA_MACRO(pgptr)			\
	(pgptr)->dmpp_v2_index.bt_data 

#define DM1B_VPT_GET_BT_DATA_MACRO(page_type, pgptr)			\
        (page_type != DM_COMPAT_PGTYPE ?				\
	 DM1B_V2_GET_BT_DATA_MACRO(pgptr) :			\
	 DM1B_V1_GET_BT_DATA_MACRO(pgptr))
 
#define DM1B_VPT_SET_BT_DATA_MACRO(page_type, pgptr, value)		\
     {								\
	if (page_type != DM_COMPAT_PGTYPE)			\
	  (pgptr)->dmpp_v2_index.bt_data = value;		\
	else							\
          (pgptr)->dmpp_v1_index.bt_data = value;		\
     }
 
#define DM1B_V1_ADDR_BT_DATA_MACRO(pgptr)			\
        &(pgptr)->dmpp_v1_index.bt_data

#define DM1B_V2_ADDR_BT_DATA_MACRO(pgptr)			\
	&(pgptr)->dmpp_v2_index.bt_data 

#define DM1B_VPT_ADDR_BT_DATA_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ?				\
	 DM1B_V2_ADDR_BT_DATA_MACRO(pgptr) :			\
	 DM1B_V1_ADDR_BT_DATA_MACRO(pgptr))
 
#define DM1B_V1_GET_BT_KIDS_MACRO(pgptr)			\
        (pgptr)->dmpp_v1_index.bt_kids

#define DM1B_V2_GET_BT_KIDS_MACRO(pgptr)			\
	(pgptr)->dmpp_v2_index.bt_kids 

#define DM1B_VPT_GET_BT_KIDS_MACRO(page_type, pgptr)			\
        (page_type != DM_COMPAT_PGTYPE ?				\
	 DM1B_V2_GET_BT_KIDS_MACRO(pgptr) :			\
	 DM1B_V1_GET_BT_KIDS_MACRO(pgptr))
 
#define DM1B_VPT_INCR_BT_KIDS_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ?				\
	(pgptr)->dmpp_v2_index.bt_kids++ :			\
        (pgptr)->dmpp_v1_index.bt_kids++ )
 
#define DM1B_VPT_DECR_BT_KIDS_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ?				\
	(pgptr)->dmpp_v2_index.bt_kids-- :			\
        (pgptr)->dmpp_v1_index.bt_kids-- )
 
#define DM1B_VPT_SET_BT_KIDS_MACRO(page_type, pgptr, value)		\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
          (pgptr)->dmpp_v2_index.bt_kids = value; 		\
	else							\
          (pgptr)->dmpp_v1_index.bt_kids = value;		\
     }
 
#define DM1B_V1_BT_SEQUENCE_MACRO(pgptr)			\
        (DM_LINEID *)((pgptr)->dmpp_v1_index.bt_sequence)

#define DM1B_V2_BT_SEQUENCE_MACRO(pgptr)			\
        (DM_LINEID *)((pgptr)->dmpp_v2_index.bt_sequence)

#define DM1B_VPT_BT_SEQUENCE_MACRO(page_type, pgptr)			\
        (page_type != DM_COMPAT_PGTYPE ?				\
	 DM1B_V2_BT_SEQUENCE_MACRO(pgptr) : 			\
	 DM1B_V1_BT_SEQUENCE_MACRO(pgptr))
 
#define DM1B_VPT_GET_BT_SEQUENCE_MACRO(page_type, pgptr, index, btseq)	\
    {									\
        if (page_type != DM_COMPAT_PGTYPE)				\
          btseq.dm_v2_lineid = ((pgptr)->dmpp_v2_index.bt_sequence[index]);\
	else								\
          btseq.dm_v1_lineid = ((pgptr)->dmpp_v1_index.bt_sequence[index]);\
    }
 
#define DM1B_VPT_SET_BT_SEQUENCE_MACRO(page_type, pgptr, index, index2)	\
     {									\
        if (page_type != DM_COMPAT_PGTYPE)				\
          (pgptr)->dmpp_v2_index.bt_sequence[index] = 		\
	  	(pgptr)->dmpp_v2_index.bt_sequence[index2];		\
	else								\
          (pgptr)->dmpp_v1_index.bt_sequence[index] = 		\
		(pgptr)->dmpp_v1_index.bt_sequence[index2];		\
     }
 
#define DM1B_VPT_ASSIGN_BT_SEQUENCE_MACRO(page_type, pgptr, index, btseq)\
     {									\
        if (page_type != DM_COMPAT_PGTYPE)				\
          (pgptr)->dmpp_v2_index.bt_sequence[index] = btseq.dm_v2_lineid; \
	else								\
          (pgptr)->dmpp_v1_index.bt_sequence[index] = btseq.dm_v1_lineid; \
     }
 
#define DM1B_V1_GET_ADDR_BT_SEQUENCE_MACRO(pgptr, index)	\
        (DM_LINEID *)&((pgptr)->dmpp_v1_index.bt_sequence[index])

#define DM1B_V2_GET_ADDR_BT_SEQUENCE_MACRO(pgptr, index)	\
        (DM_LINEID *)&((pgptr)->dmpp_v2_index.bt_sequence[index]) 

#define DM1B_VPT_GET_ADDR_BT_SEQUENCE_MACRO(page_type, pgptr, index)	\
        (page_type != DM_COMPAT_PGTYPE ?				\
         DM1B_V2_GET_ADDR_BT_SEQUENCE_MACRO(pgptr, index) :	\
         DM1B_V1_GET_ADDR_BT_SEQUENCE_MACRO(pgptr, index))
 

/* Added for Partitioned Tables, Global Btree indexes */
 
#define DM1B_V1_GET_BT_TIDSZ_MACRO(pgptr)			\
        (pgptr)->dmpp_v1_index.bt_tid_size

#define DM1B_V2_GET_BT_TIDSZ_MACRO(pgptr)			\
	(pgptr)->dmpp_v2_index.bt_tid_size 

#define DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, pgptr)		\
	((page_type != DM_COMPAT_PGTYPE) 			\
	    ? (DM1B_V2_GET_BT_TIDSZ_MACRO(pgptr))		\
		? DM1B_V2_GET_BT_TIDSZ_MACRO(pgptr)		\
		: sizeof(DM_TID)				\
	    : (DM1B_V1_GET_BT_TIDSZ_MACRO(pgptr))		\
		? DM1B_V1_GET_BT_TIDSZ_MACRO(pgptr)		\
		: sizeof(DM_TID))
 
#define DM1B_VPT_SET_BT_TIDSZ_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
          (pgptr)->dmpp_v2_index.bt_tid_size = value; 	\
	else							\
          (pgptr)->dmpp_v1_index.bt_tid_size = value;	\
     }

/* Additions for MVCC */

#define DM1B_VPT_GET_PAGE_LG_ID_MACRO(page_type, pgptr) \
	((page_type != DM_COMPAT_PGTYPE) \
	    ? (pgptr)->dmpp_v2_index.page_lg_id \
	    : 0 )

#define DM1B_VPT_SET_PAGE_LRI_MACRO(page_type, pgptr, lri) \
    {							\
	if (page_type != DM_COMPAT_PGTYPE) \
	{ \
	    (pgptr)->dmpp_v2_index.page_log_addr = (lri)->lri_lsn; \
	    (pgptr)->dmpp_v2_index.page_lg_id = (lri)->lri_lg_id; \
	    (pgptr)->dmpp_v2_index.page_lga = (lri)->lri_lga; \
	    (pgptr)->dmpp_v2_index.page_jnl_filseq = (lri)->lri_jfa.jfa_filseq; \
	    (pgptr)->dmpp_v2_index.page_jnl_block = (lri)->lri_jfa.jfa_block; \
	    (pgptr)->dmpp_v2_index.page_bufid = (lri)->lri_bufid; \
        } \
	else \
	    ((DM1B_V1_INDEX*)pgptr)->page_log_addr = (lri)->lri_lsn;	\
    }

#define DM1B_VPT_GET_PAGE_LRI_MACRO(page_type, pgptr, lri) \
    {\
	if (page_type != DM_COMPAT_PGTYPE) \
	{ \
	    (lri)->lri_lsn = (pgptr)->dmpp_v2_index.page_log_addr; \
	    (lri)->lri_lg_id = (pgptr)->dmpp_v2_index.page_lg_id; \
	    (lri)->lri_lga = (pgptr)->dmpp_v2_index.page_lga; \
	    (lri)->lri_jfa.jfa_filseq = (pgptr)->dmpp_v2_index.page_jnl_filseq; \
	    (lri)->lri_jfa.jfa_block = (pgptr)->dmpp_v2_index.page_jnl_block; \
	    (lri)->lri_bufid = (pgptr)->dmpp_v2_index.page_bufid; \
        } \
	else \
	{ \
	    (lri)->lri_lsn = ((DM1B_V1_INDEX*)pgptr)->page_log_addr; \
	    (lri)->lri_lg_id = 0; \
	    (lri)->lri_lga.la_sequence = 0; \
	    (lri)->lri_lga.la_block = 0; \
	    (lri)->lri_lga.la_offset = 0; \
	    (lri)->lri_jfa.jfa_filseq = 0; \
	    (lri)->lri_jfa.jfa_block = 0; \
	    (lri)->lri_bufid = 0; \
	}\
    }

#define DM1B_VPT_GET_PAGE_LGA_MACRO(page_type, pgptr, lga) \
    {\
	if ( page_type != DM_COMPAT_PGTYPE ) \
	    lga = ((DM1B_V2_INDEX *)pgptr)->page_lga; \
	else \
	    lga.la_sequence = lga.la_block = lga.la_offset = 0;\
    }

#define DM1B_VPT_PAGE_HAS_LGA_MACRO(page_type, pgptr) \
	(page_type != DM_COMPAT_PGTYPE \
	  ? ((DM1B_V2_INDEX*)pgptr)->page_lga.la_block \
	  : 0)

#define DM1B_VPT_GET_PAGE_JNL_FILSEQ_MACRO(page_type, pgptr) \
	(page_type != DM_COMPAT_PGTYPE \
	  ? (pgptr)->dmpp_v2_index.page_jnl_filseq \
	  : 0)

#define DM1B_VPT_GET_PAGE_JNL_BLOCK_MACRO(page_type, pgptr) \
	(page_type != DM_COMPAT_PGTYPE \
	  ? (pgptr)->dmpp_v2_index.page_jnl_block \
	  : 0)

#define DM1B_VPT_GET_PAGE_BUFID_MACRO(page_type, pgptr) \
	(page_type != DM_COMPAT_PGTYPE \
	  ? (pgptr)->dmpp_v2_index.page_bufid \
	  : 0)

/* End of MVCC additions */
 
/* any change to this macro will probably require change to DMPP counterpart */
/*
** For MVCC, check also for crow_locking(), return TRUE if
** page in question is a CR page.
*/
#define DM1B_SKIP_DELETED_KEY_MACRO(r, b, lg_id, low_tran)		\
	((!row_locking(r) && !crow_locking(r)) || !low_tran 	\
	|| DMPP_VPT_IS_CR_PAGE(r->rcb_tcb_ptr->tcb_rel.relpgtype, b) \
	|| low_tran == r->rcb_tran_id.db_low_tran		\
	|| !(LSN_GTE(DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(		\
	r->rcb_tcb_ptr->tcb_rel.relpgtype, b), &r->rcb_oldest_lsn)) ? \
	TRUE : IsTranActive(lg_id, low_tran) ? FALSE : TRUE)

/* any change to this macro will probably require change to DMPP counterpart */
#define DM1B_DUPCHECK_NEED_ROW_LOCK_MACRO(r, b, lg_id, low_tran)	\
	((!row_locking(r) && !crow_locking(r)) || !low_tran 	\
	|| DMPP_VPT_IS_CR_PAGE(r->rcb_tcb_ptr->tcb_rel.relpgtype, b) \
	|| low_tran == r->rcb_tran_id.db_low_tran		\
	|| !(LSN_GTE(DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(		\
	r->rcb_tcb_ptr->tcb_rel.relpgtype, b), &r->rcb_oldest_lsn)) ? \
	FALSE : IsTranActive(lg_id, low_tran) ? TRUE : FALSE)

#define DM1B_POSITION_INIT_MACRO(r, pop)	\
	{					\
	    r->rcb_pos_info[pop].bid.tid_tid.tid_page = 0;		\
	    r->rcb_pos_info[pop].bid.tid_tid.tid_line = DM_TIDEOF;	\
	    r->rcb_pos_info[pop].nextleaf = 0;				\
	    r->rcb_pos_info[pop].valid = FALSE;				\
	    r->rcb_pos_info[pop].clean_count = DM1B_CC_INVALID;		\
	}

#define DM1B_POSITION_INVALIDATE_MACRO(r, pop)	\
	{					\
	    r->rcb_pos_info[pop].bid.tid_tid.tid_page = 0;		\
	    r->rcb_pos_info[pop].bid.tid_tid.tid_line = DM_TIDEOF;	\
	    r->rcb_pos_info[pop].nextleaf = 0;				\
	    r->rcb_pos_info[pop].valid = FALSE;				\
	    r->rcb_pos_info[pop].clean_count = DM1B_CC_INVALID;		\
	    r->rcb_pos_info[pop].line = __LINE__;			\
	}

#define DM1B_POSITION_FORCE_MACRO(r, pop)				\
	{								\
	    r->rcb_pos_info[pop].clean_count = DM1B_CC_REPOSITION;	\
	}

/*
** DM1B_SAVE_POSITION_MACRO: leaf should be locked or pinned
** Extract the key,tidp pointed to by the entry at the specified BID.
** Don't care about the partition number, I don't think
*/
#define DM1B_POSITION_SAVE_MACRO(r, b, bidp, pop)			\
{									\
 dm1b_position_save_fcn(r,b,bidp,pop,__LINE__);				\
}

#define DM1B_POSITION_VALID_MACRO(r, pop)				\
    (r->rcb_pos_info[pop].valid ? TRUE : FALSE)

#define DM1B_POSITION_PAGE_COMPARE_MACRO(pgtype, b, p)			\
((LSN_EQ(DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(pgtype, b), &(p)->lsn)	\
&& (p)->clean_count == DM1B_VPT_GET_BT_CLEAN_COUNT_MACRO(pgtype,b)) 	\
&& (((p)->page_stat & DMPP_CREAD) ==					\
 (DM1B_VPT_GET_PAGE_STAT_MACRO(pgtype,b) & DMPP_CREAD)) ?		\
DM1B_SAME : DM1B_NOTSAME)

/*
** If CLUSTERED, leaf entry contains all columns
** we could optimize by selectively copying one key col at a time
*/
#define DM1B_POSITION_SET_FETCH_FROM_GET(r)				\
{									\
	STRUCT_ASSIGN_MACRO(r->rcb_pos_info[RCB_P_GET],			\
				r->rcb_pos_info[RCB_P_FETCH]);		\
	MEcopy(r->rcb_repos_key_ptr, t->tcb_klen, r->rcb_fet_key_ptr);	\
}
