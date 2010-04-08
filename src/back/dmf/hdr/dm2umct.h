/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM2UMCT.H - Definitions for modify table utility routines.
**
** Description:
**      This file decribes the routines that modify table structures.
**
** History:
**      25-mar-1987 (Jennifer)
**          Split from dm2u.h.
**      17-may-90 (jennifer)
**          Added MCT_KEEPDATA and MCT_USEFHDR to mct_rebuild flag.
**	17-dec-1990 (bryanp)
**	    Added new fields to support Btree Index Compression
**	14-jun-1991 (Derek)
**	    Reorganized a little in support for new build interface.
**	10-mar-1992 (bryanp)
**	    Added new fields for in-memory builds.
**	7-July-1992 (rmuth)
**	    Prototyping DMF, changed mct_fhdrnext and mct_fmapnext into
**	    DM_PAGENO from i4s
**    	05-jun-1992 (kwatts)
**          Added mct_acc_tlv to DM2U_M_CONTEXT and changed mct_atts_ptr
**          to mct_data_atts and gave one more level of indirection for
**          new compression interface that bryanp first introduced.
**	29-August-1992 (rmuth)
**	    New build code add mct_lastused_data,mct_fhdr_pageno and
**	    mct_last_dpage removed mct_fmapnext and mct_fhdrnext.
**	22-oct-92 (rickh)
**	    Replaced all references to DMP_ATTS with DB_ATTS.
**	30-October-1992 (rmuth)
**	    Add flag mct_guarantee_on_disk to the mct control_block.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	26-July-1993 (rmuth)
**	    Add mct_override_alloc, whilst building a table we usually
**	    preallocate mct_allocation amount. Sometimes we know we are
**	    going to use more than this amount this field allows us to
**	    allocate accordingly. For example if a user has specified an
**	    allocation of x pages for a HASH table but we calculate that
**	    we will use x+N hash buckets. This allows us to pre-allocate x+N 
**	    pages hopefully reducing fragmentation on extent based file 
**	    systems.
**	08-may-1995 (shero03)
**	    Added RTREE support
**	06-mar-1996 (stial01 for bryanp)
**	    Add mct_page_size to hold the page size for build routines.
**	06-may-1996 (nanpr01)
**	    Add mct_acc_plv for supporting different page accessor function.
**	22-nov-1996 (nanpr01)
**	    Add mct_ver_number for loading data with appropriate version 
**	    number.
**      10-mar-1997 (stial01)
**          Added mct_crecord, a buffer to use for compression
**      07-Dec-1998 (stial01)
**          Added mct_kperleaf to distinguish from mct_kperpage
**      12-apr-1999 (stial01)
**          Different att/key info in DM2U_M_CONTEXT for (btree) 
**          LEAF vs. INDEX pages
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**      24-nov-2009 (stial01)
**          Added mct_seg_rows
**/

/*
**  Forward typedef references.
*/

typedef struct  _DM2U_M_CONTEXT	DM2U_M_CONTEXT;


typedef struct _MCT_BUFFER
{
    i4		    mct_start;	    /* Offset into mct_data buffer. */
    i4		    mct_last;	    /* Current used offset for this window. */
    i4		    mct_finish;	    /* Maximum offset for this window. */
    DMPP_PAGE	    *mct_pages;	    /* Window of file. */
}   MCT_BUFFER;

/*}
** Name: DM2U_M_CONTEXT - Context block used by modify.
**
** Description:
**      This typedef is used by the modify dmu to keep context
**      between calls to the lower level routines which are 
**      building the new table one record at a time.  The 
**      normal high level put routines are not used to provide
**      for a more efficient method of restructuring tables.
**      Since this context is used by all access methods, it
**      contains many entries which may be ignored by any
**      one access method.
**
** History:
**     27-jan-1986 (Jennifer)
**          Created new for Jupiter.
**	19-aug-1991 (bryanp)
**	    Added new fields to support Btree Index Compression: mct_index_comp,
**	    mct_index_atts, mct_index_attcount. We need attribute information
**	    and a count of the number of attributes in order to pass to the
**	    compression routine, and we need to know what type of index
**	    compression (if any) is in effect.
**	    We use mct_index_maxlen to determine the worst case length of an
**	    entry on a page, to determine when a Btree index page is full.
**	20-aug-1991 (bryanp)
**	    Removed mct_parent. It's obsolete in 6.5.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	10-mar-1992 (bryanp)
**	    Added new fields for in-memory builds.
**    	05-jun-1992 (kwatts)
**          Added mct_acc_tlv to DM2U_M_CONTEXT and changed mct_atts_ptr
**          to mct_data_atts and gave one more level of indirection for
**          new compression interface that bryanp first introduced.
**	29-August-1992 (rmuth)
**	    New build code add mct_lastused_data,mct_fhdr_pageno and
**	    mct_last_dpage removed mct_fmapnext and mct_fhdrnext.
**	30-October-1992 (rmuth)
**	    Add flag mct_guarantee_on_disk to the mct control_block.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	06-mar-1996 (stial01 for bryanp)
**	    Add mct_page_size to hold the page size for build routines.
**	06-may-1996 (nanpr01)
**	    Add mct_acc_plv for supporting different page accessor function.
**	22-nov-1996 (nanpr01)
**	    Add mct_ver_number for loading data with appropriate version 
**	    number.
**	06-Jan-2004 (jenjo02)
**	    Added mct_tidsize, mct_partno for Global Indexes.
**	28-Feb-2004 (jenjo02)
**	    Deleted some unused defines, fields, realigned stuff
**	    to make it easier on the eyes.
**	12-Aug-2004 (schka24)
**	    Build data structure cleanup: delete unused members.
**	13-Apr-2006 (jenjo02)
**	    Add mct_cx_ixkey_atts, mct_clustered for CLUSTERED
**	    primary Btree tables.
**	24-Feb-2008 (kschendel) SIR 122739
**	    Reflect the new row-accessor scheme in the MCT.
*/
struct _DM2U_M_CONTEXT
{
    DMP_RCB         *mct_oldrcb;        /* Pointer to the rcb of table
                                        ** that is being modified. */

