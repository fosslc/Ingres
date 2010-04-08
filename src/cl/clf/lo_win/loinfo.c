/*
** Copyright (c) 2004 Ingres Corporation
**
*/

#include        <compat.h>
#include        <gl.h>
#include	<systypes.h>
#include	<clconfig.h>
#include        "lolocal.h"
#include        <bt.h>
#include        <lo.h>
#include        <me.h>
#include	<errno.h>
#include	<sys/stat.h>
#include	<shellapi.h>  /* for SHGetFileInfo() */
#include    <aclapi.h>

# ifdef xDEBUG
#define PERR(api) PRINT("\n%s: Error %d from %s on line %d",  \
    __FILE__, GetLastError(), api, __LINE__); 
#define PMSG(msg) PRINT("\n%s line %d: %s",  \
    __FILE__, __LINE__, msg); 
# define PRINT1(a) PRINT(a)
# define PRINT2(a,b) PRINT(a,b)
# define PRINT3(a,b,c) PRINT(a,b,c)
# define PRINT4(a,b,c,d) PRINT(a,b,c,d)
# define PRINT5(a,b,c,d,e) PRINT(a,b,c,d,e)
# else /* xDEBUG */
# define PERR(api)
# define PMSG(msg)
# define PRINT1(a) 
# define PRINT2(a,b)
# define PRINT3(a,b,c)
# define PRINT4(a,b,c,d)
# define PRINT5(a,b,c,d,e)
# endif /* xDEBUG */

#define                           SZ_SD_BUF 8096 
#define                           SZ_INDENT_BUF 80 
#define STANDARD_RIGHTS_ALL_THE_BITS 0x00FF0000L
#define GENERIC_RIGHTS_ALL_THE_BITS  0xF0000000L


typedef enum _KINDS_OF_ACCESSMASKS_DECODED { 
    FileAccessMask, 
    ProcessAccessMask, 
    WindowStationAccessMask, 
    DesktopAccessMask, 
    RegKeyAccessMask, 
    ServiceAccessMask, 
    DefaultDaclInAccessTokenAccessMask 
    } KINDS_OF_ACCESSMASKS_DECODED, * PKINDS_OF_ACCESSMASKS_DECODED; 

static STATUS GetACLPerms(char *pathname,i4 *perms);
static BOOL SetPrivilegeInAccessToken( VOID );

static HANDLE           hAccessToken = INVALID_HANDLE_VALUE; 

/*
** Name: LOinfo()	- get information about a LOCATION
**
** Description:
**
**	Given a pointer to a LOCATION and a pointer to an empty
**	LOINFORMATION struct, LOinfo() fills in the struct with information
**	about the LOCATION.  A bitmask passed to LOinfo() indicates which
**	pieces of information are being requested.  This is so LOinfo()
**	can avoid unneccessary work.  LOinfo() clears any of the bits in
**	the request word if the information was unavailable.  If the
**	LOCATION object doesn't exist, all these bits will be clear.
**
**	The recognized information-request bit flags are:
**		LO_I_TYPE	- what is the location's type?
**		LO_I_PERMS	- give process permissions on the location.
**		LO_I_SIZE	- give size of location.
**		LO_I_LAST	- give time of last modification of location.
**		LO_I_ALL	- give all available information.
**
**	Note that LOinfo only gets information about the location itself,
**	NOT about the LOCATION struct representing the location.  That's
**	why LOisfull, LOdetail, and LOwhat are not subsumed by this
**	interface.  
**
**	This interface makes LOexists, LOisdir, LOsize,
**	and LOlast obsolete, and they will be phased out in the usual way.
**	(meaning: they will hang on for years.)
**
** Inputs:
**	loc		The location to check.
**	flags		A word with bits set that indicate which pieces of
**			 information are being requested.  This can save a
**			 lot of work on some machines...
**
** Outputs:
**	locinfo		The LOINFORMATION struct to fill in.  If any 
**			 information was unavailable, then the appropriate
**			 struct member(s) are set to zero (all integers), 
**			 or zero-ed out SYSTIME struct (li_last).
**			 Information availability is also indicated by the
**			 'flags' word... 
**	flags		The same word as input, but bits are set only for
**			 information that was successfully obtained.
**			 That is, if LO_I_TYPE is set and 
**			 .li_type == LO_IS_DIR, then we know the location is 
**			 a directory.  If LO_I_TYPE is not set, then we 
**			 couldn't find out whether or not the location 
**			 is a directory.
**
** Returns:
**	STATUS - OK if the LOinfo call succeeded and the LOCATION
**		exists.  FAIL if the call failed or the LOCATION doesn't exist.
**
** Exceptions:
**	none.
**
** Side Effects:
**	none.
**
** Example Use:
**	LOCATION loc;
**	LOINFORMATION loinf;
**	i4 flagword;
**
** History:
**	19-may-95 (emmag)
**	    Branched for NT.
**	08-dec-1997 (canor01)
**	    We need to look at the file attributes and the Access Control
**	    Lists for the object and report the most restrictive permissions.
**	15-jan-1998 (canor01)
**	    Ignore all but access-permission ACEs.  Remove permission bits
**	    for access that is either denied or not allowed.
**	17-jan-1998 (canor01)
**	    Allow all permissions with a NULL DACL.
**	26-jan-1998 (canor01)
**	    Standard Rights access does not really map into permissions.
**	    Combined denied with not-explicitly-allowed before masking
**	    permission field.
**	17-may-1998 (canor01)
**	    Retain the handle returned by OpenProcessToken() for use by all
**	    subsequent calls in this process.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	31-jul-2001 (somsa01)
**	    Use stati64() when LARGEFILE64 is defined.
**	21-oct-2001 (somsa01)
**	    Return errno if stat() fails.
**	20-ict-2004 (somsa01)
**	    Updated typecast for setting li_size to OFFSET_TYPE. This is
**	    consistent with UNIX.
**  23-Jun-2005 (fanra01)
**      Bug 114803
**      Replaced the lengthy examination of ACLs and ACEs with the 
**      GetEffectiveRightsFromAcl call which returns the rights
**      mask for a specified trustee.
**      Separated the setting of read and execute permissions.
**  22-Jul-2005 (fanra01)
**      Bug 114903
**      Replace GetEffectiveRightsFromAcl with individual calls to
**      AccessCheck which tests the rights against the security
**      descriptor of the file object.
**      The AccessCheck function determines whether a security descriptor
**      grants a specified set of access rights to the client identified by
**      their access token.
**  31-May-2006 (hanal04) Bug 116137
**      Removed code that checks SACL_SECURITY_INFORMATION. Users that are
**      not a member of the administrators group do not have the privilges
**      required to access this information and were then unable to
**      perform permissions checks for files and directories.
**  10-apr-2006 (horda03) BUG 115913
**      Extend fix for Windows.
*/

