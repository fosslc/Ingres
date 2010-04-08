/*
** Copyright (c) 1987, 2010 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<er.h>
# include	<gc.h>
# include	<gcccl.h>
# include	<id.h>
# include	<lo.h>
# include	<nm.h>
# include	<pc.h>
# include	<cm.h>
# include	<si.h>
# include	<me.h>
# include	<meprivate.h>
# include	<st.h>
# include	"nmlocal.h"
# if defined(su4_cmw)
# include	<sys/label.h>
# include	<errno.h>
# endif

/*
NO_OPTIM = sgi_us5
*/
/**
** Name: NMFILES.C - Functions used to implement the nm file manipulations:
**
** Description:
**      This file contains the following external routines:
**    
**      NMloc() 	       -  Return location associated with input value
**      NMf_open() 	       -  Open location associated with file in the
**				  files directory.
**      NMt_topen() 	       -  Open location associated with file in the
**				  temp location.
**	NMlogOperation()       -  Log a symbol table update action.
**
**      This file contains the following internal routines:
**
**      NMopensyms()	       -  Open table of symbols.
**      NMaddsym()	       -  add symbol.
**      NMreadsyms()	       -  read file with symbols.
**      NMgtIngAt()            -  Get an ingres symbol.
**      NMlocksyms()           -  Lock the symbol.lck file for 
**				  updating by this process.
**      NMunlocksyms()         -  Unlock the symbol.lck file.
**
** History: Log:	nmfiles.c,v
 * Revision 1.3  88/12/12  17:23:21  billc
 * bugfix -- don't cache open failure except for reads.
 * 
 * Revision 1.2  88/11/30  13:41:26  billc
 * if opensym fails, don't keep trying.
 * 
 * Revision 1.1  88/08/05  13:43:12  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
** Revision 1.5  88/07/01  09:39:46  mikem
** handle UTILITY case correctly on UNIX.
**      
** Revision 1.4  88/01/07  13:15:39  mikem
** use new environment variables.
**      
** Revision 1.3  87/11/13  11:33:49  mikem
** use nmlocal.h rather than "nm.h" to disambiguate the global nm.h header from
** the local nm.h header.
**      
** Revision 1.2  87/11/10  14:44:29  mikem
** Initial integration of 6.0 changes into 50.04 cl to create 
** 6.0 baseline cl
**      
**	12-Mar-89 (GordonW)
**	    added call to LOlast to update symbol table modification time and
**	    added NMflushIng() routine to flush symbol cache.
**      26-aug-89 (rexl)
**          Added calls to protect stream allocator.
**	16-feb-90 (greg)
**	    NMgtAt() is a VOID not STATUS.
**	27-Mar-90 (seiwald)
**	    Added support for II_ADMIN, the environment variable which 
**	    points to the ADMIN location, which houses (at least) the
**	    symbol table file.   If unset, II_ADMIN defaults to the
**	    directory $II_SYSTEM/ingres/admin/<hostname>, if present,
**	    and then to $II_SYSTEM/ingres/files.  Specific changes are:
**	
**	    o	Introduced NM_initsym(), to determine II_SYSTEM and II_ADMIN.
**	    o	Restructured NMopensyms() to use the ADMIN directory.
**	    o	Restructured NMloc() to understand the ADMIN location.
**	    o	Removed NMsyspath(); NM_initsym() does its job.
**	    o	Collected some statics into central structure.
**	    o	Removed SEINGRES references.
**      30-May-90 (seiwald)
**          Fixed SUBDIR case broken by above change.
**    11-aug-93 (ed)
**      add missing include
**    14-oct-94 (nanpr01)
**      Removed #include "nmerr.h". Bug # 63157.
**    19-jun-95 (emmag)
**       Desktop porting changes - directory separation, and changed call
**	 to gethostname to GetComputerName.
**    06-jun-1995 (canor01)
**      Semaphore protect memory allocation routines
**      12-jun-1995 (canor01)
**          semaphore protect NMloc()'s static buffer (MCT)
**	22-aug-95 (canor01)
**	    NMopensyms(), NMreadsyms() semaphore protected (MCT)
**     8-aug-95 (allst01)
**	Fixed the truncated symbol.tbl for su4_cmw ONLY.
**	A generic fix for both secure and normal systems
**	may be very difficult. See the later discussion.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	27-mar-96 (thoda04)
**	    In NMloc(), pass copy of dname to LOfroms, else LOfroms() 
**	    may try to change the constant string.
**	03-jun-1996 (canor01)
**	    Clean up semaphore usage for use with operating-system threads.
**	    Use local semaphore to protect static structure.
**	    Make NMloc()'s static buffer thread-specific.
**	23-sep-1996 (canor01)
**	    Move global data definitions to nmdata.c.
**	10-oct-1996 (canor01)
**	    Allow generic system parameter overrides.
**      22-nov-1996 (canor01)
**          Changed names of CL-specific thread-local storage functions
**          from "MEtls..." to "ME_tls..." to avoid conflict with new
**          functions in GL.  Included <meprivate.h> for function prototypes.
**      07-mar-1997 (canor01)
**          Allow functions to be called from programs not linked with
**          threading libraries.
**	13-mar-1996 (harpa06)
**	    If II_SYSTEM cannot be obtained by the environment, get rid of the
**	    semaphore required to read the symbol table in NM_initsym().
**      15-jan-1996 (reijo01)
**          Allow generic system parameter overrides on symbol table access.
**	29-may-1997 (canor01)
**	    Clean up some compiler warnings.
**	09-jul-1998 (kosma01)
**	    Add no optim for sgi_us5 IRIX64 6.4.
**      13-Oct-1998 (yeema01)
**          Added LOG type in NMloc for OpenRoad 4.0
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-jan-2004 (somsa01)
**	    In NM_initsym(), added initialization of NMBakSymloc, NMLogSymloc
**	    and NMLckSymloc. In NMopensyms(), do not create the symbol.tbl on
**	    a failed read since will be created properly when we have
**	    something to add to it. In NMreadsyms(), treat an empty symbol.tbl
**	    file as a fatal error. In NMgtIngAt(), make sure symbol.tbl exists
**	    before testing for its modification. In NMflushIng(), reset
**	    ReadFailed so that a proper read can be performed again. Also
**	    added NMlogOperation(), which will log update operations on the
**	    symbol table file to symbol.log.
**	03-feb-2004 (somsa01)
**	    Backed out last changes for now.
**	29-mar-2004 (somsa01)
**	    In NM_initsym(), added initialization of NMBakSymloc and
**	    NMLogSymloc. Also, added NMlogOperation(), which will log update
**	    operations on the symbol table file to symbol.log.
**	13-May-2004 (hanje04)
**	    BUG 112309
**	    When using $II_SYSTEM/ingres/admin/<hostname> as the location for
**	    symbol.tbl (as in the clustered case) the hostname needs to be
**   	    of the form returned by PMhost(). Unfortunately PMhost() calls 
**	    GChostname() which in turn calls NM again to try to resolve 
**  	    II_HOSTNAME and we end up with an infinite recursion. 
**	    Add NMhostname() to get the local hostname in PMhost() format but
**	    without calling GChostname(). 
**	    Replace system call gethostname() in NMinitsym() with NMhostname() 
**	    so that '.' in a fully qualified hostname is correctly replaced 
**	    with '_'.
**	15-Jun-2004 (schka24)
**	    Safer env variable handling.
**	31-Aug-2004 (jenjo02)
**	    Added symbol.lck file to serialize cross-process 
**	    updates to the symbol file.
**      11-apr-2008 (stial01)
**          NMloc() is incorrectly assuming 0 is invalid tls key
**      01-may-2008 (stial01)
**          NMloc() After calling ME_tls_createkey, check status NOT key value
**	16-Feb-2010 (hanje04)
**	    SIR 123296
**	    Add conf_LSB_BUILD option for LSB compliant builds. This redefines
**	    the locations for files depending on their type:
**		SYS:	Fix II_SYSTEM for LSB builds
**		FILES:	Static configuration and message files (II_CONFIG)
**		ADMIN:	Dynamic configuration files (config.dat symbol.tbl)
**			Any files opened for writing or created by the server
**			or other processes now live in the this location.
**			II_ADMIN
**		UTILITY &
**		LOG:	Any and all .log files (errlog.log etc.)
**		LIB:	Libraries
**	24-Mar-2010 (hanje04)
**	    SIR 123296
**	    Add explicit case for FILE for LSB builds and use the generic 
**	    default. Other wise SUBDIR returns values under $II_CONFIG and
**	    not $II_SYSTEM
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
 
/* Globals */
GLOBALREF SYM		*s_list;		/* list of symbols */
GLOBALREF SYSTIME	NMtime;			/* modification time */
GLOBALREF LOCATION	NMSymloc;		/* location of symbol.tbl */
GLOBALREF II_NM_STATIC  NM_static;
GLOBALREF LOCATION	NMBakSymloc;		/* location of symbol.bak */
GLOBALREF LOCATION	NMLogSymloc;		/* location of symbol.log */
GLOBALREF LOCATION	NMLckSymloc;		/* location of symbol.lck */

