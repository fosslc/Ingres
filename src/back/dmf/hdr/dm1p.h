/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM1P.H - Types, Constants and Macros for Page allocation.
**
** Description:
**      This file contains all the page allocation specific typedefs,
**      constants, and macros.
**
** History:
**      17-may-90 (Derek)
**          Created.
**	08-jul-1991 (Derek)
**	    Added DM1P_NOREDO and DM1P_NEEDBI flags to dm1p_getfree routine to
**	    handle special situations for page allocations.
**	20-Nov-1991 (rmuth)
**	    Added DM1P_MINI_TRAN, to indicate that we are inside a 
**	    mini-transaction. This is currently used during page allocation 
**	    it causes the code to check that page has not been deallocated
**	    by this transaction as this would cause problems during 
**	    REDO/UNDO processing. See code for more comments.
**	7-July-1992 (rmuth)
**	    Prototyping DMF.
**    	16-jul-1992 (kwatts)
**          MPF project. Amended the descriptions of FHDR and FMAP for VME.
**	29-August-1992 (rmuth)
**	    - Removed the on-the-fly table conversion so removed dm1pxfhdr
**	      and dm1pxfmap and replaced with dm1p_init_fhdr and dm1p_add_fmap.
**	    - Changed all counts to signed as using the ANSI C compiler
**	      introduced loads of warnings. Also bryan standards don't
**	      like it for valid reasons.
**	    - Add the extend_count to the FHDR page, this records the number
**	      of times the table has been extended. It is zeroed when the table
**	      is first created or modified.
**	    - Add the DM1P_LASTUSED_DATA flag.
**	    - add dm1p_convert_table_to_65
**	08-oct-1992 (rogerk/jnash)
**	    6.5 Recovery project.
**	       - page_checksum added, fhdr_spare reduced in size
**	30-October-1992 (rmuth)
**	    - Add the DM1P_BUILD_SMS_CB control block.
**	    - Move NUMBER_FROM_MAP_MACRO macro into here so we can
**	      use in dm1x.c
**	14-dec-1992 (rogerk)
**	    - Add dm1p_fmfree, dm1p_fmused, and dm1p_fminit routines.
**	    - Included macro's from dm1p.c so they could be used by recovery.
**	    - Removed dm1p_force_map routine.
**	04-jan-1993 (rmuth)
**	    Add DM1P_MAX_TABLE_SIZE
**	08-feb-1993 (rmuth)
**	    Various changes for the cleanup of FHDR/FMAP verifydb gunk.
**	    - Remove flag arg from dm1p_verify
**	    - Add dm1p_repair, dm1p_rebuild, dm1p_verify
**	    - Remove DM1P_FCVT, this flag was used to indicate if thie
**	      was the first modify to merge on a converted table and
**	      hence scan for disassociated data pages. For various
**	      reasons this mechanism has been removed and all pages
**	      are marked as used at convert time.
**	30-mar-1993 (rmuth)
**	    Add prototype for dm1p_add_extend.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**          	Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	16-may-1995 (morayf)	
**	    NUMBER_FROM_MAP_MACRO macro had top byte corrupted on odt_us5
**	    (SCO Unix port) when called from dm1p_used_range(). This caused
**	    createdb -S iidbdb to fail with a E_DM92F1_DM1P_BUILD_SMS error.
**	    Added a 0x00FFFFFF bit-mask to the macro to mask out the top byte.
**	04-jan-1995 (cohmi01)
**	    Add flags parameter/defines for dm1p_add_extend().
**	06-mar-1996 (stial01 for bryanp)
**	    Added SMS_page_size field to DM1P_BUILD_SMS_CB.
**	14-mar-1996 (thaju02 & nanpr01)
**	    New Page Format Project.
**	      - Created structures DM1P_V2_FHDR, DM1P_V2_FMAP.
**	      - Renamed structures DM1P_FHDR to DM1P_V1_FHDR, and
**		DM1P_FMAP to DM1P_V1_FMAP.
**	      - Created DM1P_FHDR union and DM1P_FMAP union.
**	      - Created macros to access FHDR/FMAP page fields.
**	      - Added page_size parameter to function prototypes for
**		dm1p_init_fhdr, dm1p_add_fmap, dm1p_single_fmap_free, 
**		dm1p_used_range, dm1p_fmfree, dm1p_fmused, dm1p_fminit.
**      21-may-1996 (nanpr01 & thaju02)
**          Account for 64-bit tid support and multiple FHDR pages.
**	     - Modified NUMBER_FROM_MAP_MACRO, MAP_FROM_NUMBER_MACRO,
**	       SET_FREE_HINT_MACRO, CLEAR_FREE_HINT_MACRO, TEST_FREE_HINT_MACRO.
**	25-Jun-1998 (jenjo02)
**	    Added DM1P_LOCK structure.
**	    Added **lastused_page parm to dm1p_lastused() prototype.
**	10-may-1999 (walro03)
**	    Remove obsolete version string i39_vme.
**	21-jun-1999 (somsa01)
**	    Corrected typo in DM1P_GET_FHDR_PAGE_MAIN_MACRO and
**	    DM1P_GET_FMAP_PAGE_MAIN_MACRO; DM1B_V2_FHDR
**	    should be DM1P_V2_FHDR and DM1B_V1_FHDR should be DM1P_V1_FHDR.
**      16-oct-2000 (stial01)
**          Fixed DM1P_V2_FHDR_OVERHEAD, added 2 bytes for alignment
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Macro changes for Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      21-may-2001 (stial01)
**          Changes to fix FHDR fhdr_map initialization (B104754)
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_? functions converted to DB_ERROR *
**	17-Nov-2009 (kschendel) SIR 122890
**	    Remove unused setfree/testfree headers.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Embed DMP_PINFO in DM1P_LOCK.
**	    Replace DMPP_PAGE** parameter with DMP_PINFO* in
**	    dm1p_getfree, dm1p_putfree, dm1p_lastused prototypes.
**/



/*
**  Flags for DM1P_GETFREE.
**
**	DM1P_PHYSICAL	- Get a physical lock on the page.
*/

#define	DM1P_PHYSICAL	0x01
#define DM1P_NEEDBI	0x02
#define	DM1P_NOREDO	0x04
#define DM1P_MINI_TRAN	0x08

/*
**  Flags for DM1P_DUMP.
**
**	DM1P_D_BITMAP	-   Dump bitmap for debugging.
**      DM1P_D_PAGETYPE -   Dump page types for debugging.
*/

#define	DM1P_D_BITMAP	0x01
#define DM1P_D_PAGETYPE 0x02

/*
**  Flags for DM1P_VERIFY.
**
**	DM1P_V_BITMAP	-   Verify bitmaps from page information.
**	DM1P_V_REBUILD	-   Rebuild bitmaps from page information.
*/

#define	DM1P_V_BITMAP	0x01
#define	DM1P_V_REBUILD	0x02

/*
** Flags for dm1p_lastused
**
**    DM1P_LASTUSED_DATA	- Find last used data page, ie do not
**				  count FHDR/FMAP pages. Only used with
**				  heaps.
*/

#define	DM1P_LASTUSED_DATA		0x01

#define DM1P_MAX_TABLE_SIZE		8388608

/*
** Flags for dm1p_add_extend
**
**	These flags indicate whether the amount being requested represents
**	the NEW amount to add to existing pages, or the TOTAL amount of
**	pages desired in the table, including existing pages and FMAPs.
*/

