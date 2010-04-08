/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <iicommon.h>
#include <cm.h>
#include <adf.h>
#include <adulcol.h>
#include <st.h>
#include <me.h>

/**
**
**  Name: ADUCMP.C - Collation modified general string compare
**
**  Description:
**	This file contains a general string compare function supporting
**	the three major semantics used by the Ingres DBMS.
**
**	    adugcmp - string compare routine
**
**  Function prototypes defined in ADULCOL.H
** 
**  History:
**      03-may-89 (anton)
**	    Created.
**	15-Jun-89 (anton)
**	    Moved to ADU from CL.
**      31-dec-1992 (stevet)
**          Added function prototypes.
**	19-may-93 (geri)
**		Changed order of arguments to adugcmp()- the argument order
**		erroneously changed when routine was prototyped
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      17-aug-1993 (pearl)
**          Bug 53379.
**          Adugcmp() has problems with trailing skip characters in
**          char and varchar strings (i. e., when ADUL_SKIPBLANK is not
**          set).  The loop in adugcmp() checks for EOS, before calling
**          adulcmp, which is a macro that calls adultrans() on both strings
**          and then finds the difference between their weights from the
**          collation table.  The problem is that adultrans() will increment
**          the pointer as long as it sees a skip character, and it will
**          return the weight of the next non-skip character it encounters.
**          Since it has no awareness of EOS, it may walk off the EOS and
**          possibly beyond, if there is garbage at EOS and it happens to be
**          a skip character.
**          The fix is to check for EOS after the call to adulcmp, and employ
**          additional checks to see if the comparison results are valid.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      18-mar-1994 (pearl)
**          Bug 57982.  The fix for 53379 broke something else.  adugcmp()
**          had to be reworked; see in line comments for details.  One
**          change of note is to copy the strings into a buffer, get rid
**          of trailing white space, and null terminate them.  This solved
**          the problem of garbage at the end of the string that adultrans()
**          would keep devouring.  A particularly difficult case was that
**          of multiple characters that were treated as one character, e. g.
**          'LL'; if the first L was the last character in the string, and
**          the 2nd L was garbage, adultrans() would treat them as one
**          character.  Copying the strings and null terminating them avoided
**          this problem.
**	30-nov-1994 (ramra01)
**			Code Integration from 6.4. There was  a problem with string
**			comparison.
**  05-Aug-1998 (horda03) Cross-Integration of change 437013.
**      24-Jul-1998 (horda03)
**          The fix for Bug 77496 broke the Compilation on AXP platforms. The fix
**          uses MEcopy. So I've included the header file ME.H.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	03-Apr-2002 (hanje04)
**	    BUG 107501
**	    To stop the stack being corrupted when a string >2K is copied
**	    into bsal or bsbl, they are now declared as pointers and dynamically
**	    allocated memory using MEreqmem instead of declaring them as 
**	    u_char arrays on the stack.
**	    This memory is also freed using MEfree before each return()
**	    following the MEreqmem.
**/

