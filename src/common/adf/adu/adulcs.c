/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <cm.h>
#include    <st.h>
#include    <me.h>
#include    <adf.h>
#include    <adulcol.h>

/**
**
**  Name: ADULCS.C - Local collation support routines
**
**  Description:
**	Core support routines for local collation.
**
**	    aducolinit - initialize a collation sequence
**	    adultrans - produce the next weight value for a string
**	    adulccmp - compare a character to the next weight value of a string
**
**  Function prototypes defined in ADULCOL.H file.
** 
**  History:
**      02-may-89 (anton)
**	    Created.
**	15-Jun-89 (anton)
**	    moved to ADU from CL
**	22-mar-91 (jrb)
**	    Added #include of me.h.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**      23-mar-93 (swm)
**          Change aducolinit function declaration to match new prototype
**          in adulcol.h.
**      14-jun-1993 (stevet)
**          Since aducolinit() is called by createdb so we need to have
**          proto and non-proto versions because FE will be linked without
**          proto.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	30-nov-94 (ramra01)
**	    Cross integ from 6.4, change in comment to indicate use of adulstrinit
**      13-feb-96 (thoda04)
**          Define return type of STATUS for non-proto version of aducolinit().
**          Default of int is not good (especially 16 bit Window 3.1 systems).
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name: aducolinit - initialize a collation sequence
**
** Description:
**	Read a compiled collation description into a collation table in memory.
**
** Inputs:
**	col			collation name
**	alloc			MEreqmem like memory allocator
**
** Outputs:
**	tbl			pointer to collation table
**	syserr			operating system specific error information
**
**	Returns:
**	    E_DB_OK
**	   !OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-may-89 (anton)
**          Created.
**	15-Jun-89 (anton)
**	    renamed and moved to ADU from CL.
**	14-jan-2004 (gupsh01)
**	    Included the location type in call to CMopen_col 
**	    & CMclose_col. 
**	11-May-2009 (kschendel) b122041
**	    Correct alloc signature to match the real MEreqmem.
*/
STATUS
aducolinit(
char		*col,
PTR		(*alloc)(u_i2  tag,
                         SIZE_TYPE size,
                         bool   clear,
                         STATUS *status ),
ADULTABLE	**tbl,
CL_ERR_DESC	*syserr)
{
    ADULTABLE	*tabp;
    union {
	char	ubuf[COL_BLOCK];
	ADULTABLE utbl;
    } uu;
    char	colname[DB_MAXNAME+1];
    int		len;
    STATUS	stat = OK;
    char	*chrp;

    STncpy(colname, col, DB_MAXNAME);
    colname[ DB_MAXNAME ] = EOS;
    STtrmwhite(colname);

    *tbl = tabp = NULL;

    if (colname[0] == EOS)
    {
	return OK;
    }

    /* read in new collation table */

    if (CMopen_col(colname, syserr, CM_COLLATION_LOC) != OK)
    {
	return FAIL;
    }

    if (CMread_col(uu.ubuf, syserr) != OK ||
	uu.utbl.magic != ADUL_MAGIC)
    {
	stat = FAIL;
    }

    if (stat == OK)
    {
	len = sizeof uu.utbl + uu.utbl.nstate * sizeof *uu.utbl.statetab;
	len = (len + COL_BLOCK-1) & ~(COL_BLOCK-1);
	chrp = (char *)(*alloc)(0, sizeof(*tabp) + len - sizeof uu.utbl,
				     FALSE, &stat);
	if (tabp = (ADULTABLE *)chrp)
	{
	    MEcopy((PTR)uu.ubuf, COL_BLOCK, (PTR)chrp);
	}
    }

    while (stat == OK && (len -= COL_BLOCK))
    {
	chrp += COL_BLOCK;
	stat = CMread_col(chrp, syserr);
    }

    _VOID_ CMclose_col(syserr, CM_COLLATION_LOC);

    if (len == 0)
    {
	*tbl = tabp;
    }

    /* XXX - If there was a problem reading the collation file, the
       memory will be allocated and not used or released */

    return stat;
}

/*{
** Name: adultrans - translate a character in a string to a weight value
**
** Description:
**	Given a state structure initialized by adulstrinit, produce the
**	next weight value from that string.
**
** Inputs:
**	d		string state descriptor
**
** Outputs:
**	d		state structure may be modified
**
**	Returns:
**	    weight value
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-may-89 (anton)
**          Created.
**	15-Jun-89 (anton)
**	    Moved to ADU from UNIX CL
**	05-Sep-90 (anton)
**	    Fix bug 31993 - incorrect LCS results
**	05-Mar-2009 (kiria01) b121748
**	    Add support for end of string to be marked by pointer/len
**	    instead of just NULL terminated.
*/
i4
adultrans(
register ADULcstate	*d)
{
    register struct ADUL_tstate	*sp;
    register u_char		*s;
    register i4		val;

    s = d->nstr;
    if (d->nextstate != -1)
    {
	val = d->nextstate;
    }
    else
    {
	s = d->lstr;
skip:
	if (d->estr && s >= d->estr)
	    val = 0;
	else
	{
	    val = d->tbl->firstchar[*s++];
	    d->nstr = s;
	}
    }
    while (val & ADUL_TMULT)
    {
	sp = d->tbl->statetab + (val & ADUL_TDATA);
	if (sp->flags & ADUL_LMULT)
	{
	    val = sp->match;
	    d->holdstate = sp->nomatch;
	}
	else if (*s == sp->testchr)
	{
	    val = sp->match;
	    if (d->estr && s >= d->estr)
		break;
	    s++;
	    if (!(sp->flags & ADUL_FINAL))
	    {
		d->nstr = s;
	    }
	}
	else
	{
	    val = sp->nomatch;
	}
    }
    if (val == ADUL_SKVAL)
	goto skip;
    return val;
}

/*{
** Name: adulccmp - compare a character with a weight value from a string
**
** Description:
**	This is a small function to compare a single character with
**	a weight value from a string descriptor initilized by adulstrinit.
**	The character may be affected by trailing context must be passed
**	by a pointer which points to that character and its context.
**
** Inputs:
**	d		string descriptor from adulstrinit 
**	c		pointer to character and context
**
** Outputs:
**	none
**
**	Returns:
**	<0, =0, >0 indicating less than, equal to, or greater than respectively
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-may-89 (anton)
**          Created.
**	15-Jun-89 (anton)
**	    Moved to ADU from CL.
*/
i4
adulccmp(
ADULcstate	*d,
u_char		*c)
{
    ADULcstate	ad;

    adulstrinit(d->tbl, c, &ad);
    return(adulcmp(d, &ad));
}