#define	DM1P_ADD_NEW		0x01	/* add x number of new pages  */
#define	DM1P_ADD_TOTAL		0x02	/* add pages for a total of x */


/*}
** Name: DM1P_V1_FHDR- BTREE Free block header (version 1).
**
** Description:
**      This structure contains the layout of the free block
**	header page.  This page contains the pointers to the actual
**	free block bitmaps and hints as to whether each bitmap has
**	any free space.  The header is located in different spots in
**	different storage structures.  The following table gives the
**	location for various storage structures:
**
**		    NEW			CONVERTED
**      ------------------------------------------------------------
**	BTREE	    Page 1		Page 1
**	HEAP        Page 1		Last Page
**	ISAM	    Page Relmain	Last Page
**	HASH	    Page Relprime	Last Page
**
**	The BTREE storage structures has an existing free list.  This list
**	is converted in place and is recognized as being in the in the new
**	format via a special value in page_main.  The two special values
**	for page_main, DM1P_FCVT and DM1P_FNEW our just special numbers that
**	are not legal page numbers.
**
**	In order to accomadate maximum size files, the bitmap pointers are
**	stored as 3-byte integers.  Only 23-bits are used to address the
**	bitmap block, the other bit is used as a hint of free space available
**	on that bitmap page.  
**
**	This allocation mechanism has the following limits:
**
**	    2000 * 8 * (2000 / 3) = 10,656,000 pages.
**
**	Tables can't currently grow beyond 8388608 (2^23) pages.  The extra
**	pages are not usable because of the 23-bit page number and a minor
**	restriction that FMAP pages must be 23-bit addressable.
**
**	The first part of the page uses the same structure as an existing
**	page.  This allows all the buffer manager algorithms to function
**	correctly on this new page format.
**
**	The fhdr_count field tracks the highest FMAP allocated, and the
**	fhdr_hwmap field tracks the highest FMAP that has had a page allocated.
**	The fhdr_count field can be larger then the fhdr_hwmap field when a
**	large pre-allocation has been performed.
**
**	The fhdr_pages field tracks the number of pages in the physical file.
**	This number can be less then the value reported by the operating system
**	because of crashes.  The code handles this as a normal possibility.
**
** History:
**     02-Feb-90 (Derek)
**          Created.
**	3-June-1992 (rmuth)
**	    Add extend_count field which is only used for informationly
**	    purposes only.
**	07-oct-1992(jnash & rogerk)
**	    6.5 Recovery Project : Added checksum support
**	      - page_checksum added, fhdr_spare reduced in size.
**	      - Moved stuff around for VME definitions because the previous
**		page layout did not actually fit within 2048 bytes.  Changed
**		the extend_count field to be an i2 to preserve more potential
**		spare space.
**	      - Took out ifdefs for VME that declared less spare fields in
**		the fmap definition.  Using these was preventing us from being
**		able to ever use the free space.  Testing VME code now requires
**		pages of at least 2056 bytes to be used.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	14-mar-1996 (thaju02)
**	    New Page Format Project. Renamed to DM1P_V1_FHDR.
*/
typedef struct _DM1P_V1_FHDR
{                                       
    DM_PAGENO    page_main;		/* Next page pointer. */
#define		    DM1P_FNEW		    0x9abcdef0
    DM_PAGENO    page_ovfl;             /* Not used for FHDR pages */
    DM_PAGENO    page_page;             /* Page number of this page. */
    u_i2	 page_stat;             /* Page status. See DMPP.H */
    u_i2         page_seq_number;	/* N.A., just a placeholder. */
    DB_TRAN_ID	 page_tran_id;		/* Last transaction id to change page */
    LG_LSN	 page_log_addr;		/* The log sequence number of the log
					** record which describes the most
					** recent update made to this page. This
					** log sequence number is used by our
					** recovery protocols to determine
					** whether a particular log record's
					** changes have already been applied
					** to this page.
					*/
    u_i2	 page_checksum;		/* Checksum of all bytes on page */
    i2           fhdr_count;		/* Number of Bitmap pointers. */
    i4           fhdr_pages;		/* Last page number allocated. */
    i2           fhdr_hwmap;		/* Current highwater map page index. */
    u_i2	 extend_count;		/* records the number of times that the
					** table has been extended. */
    u_i2         fhdr_spare[3];		/* Spare for extension. */
#define             DM1P_V1_FHDR_OVERHEAD   50
#define             DM1P_V1_MAP_INT_SIZE    3
#define		    DM1P_FHINT		    0x80
    u_char	fhdr_map[1][DM1P_V1_MAP_INT_SIZE];
}   DM1P_V1_FHDR;

/*
** Name: DM1P_V2_FHDR - BTREE Free block header (version 2).
**
** History:
**	14-mar-1996 (thaju02)
**	    Created for New Page Format Project. 
**      21-may-1996 (nanpr01)
**          Account for 64-bit tid support and multiple FHDR pages.
**	    Also added next and previous page pointer for multiple FHDR's.
*/
typedef struct _DM1P_V2_FHDR
{
    DM_PAGESTAT  page_stat;             /* Page status. See DMPP.H */
    u_i4         page_spare1;           /* 64 bit tid project */
    DM_PAGENO    page_main;             /* Next page pointer. */
    u_i4         page_spare2;           /* 64 bit tid project */
    DM_PAGENO    page_ovfl;             /* Not used for FHDR pages */
    u_i4         page_spare3;           /* 64 bit tid project */
    DM_PAGENO    page_page;             /* Page number of this page. */
    DM_PGSZTYPE  page_sztype;           /* Page size + type */
    DM_PGSEQNO   page_seq_number;       /* N.A., just a placeholder. */
    u_i2         page_spare4;           /* spare for i4 checksum */
    DM_PGCHKSUM  page_checksum;         /* Checksum of all bytes on page */
    DB_TRAN_ID   page_tran_id;          /* Last transaction id to change page */
    LG_LSN       page_log_addr;         /* The log sequence number of the log
                                        ** record which describes the most
                                        ** recent update made to this page. This
                                        ** log sequence number is used by our
                                        ** recovery protocols to determine
                                        ** whether a particular log record's
                                        ** changes have already been applied
                                        ** to this page.
                                        */
    u_i4         page_spare5;           /* 64 bit tid project */
    DM_PAGENO    fhdr_pages;            /* Last page number allocated. */
    i2           fhdr_count;            /* Number of Bitmap pointers. */
    i2           fhdr_hwmap;            /* Current highwater map page index. */
    u_i2         extend_count;          /* records the number of times that the
                                        ** table has been extended. */
    u_i2         page_spare6;
    u_i4         page_spare7;           /* 64 bit tid project */
    DM_PAGENO    fhdr_next;             /* Next FHDR page pointer. */
    u_i4         page_spare8;           /* 64 bit tid project */
    DM_PAGENO    fhdr_prev;             /* Prev FHDR page pointer. */
    u_i2         fhdr_spare[3];         /* Spare for extension. */
#define		    DM1P_V2_FHDR_OVERHEAD   94
#define		    DM1P_V2_MAP_INT_SIZE    8
#define		    DM1P_FHINT		    0x80
    u_char      fhdr_map[1][DM1P_V2_MAP_INT_SIZE];
}   DM1P_V2_FHDR;