STATUS
LOinfo(loc, flags, locinfo)
    LOCATION *loc;
    i4 *flags;
    LOINFORMATION *locinfo;
{
    bool    isDirectory = FALSE;
#ifdef LARGEFILE64
    struct _stati64    statinfo;
#else
    struct stat    statinfo;
#endif  /* LARGEFILE64 */

    MEfill((u_i2) sizeof(*locinfo), '\0', (PTR) locinfo);

#ifdef LARGEFILE64
    if( _stati64( loc->string, &statinfo ) < 0 )
#else
    if( stat( loc->string, &statinfo ) < 0 )
#endif  /* LARGEFILE64 */
    {
        /* loc doesn't exist (or the call failed) */
        *flags = 0;
        return ( LOerrno((i4)errno) );
    }

    /*
    ** Set a flag for a directory
    */
    isDirectory = ((statinfo.st_mode & _S_IFDIR) == _S_IFDIR) ? TRUE : FALSE;
    
    if ((*flags & LO_I_SIZE) != 0)
    {
        locinfo->li_size = (OFFSET_TYPE) statinfo.st_size;
    }
        
    if ((*flags & LO_I_TYPE) != 0)
    {
        /* 
        ** We should be handling many different kinds of
        ** objects here.  This is definitely a punt.
        */
        DWORD  attr;

        if ((attr = GetFileAttributes(loc->string)) == -1)
            return(FAIL);

        if (attr & FILE_ATTRIBUTE_DIRECTORY)
        {
            locinfo->li_type = LO_IS_DIR;
        }
        else
        {
            locinfo->li_type = LO_IS_FILE;
        }
    }
        
    if ((*flags & LO_I_XTYPE) != 0)
    {
        /*
        ** The extended info type query overrides the old
        ** version, so process the new version last.
        **
        ** Modified from Unix.
        */
        if (statinfo.st_mode & S_IFDIR)
            locinfo->li_type = LO_IS_DIR;
        else
        if (statinfo.st_mode & S_IFREG)
            locinfo->li_type = LO_IS_FILE;
        else
        if (statinfo.st_mode & S_IFCHR)
            locinfo->li_type = LO_IS_CHR;
        else
            locinfo->li_type = LO_IS_UNKNOWN;
    }
		
    if ((*flags & LO_I_PERMS) != 0)
    {
        /*
        ** Test the owner's permission bits as returned from the
        ** statinfo and translate into LO permissions.
        */
        /*
        ** If this is a directory and running on Windows NT enable all
        ** the permissions.  Individual permissions will be removed by
        ** inspection of the ACL.
        */
        if ((isDirectory == TRUE) && (!(GetVersion() & 0x80000000)))
        {
           locinfo->li_perms |= (LO_P_READ|LO_P_WRITE|LO_P_EXECUTE);
        }
        else
        {
            /*
            ** Either a file or not Windows NT set the permissions
            ** according to the attributes.
            */
            if (statinfo.st_mode & S_IREAD)
               locinfo->li_perms |= LO_P_READ;
            if (statinfo.st_mode & S_IWRITE)
               locinfo->li_perms |= LO_P_WRITE;
            if (statinfo.st_mode & S_IEXEC)
               locinfo->li_perms |= LO_P_EXECUTE;

            /*
            ** SHGetFileInfo() with SHGFI_EXETYPE will return information
            ** on the type of executable the file is (exe, com, pif, etc.).
            ** It returns 0 for non-executable or error.  It works with
            ** both Windows 95 and Windows NT (in Shell32.dll).
            */
            if ( SHGetFileInfo( loc->string, 0, NULL, 0, SHGFI_EXETYPE ) != 0 )
                locinfo->li_perms |= LO_P_EXECUTE;
        }
        /*
        ** If this is Windows NT only, we must check the ACL
        ** as well and report most restrictive permissions.
        */
        if ( !(GetVersion() & 0x80000000) )
            GetACLPerms( loc->string, &locinfo->li_perms );
    }
        
    if ((*flags & LO_I_LAST) != 0)
    {
        locinfo->li_last.TM_secs = statinfo.st_mtime;
        locinfo->li_last.TM_msecs = 0;
    }

    return ( OK ); 
}

