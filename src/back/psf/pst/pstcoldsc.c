/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <qu.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <cui.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSTCOLDSC.C - Get a column description from a range variable
**
**  Description:
**      This function looks up a column description for a range variable.
**	It returns a pointer to the DMT_ATTR_ENTRY that represents the
**	column.  If it doesn't find the column, it will return NULL.  It will
**	not report an error for column not found, because SQL may need to
**	look at several tables to determine whether it can find a column.
**
**          pst_coldesc - Look up a column description for a range variable
**
**
**  History:    $Log-for RCS$
**      24-mar-86 (jeff)    
**          written
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
DMT_ATT_ENTRY *pst_coldesc(
	PSS_RNGTAB *rngentry,
	char *colname,
	i4 colnamelen);

/*{
** Name: pst_coldesc	- Look up a column description for a table.
**
** Description:
**      Given a column name and a range table entry, this function will look
**	up a column description for the column of the given name in the
**	given table.  If it finds the column, it returns a pointer to its
**	description.  If not, it returns NULL.  It won't give an error in
**	this latter case because SQL might have to look through several
**	range variables to find a column.
**
** Inputs:
**      rngentry                        Pointer to range table entry
**	colname				Name of the column
**
** Outputs:
**      NONE
**	Returns:
**	    Pointer to attribute description.  NULL if not found.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-mar-86 (jeff)
**          written
**	27-mar-90 (andre)
**	    Since column name may be Kanji, use (u_char *) instead of (char *)
**	    when computing the hash function. (bug 9473)
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-jan-06 (dougi)
**	    Search pss_coldesc array if pss_colhsh is NULL. This happens when
**	    "table" is actually a subselect in the from clause (or elsewhere).
*/
DMT_ATT_ENTRY *
pst_coldesc(
	PSS_RNGTAB         *rngentry,
	char	           *colname,
	i4		   colnamelen)
{
    register i4	bucket;
    register u_char	*p;
    register RDD_BUCKET_ATT *column;
    register i4	i;

    /* Don't assume the name was blank trimmed */
    colnamelen = cui_trmwhite(colnamelen, colname);

    /* First, check for existence of column hash. If not there, just 
    ** loop over all the attr descriptors. */
    if (rngentry->pss_colhsh == NULL)
    {
	for (i = 1; i <= rngentry->pss_tabdesc->tbl_attr_count; i++)
	if (cui_compare(rngentry->pss_attdesc[i]->att_nmlen,
		rngentry->pss_attdesc[i]->att_nmstr,
		colnamelen, colname) == 0)
	    return(rngentry->pss_attdesc[i]);

	return ((DMT_ATT_ENTRY *) NULL);	/* no match - return NULL */
    }

    /* Otherwise, find the right hash bucket */
    bucket = 0;
    p = (u_char *) colname;
    for (i = 0; i < colnamelen; i++)
    {
	bucket += *p;
	p++;
    }
    bucket %= RDD_SIZ_ATTHSH;

    column = rngentry->pss_colhsh->rdd_atthsh_tbl[bucket];

    /* Now, search through the bucket for the column name */
    while (column != (RDD_BUCKET_ATT *) NULL)
    {
	if (cui_compare(column->attr->att_nmlen, column->attr->att_nmstr,
		colnamelen, colname) == 0)
	{
	    break;
	}

	column = column->next_attr;
    }

    if (column == (RDD_BUCKET_ATT *) NULL)
    {
	return ((DMT_ATT_ENTRY *) NULL);
    }
    else
    {
	return (column->attr);
    }
}