/* statics */

static char	NMSymbuf[ MAX_LOC + 1 ];	/* buffer for Symloc */
static char	NMBakSymbuf[MAX_LOC + 1];	/* buffer for BakSymloc */
static char	NMLogSymbuf[MAX_LOC + 1];	/* buffer for LogSymloc */
static char	NMLckSymbuf[MAX_LOC + 1];	/* buffer for LckSymloc */

static void	NMhostname( char *, int );

#if defined(su4_cmw)
char		*NM_path;
bclabel_t	NM_saved_label;
int		NM_got_label;
/* These variables are used to ensure that symbol.tbl's
 * MAC label does not float upwards
 */
#endif

# ifdef OS_THREADS_USED
i4			NMtlsinit = 0;
static ME_TLS_KEY	NMlockey = -1;
# endif


/*{
** Name: NM_initsym - initialize known, frequently requested locations
**       (The function call must be semaphore protected for MCT).
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	OK
**	status
**
** History:
**	28-Mar-90 (seiwald)
**	    Written.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**  	13-mar-1996 (harpa06)
**	    If II_SYSTEM cannot be obtained by the environment, get rid of the
**	    semaphore required to read the symbol table.
**	29-sep-2000 (devjo01)
**	    Make sure user does not pass a value of II_ADMIN that will
**	    overflow 'Admbuf' (b102777).
**	02-oct-2000 (devjo01)
**	    Put in similar check for II_SYSTEM (b102788).
**	29-mar-2004 (somsa01)
**	    Added initialization of NMBakSymloc and NMLogSymloc.
**	13-Aug-2004 (jenjo02)
**	    Added init of NMLckSymloc.
**	06-Aug-2007 (smeke01) b118877
**	    Set NM_static.Init = FALSE for all error return paths.
*/