static STATUS
AccessTest( HANDLE hToken, PSECURITY_DESCRIPTOR psdSD, DWORD DesiredAccess,
    BOOL* bAccessGranted )
{
    STATUS                  status = OK;
    GENERIC_MAPPING         GenericMapping;
    DWORD                   GrantedAccess;
    PRIVILEGE_SET           PrivilegeSet;
    DWORD                   PrivSetSize = sizeof(PRIVILEGE_SET);
    
    /*
    ** Initialize the generic mapping structure.
    ** The GENERIC_MAPPING structure defines the mapping of generic access
    ** rights to specific and standard access rights for an object. When a
    ** client application requests generic access to an object, that request
    ** is mapped to the access rights defined in this structure.
    */
    MEfill( sizeof(GENERIC_MAPPING), 0, &GenericMapping );
    
    /*
    ** Test for access, set the required access flags.
    */
    switch(DesiredAccess)
    {
        case FILE_READ_DATA:
            GenericMapping.GenericRead = FILE_GENERIC_READ;
            break;
        case FILE_WRITE_DATA|FILE_APPEND_DATA:
            GenericMapping.GenericWrite = FILE_GENERIC_WRITE;
            break;
        case FILE_EXECUTE:
            GenericMapping.GenericExecute = FILE_GENERIC_EXECUTE;
            break;
    }
            
    MapGenericMask (&DesiredAccess, &GenericMapping);
    if (!AccessCheck (psdSD, hToken, DesiredAccess, &GenericMapping,
        &PrivilegeSet, &PrivSetSize, &GrantedAccess,
        bAccessGranted))
    {
        status = GetLastError();
    }
    return(status);
}