/*}
** Name: DM1P_V1_FMAP - BTREE Free block bitmap (version 1).
**
** Description:
**      This structure contains the layout of the free block
**	bitmap pages.  These pages contain bitmaps that correspond to
**	free pages in the table.
**
**	Each bitmap block maps 16000 (500 * 32) pages.  The bitmap
**	pages exist at various spots in a file.  The following table gives
**	an indication of where a bitmap page is for each table type.
**
**		    New		    Converted
**      -----------------------------------------------------------------
**	BTREE	    Page 2	    First page is free list or last page.
**	HEAP	    Page 3	    Last page after header.
**	HASH	    Relprime + 1    Last page after header.
**	ISAM	    Relmain + 1	    Last page after header.
**
**	After the first bitmap page has been placed bitmap pages occur at
**	a regular interval of every 16000 pages.  Large hash buckets sizes
**	will cause bitmaps to be bunched up at the next available page.  Also,
**	large converted tables will tend to have their bitmaps bunch up at
**	the end of the file.
**
**	The fmap_firstbit field is a hint to the lowest set bit on the page.
**	It is not a hard value, it's only the best guess place to start a
**	search.
**
**	The fmap_lastbit field is a hard value that notes the highest page
**	in this FMAP page that has been allocated by the OS.  When fmap_firstbit
**	equals fmap_lastbit the page is empty.
**
**	The fmap_hw_mark field is a hard value that notes the highest page
**	that has been allocated and thus initialized on disk.  This field
**	is always less than or equal to fmap_lastbit.
**
**	Stored on the page is the location of the last free bit found.
**	This allows searches to continue from a spot after the last searched
**	area, and tends to allocate pages in increasing page number order.
**	This is complemented by the buffer managers multi-block read mechanism.
**
**	The first part of the page uses the same structure as an existing
**	page.  This allows all the buffer manager algorithms to function
**	correctly on this new page format.
**
** History:
**     02-Feb-90 (Derek)
**          Created.
**	07-oct-1992 (jnash)
**	    6.5 Recovery Project
**	      - page_checksum added, fmap_spare reduced in size.
**	      - Took out ifdefs for VME that declared less spare fields in
**		the fmap definition.  Using these was preventing us from being
**		able to ever use the free space.  Testing VME code now requires
**		pages of at least 2056 bytes to be used.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	14-mar-1996 (thaju02)
**	    New Page Format Project. Rename to DM1P_V1_FMAP.
*/
typedef struct _DM1P_V1_FMAP
{                                       
    DM_PAGENO    page_main;             /* Next page pointer. */
    DM_PAGENO    page_ovfl;             /* Checksum for this page. */
    DM_PAGENO    page_page;             /* Page number of this page. */
    u_i2	 page_stat;             /* Page status. */
    u_i2         page_seq_number;	/* N.A. just a placeholder. */
    DB_TRAN_ID	 page_tran_id;		/* Last transaction id to change page */
    LG_LSN	 page_log_addr;		/* The log sequence number of the log
					** record which describes the most
					** recent update made to this page. This
					** log sequence number is used by our
					** recovery protocols to determine
					** whether a particular log record's
					** changes have already been applied
					** to this page.
					*/
    u_i2 	 page_checksum;		/* Checksum of all bytes on page */
    i2	 	 fmap_firstbit;		/* First free bit in page. */
    i2	 	 fmap_lastbit;		/* Last defined page. */
    i2	 	 fmap_hw_mark;		/* High water mark for allocation. */
    i2	 	 fmap_sequence;		/* FMAP sequence number. From 1..666 */
    u_i2	 fmap_spare[3];		/* Space for extensions. */
#define             DM1P_V1_FMAP_OVERHEAD   48
    u_i4	 fmap_bitmap[1]; 	/* Free bitmap array. */
}   DM1P_V1_FMAP;

/*
** Name: DM1P_V2_FMAP - BTREE Free block bitmap (version 2).
**
** History:
**	14-mar-1996 (thaju02)
**	    Created for New Page Format Project.
**      21-may-1996 (nanpr01)
**          Account for 64-bit tid support and multiple FHDR pages.
*/
typedef struct _DM1P_V2_FMAP
{
    DM_PAGESTAT  page_stat;             /* Page status. */
    u_i4         page_spare1;           /* 64 bit tid project */
    DM_PAGENO    page_main;             /* Next page pointer. */
    u_i4         page_spare2;           /* 64 bit tid project */
    DM_PAGENO    page_ovfl;             /* Checksum for this page. */
    u_i4         page_spare3;           /* 64 bit tid project */
    DM_PAGENO    page_page;             /* Page number of this page. */
    DM_PGSZTYPE  page_sztype;           /* Page size + type */
    DM_PGSEQNO   page_seq_number;       /* N.A. just a placeholder. */
    u_i2         page_spare4;           /* spare for checksum */
    DM_PGCHKSUM  page_checksum;         /* Checksum of all bytes on page */
    DB_TRAN_ID   page_tran_id;          /* Last transaction id to change page */
    LG_LSN       page_log_addr;         /* The log sequence number of the log
                                        ** record which describes the most
                                        ** recent update made to this page. This
                                        ** log sequence number is used by our
                                        ** recovery protocols to determine
                                        ** whether a particular log record's
                                        ** changes have already been applied
                                        ** to this page.
                                        */
    i4           fmap_firstbit;         /* First free bit in page. */
    i4           fmap_lastbit;          /* Last defined page. */
    i4           fmap_hw_mark;          /* High water mark for allocation. */
    i2           fmap_sequence;         /* FMAP sequence number. From 1..666 */
    u_i2         fmap_spare[3];         /* Space for extensions. */
#define		    DM1P_V2_FMAP_OVERHEAD   76
    u_i4         fmap_bitmap[1];	/* Free bitmap array. */
}   DM1P_V2_FMAP;

/*
** History:
**	14-mar-1996 (thaju02)
**	    Created for New Page Format Project.
*/
typedef union _DM1P_FHDR
{
    DM1P_V1_FHDR	dm1p_v1_fhdr;
    DM1P_V2_FHDR	dm1p_v2_fhdr;
} DM1P_FHDR;

/*
** History:
**      14-mar-1996 (thaju02)
**          Created for New Page Format Project.
*/
typedef union _DM1P_FMAP
{
    DM1P_V1_FMAP        dm1p_v1_fmap;
    DM1P_V2_FMAP        dm1p_v2_fmap;
} DM1P_FMAP;

/*
** Name: DM1P_LOCK - FHDR/FMAP lock information.
**
** Description:
**
**		Structure used by dm1p to contain FHDR/FMAP
**	        lock information.
**
** History:
**      17-Jun-1998  (jenjo02)
** 	    created.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Embed DMP_PINFO structure.
*/
typedef struct _DM1P_LOCK
{
    LK_LKID		lock_id;	/* lock_id */
    LK_VALUE		lock_value;	/* lock_value (used with FMAPs) */
    i4		fix_lock_mode;	/* lock mode used when FHDR/FMAP
					** was initially fixed */
    i4		new_lock_mode;	/* lock mode for conversion */ 
    DMP_PINFO	pinfo;		/* Page fix info */
} DM1P_LOCK;

