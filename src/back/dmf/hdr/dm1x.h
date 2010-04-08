/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM1X.H - Types, Constants and Macros for Build buffering.
**
** Description:
**      This file contains all the typedefs, constants, and macros
**	associated with the Special build buffering routines.
**
** History:
**      11-may-90 (Derek)
**          Created.
**	16-apr-1992 (bryanp)
**	    Function prototypes.
**	7-July-1992 (rmuth)
**	    Prototyping DMF.
**	29-August-1992 (rmuth)
**	    Add dm1x_build_SMS prototype.
**	30-October-1992 (rmuth)
**	    Remove dm1x_build_SMS proto, as now local to dm1x.c.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1x? functions converted to DB_ERROR *
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DB_STATUS	dm1xstart(DM2U_M_CONTEXT *mct, i4 flag,
				    DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS	dm1xfinish(DM2U_M_CONTEXT *mct, i4 operation,
				    DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS	dm1xnewpage(DM2U_M_CONTEXT *mct, i4 page,
				    DMPP_PAGE **newpage, DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS	dm1xnextpage(DM2U_M_CONTEXT *mct, i4 *nextpage,
				    DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS	dm1xreadpage(DM2U_M_CONTEXT *mct, i4 update,
				    i4 pageno, DMPP_PAGE **page,
				    DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS	dm1xheader(DM2U_M_CONTEXT *mct, i4 reserve,
				    DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS	dm1xreserve(DM2U_M_CONTEXT *mct, i4 reserve,
				    DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS	dm1xfree(DM2U_M_CONTEXT *mct,  DM_PAGENO start,
				    DM_PAGENO end, DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS   dm1xbput(DM2U_M_CONTEXT *mct, DMPP_PAGE *data,
				    char *record, i4 rec_size,
				    i4 record_type, i4 fill, DM_TID *tid,
				    u_i2 current_table_version,
				    DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS	dm1x_get_segs(DM2U_M_CONTEXT  *mct, 
				    DMPP_SEG_HDR *first_seg_hdr,
				    char *first_seg, char *record, 
				    DB_ERROR *dberr);


/*  Defines of constants for dm1xstart(). */

#define		DM1X_OVFL	0x01
#define		DM1X_SAVE	0x02
#define		DM1X_KEEPDATA   0x04

/*  Defines of constants for dm1xfinish(). */

#define		DM1X_CLEANUP	0x00
#define		DM1X_COMPLETE	0x01

/*  Defines of constants for dm1xreadpage(). */

#define		DM1X_FORREAD	0x00
#define		DM1X_FORUPDATE	0x01

