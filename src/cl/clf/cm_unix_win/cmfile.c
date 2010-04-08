/*
**Copyright (c) 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <clconfig.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
#include    <nm.h>
#include    <st.h>
#include    <lo.h>
#include    <me.h>
#include    <meprivate.h>
#include    <mu.h>
#include    <errno.h>

#ifdef	UNIX
#include    <ernames.h>
#endif

#ifdef DESKTOP
#include    <er.h>
#endif /* DESKTOP */

/**
**
**  Name: CMFILE.C - Routines to read and write collation files
**
**  Description:
**	The file contains routines to read and write collation sequence
**	description file.  This seporate from DI because it is needed
**	by FE programs and it is not SI because that is not legal for
**	the DB server.
**
**		CMopen_col	- open collation file for reading
**		CMread_col	- read collation file
**		CMclose_col	- close collation file
**		CMdump_col	- create and write a collation file.
**
**  History:
**      16-Jun-89 (anton)
**	    Created.
**	31-may-90 (blaise)
**	    Added include <me.h> to pick up redefinition of MEfill().
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-apr-95 (emmag)
**	    Desktop porting changes.
**	16-apr-95 (emmag)
**	    8.3 file name restrictions require us to change collation
**	    to collate.
**	19-may-95 (canor01)
**	    added <errno.h>
**	06-jun-1995 (canor01)
**	    added VMS semaphore protection for Unix (MCT)
**      12-jun-1995 (canor01)
**          semaphore protect NMloc()'s static buffer (MCT)
**	03-jun-1996 (canor01)
**	    Remove NMloc() semaphore.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**      24-Feb-2000 (consi01) Bug 100051 INGSRV 1114
**          Modified CMclose_col to use fclose() for NT. The original
**          call to close() was failing causing exhaustion of the
**          available file descriptors.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-jan-2002 (stial01)
**          Synchronize ER_SEMFUNCS with the ER_SEMFUNCS in erloc.h
**	05-sep-2002 (hanch04)
**          For the 32/64 bit build,
**	    Add lp64 directory for 64-bit add on.
**	14-jan-2004 (gupsh01)
**	    Added CM_dumpUniMap function, to write and read data from
**	    $II_SYSTEM/ingres/files/unicharmaps, which contain mapping
**	    between unicode and local character sets.
**	16-jan-2004 (gupsh01)
**	    Fixed CMopen_col function.
**      18-feb-2004 (gupsh01)
**          Added CM_DEFAULTFILE_LOC which allows looking for default unicode 
**          coercion mapping file in $II_SYSTEM/ingres/files directory which 
**          is not yet copied to $II_SYSTEM/ingres/files/ucharmaps directory,
**          at install time.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**      07-Mar-2005 (hanje04)
**          SIR 114034
**          Add support for reverse hybrid builds. First part of much wider
**          change to allow support of 32bit clients on pure 64bit platforms.
**          Changes made here allow application statically linked against
**          the standard 32bit Intel Linux release to run against the new
**          reverse hybrid ports without relinking.
**      10-Oct-2006 (hanal04) Bug 116717
**          Move collation file statics into TLS if we are OS threaded.
**          Whilst the dcb_mutex may limit or even prevent concurrent
**          access for the same DB it can not prevent concurrent access
**          as multiple DBs are opened at the same time.
**      03-Jun-2009 (whiro01) SD 136525
**          For the OpenROAD FUSED runtime, we need to use SIopen et.al.
**          to locate these resources because they won't be actual files
**          in this case, but resources in the DLL.
**          Removed the VMS code since it now resides in its own file.
**          Simplified the overall code by eliminating redundancies across
**          #ifdef's and making two helper routines.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
**	24-Aug-2009 (kschendel) 121804
**	    Update cer_xxx function declarations to fix gcc 4.3 problems.
**     22-Mar-2010 (hanje04)
**         SIR 123296
**         Use ADMIN not FILES for accessing ucharmaps. For LSB builds
**         ADMIN is the writable location, no net effect for regular builds.
**/


