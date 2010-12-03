/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSTHSHNAM.C - Functions for hashing names
**
**  Description:
**      This files contains the functions for hashing names (specifically
**	column names) for quick lookup.
**
**          pst_hshname - Hash a column name
**
**
**  History:    $Log-for RCS$
**      26-mar-86 (jeff)    
**          written
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 pst_hshname(
	DB_ATT_NAME *colname);

/*{
** Name: pst_hashname	- Hash a column name
**
** Description:
**      This function takes a column name, hashes it, and returns a hash
**	bucket number.  It is designed to be used with the hash table of
**	columns for each range variable.
**
**	The hashing method is pretty simple: add up all of the characters
**	in the name, and return the result modulo the hash table size.
**
** Inputs:
**      colname                         Pointer to column name
**
** Outputs:
**      NONE
**	Returns:
**	    Hash bucket number
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-mar-86 (jeff)
**          written
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/
i4
pst_hshname(
	DB_ATT_NAME        *colname)
{
    register i4         ret_val = 0;
    register i4	i;
    register char	*p;

    /* Add up all the chars in the name */
    p = (char *) colname;
    for (i = 0; i < (i4)sizeof(DB_ATT_NAME); i++)
    {
	ret_val += *p;
	p++;
    }

    /* Take the result modulo the hash table size */
    ret_val %= PSS_COLTABSIZE;

    return (ret_val);
}
