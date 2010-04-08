/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMPPN.H - Describes a Physical Page's Normal format attributes.
**
** Description:
**      This file describes structures and constants that pertain to a 
**	page on disk.  This describes the special parts of a standard
**	data page.
**
** History: 
**	04_jun_92 (kwatts)
**	    Created new for 6.5 MPF project
**	14-sep-1992 (jnash)
**	    6.5 Recovery project: Add support for new Syscat Page Types.
**	      - Add DMPP_FREE_SPACE bit and associated macros
**	06-may-1996 (thaju02)
**	    New Page Format Project:
**	      - Modify dmppn_get_offset_macro, dmppn_put_offset_macro,
**		dmppn_test_new_macro, dmppn_set_new_macro, 
**		dmppn_clear_new_macro, dmppn_test_free_macro,
**		dmppn_set_free_macro, dmppn_clear_free_macro due to tuple
**		header implementation.
**	      - Rename DMPPN_CONTENTS structure to DMPPN_V1_CONTENTS.
**		Create DMPPN_V2_CONTENTS structure.
**		Create union DMPPN_CONTENTS.
**	12-may-1996 (thaju02)
**	    Add dmppn_clear_flags_macro.
**	20-jun-1996 (thaju02)
**	    Modified offset macros, flags macros due to change made
**	    to DM_V2_LINEID structure.
**	06-jun-1998 (nanpr01)
**	    Converted the macros to v1 and v2 macros so that from selective
**	    places they can be called directly to save the pagesize comparison.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      01-feb-2001 (stial01)
**          Macro changes for Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**          Added macros for use in sig/dp/dp (Variable Page Type SIR 102955)
*/

/*
**  Macros that manipulate line table entries.
**  Note that these macros are page format specific. 
**
**  A line table entry is common to all page formats and is composed of two
**  parts:
**	bits 0-13	Offset to record from beginning of page.
**	bit  14		Free space bit.
**	bit  15		Record new indicator.
**
*/

/* Standard page macros for use by accessor routines */

#define dmppn_v1_get_offset_macro(line_tab, index)			\
	  (((DM_V1_LINEID *)line_tab)[(index)] & 0x3fff )

#define dmppn_v2_get_offset_macro(line_tab, index)			\
	  (((DM_V2_LINEID *)line_tab)[(index)] & DM_OFFSET_MASK)

#define dmppn_vpt_get_offset_macro(page_type, line_tab, index)		\
	(page_type != DM_COMPAT_PGTYPE ?				\
	dmppn_v2_get_offset_macro(line_tab, index) :			\
	dmppn_v1_get_offset_macro(line_tab, index))

#define dmppn_v1_put_offset_macro(line_tab, index, value)		\
    {									\
	((DM_V1_LINEID *)line_tab)[index] =	        		\
		(((DM_V1_LINEID *)line_tab)[(index)] & 0x8000) |	\
		((value) & 0x7fff);					\
    }

#define dmppn_v2_put_offset_macro(line_tab, index, value)		\
    {									\
        ((DM_V2_LINEID *)line_tab)[index] = 				\
		(((DM_V2_LINEID *)line_tab)[(index)] & DM_FLAGS_MASK) |	\
		((value) & DM_OFFSET_MASK);				\
    }

#define dmppn_vpt_put_offset_macro(page_type, line_tab, index, value)	\
{									\
	if (page_type != DM_COMPAT_PGTYPE)				\
	    dmppn_v2_put_offset_macro(line_tab, index, value)		\
	else								\
	    dmppn_v1_put_offset_macro(line_tab, index, value)		\
}

#define dmppn_v1_clear_flags_macro(line_tab, index)      		\
    {                                                           	\
        (((DM_V1_LINEID *)line_tab)[index] =                		\
                (((DM_V1_LINEID   *)line_tab)[index] & 0x07fff));	\
    }

#define dmppn_v2_clear_flags_macro(line_tab, index)      		\
    {                                                           	\
        ((DM_V2_LINEID *)line_tab)[index] =				\
		(((DM_V2_LINEID *)line_tab)[index] & DM_OFFSET_MASK);	\
    }

#define dmppn_v1_test_new_macro(line_tab, index)		\
	(((DM_V1_LINEID *)line_tab)[(index)] & 0x8000 )

#define dmppn_v2_test_new_macro(line_tab, index)		\
	(((DM_V2_LINEID *)line_tab)[(index)] & DM_REC_NEW )

#define dmppn_v1_set_new_macro(line_tab, index)   		\
    {								\
	((DM_V1_LINEID *)line_tab)[(index)] |= 0x8000;		\
    }

#define dmppn_v2_set_new_macro(line_tab, index)   		\
    {								\
	((DM_V2_LINEID *)line_tab)[(index)] |= DM_REC_NEW;	\
    }