/*{
** Name: adugcmp() - general string compare routine.
**
** Description:
**      longer comment telling what function does.
**      usually spans more than one line. 
**
** Inputs:
**	tbl		collation description table
**	flags		indicate compare semantics
**			    either 0, ADUL_SKIPBLANK, or ADUL_BLANKPAD
**	bsa		begining of first string
**	esa		pointer past end of first string
**	bsb		begining of second string
**	esb		pointer past end of second string
**
** Outputs:
**	none
**
**	Returns:
**	<0, =0, >0 for less than, equel, or greater than respectively
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-may-89 (anton)
**          Created.
**	15-Jun-89 (anton)
**	    Moved to ADU from CL.
**	19-may-93 (geri)
**		Changed order of arguments to adugcmp()- the argument order
**		erroneously changed when routine was prototyped
**      17-aug-1993 (pearl)
**          Bug 53379.
**          Adugcmp() has problems with trailing skip characters in
**          char and varchar strings (i. e., when ADUL_SKIPBLANK is not
**          set).  The loop in adugcmp() checks for EOS, before calling
**          adulcmp, which is a macro that calls adultrans() on both strings
**          and then finds the difference between their weights from the
**          collation table.  The problem is that adultrans() will increment
**          the pointer as long as it sees a skip character, and it will
**          return the weight of the next non-skip character it encounters.
**          Since it has no awareness of EOS, it may walk off the EOS and
**          possibly beyond, if there is garbage at EOS and it happens to be
**          a skip character.
**          The fix is to check for EOS after the call to adulcmp, and employ
**          additional checks to see if the comparison results are valid.
**      18-mar-1994 (pearl)
**          Bug 57982.  The fix for 53379 broke something else.  adugcmp()
**          had to be reworked; see in line comments for details.  One
**          change of note is to copy the strings into a buffer, get rid
**          of trailing white space, and null terminate them.  This solved
**          the problem of garbage at the end of the string that adultrans()
**          would keep devouring.  A particularly difficult case was that
**          of multiple characters that were treated as one character, e. g.
**          'LL'; if the first L was the last character in the string, and
**          the 2nd L was garbage, adultrans() would treat them as one
**          character.  Copying the strings and null terminating them avoided
**          this problem.
**		30-nov-1994 (ramra01)
**			Code integration from 64 based on describe and documentation
**			describing the bug with str cmp.
**	10-dec-02 (inkdo01)
**	    Changes to avoid MEreqmem unless the amt required is large (> 4K).
*/
i4
adugcmp(
ADULTABLE		*tbl,
i4				flags,
u_char			*bsa, 
register u_char	*esa, 
u_char			*bsb,
register u_char	*esb)
{
    auto ADULcstate	ast, bst;
    register i4	val, spval;
    register i4	aval, bval;
    bool                aend, bend;
    u_char      *bsal, *bsbl;
    register u_char     *esal, *esbl;
    STATUS 	status;
    char	local_str[4096];
    i4		strsize;
    bool	localuse = FALSE;

    if ((bsa == bsb) && (esa == esb))
		/* i.e. strings are the same */
	return 0;
/* bug 77496 */

    strsize = (esa-bsa)+(esb-bsb)+2;
    if (strsize > 4096)
    {
	bsal = (u_char *)MEreqmem((u_i2)0,(SIZE_TYPE)((esa-bsa) + (esb-bsb) \
    			+ 2), TRUE, &status);
    }
    else
    {
	bsal = (u_char *)&local_str[0];
	localuse = TRUE;
    }
			
    bsbl = (bsal + (esa-bsa) + 1);
    MEcopy(bsa, (esa-bsa), bsal);
    esal = (u_char *)(bsal + (i4)(esa - bsa));
    *esal='\0';
    MEcopy(bsb, (esb-bsb), bsbl);
    esbl = (u_char *)(bsbl + (i4)(esb - bsb));
    *esbl='\0';
/* bug 77496 */

    /* get the weight value of a space character */
    adulstrinit(tbl, (u_char *)" ", &ast);
    spval = adultrans(&ast);
	
	adulstrinit(tbl, bsal, &ast);
    adulstrinit(tbl, bsbl, &bst);

    if (flags & ADUL_SKIPBLANK)
    {

	while (adulptr(&ast) < esal && adulptr(&bst) < esbl)
	{
	    if ((aval = adultrans(&ast)) == spval)
	    {
		adulnext(&ast);
		continue;
	    }
	    
	    if ((bval = adultrans(&bst)) == spval)
	    {
		adulnext(&bst);
		continue;
	    }
	
		/* has adultrans() taken us past end of string ? */    
	    if ((ast.nstr <= esal) && (bst.nstr <= esbl))
	    {
	        if (val = aval - bval)
		{
		    if (!localuse)
			MEfree((char *)bsal);
		    return (val);
		}
	    }
		 else
            {
                /*
                ** only move the string that's gone past
                ** EOS forward otherwise we are moving the
                ** string that hasn't gone past EOS forward
                ** twice below.  If the next character in that
                ** string is the last, then we would think we
                ** had already reached EOS on that as well.
                */
                if (ast.nstr > esal)
                    adulnext(&ast);
 
                if (bst.nstr > esbl)
                    adulnext(&bst);
 
                continue;
            }
	    adulnext(&ast);
	    adulnext(&bst);
	}
    }
    else
    {
	while (adulptr(&ast) < esal && adulptr(&bst) < esbl)
	{
	    /*
	    ** if there were skip characters at the end of the string
	    ** encountered in adultrans(), nstr may have been incremented 
	    ** beyond the end of string, in which case the value returned
	    ** would be that of the terminating null char.
	    */
	    aval = adultrans (&ast);
	    bval = adultrans (&bst);
	    if ((ast.nstr <= esal) && (bst.nstr <= esbl))
	    {
	        if (val = aval - bval)
		{
		    if (!localuse)
			MEfree((char *)bsal);
		    return (val);
		}
	    }
		else
            {
                /*
                ** only move the string that's gone past
                ** EOS forward otherwise we are moving the
                ** string that hasn't gone past EOS forward
                ** twice below.  If the next character in that
                ** string is the last, then we would think we
                ** had already reached EOS on that as well.
                */
                if (ast.nstr > esal)
                    adulnext(&ast);
 
                if (bst.nstr > esbl)
                    adulnext(&bst);
 
                continue;
            }
	    adulnext(&ast);
	    adulnext(&bst);
	}
    }

    /* check if both strings ran out at the same time; if
    ** they did, then they match.  At least one value must be true.
    */
    aend = ((esal - adulptr(&ast) <= 0 ));
    bend = ((esbl - adulptr(&bst) <= 0 ));
 
    if (aend && bend)
    {
        /* both finished */
	if (!localuse)
	    MEfree((char *)bsal);
        return(0);
    }
    else if (bend)
    {
        /* b finished, a remaining */
        while (adulptr(&ast) < esal)
        {
            aval = adultrans(&ast);
            if (ast.nstr <= esal)
            {
                if ((flags & (ADUL_SKIPBLANK | ADUL_BLANKPAD))
                    && (aval == spval))
                {
                    adulnext(&ast);
                    continue;
                }
                else if ((flags & ADUL_BLANKPAD) && (aval < spval))
                {
		    if (!localuse)
			MEfree((char *)bsal);
                    return(-1);
                }
                else
                {
		    if (!localuse)
			MEfree((char *)bsal);
                    return(1);
                }
            }
            else
            {
                /* finished string */
                break;
            }
        }
    }
    else /* aend */
    {
        /* a finished, b remaining */
        while (adulptr(&bst) < esbl)
        {
            bval = adultrans(&bst);
            if (bst.nstr <= esbl)
            {
                if ((flags & (ADUL_SKIPBLANK | ADUL_BLANKPAD))
                    && (bval == spval))
                {
                    adulnext(&bst);
                    continue;
                }
                else if ((flags & ADUL_BLANKPAD) && (bval < spval))
                {
		    if (!localuse)
			MEfree((char *)bsal);
                    return(1);
                }
                else
                {
		    if (!localuse)
			MEfree((char *)bsal);
                    return(-1);
                }
            }
            else
            {
                /* finished string */
                break;
            }
        }
    }
 
    /* fall through */
    if (!localuse)
	MEfree((char *)bsal);
    return 0;
}
