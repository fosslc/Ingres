/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPDMFLIBDATA
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <tr.h>
#include    <st.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpl.h>
#include    <dmpp.h>

/**
**
**  Name: DMPLKUP.ROC - DMF read only lookup tables.
**
**  Description:
** 	This file initializes dmf/dmp lookup tables
**
**  History:
**	01-feb-2001 (stial01)
**	    Created.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages -> page type V5
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPDMFLIBDATA
**	    in the Jamfile.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add page type V6, lg_id in tuple header
**/

GLOBALCONSTDEF	DMPP_PAGETYPE Dmpp_pgtype[] = {
    /*
    ** This table is the page type information lookup table,
    ** a DMF internal structure used for quick access to page type informatino
    ** By having this table, we can easily add additional page types having
    ** different tuple headers and different capabilities
    **
    ** Each array element in the lookup table looks like this:
    **
    i4  dmpp_tuphdr_size;	Tuple header size 
    i4  dmpp_lowtran_offset;	Offset of low tranid in tup hdr 
    i4	dmpp_hightran_offset;	Offset of high tranid in tup hdr 
    i4  dmpp_seqnum_offset;	Offset of deferred cursor seq# in tup hdr 
    i4  dmpp_vernum_offset;	Offset of alter table version in tup hdr 
    i4  dmpp_seghdr_offset;	Offset of next segment information
    i4  dmpp_lgid_offset;	Offset of lg_id in tup hdr
    i1  dmpp_row_locking;	True if page type supports row locking 
    i1	dmpp_has_versions;	True if page type supports alter table
    i1  dmpp_has_seghdr;        True if page type supports rows span pages
    i1  dmpp_has_lgid;          True if page type supports lg_id
    i1  dmpp_has_rowseq;        True if deferred cursor seqn# in row hdr
    **
    */

    /* 0 is invalid pgtype */ 
    { -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0},

    /*
    ** TCB_PG_V1
    **  V1 pages have no tuple headers but there is special code for etabs
    **  and core catalogs which do physical page locking to overlay deleted
    **  tuples with the low tranid (for space reservation algorithms)
    **  That is why this structure is defined with 0 byte tuple header,
    **  and a non-zero low tranid offset.
    */
    {  /* TCB_PG_V1 */
	0,
	CL_OFFSETOF(DMPP_TUPLE_HDR_V1, low_tran), 
	-1,
	-1,
	-1,
	-1,	/* No seghdr in tuple hdr */
	-1,	/* No lg_id in tuple hdr */
	0,	/* No Row locking */
	0,	/* No Alter table */
	0,	/* No rows spanning pages */
	0,	/* No lg_id in tuple hdr */
	0       /* No deferred update cursor seq# in row hdr */
    },

    /* 
    ** TCB_PG_V2
    */
    {
	sizeof(DMPP_TUPLE_HDR_V2),
	CL_OFFSETOF(DMPP_TUPLE_HDR_V2, low_tran),
	CL_OFFSETOF(DMPP_TUPLE_HDR_V2, high_tran),
	CL_OFFSETOF(DMPP_TUPLE_HDR_V2, sequence_num),
	CL_OFFSETOF(DMPP_TUPLE_HDR_V2, ver_number),
	-1,	/* No seghdr in tuple hdr */
	CL_OFFSETOF(DMPP_TUPLE_HDR_V2, lg_id),
	1,	/* Row locking supported */
	1,	/* Alter table supported */
	0,	/* No rows spanning pages */
	1,	/* Has lg_id in tuple header */
	1       /* Has deferred update cursor seq# in row hdr */
    },

    /*
    ** TCB_PG_V3
    */
    {
	sizeof(DMPP_TUPLE_HDR_V3),
	CL_OFFSETOF(DMPP_TUPLE_HDR_V3, low_tran),
	-1,	/* No high tran in hdr */
	-1,	/* No deferred cursor sequence# in hdr */
	CL_OFFSETOF(DMPP_TUPLE_HDR_V3, ver_number),
	-1,	/* No seghdr in tuple hdr */
	-1,	/* No lg_id in tuple hdr */
	1,	/* Row locking supported */
	1,	/* Alter table supported */
	0,	/* No rows spanning pages */
	0,	/* No lg_id in tuple hdr */
	0       /* No deferred update cursor seq# in row hdr */
    },

    /*
    ** TCB_PG_V4
    */
    {
	sizeof(DMPP_TUPLE_HDR_V4),
	CL_OFFSETOF(DMPP_TUPLE_HDR_V4, low_tran),
	-1,	/* No high tran in hdr */
	-1,	/* No deferred cursor sequence# in hdr */
	-1,	/* No alter table version number in hdr */
	-1,	/* No seghdr in tuple hdr */
	-1,	/* No lg_id in tuple hdr */
	1,	/* Row locking supported */
	0,	/* No Alter table */
	0,	/* No rows spanning pages */
	0,	/* No lg_id in tuple hdr */
	0       /* No deferred update cursor seq# in row hdr */
    },

    /*
    ** TCB_PG_V5
    */
    {
	sizeof(DMPP_TUPLE_HDR_V5),
	CL_OFFSETOF(DMPP_TUPLE_HDR_V5, low_tran),
	-1,	/* No high tran in hdr */
	-1,	/* No deferred cursor sequence# in hdr */
	CL_OFFSETOF(DMPP_TUPLE_HDR_V5, ver_number),
	CL_OFFSETOF(DMPP_TUPLE_HDR_V5, seg_hdr),
	-1,	/* No lg_id in tuple hdr */
	1,	/* Row locking supported */
	1,	/* Alter table supported */
	1,	/* Rows spanning pages supported */
	0,	/* No lg_id in tuple hdr */
	0       /* No deferred update cursor seq# in row hdr */
    },

    /*
    ** TCB_PG_V6
    */
    {
	sizeof(DMPP_TUPLE_HDR_V6),
	CL_OFFSETOF(DMPP_TUPLE_HDR_V6, low_tran),
	-1,	/* No high tran in hdr */
	CL_OFFSETOF(DMPP_TUPLE_HDR_V6, sequence_num),
	CL_OFFSETOF(DMPP_TUPLE_HDR_V6, ver_number),
	-1,	/* No seghdr in tuple hdr */
	CL_OFFSETOF(DMPP_TUPLE_HDR_V6, lg_id),
	1,	/* Row locking supported */
	1,	/* Alter table supported */
	0,	/* No rows spanning pages */
	1,	/* Has lg_id in tuple hdr */
	1       /* Has deferred update cursor seq# in row hdr */
    },

    /*
    ** TCB_PG_V7
    */
    {
	sizeof(DMPP_TUPLE_HDR_V7),
	CL_OFFSETOF(DMPP_TUPLE_HDR_V7, low_tran),
	-1,	/* No high tran in hdr */
	CL_OFFSETOF(DMPP_TUPLE_HDR_V7, sequence_num),
	CL_OFFSETOF(DMPP_TUPLE_HDR_V7, ver_number),
	CL_OFFSETOF(DMPP_TUPLE_HDR_V7, seg_hdr),
	CL_OFFSETOF(DMPP_TUPLE_HDR_V7, lg_id),
	1,	/* Row locking supported */
	1,	/* Alter table supported */
	1,	/* Rows spanning pages supported */
	1,	/* Has lg_id in tuple hdr */
	1       /* Has deferred update cursor seq# in row hdr */
    }
};