STATUS
NM_initsym()
{
	char	*sym;
	STATUS	status;
	bool	found;

	MUw_semaphore( &NM_static.Sem, "NM local sem" );
	MUp_semaphore( &NM_static.Sem );

	NM_static.Init = TRUE;
	NM_static.SymIsLocked = FALSE;

	/*
	** Get II_SYSTEM from the environment.
	*/

	if( !( sym = getenv( SystemLocationVariable ) ) || !*sym )
	{
# ifdef conf_LSB_BUILD
	    /* for LSB Builds use /usr/libexec when II_SYSTEM isn't set */
            sym = NM_LSB_SYSDIR;
# else
	    NM_static.Init = FALSE;
	    MUv_semaphore( &NM_static.Sem );            
	    return NM_INGUSR;
# endif
	}
	if ( STlength(sym) + 1 + STlength(SystemLocationSubdirectory)
		 > MAX_LOC )
	{
	    NM_static.Init = FALSE;
	    MUv_semaphore( &NM_static.Sem );
	    return NM_TOOLONG;
	}

	STcopy( sym, NM_static.Sysbuf );
	LOfroms( PATH, NM_static.Sysbuf, &NM_static.Sysloc );
	LOfaddpath( &NM_static.Sysloc, 
		    SystemLocationSubdirectory,
		    &NM_static.Sysloc );

	/*
	** Get II_ADMIN, from one of:
	**	the environment 			(if set)
	**	II_SYSTEM/ingres/admin/<hostname> 	(if present)
	**	II_SYSTEM/ingres/files 			(otherwise)
	*/

	found = FALSE;

	/* Try II_ADMIN */

	if( ( ( sym = getenv( "II_ADMIN" ) ) && *sym )
# ifndef conf_LSB_BUILD
	    || ( ( sym = getenv( "II_CONFIG" ) ) && *sym )
# endif
	    )
	{
	    if ( STlength(sym) > MAX_LOC )
	    {
		NM_static.Init = FALSE;
		MUv_semaphore( &NM_static.Sem );
		return NM_TOOLONG;
	    }
	    STcopy( sym, NM_static.Admbuf );
	    LOfroms( PATH, NM_static.Admbuf, &NM_static.Admloc );
	    found = TRUE;
	} 

	/* Try $II_SYSTEM/ingres/admin/<hostname> */

	if( !found )
	{
	    /* IDhost() anyone? */
	    char buf[ 32 ];
	    i2 lojunk;

# ifdef NT_GENERIC
            DWORD dwMaxNameSize = MAX_COMPUTERNAME_LENGTH + 1;
            GetComputerName (buf, &dwMaxNameSize);
            STpolycat( 3, NM_static.Sysbuf, "\\admin\\",buf,NM_static.Admbuf );
# else
	    NMhostname( buf, sizeof( buf ) );
	    STpolycat( 3, NM_static.Sysbuf, "/admin/", buf, NM_static.Admbuf );
# endif
	    LOfroms( PATH, NM_static.Admbuf, &NM_static.Admloc );
	    found = LOisdir( &NM_static.Admloc, &lojunk ) == OK;
	}

	/* Use $II_SYSTEM/ingres/files */

	if( !found )
	{
# ifdef NT_GENERIC
	    STpolycat( 2, NM_static.Sysbuf, "\\files", NM_static.Admbuf );
# elif defined(conf_LSB_BUILD)
	    STcopy(NM_LSB_ADMDIR, NM_static.Admbuf);
# else
	    STpolycat( 2, NM_static.Sysbuf, "/files", NM_static.Admbuf );
# endif
	    LOfroms( PATH, NM_static.Admbuf, &NM_static.Admloc );
	}

	/*
	** Build symbol.tbl path.
	*/

	LOcopy( &NM_static.Admloc, NMSymbuf, &NMSymloc );
	(void)LOfstfile( "symbol.tbl", &NMSymloc );

	/*
	** Build symbol.bak path. This file will be stored in the same
	** location as the symbol.tbl file.
	*/

	STcopy(NM_static.Admbuf, NM_static.Bakbuf);
	LOfroms(PATH, NM_static.Bakbuf, &NM_static.Bakloc);
	LOcopy(&NM_static.Bakloc, NMBakSymbuf, &NMBakSymloc);
	LOfstfile("symbol.bak", &NMBakSymloc);

	/*
	** Build symbol.log path. This file will be stored in the same
	** location as the symbol.tbl file.
	*/

	STcopy(NM_static.Admbuf, NM_static.Logbuf);
	LOfroms(PATH, NM_static.Logbuf, &NM_static.Logloc);
	LOcopy(&NM_static.Logloc, NMLogSymbuf, &NMLogSymloc);
	LOfstfile("symbol.log", &NMLogSymloc);

	/*
	** Build symbol.lck path. This file will be stored in the same
	** location as the symbol.tbl file.
	*/

	STcopy(NM_static.Admbuf, NM_static.Lckbuf);
	LOfroms(PATH, NM_static.Lckbuf, &NM_static.Lckloc);
	LOcopy(&NM_static.Lckloc, NMLckSymbuf, &NMLckSymloc);
	LOfstfile("symbol.lck", &NMLckSymloc);

# ifdef conf_LSB_BUILD
	/*
  	** For an LSB install, logs are written to /var/log/ingres,
  	** static config is written to /usr/share/ingres/files,
	** libraries are under /usr/lib/ingres
  	*/
	/* II_LOG */
	STcopy(NM_LSB_LOGDIR, NM_static.IILogbuf);
	LOfroms(PATH, NM_static.IILogbuf, &NM_static.IILogloc);

	/* II_CONFIG (static) */
	STcopy(NM_LSB_CFGDIR, NM_static.Cfgbuf);
	LOfroms(PATH, NM_static.Cfgbuf, &NM_static.Cfgloc);

	/* LIB dir */
	STcopy(NM_LSB_LIBDIR, NM_static.Libbuf);
	LOfroms(PATH, NM_static.Libbuf, &NM_static.Libloc);

       /* UBIN dir */
	STcopy(NM_LSB_UBINDIR, NM_static.Ubinbuf);
	LOfroms(PATH, NM_static.Ubinbuf, &NM_static.Ubinloc);
# endif
	
	/*
	** Done.
	*/

	MUv_semaphore( &NM_static.Sem );
	return OK;
}