/*
** History:
**	06-mar-1996 (stial01 for bryanp)
**	    Added SMS_page_size field to DM1P_BUILD_SMS_CB.
*/
typedef struct _DM1P_BUILD_SMS_CB
{
    i4		SMS_pages_allocated;  
				  /* Number of pages currently allocated to
				  ** the table, the build code may increase
				  ** this value
				  */
    i4		SMS_pages_used;	  
				  /* The number of pages sofar used in this
				  ** table, the build code may increase this
				  ** value
				  */
    DM_PAGENO		SMS_fhdr_pageno;
				  /* This can be used to pass in a tables 
				  ** fhdr pageno, if a new table the build
				  ** code will fill in this value
				  */
    i4		SMS_extend_size;
				  /* The build code may need to extend the
				  ** the table for the FHDR or FMAP(s) if so
				  ** it is extended in this size chunks.
				  */
    i4		SMS_page_type;    /* page type for the new table */
    i4		SMS_page_size;
				  /* page size for the new table */
    DM_PAGENO		SMS_start_pageno;
				  /* This is the position from where we
				  ** started to load data into the table.
				  */
    BITFLD		SMS_build_in_memory:1; 
				  /* A flag to indicate if the table is
				  ** being built in the Buffer Manager
				  */
    BITFLD		SMS_new_table:1;
				  /* A flag to indicate that we should build
				  ** a new FHDR/FMAP scheme
				  */
    BITFLD		SMS_bits_free:30;
    DMP_LOCATION	*SMS_location_array;
    i4		SMS_loc_count;
    DB_TAB_NAME		SMS_table_name;
    DB_DB_NAME		SMS_dbname;
    DMPP_ACC_PLV        *SMS_plv; 
				  /* Page level accessors used when
				  ** creating the FHDR and FMAP
				  */
    DMP_RCB		*SMS_inmemory_rcb;     
				  /* If building table in memory need 
				  ** this is the rcb used to fix pages
				  ** in the buffer cache. Otherwise
				  ** unsed
				  */
} DM1P_BUILD_SMS_CB;

/*
**  Forward and/or External function references.
*/


			    /* Allocate a new page. */
FUNC_EXTERN DB_STATUS dm1p_getfree(
			    DMP_RCB	*rcb,
			    i4	flag,
			    DMP_PINFO	*pinfo,
			    DB_ERROR	*dberr );


			    /* Deallocate a page. */
FUNC_EXTERN DB_STATUS dm1p_putfree(
			    DMP_RCB	*rcb,
			    DMP_PINFO	*freePinfo,
			    DB_ERROR	*dberr );


			    /* Check high water mark. */
FUNC_EXTERN DB_STATUS dm1p_checkhwm(
			    DMP_RCB	*rcb,
			    DB_ERROR	*dberr );


			    /* Find last used page. */
FUNC_EXTERN DB_STATUS dm1p_lastused(
			    DMP_RCB	*rcb,
			    u_i4	flag,
			    DM_PAGENO	*page_number,
			    DMP_PINFO   *lastusedPinfo,
			    DB_ERROR	*dberr );

			    /* Patch FHDR/FMAP(s) */
FUNC_EXTERN DB_STATUS dm1p_repair(
			    DMP_RCB	*rcb,
			    DM1U_CB	*dm1u_cb,
			    DB_ERROR	*dberr );

			    /* Verify FHDR/FMAP(s) */
FUNC_EXTERN DB_STATUS dm1p_verify(
			    DMP_RCB	*rcb,
			    DM1U_CB	*dm1u_cb,
			    DB_ERROR	*dberr );


			    /* Dump bitmaps. */
FUNC_EXTERN DB_STATUS dm1p_dump(
			    DMP_RCB     *rcb,
			    i4     flag,
			    DB_ERROR	*dberr );


			    /* Format a FHDR. */
FUNC_EXTERN VOID      dm1p_init_fhdr(
			    DM1P_FHDR   *fhdr,
			    DM_PAGENO   *next_fmap_pageno,
			    i4		pgtype,
			    DM_PAGESIZE page_size);


			    /* Format a FMAP and add it to the FHDR */
FUNC_EXTERN VOID      dm1p_add_fmap(
			    DM1P_FHDR   *fhdr,
			    DM1P_FMAP   *fmap,
			    DM_PAGENO   *next_fmap_pageno,
			    i4          pgtype,
			    DM_PAGESIZE	page_size);


			    /* Mark range of pages free. */
FUNC_EXTERN DB_STATUS dm1p_single_fmap_free(
			    DM1P_FHDR   *fhdr,
			    DM1P_FMAP   *fmap,
			    i4		pgtype,
			    DM_PAGESIZE page_size,
			    DM_PAGENO   *first_free,
			    DM_PAGENO   *last_free,
			    DM_PAGENO	*new_map,
			    DB_ERROR	*dberr );


			    /* Mark range of pages used. */
FUNC_EXTERN DB_STATUS dm1p_used_range(
			    DM1P_FHDR   *fhdr,
			    DM1P_FMAP   *fmap,
			    i4		pgtype,
			    DM_PAGESIZE page_size,
			    DM_PAGENO   *first_used,
			    DM_PAGENO   last_used,
			    DM_PAGENO   *new_map,
			    DB_ERROR	*dberr );


FUNC_EXTERN DB_STATUS dm1p_free(
    			    DMP_RCB     *rcb,
			    i4     first_free,
	    		    i4     last_free,
			    DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS dm1p_rebuild(
    			    DMP_RCB     *rcb,
			    i4     last_pageno_allocated,
			    i4     last_pageno_used,
			    i4     *fhdr_pageno,
			    DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS dm1p_build_SMS(
			    DM1P_BUILD_SMS_CB *build_cb,
			    DB_ERROR	*dberr );

FUNC_EXTERN DB_STATUS dm1p_convert_table_to_65(
			    DMP_TCB	    *tcb,
			    DB_ERROR	*dberr );

FUNC_EXTERN VOID 	dm1p_fmfree(
			    DM1P_FMAP       *fmap,
			    DM_PAGENO       first_free,
			    DM_PAGENO       last_free,
			    i4		    pgtype,
			    DM_PAGESIZE     page_size);

FUNC_EXTERN VOID 	dm1p_fmused(
			    DM1P_FMAP       *fmap,
			    DM_PAGENO       first_used,
			    DM_PAGENO       last_used,
			    i4		    pgtype,
			    DM_PAGESIZE     page_size);

FUNC_EXTERN VOID 	dm1p_fminit(
			    DM1P_FMAP       *fmap,
			    i4		    pgtype,
			    DM_PAGESIZE     page_size);

FUNC_EXTERN DB_STATUS dm1p_add_extend(
    			    DMP_RCB     *rcb,
			    i4     extend_amount,
			    i4     extend_flag,  
			    DB_ERROR	*dberr );

/*
** DM1P Macros
*/

/*  Macros to convert 3-byte to i4 and vice-versa. */

#define	VPS_NUMBER_FROM_MAP_MACRO(page_type, map)				\
    (page_type != DM_COMPAT_PGTYPE ?					\
	((map)[0] + ((map)[1] << 8) + ((map)[2] << 16) + 		\
	    ((map)[3] << 24) & 0x00FFFFFF) :				\
	(((map)[0] + ((map)[1] << 8) + (((map)[2] & ~DM1P_FHINT) << 16))\
	    & 0x00FFFFFF))

#define	VPS_MAP_FROM_NUMBER_MACRO(page_type, map, number)			\
  {									\
    if (page_type != DM_COMPAT_PGTYPE)					\
	((map)[0] = (number), (map)[1] = ((number) >> 8),		\
	    (map)[2] = ((number) >> 16), (map)[3] = ((number) >> 24),	\
	    (map)[4] = 0, (map)[5] = 0,					\
	    (map)[6] = 0, (map)[7] = 0 | ((map)[7] & DM1P_FHINT)); 		\
    else								\
	((map)[0] = (number), (map)[1] = ((number) >> 8),                   \
	    (map)[2] = ((number) >> 16) | ((map)[2] & DM1P_FHINT));		\
  }

/*  Macros to set and test the bits in a bitmap. */
#define		    DM1P_BTSZ		    32
#define		    DM1P_LG2_BTSZ	    5

#define	BITMAP_SET_MACRO(bitmap, bit)\
     ((bitmap)[(bit) >> DM1P_LG2_BTSZ] |= (1 << ((bit) & (DM1P_BTSZ - 1))))

#define BITMAP_CLR_MACRO(bitmap, bit)\
     ((bitmap)[(bit) >> DM1P_LG2_BTSZ] &= ~(1 << ((bit) & (DM1P_BTSZ - 1))))

#define BITMAP_TST_MACRO(bitmap, bit)\
     ((bitmap)[(bit) >> DM1P_LG2_BTSZ] & (1 << ((bit) & (DM1P_BTSZ - 1))))

/*  Macros to set and test the FREE HINT bit. */

#define	VPS_SET_FREE_HINT_MACRO(page_type, map)	\
        (page_type != DM_COMPAT_PGTYPE ?		\
	    ((map)[7] |= DM1P_FHINT) :		\
	    ((map)[2] |= DM1P_FHINT))

#define	VPS_CLEAR_FREE_HINT_MACRO(page_type, map)	\
        (page_type != DM_COMPAT_PGTYPE ?		\
            ((map)[7] &= ~DM1P_FHINT):            \
            ((map)[2] &= ~DM1P_FHINT))

#define VPS_TEST_FREE_HINT_MACRO(page_type, map)	\
        (page_type != DM_COMPAT_PGTYPE ?		\
            ((map)[7] & DM1P_FHINT):		\
            ((map)[2] & DM1P_FHINT))

/*
** Macros to access fhdr page header fields
**
** History:
**	06-may-1996 (thaju02)
**	    Created for new page format support.
*/
#define DM1P_VPT_GET_FHDR_PAGE_STAT_MACRO(page_type,pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?                  \
        ((DM1P_V2_FHDR *)pgptr)->page_stat :			\
        ((DM1P_V1_FHDR *)pgptr)->page_stat)
 
#define DM1P_VPT_SET_FHDR_PAGE_STAT_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FHDR *)pgptr)->page_stat |= value;		\
        else 							\
	  ((DM1P_V1_FHDR *)pgptr)->page_stat |= value;		\
     }
 