typedef	struct _ER_SEMFUNCS
{
/* 
** WARNING !!!
** This structure must be kept in sync with the one in cl/clf/er/erloc.h
*/
    STATUS	(*er_p_semaphore)();
    STATUS	(*er_v_semaphore)();
    u_i2	    sem_type;
#define		    CS_SEM	0x01	/* CS semaphore routines */
#define		    MU_SEM	0x02	/* MU semaphore routines */

    CS_SEMAPHORE    er_sem;
    MU_SEMAPHORE    er_mu_sem;
} ER_SEMFUNCS;

typedef	struct _CMCOLDFLE
{
    ER_SEMFUNCS	*sems;
    FILE	*f;
} CMCOLDFLE;

static CMCOLDFLE file;

# ifdef OS_THREADS_USED
static ME_TLS_KEY       CMcolkey = 0;
# endif /* OS_THREADS_USED */

/* perhaps er_unix_win/erloc.h would have this but not sure? */
FUNC_EXTERN bool cer_issem();


/*
** Name: setuplocation - common code to setup the location for
**	 these collation table files
**
** Description:
**	Given a collation name and the location type, return the
**	proper LOCATION for this file.  Takes into account reverse
**	hybrid locations.
**
** Inputs:
**	colname		collation name
**	loctype		one of the CM_UCHARMAPS_LOC or CM_COLLATION_LOC
**			values
**
** Outputs:
**	ploc		fully initialized LOCATION for this collation file
**
** Returns:
**	void
**
** Side Effects:
**	No obvious ones ;)
*/
static void
setuplocation(char *colname, i4 loctype, LOCATION* ploc)
{
    NMloc(ADMIN, PATH, (char *)NULL, ploc);
    if (loctype == CM_UCHARMAPS_LOC)
      LOfaddpath(ploc, "ucharmaps", ploc);
    else if (loctype == CM_COLLATION_LOC)
      LOfaddpath(ploc, "collation", ploc);
#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH64)
    LOfaddpath(ploc, "lp64", ploc);
#endif /* hybrid */
#if defined(conf_BUILD_ARCH_64_32) && defined(BUILD_ARCH32)
    {
	/*
	** Reverse hybrid support must be available in ALL
	** 32bit binaries
	*/
	char	*rhbsup;
	NMgtAt("II_LP32_ENABLED", &rhbsup);
	if ( (rhbsup && *rhbsup) &&
       ( !(STbcompare(rhbsup, 0, "ON", 0, TRUE)) ||
         !(STbcompare(rhbsup, 0, "TRUE", 0, TRUE))))
	    LOfaddpath(ploc, "lp32", ploc);
    }
#endif /* reverse hybrid */
    LOfstfile(colname, ploc);
}

static CMCOLDFLE *get_coldfle(int create)
{
    STATUS	status;
    CMCOLDFLE   *cmcoldle;

# ifdef OS_THREADS_USED
    if ( create )
    {
	if ( CMcolkey == 0 )
	{
	    ME_tls_createkey( &CMcolkey, &status );
	    ME_tls_set( CMcolkey, NULL, &status );
	}
	if ( CMcolkey == 0 )
	{
	    /* not linked with threading libraries */
	    CMcolkey = -1;
	}
    }
    if ( CMcolkey == -1 )
    {
        cmcoldle = &file;
    }
    else
    {
        ME_tls_get( CMcolkey, (PTR *)&cmcoldle, &status );
        if ( cmcoldle == NULL && create )
        {
            cmcoldle = (CMCOLDFLE *) MEreqmem( 0, sizeof(CMCOLDFLE),
                                               TRUE, NULL );
            ME_tls_set( CMcolkey, (PTR)cmcoldle, &status );
        }
    }
# else /* OS_THREADS_USED */
    cmcoldle = &file;
# endif /* OS_THREADS_USED */

    return cmcoldle;
}