/*{
** Name: NMloc - get a location to a given area
**
** Description:
**    set 'ploc1' to point to LOCATION whose buffer contains the
**		path to 'fname' (which depending on the value of
**		'what' is some combination of FILENAME, PATH, DEV)
**		and which is located in a sub-directory of the home
**		directory of the INGRES superuser (ING_HOME). 'which'
**		is one of the following:
**			BIN	 for executables
**			FILES	 for error files
**			LIB	 for INGRES libraries
**			TEMP	 for temporary files
**			DBTMPLT	 database template directory
**			DBDBTMPLT dbdb database template directory
**			UTILITY	 for the users file, *.bin
**			RECOVER  for qbfrecover files
**
**	NOTES:
**		UTILITY handles files that ought really be written
**			by the backend, but can't be for various
**			reasons.  They are currently "users," 
**			"acc.bin", "ext.bin" and "loc.bin", used
**			by accessdb and by finddbs.
**			If ING_UTILITY is not set, we use ING_FILES.
**			
**		TEMP is a special case in that if ING_TEMP is not
**			defined the path chosen is the directory
**			the process is running in.
**
**		RECOVER is a special case like TEMP that uses
**			the current directory if ING_RECOVER is
**			not set.  RECOVER is used only for ingres/CS.
**
**		As a static buffer is associated with 'ploc1' it may
**			be advisable to do an LOcopy() upon return.
**			The interface should (obviously) be changed
**			to require the user to pass in the required
**			buffer.
**
**	returns OK on success.
**
** Inputs:
**	which				Constant indicating what directory
**	what				type of location.
**	fname				file in the specified location
**
** Output:
**	ploc1				Resulting location.
**
**      Returns:
**          OK                         	    Function completed normally. 
**          FAIL                            Function completed abnormally.
**
** History:
**	09/85	- (rogerk) added RECOVER as a valid directory.
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards.
**      13-Oct-98 (yeema01)
**          Added LOG type for OpenRoad 4.0
**      16-Feb-2010 (hanje04)
**          SIR 123296
**          Add cond_LSB_BUILD option for LSB compliant builds. This redefines
**          the locations for files depending on their type:
**              SYS:    Fix II_SYSTEM for LSB builds
**              FILES:  Static configuration and message files (II_CONFIG)
**              ADMIN:  Dynamic configuration files (config.dat symbol.tbl)
**                      Any files opened for writing or created by the server
**                      or other processes now live in the this location.
**                      II_ADMIN
**              LOG:    Any and all .log files (errlog.log etc.)
**              LIB:    Libraries
**	24-Mar-2010 (hanje04)
**	    SIR 123296
**	    Add explicit case for FILE for LSB builds and use the generic 
**	    default. Other wise SUBDIR returns values under $II_CONFIG and
**	    not $II_SYSTEM
*/
STATUS
NMloc(
	char		which,
	LOCTYPE		what,
	char		*fname,
	LOCATION	*ploc1)
{
	STATUS		CLerror = OK;
	char		*symbol = NULL;
	char		*dname = NULL;
	bool		found	= FALSE;
	LOCATION	tmploc;
	char		dnamebuf[MAX_LOC];
	char		*buffer;
	STATUS		local_status;
 
	if ( !NM_static.Init )
	{
	    if( ( CLerror = NM_initsym() ) != OK )
	    {
		return CLerror;
	    }
	}

# ifdef OS_THREADS_USED
        if ( NMtlsinit == 0 )
        {
	    NMtlsinit = 1;
            ME_tls_createkey( &NMlockey, &local_status );
	    if (local_status == 0)
		ME_tls_set( NMlockey, NULL, &local_status );
	    if (local_status)
	    {
		/* not linked with threading library */
		NMtlsinit = -1;
	    }
        }
	if ( NMtlsinit != 1 )
	{
	    buffer = NM_static.Locbuf;
	}
	else
	{
            ME_tls_get( NMlockey, (PTR *)&buffer, &local_status );
            if ( buffer == NULL )
            {
                buffer = MEreqmem(0, MAX_LOC+1, TRUE, NULL );
                ME_tls_set( NMlockey, (PTR)buffer, &local_status );
            }
	}
# else /* OS_THREADS_USED */
        buffer = NM_static.Locbuf;
# endif /* OS_THREADS_USED */

	switch (which)
	{
	case BIN:
		symbol = "II_BINARY";
		dname = "bin";
		break;

	case DBTMPLT:
		symbol = "II_TEMPLATE";
		dname = "dbtmplt";
		break;

	case DBDBTMPLT:
		symbol = "ING_DBDBTMP";
		dname = "dbdbtmplt";
		break;

# ifndef conf_LSB_BUILD
	case UTILITY:
# endif
	case FILES:
		symbol = "II_CONFIG";
		dname = "files";
		break;

	case LIB:
		symbol = "II_LIBRARY";
		dname = "lib";
		break;

	case SUBDIR:
		if ( !(what & PATH) )
		    return NM_LOC;
		dname = fname;
		fname = NULL;
		break;

	case TEMP:
		symbol = "II_TEMPORARY";
		break;

	case ADMIN:
		break;

        case LOG:
# ifdef conf_LSB_BUILD
	case UTILITY:
# else
		symbol = "II_LOG";
		dname   = "files";
# endif
		break;
# ifdef conf_LSB_BUILD
	case UBIN:
		break;
# endif


	default:
		return NM_LOC;
		break;
	}
 
	/*
	** Look at the user's local environment first.
	*/

	if( symbol )
	{
		char	*ptr;
 
		NMgtAt( symbol, &ptr );

		if( ptr && *ptr )
		{
			STlcopy( ptr, buffer, MAX_LOC );
			LOfroms( PATH, buffer, ploc1 );
			found = TRUE;
		}
	}
		
	/*
	** If symbol not found in environment, use a default.
	*/

	if( !found )
	    switch( which )
	{
	case TEMP:
		/* Use current directory */
		if( ( CLerror = LOgt( buffer, ploc1 ) ) != OK )
		{
			return CLerror;
		}
		break;
	case ADMIN:
		/* Use ADMIN location found by NM_initsym() */
		STcopy( NM_static.Admbuf, buffer );
		LOfroms( PATH, buffer, ploc1 );
		break;
# ifdef conf_LSB_BUILD
	case BIN:
		/* Use directory $II_SYSTEM/ingres/dname */
		STcopy( NM_static.Sysbuf, buffer );
		LOfroms( PATH, buffer, ploc1 );
		STcopy( dname, dnamebuf );
		LOfroms( PATH, dnamebuf, &tmploc );
		LOaddpath( ploc1, &tmploc, ploc1 );
		break;
	case LIB:
		STcopy( NM_static.Libbuf, buffer );
		LOfroms(PATH, buffer, ploc1);
		break;
	case UTILITY:
	case LOG:
		STcopy( NM_static.IILogbuf, buffer );
		LOfroms(PATH, buffer, ploc1);
		break;
	case UBIN:
		/* /usr/bin */
		STcopy( NM_static.Ubinbuf, buffer );
		LOfroms(PATH, buffer, ploc1);
		break;
	case FILES:
		/* /usr/share/ingres/files */
		STcopy( NM_static.Cfgbuf, buffer );
		LOfroms( PATH, buffer, ploc1 );
		break;
# endif
	default:
		/* Use directory $II_SYSTEM/ingres/dname */
		STcopy( NM_static.Sysbuf, buffer );
		LOfroms( PATH, buffer, ploc1 );
		STcopy( dname, dnamebuf );
		LOfroms( PATH, dnamebuf, &tmploc );
		LOaddpath( ploc1, &tmploc, ploc1 );
		break;
	}
		
	if( fname != NULL )
	{
		if( what == FILENAME )
		{
			LOfstfile( fname, ploc1 );
		}
		else if( what & PATH )
		{
			LOfroms( what, fname, &tmploc );
			LOaddpath( ploc1, &tmploc, ploc1 );
		}
	}
 
	return OK;
}


