/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	gwmxtab.h	- crosstab shared header.
**
** Description:
**	this file defines things shared betweent the xtab gateway
**	in gwmxtab.c and it's support routines in gwmxutil.c
**
**/

/*}
** Name:	GX_RSB	- crosstab RSB
**
** Description:
**	The cross tab RSB is complicated by the need to keep
**	lists of the attributes and their associated MO object names.
**
** History:
**	22-sep-92 (daveb)
**	    documented.
**	19-Nov-1992 (daveb)
**	    get rid of getcol and recvcol.  No value, but a lot
**	    of confusion.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Add missing prototypes.
*/

typedef struct
{
    GM_RSB	gwm_rsb;

    /* where we are processing columns in the table */

    char *tuple;		/* output tuple */
    i4  tup_len;		/* len of tuple buf */
    i4  tup_used;		/* bytes used in the tuple */
    i4  tup_cols;		/* number of columns in the tuple so far */
    i4  tup_perms;		/* and-ed perms so far for the tuple */

} GX_RSB;


/*}
** Name:	GX_PLACE_ENTRY
**
** Description:
**	Describes a place for comparing with xclassids.
**
** History:
**	29-sep-92 (daveb)
**	    created
*/
typedef struct
{
    char	*name;		/* or NULL to terminate list */
    i4		cmplen;		/* length to STbcompare */
    i4		type;		/* for gma_type field */

} GX_PLACE_ENTRY;

/* in gwxutil.c */

FUNC_EXTERN DB_STATUS GX_ad_req_col( GX_RSB *xt_rsb, i4  n );

FUNC_EXTERN DB_STATUS GX_ad_to_tuple( GX_RSB *xt_rsb, STATUS *cl_stat );
FUNC_EXTERN VOID GX_i_out_row( GWX_RCB *gwx_rcb );