    /* mct_buildrcb == mct_oldrcb normally, but if we're doing an inmemory
    ** build, buildrcb points to a temporary table rcb used for holding
    ** buffer cache pages for the table being built.  For non-inmemory,
    ** it doesn't matter which you use, oldrcb or buildrcb.
    */
    DMP_RCB         *mct_buildrcb;      /* RCB for running the build into */
    DB_ATTS	    **mct_key_atts;	/* Pointer to pointers to attributes. */

    /* Row-accessor structures for the data row, the index entry (rtree or
    ** btree), and the leaf entry (btree only).
    ** Unfortunately these have to be in MCT, because btree mucks around
    ** with the index attributes at load-begin time, before "setting up" the
    ** rowaccessor.  The revised attributes could be held in the MXCB for
    ** modify/index, but there's no place to put them for LOAD, which
    ** uses only the MCT.
    **
    ** Until the btree leaf/index business can be further rationalized,
    ** or until we manage to factor out more of the data structure into
    ** something common, we'll have to put up with the rowaccessor in
    ** every MCT.  (At least, all the (partition) MCT's for a given
    ** result table can share control-array memory, since the arrays are
    ** all necessarily the same!)
    */

    DMP_ROWACCESS   mct_data_rac;	/* Data row accessor */
    DMP_ROWACCESS   mct_leaf_rac;	/* Leaf entry accessor */
    DMP_ROWACCESS   mct_index_rac;	/* Index or RTREE key accessor */

    /* Note that there is no need for a "range-key rowaccessor" like the
    ** TCB has.  We don't have to worry about old-style range entries
    ** when loading new tables, so the range keys have index format.
    */

    i4         	    mct_keys;           /* Number of attributes in key. */
    i4	    	    mct_relwid;		/* Tuple length in bytes. */
    i4	    	    mct_index_comp;	/* DM1CX-style indicator as to whether
					** index (key) compression is in use */
    i4	    	    mct_allocation;	/* File allocation information. */
    i4	    	    mct_override_alloc; /* File allocation override */
    i4	    	    mct_extend;		/* File extend information. */
    i4	    	    mct_page_size;	/* page size for new table */
    i2      	    mct_page_type;      /* page type for new table */

    /* History note:  some of these index/leaf entries used to be in
    ** little two-element arrays, perhaps with the notion of using common
    ** code and passing an array index.  That never happened, and I have
    ** simplified things for readability.
    */
    i4         	    mct_kperpage;       /* Max BTREE/ISAM keys per index page */
    i4         	    mct_kperleaf;       /* Max BTREE/RTREE keys per leaf page */
    i4		    mct_ixklen;		/* Index key length in bytes */
    i4         	    mct_klen;           /* Leaf or RTREE key length in bytes. */

    /* There are mct_keys attributes in the next two arrays, but the offsets
    ** might be different on the leaf vs the index.
    */
    DB_ATTS	    **mct_ix_keys;	/* Key (only) atts in an index entry */
    DB_ATTS	    **mct_leaf_keys;	/* key (only) atts in a leaf entry */

    i4		    mct_index_maxentry;	/* Largest index or RTREE entry size,
					** including compression expansion
					** and tid/bid (and rtree hilbert) */
    i4		    mct_leaf_maxentry;	/* Largest leaf entry size, including
					** compression expansion and tidp */
    /*
    ** BTREE sec index: index entries may differ from leaf entries
    ** In dm1bbbegin() btree specific mct fields are init
    */
    DB_ATTS         mct_cx_tidp;	/* Att entry for index page tidp */
    DB_ATTS         mct_cx_ixkey_atts[DB_MAXKEYS];
					/* Copies of keys for CLUSTERED */
    DB_ATTS         *mct_cx_ix_keys[DB_MAXKEYS]; /* Atts/keys for Index */

