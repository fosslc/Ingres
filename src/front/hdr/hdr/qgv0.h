/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    qgv0.h -	Query Generator Module Version 0 Query Description
**				Definition File.
** Description:
**	Contains the structure definition for the version 0 query description.
**
** History:
**	Revision 6.0  87/04  wong
**	Version 1 created.  (This structure definition renamed and moved
**	here from "front/hdr/qg.h".)
**
**	Revision 3.0  84/02  joe
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*}
** Name:    QDV0 -	Query Generator Version 0 Query Description.
**
** Description:
**	The v.0 query description contains the components of a parameter
**	retrieve.  A query was built up from these lists of strings and values.
**	Values were formatted by embedding a "$[cfi]" in the target list or
**	where clause.
*/
typedef struct qd_old {
    char	    *qg_param;		/* The param target list */
    char	    **qg_argv;		/* The argument vector */
    i4		    *qg_sizelist;	/* The sizes of the values */
    char	    *qg_wparam;		/* A param where clause */
    char	    **qg_wargv;		/* arguments for the where */
    char	    **qg_sortlist;	/* arguments for sort clause */
    char	    **qg_sortdir;	/* directions for sort args */
    struct qd_old   *qg_child;		/* A detail query */
    struct qgint    *qg_internal;
    char	    **qg_tlargv;	/* arguments for aggregates
					** within param target list */
} QDV0;

/*
** Name:    QG_INPUT -	Version 0 QG Input Values.
**
** Description:
**	These input values are recognized only by version 0.
*/
# define	QI_OSL		0105
# define	QI_UOSL		0107
