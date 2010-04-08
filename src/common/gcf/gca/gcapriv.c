/*
** Copyright (c) 1992, Ingres
*/

#include <compat.h>
#include <gl.h>

#include <cs.h>
#include <mu.h>
#include <gc.h>
#include <lo.h>
#include <er.h>
#include <st.h>
#include <pm.h>
#include <sl.h>
#include <iicommon.h>
#include <gca.h>
#include <gcaint.h>


/**
**
**  Name: gcapriv.c
**
**  Description:
**      Contains the following functions:
**
**          gca_chk_priv - check a user's level of privilege
**
**  History:   
**	    
**      28-Oct-92 (brucek)
**          Created.
**      10-Nov-92 (brucek)
**          #ifndef'ed GCF64.
**      19-Nov-92 (seiwald)
**          un#ifndef'ed GCF64.  GCN relies on this now.
**      07-Dec-92 (gautam)
**          Added PM support for the #ifndef GCF64 case.
**      16-Dec-92 (gautam)
**          Changed to conform to the LRC recommendations.
**      31-Dec-92 (brucek)
**          Modified to conform to LRC whim.
**      04-Mar-93 (brucek)
**          Removed #ifndef GCF64 stuff.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      27-Jan-1994 (daveb) 58733
**          Use new PM symbol, ii.*.privileges.user.USERNAME.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.  In GCA, change from 
**	    using handed in semaphore routines to MU semaphores; these
**	    can be initialized, named, and removed.  The others can't
**	    without expanding the GCA interface, which is painful and
**	    practically impossible.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**/

/*{
**
**  Name: gca_chk_priv - validate a given user's ownership of a given privilege
**
**  Description:
**	Calls PM to determine the user's allotted privileges, then
**	checks whether the specific required privilege is among them.
**
**  Inputs:
**	usr  -	user to validate
**	priv -	name of required privilege
**
**  Returns:
**	OK - user has required privilege.
**	E_GC003F_PM_NOPERM - user does not have required privilege.
**	
**  History:
**      28-Oct-92 (brucek)
**          Created.
**      27-Jan-1994 (daveb) 58733
**          Use new PM symbol, ii.*.privileges.user.USERNAME.
**      19-apr-1995 (canor01)
**          replace STprintf() with STpolycat()
**/

STATUS
gca_chk_priv( char *user_name, char *priv_name )
{
	char	pmsym[128], *value, *valueptr ;
	char	*strbuf = 0;
	int	priv_len;

        /* 
        ** privileges entries are of the form
        **   ii.<host>.privileges.<user>:   SERVER_CONTROL, NET_ADMIN ...
        */

	STpolycat(2, "$.$.privileges.user.", user_name, pmsym);
	
	/* check to see if entry for given user */
	/* Assumes PMinit() and PMload() have already been called */

	if( PMget(pmsym, &value) != OK )
	    return E_GC003F_PM_NOPERM;	
	
	valueptr = value ;
	priv_len = STlength(priv_name) ;

	/*
	** STindex the PM value string and compare each individual string
	** with priv_name
	*/
	while ( *valueptr != EOS && (strbuf=STchr(valueptr, ',')))
	{
	    if (!STscompare(valueptr, priv_len, priv_name, priv_len))
		return OK ;

	    /* skip blank characters after the ','*/	
	    valueptr = STskipblank(strbuf+1, 10); 
	}

	/* we're now at the last or only (or NULL) word in the string */
	if ( *valueptr != EOS  && 
              !STscompare(valueptr, priv_len, priv_name, priv_len))
		return OK ;

	return E_GC003F_PM_NOPERM;
}