/*{
** Name: CMopen_col - open collation file for reading
**
** Description:
**	open collation file for reading
**
** Inputs:
**	colname				collation name
**
** Outputs:
**	syserr				OS specific error
**
**	Returns:
**	    E_DB_OK
**	    FAIL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	May acquire a semaphore to protect itself from reentry and
**	hold that semaphore until the file is closed.
**	>>>>>CMclose_col must be called to release this semaphore<<<<<<
**
** History:
**      16-Jun-89 (anton)
**          Created.
**	26-mar-1996 (canor01)
**	    Windows NT: Convert the FILE pointer to a file descriptor for 
**	    subsequent Unix-compatible file operations.
**      05-sep-2002 (hanch04)
**          Add lp64 directory for 64-bit add on.
**	14-jan-2004 (gupsh01)
**	    Added CM_dumpUniMap function, to write and read data from
**	    $II_SYSTEM/ingres/files/unicharmaps, which contain mapping
**	    between unicode and local character sets.
**	02-feb-2004 (gupsh01) 
**	    Fixed DESKTOP case to correctly open the mapping files.
**      07-Mar-2005 (hanje04)
**          SIR 114034
**          Add support for reverse hybrid builds. First part of much wider
**          change to allow support of 32bit clients on pure 64bit platforms.
**          Changes made here allow application statically linked against
**          the standard 32bit Intel Linux release to run against the new
**          reverse hybrid ports without relinking.
**      03-Jun-2009 (whiro01)
**          Calling SIread for all platforms to support the
**          OpenROAD FUSED DLL runtime where these tables are
**          not in an external file but in internal resources;
**          simplified the code by eliminating duplicate stuff
**          in #ifdef's.
*/
STATUS
CMopen_col(colname, syserr, loctype)
char		*colname;
CL_ERR_DESC	*syserr;
i4		loctype;
{
    STATUS	status;
    LOCATION	loc;
    CMCOLDFLE   *cmcoldle = get_coldfle(1);

    CL_CLEAR_ERR( syserr );

# ifndef DESKTOP
    if (cer_issem(&cmcoldle->sems))
    {
	(*cmcoldle->sems->er_p_semaphore)(TRUE, &cmcoldle->sems->er_sem);
    }
# endif /* !DESKTOP */

    setuplocation(colname, loctype, &loc);

    if ((status = SIopen(&loc,
# ifdef DESKTOP
			"rb",
# else
			"r",
# endif
			&(cmcoldle->f))) != OK)
    {
	SETCLERR(syserr, ER_open, 0);
# ifndef DESKTOP
	if (cmcoldle->sems)
	{
	    (*cmcoldle->sems->er_v_semaphore)(&cmcoldle->sems->er_sem);
	}
# endif /* !DESKTOP */
	return status;
    }
    return OK;
}

/*{
** Name: CMread_col - read from collation file
**
** Description:
**	read a COL_BLOCK size record from open collation file
**
** Inputs:
**	bufp				pointer to buffer
**
** Outputs:
**	syserr				OS specific error
**
**	Returns:
**	    E_DB_OK
**	    FAIL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	moves open file to next record
**
** History:
**      16-Jun-89 (anton)
**          Created.
**      03-Jun-2009 (whiro01)
**          Calling SIread for all platforms to support the
**          OpenROAD FUSED DLL runtime where these tables are
**          not in an external file but in internal resources;
**          simplified the logic using "get_coldfle" helper.
*/
STATUS
CMread_col(bufp, syserr)
char		*bufp;
CL_ERR_DESC	*syserr;
{
    CMCOLDFLE   *cmcoldle = get_coldfle(0);
    i4          actual_count;

    if (SIread(cmcoldle->f, COL_BLOCK, &actual_count, bufp) != OK ||
        actual_count != COL_BLOCK)
    {
	SETCLERR(syserr, ER_read, 0);
	return FAIL;
    }
    return OK;
}