#define DM1P_VPT_INIT_FHDR_PAGE_STAT_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FHDR *)pgptr)->page_stat = value;		\
        else                                                    \
          ((DM1P_V1_FHDR *)pgptr)->page_stat = value;           \
     }

#define DM1P_VPT_CLR_FHDR_PAGE_STAT_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FHDR *)pgptr)->page_stat &= ~value;		\
        else 							\
	  ((DM1P_V1_FHDR *)pgptr)->page_stat &= ~value;		\
     }
 
#define DM1P_VPT_GET_FHDR_PAGE_MAIN_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FHDR *)pgptr)->page_main :			\
        ((DM1P_V1_FHDR *)pgptr)->page_main)
 
#define DM1P_VPT_SET_FHDR_PAGE_MAIN_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FHDR *)pgptr)->page_main = value;          	\
        else 							\
	  ((DM1P_V1_FHDR *)pgptr)->page_main = value;		\
     }
 
#define DM1P_VPT_ADDR_FHDR_PAGE_MAIN_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        &((DM1P_V2_FHDR *)pgptr)->page_main :			\
        &((DM1P_V1_FHDR *)pgptr)->page_main)
 
#define DM1P_VPT_GET_FHDR_PAGE_OVFL_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FHDR *)pgptr)->page_ovfl :			\
        ((DM1P_V1_FHDR *)pgptr)->page_ovfl)
 
#define DM1P_VPT_SET_FHDR_PAGE_OVFL_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FHDR *)pgptr)->page_ovfl = value;		\
        else 							\
	  ((DM1P_V1_FHDR *)pgptr)->page_ovfl = value;		\
     }
 
#define DM1P_VPT_ADDR_FHDR_PAGE_OVFL_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        &((DM1P_V2_FHDR *)pgptr)->page_ovfl :			\
        &((DM1P_V1_FHDR *)pgptr)->page_ovfl)
 
#define DM1P_VPT_GET_FHDR_PAGE_PAGE_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FHDR *)pgptr)->page_page :			\
        ((DM1P_V1_FHDR *)pgptr)->page_page)
 
#define DM1P_VPT_SET_FHDR_PAGE_PAGE_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FHDR *)pgptr)->page_page = value;		\
        else 							\
	  ((DM1P_V1_FHDR *)pgptr)->page_page = value;		\
     }
 
#define DM1P_VPT_ADDR_FHDR_PAGE_PAGE_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        &((DM1P_V2_FHDR *)pgptr)->page_page :			\
        &((DM1P_V1_FHDR *)pgptr)->page_page)
 
#define DM1P_VPT_SET_FHDR_PAGE_SZTYPE_MACRO(page_type, page_size, pgptr)\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
          ((DM1P_V2_FHDR *)pgptr)->page_sztype = page_size + page_type;	\
     }
 
#define DM1P_VPT_GET_FHDR_PAGE_SEQNO_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?                  \
        ((DM1P_V2_FHDR *)pgptr)->page_seq_number :		\
        ((DM1P_V1_FHDR *)pgptr)->page_seq_number)
 
#define DM1P_VPT_SET_FHDR_PAGE_SEQNO_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
            ((DM1P_V2_FHDR *)pgptr)->page_seq_number = value;	\
        else 							\
	    ((DM1P_V1_FHDR *)pgptr)->page_seq_number = value;	\
     }
 
#define DM1P_VPT_GET_FHDR_PAGE_CHKSUM_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?                  \
        ((DM1P_V2_FHDR *)pgptr)->page_checksum :		\
        ((DM1P_V1_FHDR *)pgptr)->page_checksum)
 
#define DM1P_VPT_SET_FHDR_PAGE_CHKSUM_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FHDR *)pgptr)->page_checksum = value;      	\
        else 							\
	  ((DM1P_V1_FHDR *)pgptr)->page_checksum = value;	\
     }
 
#define DM1P_VPT_GET_FHDR_PAGE_TRANID_MACRO(page_type, pgptr, tran_id)	\
        (page_type != DM_COMPAT_PGTYPE ?                          	\
        STRUCT_ASSIGN_MACRO(((DM1P_V2_FHDR *)pgptr)->page_tran_id, tran_id) : \
        STRUCT_ASSIGN_MACRO(((DM1P_V1_FHDR *)pgptr)->page_tran_id, tran_id))
 
#define DM1P_VPT_SET_FHDR_PAGE_TRANID_MACRO(page_type, pgptr, tran_id)	\
     {									\
        if (page_type != DM_COMPAT_PGTYPE)                       	\
          STRUCT_ASSIGN_MACRO(tran_id, ((DM1P_V2_FHDR *)pgptr)->page_tran_id);\
        else                                                    	\
          STRUCT_ASSIGN_MACRO(tran_id, ((DM1P_V1_FHDR *)pgptr)->page_tran_id);\
     }
 