/*{
** Name: NMf_open - open a file in the files directory
**
** Description:
**    open 'fname' which is in the directory ~ingres/files with
**    'mode' of "r", "w" or "rw".  Set 'fptr' to the file
**    pointer of the opened file.
**
**    return OK on success.
**
** Inputs:
**	mode				    mode to open the file in.
**	fname				    name of file to open
**
** Output:
**	fptr				    On success FILE ptr to open file.
**                                          E_DM0000_OK                
**
**      Returns:
**          OK                         	    Function completed normally. 
**          FAIL                            Function completed abnormally.
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards.
*/
STATUS
NMf_open(mode, fname, fptr)
char	*mode;
char	*fname;
FILE	**fptr;
{
	STATUS		rval;
	LOCATION	loc;
 
	rval = NMloc(FILES, FILENAME, fname, &loc);
	if ( rval == OK )
	    rval = SIopen(&loc, mode, fptr);
	return rval;
}
 

/*{
** Name: NMt_open - Create and open temporary file.
**
** Description:
**    create and open with 'mode', a uniquly named file (prefix = 'fname'
**    and suffix = 'suffix') in the ingres temp directory.
**    set 'fptr' to be FILE * associated with 'fname' and fill in 't_loc'
**    so it may be used in subsequent call to LOdelete().
**    
**    As a static buffer is associated with 't_loc' it may
**    be advisable to do an LOcopy() upon return.
**
** Inputs:
**	mode				    mode to open the file in.
**	fname				    prefix of filename
**	suffix				    suffix of filename
**
** Output:
**	t_loc				    resulting location.
**	fptr				    resultion file pointer to open file.
**                                          E_DM0000_OK                
**
**      Returns:
**          OK                         	    Function completed normally. 
**          FAIL                            Function completed abnormally.
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards.
*/
STATUS
NMt_open(mode, fname, suffix, t_loc, fptr)
char			*mode;
char			*fname;
char			*suffix;
LOCATION		*t_loc;
FILE			**fptr;
{
	LOCATION	tmploc;
	char		tmpbuf[ MAX_LOC + 1 ];
	STATUS		rval;
 
	/*
	This is using a side effect of LOuniq() to fill in tmpbuf.
	*/
 
	tmpbuf[ 0 ] = '\0';
	LOfroms(PATH & FILENAME, tmpbuf, &tmploc);
	LOuniq(fname, suffix, &tmploc);
 
	rval = NMloc(TEMP, FILENAME, tmpbuf, t_loc);
	if ( rval == OK )
		rval = LOcreate(t_loc);
	if ( rval == OK )
		rval = SIopen(t_loc, mode, fptr);
	return( rval );
}

/*{
** Name: NMopensyms - Open table of symbols.
**
** Description:
**    Open the symbol table file in the specified mode, either "r" or "w".
**
**    Returns either a file descriptor or NULL.
**
** Inputs:
**	mode				    mode to open the table in.
**
** Output:
**	none
**
**      Returns:
**	    File pointer to opened file on success.
**	    NULL on failure.
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards.
**	14-Mar-90 (seiwald)
**	    Added new symbol II_ADMIN to point to directory of symbol
**	    table.  Default remains $II_SYSTEM/ingres/files.
**	    This is for (future) client installation support.
**	25-mar-90 (greg)
**	    PEsave/PEreset are both VOIDs don't check return status
**      29-sep-94 (cwaldman) Cross Integ 64 to 11 (ramra01) 7-Dec-94
**          Changed to return (FILE *)NULL if symbol.tbl can't be
**          opened (instead of opening a new file). This is part of
**          the fix for bug 44445 (symbol.tbl disappears).
**      27-feb-95 (cwaldman)
**          The above change caused another bug (#67049). Undid change.
**	 8-aug-95 (allst01)
**	    Fixed the symbol.tbl bug for su4_cmw
*/