/*{
** Name: CMclose_col - close collation file
**
** Description:
**	close an open collation file
**
** Inputs:
**	none
**
** Outputs:
**	syserr				OS specific error
**
**	Returns:
**	    E_DB_OK
**	    FAIL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	Releases semaphore which may have been acquired durring CMopen_col
**
** History:
**      16-Jun-89 (anton)
**          Created.
**      24-Fep-2000 (consi01) Bug 100051 INGSRV 1114
**          Changed close() to fclose() if DESKTOP is defined as file
**          was opened with fopen(), not open()
**	14-Jan-2003 (gupsh01)
**	    Added the file location type in input parameter, it can now 
**	    be CM_COLLATION_LOC for $II_SYSTEM/ingres/files/collation or 
**	    CM_UCHARMAPS_LOC for $II_SYSTEM/ingres/filese/ucharmaps 
**	    for unicode and local encoding mapping files.
**	03-Jun-2009 (whiro01)
**	    Calling SIclose for all platforms (for the OpenROAD FUSEDLL case
**	    where the collation file will be in a DLL resource rather than
**	    an external file); simplified logic using "get_coldfle" helper.
*/
STATUS
CMclose_col(syserr, loctype)
CL_ERR_DESC *syserr;
i4	loctype;
{
    STATUS      status;
    CMCOLDFLE   *cmcoldle = get_coldfle(0);

    if ((status = SIclose(cmcoldle->f)) != OK)
    {
	SETCLERR(syserr, ER_close, 0);
    }
# ifndef DESKTOP
    if (cmcoldle->sems)
    {
	(*cmcoldle->sems->er_v_semaphore)(&cmcoldle->sems->er_sem);
    }
# endif /* !DESKTOP */

    return status;
}

/*{
** Name: CMdump_col - dump the in-memory collation table to the file
**
** Description:
**	write out the in-memory collation table to the external file
**
** Inputs:
**	colname				collation / target file name
**	tablep				collation table pointer
**	tablen				collation table length
**	loctype				type of location, collation 
**					or charmap	
**
** Outputs:
**	syserr				OS specific error
**
**	Returns:
**	    E_DB_OK
**	    FAIL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**      16-Jun-89 (anton)
**          Created.
**	14-Jan-2003 (gupsh01)
**	    Added the file location type in input parameter, it can now 
**	    be CM_COLLATION_LOC for $II_SYSTEM/ingres/files/collation or 
**	    CM_UCHARMAPS_LOC for $II_SYSTEM/ingres/filese/ucharmaps 
**	    for unicode and local encoding mapping files.
**      07-Mar-2005 (hanje04)
**          SIR 114034
**          Add support for reverse hybrid builds. First part of much wider
**          change to allow support of 32bit clients on pure 64bit platforms.
**          Changes made here allow application statically linked against
**          the standard 32bit Intel Linux release to run against the new
**          reverse hybrid ports without relinking.
**     08-Jun-2009 (whiro01)
**          Simplified this since it doesn't need the CMCOLDFLE structure
**          just to dump the file; calling new function "setuplocation"
**          to do the grunt work of naming the output file.
*/
STATUS
CMdump_col(colname, tablep, tablen, syserr, loctype)
char		*colname;
PTR		tablep;
i4		tablen;
CL_ERR_DESC	*syserr;
i4		loctype;
{
    LOCATION	loc;
    char	*chrp;
# ifdef DESKTOP
    FILE	*fp;
# else
    int		desc;
# endif

    setuplocation(colname, loctype, &loc);
    LOtos(&loc, &chrp);

# ifdef DESKTOP
    if ((fp = fopen(chrp, "wb")) == NULL)
    {
        SETCLERR(syserr, 0,0);
        return(FAIL);
    }

    if ((i4) fwrite((char *) tablep, 1, (size_t) tablen, fp) != tablen)
    {
        SETCLERR(syserr, 0,0);
        return(FAIL);
    }

    if (fclose(fp) != 0)
    {
        SETCLERR(syserr, 0,0);
        return(FAIL);
    }
# else /* !DESKTOP */
    if ((desc = creat(chrp, 0777)) == -1)
    {
	SETCLERR(syserr, ER_open, 1);
	perror(chrp);
	return FAIL;
    }
    write(desc, (char *)tablep, tablen);
    close(desc);
# endif /* DESKTOP */

    return(OK);
}