#define DM1P_VPT_GET_FHDR_TRAN_ID_LOW_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FHDR *)pgptr)->page_tran_id.db_low_tran :     \
        ((DM1P_V1_FHDR *)pgptr)->page_tran_id.db_low_tran)
 
#define DM1P_VPT_GET_FHDR_TRANID_HIGH_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FHDR *)pgptr)->page_tran_id.db_high_tran :	\
        ((DM1P_V1_FHDR *)pgptr)->page_tran_id.db_high_tran)
 
#define DM1P_VPT_GET_FHDR_PG_LOGADDR_MACRO(page_type, pgptr, log_addr)	\
        (page_type != DM_COMPAT_PGTYPE ?                          	\
        STRUCT_ASSIGN_MACRO(((DM1P_V2_FHDR *)pgptr)->page_log_addr,log_addr): \
        STRUCT_ASSIGN_MACRO(((DM1P_V1_FHDR *)pgptr)->page_log_addr, logaddr))
 
#define DM1P_VPT_SET_FHDR_PG_LOGADDR_MACRO(page_type, pgptr, log_addr)	\
     {									\
        if (page_type != DM_COMPAT_PGTYPE)                       	\
        STRUCT_ASSIGN_MACRO(log_addr,((DM1P_V2_FHDR *)pgptr)->page_log_addr); \
        else                                                    	\
        STRUCT_ASSIGN_MACRO(log_addr, ((DM1P_V1_FHDR *)pgptr)->page_log_addr);\
     }
 
#define DM1P_VPT_GET_FHDR_LOGADDR_LOW_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FHDR *)pgptr)->page_log_addr.lsn_low :        \
        ((DM1P_V1_FHDR *)pgptr)->page_log_addr.lsn_low)
 
#define DM1P_VPT_GET_FHDR_LOGADDR_HI_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FHDR *)pgptr)->page_log_addr.lsn_high :       \
        ((DM1P_V1_FHDR *)pgptr)->page_log_addr.lsn_high)
 
#define DM1P_VPT_ADDR_FHDR_PG_LOGADDR_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?                  \
        (LG_LSN *)&(((DM1P_V2_FHDR *)pgptr)->page_log_addr) :	\
        (LG_LSN *)&(((DM1P_V1_FHDR *)pgptr)->page_log_addr))
 
#define DM1P_VPT_GET_FHDR_PAGES_MACRO(page_type, pgptr)		\
	(page_type != DM_COMPAT_PGTYPE ?			\
	((DM1P_V2_FHDR *)pgptr)->fhdr_pages :			\
	((DM1P_V1_FHDR *)pgptr)->fhdr_pages)

#define DM1P_VPT_SET_FHDR_PAGES_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FHDR *)pgptr)->fhdr_pages = value;		\
        else							\
	  ((DM1P_V1_FHDR *)pgptr)->fhdr_pages = value;		\
     }

#define DM1P_VPT_GET_FHDR_COUNT_MACRO(page_type, pgptr)      	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FHDR *)pgptr)->fhdr_count :           	\
        ((DM1P_V1_FHDR *)pgptr)->fhdr_count)
 
#define DM1P_VPT_SET_FHDR_COUNT_MACRO(page_type, pgptr, value)  \
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FHDR *)pgptr)->fhdr_count = value;          \
        else                                                    \
          ((DM1P_V1_FHDR *)pgptr)->fhdr_count = value;		\
     }

#define DM1P_VPT_INCR_FHDR_COUNT_MACRO(page_type, pgptr)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)		\
          ((DM1P_V2_FHDR *)pgptr)->fhdr_count++;		\
	else							\
          ((DM1P_V1_FHDR *)pgptr)->fhdr_count++;		\
     }
 
#define DM1P_VPT_DECR_FHDR_COUNT_MACRO(page_type, pgptr)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)		\
          ((DM1P_V2_FHDR *)pgptr)->fhdr_count--;		\
	else							\
          ((DM1P_V1_FHDR *)pgptr)->fhdr_count--;		\
     }
 
#define DM1P_VPT_GET_FHDR_HWMAP_MACRO(page_type, pgptr)      	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FHDR *)pgptr)->fhdr_hwmap :			\
        ((DM1P_V1_FHDR *)pgptr)->fhdr_hwmap)
 
#define DM1P_VPT_SET_FHDR_HWMAP_MACRO(page_type, pgptr, value)  \
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FHDR *)pgptr)->fhdr_hwmap = value;          \
        else                                                    \
          ((DM1P_V1_FHDR *)pgptr)->fhdr_hwmap = value;		\
     }

#define DM1P_VPT_GET_FHDR_EXTEND_CNT_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FHDR *)pgptr)->extend_count :			\
        ((DM1P_V1_FHDR *)pgptr)->extend_count)
 
#define DM1P_VPT_SET_FHDR_EXTEND_CNT_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FHDR *)pgptr)->extend_count = value;	\
        else                                                    \
          ((DM1P_V1_FHDR *)pgptr)->extend_count = value;	\
     }

#define DM1P_VPT_INCR_FHDR_EXTEND_CNT_MACRO(page_type, pgptr)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)		\
	  ((DM1P_V2_FHDR *)pgptr)->extend_count++;		\
	else							\
	  ((DM1P_V1_FHDR *)pgptr)->extend_count++;		\
     }

#define DM1P_VPT_DECR_FHDR_EXTEND_CNT_MACRO(page_type, pgptr)   \
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FHDR *)pgptr)->extend_count--;              \
        else                                                    \
          ((DM1P_V1_FHDR *)pgptr)->extend_count--;		\
     }
 
#define DM1P_VPT_SET_FHDR_SPARE_MACRO(page_type, pgptr, index, value)	\
     {								\
	if (page_type != DM_COMPAT_PGTYPE)		\
	  ((DM1P_V2_FHDR *)pgptr)->fhdr_spare[index] = value;	\
	else							\
	  ((DM1P_V1_FHDR *)pgptr)->fhdr_spare[index] = value;	\
     }

#define DM1P_VPT_FHDR_MAP_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ?			\
        (u_char *)(((DM1P_V2_FHDR *)pgptr)->fhdr_map) :		\
        (u_char *)(((DM1P_V1_FHDR *)pgptr)->fhdr_map))
 
#define DM1P_VPT_GET_FHDR_MAP_MACRO(page_type, pgptr, index)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        (((DM1P_V2_FHDR *)pgptr)->fhdr_map[index]) :		\
        (((DM1P_V1_FHDR *)pgptr)->fhdr_map[index]))

#define DM1P_VPT_ADDR_FHDR_MAP_MACRO(page_type, pgptr, index)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        (u_char *)&(((DM1P_V2_FHDR *)pgptr)->fhdr_map[index]) :	\
        (u_char *)&(((DM1P_V1_FHDR *)pgptr)->fhdr_map[index]))

#define DM1P_SIZEOF_FHDR_MAP_MACRO(page_type, pgsize)		\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((u_i2)(pgsize - DM1P_V2_FHDR_OVERHEAD)) :		\
        ((u_i2)(pgsize - DM1P_V1_FHDR_OVERHEAD)))
 

/*
** Macros to access fmap page header fields
**
** History:
**	06-may-1996 (thaju02)
**	    Created for new page format support.
*/
#define DM1P_VPT_GET_FMAP_PAGE_STAT_MACRO(page_type,pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FMAP *)pgptr)->page_stat :			\
        ((DM1P_V1_FMAP *)pgptr)->page_stat)
 