FILE *
NMopensyms( mode )
char *mode;
{
	FILE	*fp = NULL;
 
	/* Can't use NMloc() or could get into recursive infinite loop. */
 
	if ( !NM_static.Init )
	{
	    if( NM_initsym() != OK )
	    {
		return (FILE *)NULL;
	    }
	}

	/* Try to read once.  Don't keep trying if we failed already. */

	if( NM_static.ReadFailed && *mode == 'r' )
	{
		return (FILE *)NULL;
	}

	/*
	** Set fp to NULL if can't open existing file and
	**	  can't create new file and open it
	**
	** BUT only for write case ! For the read
	** case more checking must be done !
	**
	** If there is any error from the first SIopen
	** for the read case then the code zonks the
	** file, even if it is there. The errno must
	** be carefully checked. For CMW the caller
	** may be bounced if he is at a different
	** processs label, and in this case the code
	** deletes the existing file, since it can
	** write to the directory ! This leaves
	** a truncated "symbol.tbl".
	** Unfortunately there isn't a simple fix
	** since secure systems may return different
	** error values. A true B1 system may deny
	** the existance of the file even if it does
	** in fact exist. The only true way to be
	** certain is to look in the directory to see
	** if an entry exists. LOinfo and LOexists
	** cannot be used because they use stat() and
	** don't check in the directory. A file may
	** exist and yet be inaccessible on a secure
	** system. Using stat() will give an error
	** the same as open() in this case.
	**
	** I have applied a fix for CMW.
	** This is still broken generically because any error
	** from SIopen which isn't a "file does not exist"
	** error will cause the file to be zapped.
	** The fix may be generic for secure systems, but
	** this remains to be seen. It really needs to
	** be tighter to be airtight.
	**
	** The real generic fix must involve checking the
	** directory to see if it thinks that the file
	** exists. If it does then it may be assumed
	** that access to the file is blocked by
	** security. Unfortunately the CL does not
	** support this operation. Perhaps LOexists()
	** can be resurected and rewritten to look in
	** the directory so a generic fix for all systems
	** can be written ?
	*/

#if defined(su4_cmw)
	LOtos(&NMSymloc, &NM_path);
	NM_got_label = (getcmwlabel(NM_path, &NM_saved_label) == 0);
#endif
	if ( SIopen( &NMSymloc, mode, &fp ) != OK )
	{
#if defined(su4_cmw)
		if (*mode=='r' && (errno!=EACCES && errno!=EPERM))
#endif
		{
			PEsave();
			fp = NULL;

			if ( PEumask("rw-r--") == OK &&
			     LOcreate(&NMSymloc) == OK)
			{
			     PEreset();
			     if ( SIopen( &NMSymloc, mode, &fp ) != OK )
				fp = NULL;
			}
	        }		 
#if defined(su4_cmw)
		else
		{
			/* force a read error */
			fp=NULL;
		}
#endif

	}		  


	NM_static.ReadFailed = (*mode == 'r' && fp == NULL) ? TRUE : FALSE;
	return (fp);
}

/*{
** Name: NMaddsym - Append a symbol to a chain.
**
** Description:
**    Append a symbol to a chain.
**
**    Returns status, failure meaning we couldn't allocate memory.
**
** Inputs:
**	name				    name of symbol.
**	value				    value of symbol.
**	endsp				    end of chain.
**
** Output:
**	none.
**
**      Returns:
**          OK                         	    Function completed normally. 
**          FAIL                            Function completed abnormally.
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards.
*/
STATUS
NMaddsym( name, value, endsp )
register char 	*name;
register char	*value;
register SYM	*endsp;
{
	auto SYM	*sp;
	register i4	status = FAIL;
 
	/* allocate and save the new symbol, then link into chain */
 
        sp = (SYM *) MEreqmem(0, sizeof(*sp), FALSE, (STATUS *) NULL);
	if ((NULL != sp) &&
	     NULL != (sp->s_sym = STalloc(name) ) &&
	     NULL != (sp->s_val = STalloc(value) ) ) 
	{
		if( endsp != NULL )
			endsp->s_next = sp;
		else
		{
			s_list = sp;
		}

		sp->s_last = endsp;
		sp->s_next = NULL;
		status = OK;
	}
 
	return ( status );
}

/*{
** Name: NMreadsyms - read the symbol table.
**
** Description:
**    Read the symbol table into a linked list of SYMs.
**
**    This should only be called once.
**
** Inputs:
**	none.
**
** Output:
**	none.
**
**      Returns:
**          OK                         	    Function completed normally. 
**          FAIL                            Function completed abnormally.
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards.
**      29-sep-94 (cwaldman) Cross Integ from 6.4 (ramra01) 7-Dec-94
**          Changed check of NMopensyms return value to compare against
**          (FILE *)NULL (was NULL).
**	17-jan-2001 (crido01)
**	    Don't whine about a trashed symbol table if FUSEDLL is defined.
**	06-apr-2004 (somsa01)
**	    If symbol.tbl is trashed, return NM_STCORRUPT.
*/
STATUS
NMreadsyms()
{
	register SYM	*sp = NULL;
	register FILE	*fp;
	register char	*bufr;
	register i4	status = OK;
 
	char	buf[ MAXLINE + NULL_NL_NULL ];
 
        if ((FILE *)NULL == (fp = NMopensyms( "r" )))
		return (NM_STOPN);
 
	/* Read each line in the file and add to the list of symbols. */
 
	while ( !status && !SIgetrec(buf, MAXLINE + NULL_NL, fp) )
	{
		if ( STlength(buf) != MAXLINE )
		{
#ifndef FUSEDLL
			SIfprintf(stderr, "Trashed symbol table %s\n",
					NMSymloc.path);
#endif
			status = NM_STCORRUPT;
			break;
		}
 
		/*
		** Tab separates symbol from value.
		**
		** Terminate the symbol name string, and
		** leave bufr at the first char of the value.
		*/
 
		for ( bufr = buf ; bufr != '\0'; bufr++)
		{
			if (*bufr == '\t')
			{
				*bufr++ = '\0';
				break;
			}
		}
 
		STtrmwhite( bufr );
		status = NMaddsym( buf, bufr, sp );
		if( sp != NULL )
			sp = sp->s_next;
		else
		{
			sp = s_list;
		}
	}
 
	/* Uninterested in errors closing file opened only for reading */
 
	(VOID) SIclose( fp );
 
	return (status);
}
 
 

 