    bool	    mct_inmemory;	/* Indicates in-memory build yes/no */
    bool	    mct_unique;         /* Indicator for modifying to unique. */
    bool	    mct_seg_rows;       /* segmented rows if relid>maxreclen*/

    /* Most loads are executed in rebuild, recreate mode.  Existing heaps can
    ** be loaded (appended to) in nonrebuild, nonrecreate mode.  Temporary
    ** tables can be loaded in rebuild, nonrecreate mode, using an existing
    ** disk file.  (because they aren't logged and we don't care about
    ** the ability to roll back to the original.)
    */
    bool	    mct_rebuild;	/* TRUE if the table structure is to be
					** recreated (new FMAP/FHDR).  Only
					** heaps can be loaded in non-rebuild
					** mode. */
    bool	    mct_recreate;	/* Indicates new file is to be built
					** and renamed when complete. */
    bool	    mct_heapdirect;	/* Indicates to bypass sorter */
    bool	    mct_index;		/* TRUE if for secondary index. */
    bool	    mct_clustered;	/* TRUE if clustered primary btree. */
    i2		    mct_partno;		/* Partition number */
    i2		    mct_tidsize;	/* Size of TIDs */
    i4		    mct_hilbertsize;	/* RTREE hilbert size */
    DMP_LOCATION    *mct_location;	/* Locations for new files.*/
    i4         	    mct_loc_count;      /* Count of number of locations. */
    i4         	    mct_d_fill;         /* Data page fill factor, in percent. */
    i4         	    mct_i_fill;         /* Index page fill factor, in percent */
    i4         	    mct_l_fill;         /* Leaf page fill factor, in percent. */
    i4         	    mct_iperpage;       /* Max keys per index page from fill. */
    i4         	    mct_lperpage;       /* Max keys per leaf page from fill. */
    i4	    	    mct_db_fill;	/* Data page fill amount in bytes. */
    i4         	    mct_buckets;        /* Number of Hash buckets. */
    i4         	    mct_curbucket;	/* Current hash bucket being used. */
    i4	    	    mct_startmain;	/* Starting page# of main segment. */
    i4         	    mct_main;           /* Number of main pages in ISAM. */
    i4         	    mct_prim;           /* Number of primary pages in ISAM. */
    i4         	    mct_dupstart;       /* Position of start of duplicates
                                        ** in BTREE leaf page. */
    i4	    	    mct_top;            /* Top of BTREE. */
    DM_PAGENO       mct_lastleaf;       /* Last leaf page. */
    i4		    mct_mwrite_blocks;	/* Number of pages per IO in build. */
    i4	    	    mct_oldpages;	/* Number of allocated pages in file
					** before start of build. */
    i4	    	    mct_allocated;	/* Current file allocation. */
    DM_PAGENO	    mct_lastused;	/* Last page used in heap. */
    i4         	    mct_relpages;	/* Count of number of pages in file. */
    DM_PAGENO       mct_last_data_pageno;/* last data page pageno used in a HEAP
 					 */
    DM_PAGENO       mct_fhdr_pageno;    /* Location of FHDR page. */
    DMPP_ACC_PLV    *mct_acc_plv;       /* Page accessors for new table */
    DMPP_ACC_KLV    *mct_acc_klv;       /* Key accessors for new table */
    DMPP_PAGE	    *mct_curdata;	/* Points to current build data page */
    DMPP_PAGE	    *mct_curleaf;	/* Points to current build leaf page */
    DMPP_PAGE	    *mct_curindex;	/* Points to current build index page */
    DMPP_PAGE	    *mct_curovfl;	/* Points to current build ovfl page. */
    DMPP_PAGE	    *mct_curseg;
    DMPP_PAGE	    *mct_dtsave;	/* Buffer for BTREE data page */
    DMPP_PAGE	    *mct_ovsave;
    DMPP_PAGE      *mct_lfsave;	/* Buffer for BTREE leaf page */
    DMPP_PAGE      *mct_nxsave;	/* Buffer for BTREE index page */
    DMPP_PAGE       *mct_last_dpage;    /* Points to saved last data used when
					** loading data to HEAP eof */
    char            *mct_crecord;       /* Tuple buffer for compression */
    char	    *mct_segbuf;	/* Row buffer in case row spans pages*/
    MCT_BUFFER	    mct_data;		/* Data segment window. */
    MCT_BUFFER	    mct_ovfl;		/* Overflow segment window. */
    PTR		    mct_buffer;		/* Holds buffer of pages. */
    u_i2	    mct_ver_number;	/* curr. version number of the table */
    u_i2	    filler1;		/* filler for alignment */
    DM_PAGENO       mct_ancestors[sizeof(DM_TID) * 8 + 1];
};