#define DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FMAP *)pgptr)->page_stat |= value;		\
        else 							\
	  ((DM1P_V1_FMAP *)pgptr)->page_stat |= value;		\
     }
 
#define DM1P_VPT_INIT_FMAP_PAGE_STAT_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FMAP *)pgptr)->page_stat = value;		\
        else                                                    \
          ((DM1P_V1_FMAP *)pgptr)->page_stat = value;           \
     }

#define DM1P_VPT_CLR_FMAP_PAGE_STAT_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FMAP *)pgptr)->page_stat &= ~value;		\
        else 							\
	  ((DM1P_V1_FMAP *)pgptr)->page_stat &= ~value;		\
     }
 
#define DM1P_VPT_GET_FMAP_PAGE_MAIN_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FMAP *)pgptr)->page_main :           		\
        ((DM1P_V1_FMAP *)pgptr)->page_main)
 
#define DM1P_VPT_SET_FMAP_PAGE_MAIN_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FMAP *)pgptr)->page_main = value;          	\
        else 							\
	  ((DM1P_V1_FMAP *)pgptr)->page_main = value;		\
     }
 
#define DM1P_VPT_ADDR_FMAP_PAGE_MAIN_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        &((DM1P_V2_FMAP *)pgptr)->page_main :			\
        &((DM1P_V1_FMAP *)pgptr)->page_main)
 
#define DM1P_VPT_GET_FMAP_PAGE_OVFL_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FMAP *)pgptr)->page_ovfl :			\
        ((DM1P_V1_FMAP *)pgptr)->page_ovfl)
 
#define DM1P_VPT_SET_FMAP_PAGE_OVFL_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FMAP *)pgptr)->page_ovfl = value;		\
        else 							\
	  ((DM1P_V1_FMAP *)pgptr)->page_ovfl = value;		\
     }
 
#define DM1P_VPT_ADDR_FMAP_PAGE_OVFL_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?                  \
        &((DM1P_V2_FMAP *)pgptr)->page_ovfl :			\
        &((DM1P_V1_FMAP *)pgptr)->page_ovfl)
 
#define DM1P_VPT_GET_FMAP_PAGE_PAGE_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FMAP *)pgptr)->page_page :			\
        ((DM1P_V1_FMAP *)pgptr)->page_page)
 
#define DM1P_VPT_SET_FMAP_PAGE_PAGE_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FMAP *)pgptr)->page_page = value;		\
        else 							\
	  ((DM1P_V1_FMAP *)pgptr)->page_page = value;		\
     } 

#define DM1P_VPT_ADDR_FMAP_PAGE_PAGE_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        &((DM1P_V2_FMAP *)pgptr)->page_page :			\
        &((DM1P_V1_FMAP *)pgptr)->page_page)
 
#define DM1P_VPT_SET_FMAP_PAGE_SZTYPE_MACRO(page_type, page_size, pgptr)\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)			\
          ((DM1P_V2_FMAP *)pgptr)->page_sztype = page_size + page_type;	\
     }
 
#define DM1P_VPT_GET_FMAP_PAGE_SEQNO_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FMAP *)pgptr)->page_seq_number :		\
        ((DM1P_V1_FMAP *)pgptr)->page_seq_number)
 
#define DM1P_VPT_SET_FMAP_PAGE_SEQNO_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
            ((DM1P_V2_FMAP *)pgptr)->page_seq_number = value;	\
        else 							\
	    ((DM1P_V1_FMAP *)pgptr)->page_seq_number = value;	\
     }
 
#define DM1P_VPT_GET_FMAP_PAGE_CHKSUM_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?                  \
        ((DM1P_V2_FMAP *)pgptr)->page_checksum :		\
        ((DM1P_V1_FMAP *)pgptr)->page_checksum)
 
#define DM1P_VPT_SET_FMAP_PAGE_CHKSUM_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FMAP *)pgptr)->page_checksum = value;      	\
        else 							\
	  ((DM1P_V1_FMAP *)pgptr)->page_checksum = value;	\
     }
 
#define DM1P_VPT_GET_FMAP_PAGE_TRANID_MACRO(page_type, pgptr, tran_id)	\
        (page_type != DM_COMPAT_PGTYPE ?                          	\
        STRUCT_ASSIGN_MACRO(((DM1P_V2_FMAP *)pgptr)->page_tran_id, tran_id) : \
        STRUCT_ASSIGN_MACRO(((DM1P_V1_FMAP *)pgptr)->page_tran_id, tran_id))
 
#define DM1P_VPT_SET_FMAP_PAGE_TRANID_MACRO(page_type, pgptr, tran_id)	\
     {									\
        if (page_type != DM_COMPAT_PGTYPE)                       	\
          STRUCT_ASSIGN_MACRO(tran_id, ((DM1P_V2_FMAP *)pgptr)->page_tran_id);\
        else                                                    	\
          STRUCT_ASSIGN_MACRO(tran_id, ((DM1P_V1_FMAP *)pgptr)->page_tran_id);\
     }
 
#define DM1P_VPT_GET_FMAP_TRAN_ID_LOW_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FMAP *)pgptr)->page_tran_id.db_low_tran :     \
        ((DM1P_V1_FMAP *)pgptr)->page_tran_id.db_low_tran)
 
#define DM1P_VPT_GET_FMAP_TRANID_HIGH_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FMAP *)pgptr)->page_tran_id.db_high_tran :    \
        ((DM1P_V1_FMAP *)pgptr)->page_tran_id.db_high_tran)
 
#define DM1P_VPT_GET_FMAP_PG_LOGADDR_MACRO(page_type, pgptr, log_addr)	\
        (page_type != DM_COMPAT_PGTYPE ?                          	\
        STRUCT_ASSIGN_MACRO(((DM1P_V2_FMAP *)pgptr)->page_log_addr,log_addr): \
        STRUCT_ASSIGN_MACRO(((DM1P_V1_FMAP *)pgptr)->page_log_addr, logaddr))
 
#define DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(page_type, pgptr, log_addr)	\
     {									\
        if (page_type != DM_COMPAT_PGTYPE)                       	\
        STRUCT_ASSIGN_MACRO(log_addr,((DM1P_V2_FMAP *)pgptr)->page_log_addr); \
        else                                                    	\
        STRUCT_ASSIGN_MACRO(log_addr, ((DM1P_V1_FMAP *)pgptr)->page_log_addr);\
     }
 
#define DM1P_VPT_GET_FMAP_LOGADDR_LOW_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FMAP *)pgptr)->page_log_addr.lsn_low :        \
        ((DM1P_V1_FMAP *)pgptr)->page_log_addr.lsn_low)
 
#define DM1P_VPT_GET_FMAP_LOGADDR_HI_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FMAP *)pgptr)->page_log_addr.lsn_high :       \
        ((DM1P_V1_FMAP *)pgptr)->page_log_addr.lsn_high)
 
#define DM1P_VPT_ADDR_FMAP_PG_LOGADDR_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?                  \
        (LG_LSN *)&(((DM1P_V2_FMAP *)pgptr)->page_log_addr) :	\
        (LG_LSN *)&(((DM1P_V1_FMAP *)pgptr)->page_log_addr))
 
#define DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(page_type, pgptr)	\
	(page_type != DM_COMPAT_PGTYPE ?			\
	((DM1P_V2_FMAP *)pgptr)->fmap_firstbit :		\
	((DM1P_V1_FMAP *)pgptr)->fmap_firstbit)