/*{
** Name: NMgtIngAt - get a symbol from the symbol.tbl
**
** Description:
**    get a symbol from the symbol.tbl
**
** Inputs:
**	name				    name of symbol
**
** Output:
**	value				    returned value of symbol.
**
**      Returns:
**          OK                         	    Function completed normally. 
**          FAIL                            Function completed abnormally.
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards.
**      15-jan-1996 (reijo01)
**          Allow generic system parameter overrides on symbol table access.
*/
NMgtIngAt(name, value)
register char	*name;
register char	**value;
{
	register	SYM	*sp;
	register 	i4	rval = OK;
	SYSTIME		thistime;
	char 		newname[MAXLINE];
	STATUS		CLerror = OK;
 
    if ( MEcmp( name, "II", 2 ) == OK )
        STpolycat( 2, SystemVarPrefix, name+2, newname );
    else
        STcopy( name, newname );

	/* prevent user from referencing a NULL pointer */
 
	*value = NULL_STRING;
 
	/* make sure all globals have been initialized */
	if ( !NM_static.Init )
	{
	    if( ( CLerror = NM_initsym() ) != OK )
	    {
		return CLerror;
	    }
	}
	MUp_semaphore( &NM_static.Sem );

	/* Read the symbol table file once.  Error if can't read symbols. */
 
	if ( s_list == NULL )
	{
		rval = NMreadsyms();
		if(rval == OK)
			LOlast(&NMSymloc, &NMtime);
	}
	else
	{
		/* test if symbol tbale file was modified ? */

		LOlast(&NMSymloc, &thistime);
		if(MEcmp((PTR)&thistime, (PTR)&NMtime, sizeof(NMtime)) != 0)
		{
			NMflushIng();
			rval = NMreadsyms();
			if(rval == OK)
				LOlast(&NMSymloc, &NMtime);
		}
	}
 
	if( OK == rval )
	{
		/*
		**  Search symbol table for the desired symbol.  If found,
		**    set value to that record and return.   If not found,
		**    value is still set to NULL_STRING.
		**
		**  Tedious linear search, but the list isn't very big.
		*/
	
		for ( sp = s_list; sp != NULL; sp = sp->s_next )
		{
			/* Check one char, then call slow function */
	
			if (*sp->s_sym == *newname && !STcompare(sp->s_sym,newname))
				break;
		}
	
		if ( sp != NULL )
			*value = sp->s_val;
	}
	MUv_semaphore( &NM_static.Sem );

	return( rval );
}


/*{
** Name: NMflushIng - flush the internal cache
**
** Description:
**
** Inputs:
**
** Output:
**
**      Returns:
**
** History:
**      10-Mar-89 (GordonW)
*/
STATUS
NMflushIng()
{
	register SYM	*sp, *lsp;

	/* first determine if any cache */
	sp = s_list;
	if(sp == NULL)
		return(OK);

	/* find end of chain  */
	while (sp->s_next != NULL)
		sp = sp->s_next;

	/* now free space */
	while(sp != NULL)
	{
		lsp = sp->s_last;
		MEfree((PTR)sp);
		sp = lsp;
	}
	s_list = NULL;

	return(OK);
}

/*{
**  Name: NMlogOperation - log ingsetenv / ingunset operations.
**
**  Description:
**	This function logs update operations on the symbol.tbl file to
**	a symbol.log file.
**
**  Inputs:
**	oper			name of operation to log
**	name			name of symbol being added/updated/deleted
**	value			value of symbol, if added/updated
**	new_value		new value of symbol, if updated
**
**  Output:
**	none
**
**      Returns:
**	    none
**
**  History:
**	07-jan-2004 (somsa01)
**	    Created.
*/
VOID
NMlogOperation(char *oper, char *name, char *value, char *new_value,
	       STATUS status)
{
    FILE	*fptr;
    char	host[GCC_L_NODE + 1];
    char	*username;
    SYSTIME	timestamp;
    char	buf[100], errbuf[ER_MAX_LEN];

    if ((status == OK || status == NM_STCORRUPT) &&
	SIopen(&NMLogSymloc, "a", &fptr) == OK)
    {
	SIfprintf(fptr, "*********************\n");
	SIfprintf(fptr, "*** BEGIN SESSION ***\n");
	SIfprintf(fptr, "*********************\n\n");

	/* Get host name */
	GChostname(host, sizeof(host));
	SIfprintf(fptr, "%-15s%s\n", "host:", host);

	/* Get user name */
	username = (char *)MEreqmem(0, 100, FALSE, NULL);
	IDname(&username);
	SIfprintf(fptr, "%-15s%s\n", "user:", username);
	MEfree(username);

	/* Get timestamp */
	TMnow(&timestamp);
	TMstr(&timestamp, buf);
	SIfprintf(fptr, "%-15s%s\n", "time:", buf);

	if (status == NM_STCORRUPT)
	{
	    ERreport(status, errbuf);
	    SIfprintf(fptr, "\n%s\n\n", errbuf);
	}
	else
	{
	    if (!STbcompare(oper, 0, "DELETE", 0, TRUE))
		SIfprintf(fptr, "\nDELETE %s\n\n", name);
	    else if (!STbcompare(oper, 0, "ADD", 0, TRUE))
	    {
		if (STstrindex(name, "II_GCN", 6, TRUE) &&
		    STstrindex(name, "_PORT", 0, TRUE))
		{
		    /* As a special case, do not log II_GCNxx_PORT value */
		    SIfprintf(fptr, "\nADD %s\n\n", name);
		}
		else
		    SIfprintf(fptr, "\nADD %s ... (%s)\n\n", name, value);
	    }
	    else if (!STbcompare(oper, 0, "CHANGE", 0, TRUE))
	    {
		if (STstrindex(name, "II_GCN", 6, TRUE) &&
		    STstrindex(name, "_PORT", 0, TRUE))
		{
		    /* As a special case, do not log II_GCNxx_PORT value */
		    SIfprintf(fptr, "\nCHANGE %s\n\n", name);
		}
		else
		{
		    SIfprintf(fptr, "\nCHANGE %s: (%s)...(%s)\n\n", name,
			      value, new_value);
		}
	    }
	}

	SIclose(fptr);
    }
}

