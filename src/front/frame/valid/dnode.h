/*
**	dnode.h
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	dnode.h - Include header file for derivation execution.
**
** Description:
**	File contains various definitions to support executing
**	derivation formulas.
**
** History:
**	06/05/89 (dkh) - Initial version.
**	06/23/92 (dkh) - Added support for decimal.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/



/*}
** Name:	DNODE - A node in derivation execution tree.
**
** Description:
**	This describes a node in the execution tree for a
**	derivation formula.
**
** History:
**	06/05/89 (dkh) - Initial version.
*/
typedef struct dnode
{
	i4		nodetype;	/* type of the node */

# define	ICONST_NODE	0	/* integer constant node */
# define	FCONST_NODE	1	/* float constant node */
# define	SCONST_NODE	2	/* string constant node */
# define	PLUS_NODE	3	/* plus operator node */
# define	MINUS_NODE	4	/* minus operator node */
# define	MULT_NODE	5	/* multiply operator node */
# define	DIV_NODE	6	/* divide operator node */
# define	EXP_NODE	7	/* exponentiation operator node */
# define	UMINUS_NODE	8	/* unary minus operator node */
# define	MAX_NODE	9	/* max aggregate node */
# define	MIN_NODE	10	/* min aggregate node */
# define	AVG_NODE	11	/* average aggregate node */
# define	SUM_NODE	12	/* sum aggregate node */
# define	CNT_NODE	13	/* count aggregate node */
# define	SFLD_NODE	14	/* simple field node */
# define	TCOL_NODE	15	/* table field node */
# define	DCONST_NODE	16	/* decimal constant node */

	FIELD		*fld;		/* field that node depends on */
	i4		colno;		/* column number if TCOL_NODE */
	DB_DATA_VALUE	*result;	/* result for node */
	DB_DATA_VALUE	*cresult;	/* coerced result for node */
	ADI_FI_DESC	*fdesc;		/* ADF info for operator nodes */
	struct	dnode	*left;		/* left branch pointer */
	struct	dnode	*right;		/* right branch pointer */
	FLDHDR		*agghdr;	/* FLDHDR for aggregate source */
	i4		workspace;	/* workspace for aggregate node */

# define	NO_AGG		(-1)	/* indicates no aggregate info */

	ADI_FI_ID	fid;		/* operator function instance */
	ADE_EXCB	*drv_excb;	/* pointer to CX for root node */
	i4		res_inx;	/* CX index for result DBV */
	i4		cres_inx;	/* CX index for coerce result DBV */
	ADI_FI_ID	coer_fid;	/* aggregate function instance */
} DNODE;