#define DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(page_type, pgptr, value)   	\
     {								\
	if (page_type != DM_COMPAT_PGTYPE)		\
	  ((DM1P_V2_FMAP *)pgptr)->fmap_firstbit = value;	\
	else							\
	  ((DM1P_V1_FMAP *)pgptr)->fmap_firstbit = value;	\
     }
        
#define DM1P_VPT_GET_FMAP_LASTBIT_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FMAP *)pgptr)->fmap_lastbit :			\
        ((DM1P_V1_FMAP *)pgptr)->fmap_lastbit) 
 
#define DM1P_VPT_SET_FMAP_LASTBIT_MACRO(page_type, pgptr, value)\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FMAP *)pgptr)->fmap_lastbit = value;	\
        else                                                    \
          ((DM1P_V1_FMAP *)pgptr)->fmap_lastbit = value;	\
     }

#define DM1P_VPT_GET_FMAP_HW_MARK_MACRO(page_type, pgptr)    	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((DM1P_V2_FMAP *)pgptr)->fmap_hw_mark :			\
        ((DM1P_V1_FMAP *)pgptr)->fmap_hw_mark) 
 
#define DM1P_VPT_SET_FMAP_HW_MARK_MACRO(page_type, pgptr, value)\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FMAP *)pgptr)->fmap_hw_mark = value;        \
        else                                                    \
          ((DM1P_V1_FMAP *)pgptr)->fmap_hw_mark = value;	\
     }
 
#define DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(page_type, pgptr)	\
        (page_type != DM_COMPAT_PGTYPE ?                  \
        ((DM1P_V2_FMAP *)pgptr)->fmap_sequence :		\
        ((DM1P_V1_FMAP *)pgptr)->fmap_sequence) 
 
#define DM1P_VPT_SET_FMAP_SEQUENCE_MACRO(page_type, pgptr, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)                \
          ((DM1P_V2_FMAP *)pgptr)->fmap_sequence = value;	\
        else                                                    \
          ((DM1P_V1_FMAP *)pgptr)->fmap_sequence = value;	\
     }
 
#define DM1P_VPT_ADDR_FMAP_SEQUENCE_MACRO(page_type, pgptr)   	\
        (page_type != DM_COMPAT_PGTYPE ?                  \
        &((DM1P_V2_FMAP *)pgptr)->fmap_sequence :        	\
        &((DM1P_V1_FMAP *)pgptr)->fmap_sequence)

#define DM1P_VPT_SET_FMAP_SPARE_MACRO(page_type, pgptr, index, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)		\
          ((DM1P_V2_FMAP *)pgptr)->fmap_spare[index] = value;	\
        else                                                  	\
          ((DM1P_V1_FMAP *)pgptr)->fmap_spare[index] = value;	\
     }

#define DM1P_VPT_FMAP_BITMAP_MACRO(page_type, pgptr)		\
        (page_type != DM_COMPAT_PGTYPE ?                  \
        (u_i4 *)((DM1P_V2_FMAP *)pgptr)->fmap_bitmap :		\
        (u_i4 *)((DM1P_V1_FMAP *)pgptr)->fmap_bitmap)

#define DM1P_SIZEOF_FMAP_BITMAP_MACRO(page_type, pgsize)	\
        (page_type != DM_COMPAT_PGTYPE ?			\
        ((u_i2)(pgsize - DM1P_V2_FMAP_OVERHEAD)) :		\
        ((u_i2)(pgsize - DM1P_V1_FMAP_OVERHEAD)))
 
#define DM1P_VPT_GET_FMAP_BITMAP_MACRO(page_type, pgptr, index)	\
        (page_type != DM_COMPAT_PGTYPE ?                 	\
        ((DM1P_V2_FMAP *)pgptr)->fmap_bitmap[index] :		\
        ((DM1P_V1_FMAP *)pgptr)->fmap_bitmap[index])
 
#define DM1P_VPT_SET_FMAP_BITMAP_MACRO(page_type, pgptr, index, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)		\
          ((DM1P_V2_FMAP *)pgptr)->fmap_bitmap[index] |= value;	\
	else							\
          ((DM1P_V1_FMAP *)pgptr)->fmap_bitmap[index] |= value; \
     }
 
#define DM1P_VPT_CLR_FMAP_BITMAP_MACRO(page_type, pgptr, index, value)	\
     {								\
        if (page_type != DM_COMPAT_PGTYPE)		\
          ((DM1P_V2_FMAP *)pgptr)->fmap_bitmap[index] &= ~value;\
	else							\
          ((DM1P_V1_FMAP *)pgptr)->fmap_bitmap[index] &= ~value;\
     }
 
#define DM1P_FSEG_MACRO(page_type, pgsize)				\
        (page_type != DM_COMPAT_PGTYPE ?                 	\
	(((pgsize - DM1P_V2_FMAP_OVERHEAD)/((i4)sizeof(u_i4))) * DM1P_BTSZ) :\
	(((pgsize - DM1P_V1_FMAP_OVERHEAD)/((i4)sizeof(u_i4))) * DM1P_BTSZ))

#define DM1P_MBIT_MACRO(page_type, pgsize)				\
        (page_type != DM_COMPAT_PGTYPE ?                 	\
	((pgsize - DM1P_V2_FMAP_OVERHEAD)/((i4)sizeof(u_i4))) : \
	((pgsize - DM1P_V1_FMAP_OVERHEAD)/((i4)sizeof(u_i4))))


#define DM1P_VPT_MAP_INT_SIZE_MACRO(page_type)				\
	(page_type != DM_COMPAT_PGTYPE ? DM1P_V2_MAP_INT_SIZE :		\
	DM1P_V1_MAP_INT_SIZE)

#define DM1P_NUM_FMAP_PAGENO_ON_FHDR_MACRO(page_type, page_size)	\
	DM1P_SIZEOF_FHDR_MAP_MACRO(page_type, page_size)/		\
	DM1P_VPT_MAP_INT_SIZE_MACRO(page_type)

/*
** The number of pages in a table for any page type or page size is
** limited by the 23 bits available in the tid for the page number
** 2^23 = 8388608
** The number of pages in a table is also limited by the number of
** fmap page numbers on the fhdr, and the number of pages mapped in
** the fmap_bitmap 
** That is, the maximum pages in a table is the minimum of those two numbers
** 
** This macro was added because 2k (V2,V3,V4) pages can address LESS than
** 8388608 pages because DM1P_V2_FHDR pages can fit less than half as many 
** fmap page numbers ((due to V2_FHDR_OVERHEAD and sizeof entries in fhdr_map),
** and the DM1P_V2_FMAP fmap_bitmap is smaller (due to V2_FMAP_OVERHEAD). 
**
** WARNING for large page sizes the number of pages calculated with:
**  DM1P_NUM_FMAP_PAGENO_ON_FHDR_MACRO * DM1P_SIZEOF_FMAP_BITMAP_MACRO *
**  BITSPERBYTE 
** can overflow a 4 byte integer.  Be careful changing this macro.
**/
#define DM1P_VPT_MAXPAGES_MACRO(page_type, page_size)			\
    ((page_size == DM_COMPAT_PGSIZE && page_type != DM_COMPAT_PGTYPE) ?	\
    DM1P_NUM_FMAP_PAGENO_ON_FHDR_MACRO(page_type, page_size) *		\
    DM1P_SIZEOF_FMAP_BITMAP_MACRO(page_type, page_size) * BITSPERBYTE :	\
    DM1P_MAX_TABLE_SIZE )

