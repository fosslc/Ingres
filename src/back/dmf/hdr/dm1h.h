/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM1H.H - Hash Access Method interface definitions.
**
** Description:
**      This file describes the interface to the Hash Access Method
**	services.
**
** History:
**	 7-jul-1992 (rogerk)
**	    Created for DMF Prototyping.
**	22-oct-92 (rickh)
**	    Replaced all references to DMP_ATTS with DB_ATTS.
**	13-sep-1996 (canor01)
**	    Pass tuple buffer to dm1h_hash and dm1h_newhash.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Added prototype for dm1h_keyhash
**      10-mar-97 (stial01)
**          Pass tuple buffer to dm1h_keyhash
**      7-jan-98 (stial01)
**          Removed buffer parameter from dm1h_hash,dm1h_newhash,dm1h_keyhash
**      30-oct-2000 (stial01)
**          Changed prototype for dm1h_allocate
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      31-Jan-2002 (hweho01)
**          Added prototype of dm1h_hash_uchar().
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1h_? functions converted to DB_ERROR *
**      21-may-2009 (huazh01)
**          dm1h_hash, dm1h_newhash, dm1h_keyhash, dm1h_hash_uchar now
**          includes DMP_RCB and err_code in the parameter list.
**          output of dm1h_hash and dm1h_newhash() will be written into
**          hash_val. All these functions now return DB_STATUS instead
**          of i4. (b122075)
**	12-Nov-2009 (kschendel) SIR 122882
**	    6.0 (T0) tables no longer supported, remove old hasher.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE** with DMP_PINFO*
**	    in prototypes.
**/

FUNC_EXTERN DB_STATUS	dm1h_allocate(
				    DMP_RCB        *rcb,
				    DMP_PINFO      *pinfo,
				    DM_TID         *tid,
				    char           *record,
				    char	   *crecord,
				    i4        need,
				    i4        dupflag,
				    DB_ERROR       *dberr);

FUNC_EXTERN DB_STATUS	dm1h_search(
				    DMP_RCB         *rcb,
				    DMP_PINFO      *pinfo,
				    char            *tuple,
				    i4         mode,
				    i4         direction,
				    DM_TID          *tid,
				    DB_ERROR       *dberr);

FUNC_EXTERN DB_STATUS	dm1h_newhash(
                                    DMP_RCB        *rcb,
				    DB_ATTS        **att,
				    i4         count,
				    char            *tuple,
				    i4         buckets, 
                                    i4         *hash_val, 
                                    DB_ERROR       *dberr);

FUNC_EXTERN VOID	dm1h_nofull(DMP_RCB *rcb, DM_PAGENO bucket);

FUNC_EXTERN DB_STATUS   dm1h_keyhash(
                                    DMP_RCB        *rcb, 
				    DB_ATTS        **att,
				    i4         count,
				    char            *tuple,
				    i4              *key4,
				    i4              *key5,
				    i4              *key6, 
                                    DB_ERROR       *dberr);