#define dmppn_v1_clear_new_macro(line_tab, index)		\
    {								\
	((DM_V1_LINEID *)line_tab)[(index)] &= ~0x8000;		\
    }

#define dmppn_v2_clear_new_macro(line_tab, index)		\
    {								\
	((DM_V2_LINEID *)line_tab)[(index)] &= ~DM_REC_NEW; 	\
    }

#define dmppn_v1_test_free_macro(line_tab, index)		\
	(((DM_V1_LINEID *)line_tab)[(index)] & 0x4000)

#define dmppn_v2_test_free_macro(line_tab, index)		\
	(((DM_V2_LINEID *)line_tab)[(index)] & DM_FREE_SPACE )

#define dmppn_v1_set_free_macro(line_tab, index)		\
    {								\
	((DM_V1_LINEID *)line_tab)[(index)] |= 0x4000;		\
    }

#define dmppn_v2_set_free_macro(line_tab, index)		\
    {								\
	((DM_V2_LINEID *)line_tab)[(index)] |= DM_FREE_SPACE; 	\
    }

#define dmppn_v1_clear_free_macro(line_tab, index)		\
    {								\
	((DM_V1_LINEID *)line_tab)[(index)] &= ~0x4000;		\
    }

#define dmppn_v2_clear_free_macro(line_tab, index)		\
    {								\
	((DM_V2_LINEID *)line_tab)[(index)] &= ~DM_FREE_SPACE; 	\
    }



/*}
** Name: DMPPN_V1_CONTENTS - The layout of a Physical Data CONTENTS accessed 
**                         by normal format accessor functions.
**
** Description:
**      This defines the layout of a generic data page.
**
** History:
**      01-jun-1992 (kwatts)
**          Created.
**	02-apr-1996 (thaju02)
**	    New Page Format Support:
**		Rename structure from DMPPN_CONTENTS to DMPPN_V1_CONTENTS.
*/
typedef struct _DMPPN_V1_CONTENTS
{
    DM_V1_LINEID    page_line_tab[1];	/* Array of offset from 
                                        ** beginning of page to
					** to first byte of record. 
					*/
#define			DMPPN_V1_OVERHEAD	38
    char	    page_tuple[DM_PG_SIZE   - DMPPN_V1_OVERHEAD];
					/* The ammount of useable space on the
					** page.
					*/
} DMPPN_V1_CONTENTS;

/*}
** Name: DMPPN_V2_CONTENTS - The layout of a Physical Data CONTENTS accessed 
**                         by normal format accessor functions.
**
** Description:
**      This defines the layout of a generic data page.
**
** History:
**	02-apr-1996 (thaju02 & nanpr01)
**          Created for New Page Format Project.
**	    Changed the value of DMPPN_V2_OVERHEAD.
*/
typedef struct _DMPPN_V2_CONTENTS
{
    DM_V2_LINEID    page_line_tab[1];	/* Array of offset from 
                                        ** beginning of page to
					** to first byte of record. 
					*/
#define			DMPPN_V2_OVERHEAD	80
    char	    page_tuple[DM_PG_SIZE   - DMPPN_V2_OVERHEAD];
					/* The ammount of useable space on the
					** page.
					*/
} DMPPN_V2_CONTENTS;

/*
**  Name: DMPPN_CONTENTS - union of data page content format.
**
**  Description : This defines a union of the old and new data page
**		  contents page format.
**
**  History:
**	02-apr-1996 (thaju02)
**	    Created for New Page Format Project.
*/
typedef union _DMPPN_CONTENTS
{
    DMPPN_V1_CONTENTS	dmppn_v1_contents;
    DMPPN_V2_CONTENTS	dmppn_v2_contents;
} DMPPN_CONTENTS;

#define DMPPN_V1_PAGE_LINE_TAB_MACRO( page )      		\
        ((DM_LINEID *)(((DMPPN_V1_CONTENTS *)			\
		DMPP_V1_PAGE_CONTENTS_MACRO(page))->page_line_tab))

#define DMPPN_V2_PAGE_LINE_TAB_MACRO( page )      		\
        ((DM_LINEID *)(((DMPPN_V2_CONTENTS *)			\
		DMPP_V2_PAGE_CONTENTS_MACRO(page))->page_line_tab))
 
#define DMPPN_VPT_PAGE_LINE_TAB_MACRO(page_type, page)		\
	( page_type == DM_COMPAT_PGTYPE ?			\
	DMPPN_V1_PAGE_LINE_TAB_MACRO( page ) :			\
	DMPPN_V2_PAGE_LINE_TAB_MACRO( page ))

#define DMPPN_VPT_OVERHEAD_MACRO(page_type)			\
	( page_type == DM_COMPAT_PGTYPE ? DMPPN_V1_OVERHEAD : DMPPN_V2_OVERHEAD)