/*{
** Name: NMhost - Get 'Ingres' hostname
**
** Description:	Private NM version of PMhost/GChostname which returns 
**		"massaged" hostname without trying to read II_HOSTNAME.
**
** Input:	buf - pointer to location to store hostname
**		size - size of buf
**
** Output:	
**		None
** History:
**	12-May-2004 (hanje04)
**	    Created.
*/

VOID
NMhostname(char *buf, i4 size)
{
    char *ptr;
    i4  i;

    if ( gethostname( buf, size ) != 0 ) 
    {
        /* failed to get hostname set EOS and return */
	*buf = EOS;
        return;
    }

    for( ptr = buf; *ptr != EOS ; CMnext( ptr ) )
    {
	switch( *ptr )
        {
             case '.':
# ifdef NT_GENERIC
             case '&':
             case '\'':
             case '$':
             case '#':
             case '!':
             case ' ':
             case '%':
# endif
             *ptr = '_';
        }

    }
}

/*{
** Name: NMlocksyms - Lock the symbols file for update.
**
** Description:
**		Atomically creates symbol.lck file
**		and populates it with the PID of
**		this process.
**		If the lock files exists, checks to
**		see if that owner is still alive;
**		if so, we enter an indefinite
**		wait/retry loop.
**		If the owner is dead, destroy the
**		symbol.lck file and try again.
**
**		Returns non-OK status if any of those
**		good intentions failed.
**
** Input:	NM_static
**
** Output:	
**		symbol.lck file is created.
**		NM_static.SymIsLocked is TRUE.
** History:
**	31-Aug-2004 (jenjo02)
**	    Created.
*/

STATUS
NMlocksyms( void )
{
    FILE	*fp;
    STATUS	status;
    char	buf[32];
    PID		LckPid;
    i4		retry = 0;
 
    /* First, make sure we're init'd */
    if ( !NM_static.Init && (status = NM_initsym()) )
	return(status);

    /* If we've already done this, just return */
    if ( NM_static.SymIsLocked )
	return(OK);

    /*
    ** SIcreate() returns OK if the file was (atomically)
    ** created by this process, not OK if the file exists.
    */
    while ( (status = SIcreate( &NMLckSymloc )) != OK  )
    {
	fp = NULL;
	LckPid = (PID)0;

	/* The lock file exists. Open and read the owner's PID */
	if ( (status = SIopen( &NMLckSymloc, "r", &fp )) == OK &&
	     (status = SIgetrec( buf, sizeof(buf), fp )) == OK )
	{
	    CVal(buf, &LckPid);
	}

	if ( fp )
	    (void) SIclose( fp );

	/*
	** If ENDFILE return from SIgetrec, it may be that
	** another process has created the file, below, but
	** has not yet opened and written its PID to the file
	** for us to see. In that case, wait/retry 10 times,
	** then assume that's not going to happen, 
	** delete the file and start over.
	**
	** If the owning process is still alive, wait a bit,
	** then try again indefinitely.
	**
	** If the owner has expired, delete the lock file,
	** then try again; this prevents multiple lockers from
	** both acquiring the dead lock.
	*/
	if ( (status == ENDFILE && ++retry < 10) ||
	     (LckPid && PCis_alive(LckPid)) )
	{
	    PCsleep(500);
	}
	else
	{
	    LOdelete( &NMLckSymloc );
	    retry = 0;
	}
    }

    /*
    ** We have the symbol.lck file to ourselves.
    ** Open it and write our PID.
    */
    if ( (status = SIopen( &NMLckSymloc, "w", &fp )) == OK )
    {
	PCpid(&LckPid);
	CVla(LckPid,buf);

	if ( SIputrec(buf, fp) == ENDFILE )
	    status = FAIL;
	else
	    NM_static.SymIsLocked = TRUE;
    }

    if ( fp )
	(void) SIclose( fp );

    return(status);
}

/*{
** Name: NMunlocksyms - Unlock the symbols file.
**
** Description:
**		If locked by us (SymIsLocked == TRUE),
**		close and destroy the symbol.lck file,
**		making it available to other
**		processes.
**
** Input:	NM_static
**
** Output:	
**		NM_static.SymIsLocked is FALSE;
**		symbol.lck file is deleted.
** History:
**	31-Aug-2004 (jenjo02)
**	    Created.
*/

void
NMunlocksyms( void )
{
    if ( NM_static.SymIsLocked )
    {
	NM_static.SymIsLocked = FALSE;
	LOdelete( &NMLckSymloc );
    }
    return;
}
