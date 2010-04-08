/*
** Copyright 1995, 2000 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <rms.h>
# include <descrip.h>
# include <er.h>
# include <st.h>
# include <lo.h>
# include "lolocal.h"
# include "loparse.h"

/*
** Name: LOnoconceal.c
**
** Description:
**	Contains an interface to SYS$PARSE() which guarantees that the
**	resultant string is completely expanded.  Intended to be used from
**	the dladtreg.c module.
**
**	This module should *not* be referenced outside of the CL.
**
** History:
**	1-aug-95 (dougb)
**	    Created.
**	22-aug-95 (albany)
**	    Fixed minor typo.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

/*
** Name: LOnoconceal
**
** Description:
**	Call SYS$PARSE() for the specified file name.  This handles all of the
**	messy RMS structures -- including any required required cleanup.   This
**	routine is intended to be used from the new dladtreg.c module.  It is
**	*not* intended to be called outside of a VMS Compatibility Library.
**
**	In essence, this is similar to LOparse().  But, the caller does not
**	need to allocate a LOpstruct structure or worry about cleanup.
**
** Input Parameters:
**	char		*input		Name of file to expand.
**
** Output Parameters:
**	char		*expand		Expanded name of file.  This
**					must be MAX_LOC + 1 in length.
**	char		*filename	Actual filename portion of expand.
**	i4		*vms_stat	Return status from VMS routines
**					used by this routine.
**
** Returns:
**	STATUS				CL return from other LO routines.
**
** Assumptions:
**	Filename must exist or LOnoconceal() will return an error (from
**	LOwcard()).
**
** Side Effects:
**	May temporarily open a channel.
**
** History:
**	1-aug-95 (dougb)
**	    Created new routine.
*/
STATUS
LOnoconceal( char *input, char *expand, char *filename, i4 *vms_stat )
{
    char	fullnm	[MAX_LOC + 1];
    LOCATION	loc;
    LO_DIR_CONTEXT lc;
    LOpstruct	pstruct;

    /* Portions of the input filename. */
    char	dev	[MAX_LOC + 1];
    char	path	[MAX_LOC + 1];
    char	prefix	[MAX_LOC + 1];
    char	suffix	[MAX_LOC + 1];
    char	vers	[MAX_LOC + 1];

    /* Support for string manipulations. */
    register char *p = expand;		/* Current output loc. */
    register char *inp = pstruct.pesa;	/* Current start. */
    register char *nextbrk;		/* Following bracket. */

    bool	done_parse = FALSE;
    STATUS	stat;

    *vms_stat = RMS$_NORMAL;
    do {
	/*
	** Get basic filename.  We need the name's various components for
	** the following search.
	*/
	STcopy( input, fullnm );
	stat = LOfroms( PATH & FILENAME, fullnm, &loc );
	if ( OK != stat )
	    break;

	stat = LOdetail( &loc, dev, path, prefix, suffix, vers );
	if ( OK != stat )
	    break;

	/*
	** Find out where the file *really* is.
	*/
	stat = LOwcard( &loc, prefix, suffix, vers, &lc );
	if ( OK != stat )
	    break;
	stat = LOwend( &lc );
	if ( OK != stat )
	    break;

	/*
	** Now, expand filename completely.
	*/
	stat = FAIL;
	done_parse = TRUE;
	*vms_stat = LOparse( loc.string, &pstruct, TRUE );
	if ( 0 == ( *vms_stat & 1 ))
	    break;

	if ( NAM$M_CNCL_DEV == ( NAM$M_CNCL_DEV & pstruct.pnam.nam$l_fnb ))
	{
	    pstruct.pnam.nam$b_nop |= NAM$M_NOCONCEAL;
	    *vms_stat = LOparse( loc.string, &pstruct, FALSE );
	    if ( 0 == ( *vms_stat & 1 ))
		break;
	}

	/*
	** Everything worked, start filling in our output parameters.
	*/
	stat = OK;
	pstruct.pnam.nam$l_ver [pstruct.pnam.nam$b_ver] = EOS;
	STlcopy( pstruct.pnam.nam$l_name, filename, pstruct.pnam.nam$b_name );

	/*
	** Get rid of any "][" or "><" (et cetera) pairings while copying
	** into the caller's storage for the expanded name.
	*/
	nextbrk = STindex( inp, CBRK, 0 );
	if ( NULL == nextbrk )
	    nextbrk = STindex( inp, CABRK, 0 );

	/* Check for an immediately following open bracket. */
	while ( NULL != nextbrk
	       && ( *OBRK == nextbrk [1] || *OABRK == nextbrk [1] ))
	{
	    /* Copy up to the first offending character. */
	    STlcopy( inp, p, nextbrk - inp );
	    p += ( nextbrk - inp );

	    inp = nextbrk + 2;

	    /* Find next matching close bracket. */
	    if ( *OBRK == nextbrk [1] )
		nextbrk = STindex( inp, CBRK, 0 );
	    else
		nextbrk = STindex( inp, CABRK, 0 );
	}

	/* Copy the remainder of the expanded name. */
	STcopy( inp, p );
    } while ( FALSE );

    if ( done_parse )
	LOclean_fab( &pstruct.pfab );

    return stat;
}
