/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ADFTRACE.H - contains trace vector definitions for ADF.
**
** Description:
**        This file contains trace vector definitions for ADF.
**
** History: $Log-for RCS$
**      25-apr-86 (ericj)
**          Initial creation.
**	1-jun-86 (ericj)
**	    Added trace points associated with the dates datatype.
**	28-oct-86 (thurston)
**          Added trace point ADF_007_OPTABINIT_TRACE.
**      25-feb-87 (thurston)
**          Made changes for using server CB instead of GLOBALDEF/REFs.
**	04-may-87 (thurston)
**          Added trace point ADF_008_NO_ERLOOKUP.
**	08-feb-88 (thurston)
**          Added trace points ADF_009_EXECX_ENTRY, and ADF_010_CXBRIEF.
**	05-mar-1990 (fred)
**	    Added trace point ADF_011_RDM_TO_V_STYLE.  This trace point causes
**	    the adu_redeem() operation to insert a two-byte count into the
**	    beginning of the peripheral operation being returned.  This has the
**	    affect, when coordinated with the proper descriptor, of making the
**	    peripheral type appear like a varchar-ish thing -- primarily for
**	    testing before the FE's have peripheral datatype support.
**	23-may-1990 (fred)
**	    Added trace point ADF_012_VCH_TO_LVCH.  This trace point allows
**	    varchar input to be placed in a long varchar coupon.  Again,
**	    this allows testing of most of the functionality for I/O of blobs
**	    before FE support is available.
**      23-Nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
**/

#ifndef ADF_TRACE_HDR_INCLUDED
#define ADF_TRACE_HDR_INCLUDED

/*
**  Defines of other constants.
*/

/*
**  Define trace vector constants.
*/
#define                 ADF_MINTRACE       	0

/*
**  Define trace points associated with strings.
*/
#define                 ADF_000_STRFI_ENTRY	0
#define                 ADF_001_STRFI_EXIT	1

/*
**  Define trace points associated with money.
*/
#define			ADF_002_MNYFI_TRACE	2

/*
**  Define trace points associtated with dates.
*/
#define                 ADF_003_DATEFI_TRACE	3
#define			ADF_004_DATE_SUPPORT	4
#define			ADF_005_NULLDATE	5
#define			ADF_006_NORMDATE	6

/*
**  Define trace points associtated with ADF's internal tables.
*/
#define                 ADF_007_OPTABINIT_TRACE	7

/*
**  Define trace points associtated with ADF error processing.
*/
#define                 ADF_008_NO_ERLOOKUP	8

/*
**  Define trace points associtated with CXs.
*/
#define                 ADF_009_EXECX_ENTRY	9
#define                 ADF_010_CXBRIEF	       10

/*
**  Define trace point associated with peripheral datatypes -- adu_redeem()
**  and adu_couponify() support
*/
#define			ADF_011_RDM_TO_V_STYLE	11
#define			ADF_012_VCH_TO_LVCH	12

#define                 ADF_MAXTRACE	       12
/*  [@defined_constants@]...	*/
#endif /* define ADF_TRACE_HDR_INCLUDED */