static
STATUS
GetACLPerms( char *fullname, i4 *perms )
{
    STATUS                  status = FAIL;
    DWORD                   dwErrorMode;
    PSECURITY_DESCRIPTOR    psdSD      = NULL; 
    DWORD                   dwSDLength = SZ_SD_BUF; 
    DWORD                   dwSDLengthNeeded; 
    i4			            allowed_perms = 0;
    i4			            denied_perms = 0;
    i4			            bitposition = -1;
    BOOL                    bAccessGranted;
    HANDLE                  hToken = NULL;

    while(TRUE)
    {

        dwErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS); 

        /*
        ** Get the size of the security descriptor buffer.
        */
        if (!GetFileSecurity 
            (fullname, 
            (SECURITY_INFORMATION)( OWNER_SECURITY_INFORMATION 
                                 | GROUP_SECURITY_INFORMATION 
                                 | DACL_SECURITY_INFORMATION ), 
            NULL, 
            0, 
            (LPDWORD)&dwSDLength))
        {
            /*
            ** Stop processing for status other than the buffer size error.
            */
            if ((status = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
            {
                break;
            }
        }
        /*
        ** Allocate memory required for security descriptor.
        */
        if ((psdSD = MEreqmem( 0, dwSDLength, TRUE, &status )) == NULL)
        {
            break;
        }
        
        /*
        ** Get the security descriptor for the file object
        */ 
        if (!GetFileSecurity 
            (fullname, 
            (SECURITY_INFORMATION)( OWNER_SECURITY_INFORMATION 
                                 | GROUP_SECURITY_INFORMATION 
                                 | DACL_SECURITY_INFORMATION ),
            psdSD, 
            dwSDLength, 
            (LPDWORD)&dwSDLengthNeeded)) 
        {
            status = GetLastError();
            break;
        } 

        SetErrorMode(dwErrorMode); 
 
        /*
        ** Test for a valid security descriptor
        */
        if (!IsValidSecurityDescriptor(psdSD))
        {
            status = GetLastError();
            break;
        }
        /*
        ** Perform security impersonation of the user of the current thread.
        */
        if (!ImpersonateSelf (SecurityImpersonation))
        {
            status = GetLastError();
            break;
        }

        /*
        ** Get a thread token for the current user
        */
        if (!OpenThreadToken (GetCurrentThread (), TOKEN_DUPLICATE | 
            TOKEN_QUERY, FALSE, &hToken))
        {
            break;
        }
        RevertToSelf ();
        
        /*
        ** Test for read access
        */
        if (AccessTest( hToken, psdSD, FILE_READ_DATA, &bAccessGranted ) != OK)
        {
            break;
        }
        else
        {
            allowed_perms |= (bAccessGranted) ? LO_P_READ : 0;
        }
        /*
        ** Test for write access
        */
        if (AccessTest( hToken, psdSD, FILE_WRITE_DATA|FILE_APPEND_DATA,
            &bAccessGranted ) != OK)
        {
            break;
        }
        else
        {
            allowed_perms |= (bAccessGranted) ? LO_P_WRITE : 0;
        }
        /*
        ** Test for execute access
        */
        if (AccessTest( hToken, psdSD, FILE_EXECUTE, &bAccessGranted ) != OK)
        {
            break;
        }
        else
        {
            allowed_perms |= (bAccessGranted) ? LO_P_EXECUTE : 0;
        }
        status = OK;
        break;
    }
    /* deny = explicitly denied or not explicitly allowed */
    denied_perms |= ( allowed_perms ^ (LO_P_READ|LO_P_WRITE|LO_P_EXECUTE) );

    /* clear all denied permissions */
    bitposition = -1;
    bitposition = BTnext( bitposition, 
                          (char *)&denied_perms, sizeof(denied_perms) );
    while ( bitposition != -1 )
    {
        BTclear( bitposition, (char *)perms );
        bitposition = BTnext( bitposition, 
                              (char *)&denied_perms, sizeof(denied_perms) );
    }
    if (psdSD != NULL)
    {
        MEfree( (PTR)psdSD );
    }
    if (hToken)
    {
        CloseHandle( hToken );
    }
    return (status);
}

/****************************************************************************\ 
* 
* FUNCTION: SetPrivilegeInAccessToken 
* 
\****************************************************************************/ 

static 
BOOL 
SetPrivilegeInAccessToken(VOID) 
{ 
  HANDLE           hProcess; 
  LUID             luidPrivilegeLUID; 
  TOKEN_PRIVILEGES tpTokenPrivilege; 
 
  hProcess = GetCurrentProcess(); 
  if (!hProcess) 
  { 
    PERR("GetCurrentProcess"); 
    return(FALSE); 
  } 

  if (hAccessToken == INVALID_HANDLE_VALUE)
  {
    if (!OpenProcessToken(hProcess, 
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
                          &hAccessToken)) 
    { 
      PERR("OpenProcessToken"); 
      return(FALSE); 
    } 
  }

  /**************************************************************************\
  *
  * Get LUID of SeSecurityPrivilege privilege
  *
  \**************************************************************************/

  if (!LookupPrivilegeValue(NULL,
                            SE_SECURITY_NAME,
                            &luidPrivilegeLUID))
  { 
    PERR("LookupPrivilegeValue");
    PRINT1("\nThe above error means you need to log on as an Administrator");
    return(FALSE);
  }

  /**************************************************************************\
  *
  * Enable the SeSecurityPrivilege privilege using the LUID just
  *   obtained
  *
  \**************************************************************************/

  tpTokenPrivilege.PrivilegeCount = 1;
  tpTokenPrivilege.Privileges[0].Luid = luidPrivilegeLUID;
  tpTokenPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  AdjustTokenPrivileges( hAccessToken,
                         FALSE,  /* Do not disable all */
                         &tpTokenPrivilege,
                         sizeof(TOKEN_PRIVILEGES),
                         NULL,   /* Ignore previous info */
                         NULL ); /* Ignore previous info */

  if ( GetLastError() != NO_ERROR )
  { 
    PERR("AdjustTokenPrivileges");
    return(FALSE);
  }

  return(TRUE);
}
