/*
** Copyright (c) 1987, 2004 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <qu.h>
#include    <gc.h>
#include    <lo.h>
#include    <si.h>
#include    <cs.h>
#include    <pc.h>
#include    <lk.h>
#include    <cx.h>

#include    "gcacli.h"
#include    <clconfig.h>
#include    <diracc.h>
#include    <handy.h>
#include    <pwd.h>

/*
**
**  Name: GCACN.C - GCA CL support for GCN
**
**	See GCACL.C for list of source files.
**
*/


/*{
** Name: GCnsid	- Establish or obtain the identification of the local Name 
**		  Server
**
** Description:
**
**	GCnsid is invoked to establish and to obtain the identity of the Name
**	Server in a system-dependent way.  This function provides the 
**	"bootstrapping" capability required to allow the GCA Connection 
**	Manager to establish an association with the Name Server.
**
**	Two operations may be invoked: PUT and GET.  The former is invoked 
**	by Name Server to establish its global identity in a system specific
**	way.  This identity is the "listen_id" which it establishes as a 
**	normal server to allow clients to request associations.  The latter is
**	invoked by Connection Manager in order to obtain the listen_id which it
**	must use to create an association with Name Server.
**
**	The exact syntax of the identifier may be system dependent, although 
**	this is not of concern to GCnsid.  The identifiier consists of a 
**	nul-terminated character string.  It is established in a 
**	system-dependent way so as to be globally accessible by all GCA 
**	Connection Managers in the local node.
**
**
** Inputs:
**	flag				A constant value specifying the
**					oepration to be performed.  The 
**					following values are defined:
**
**	    GCN_SET_LISTENID		Establish a globally accessible 
**					identifier for the Name Server.
**
**	    GCN_FIND_LISTENID   	Retrieve the identifier of the Name
**					Server in the local node.
**
**	gca_listen_id			A pointer to an area containing
**					the character string specifying
**					the identity of the Name Server, 
**					if the operation is PUT, or an
**					area in which to place the character
**					string identifier if the operation 
**					is GET.
**	length				The length of listen_id to be set.
**
** Outputs (GCA_GET_NSID operation only):
**
**	*gca_listen_id			The Name Server identifier.
**
**	Returns:
**	    STATUS:
**		E_GC0025_NM_SRVR_ID_ERR	Unable to obtain NS id.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-Sept-1987 (lin,jbowers)
**          Created.
**	08-Dec-1987 (Adjusted temporarily to solve compilation problem on SUN)
**	    Since Yi-Chein did his first port attempt, changes have
**	    taken place on VMS. The port must be re-done when the name
**	    server is brought up SUN. GCnsid is not currently being
**	    used since operation is always "GCA_NO_XLATE" in gca_cm.
**	04-Aug-89 (seiwald)
**	    GCcm is dead.  Long live GCrequest.
**	09-Nov-89 (seiwald)
**	    Return error if name server listen address II_GCN_PORT not set.
**	20-Feb-90 (seiwald)
**	    NMgtAt is void.
**	13-Mar-90 (seiwald)
**	    GCnsid now takes a sys_err, not a svc_parms.
**	18-Mar-91 (seiwald)
**	    Clear II_GCN_PORT on shutdown.
**	8-jun-93 (ed)
**	    added VOID to match prototype
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-aug-93 (ed)
**	    add missing include
**	04-may-1998 (canor01)
**	    GChostname should default to using II_HOSTNAME, if it is set.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	02-jul-2001 (devjo01)
**	    "decorate" II_GCN<inst>_PORT with node name, since
**	    symbol table is shared across cluster as opposed to
**	    the group level logicals used under VMS. (s103715)
**	17-sep-2002 (devjo01)
**	    Virtual NUMA nodes on one physical node share the
**	    name server.  Use PMhost to decorate II_GCN<inst>_PORT,
**	    rather than CXnode_name.
**	15-Jun-2004 (schka24)
**	    Length-limit return value length for FIND (env var safety).
**	22-nov-2004 (devjo01)
**	    Do not "decorate" II_GCN<inst>_PORT with node name. Changes
**	    after 02-jul-2001 introduced separate symbol tables per
**	    node, and changing the name of this logical broke backward
**	    compatibility with earlier releases.
*/

STATUS
GCnsid( i4  flag, char *gca_listen_id, i4  length, CL_ERR_DESC *sys_err)
{
	STATUS		CLerror;
	char		instid[10+1];	/* For safety */
	char		mysym[ 200 ];
	char		*inst;
	char		*port;

	NMgtAt( "II_INSTALLATION", &inst );
	if ( !inst ) inst = "";
	STlcopy(inst, &instid[0], sizeof(instid)-1);
	STprintf( mysym, "II_GCN%s_PORT", instid );

	switch( flag )
	{
	case GC_SET_NSID:
		if( ( CLerror = NMstIngAt( mysym, gca_listen_id ) ) != OK )
			return CLerror;
		break;

	case GC_CLEAR_NSID:
		if( ( CLerror = NMstIngAt( mysym, "" ) ) != OK )
			return CLerror;
		break;

	case GC_RESV_NSID:
		break;

	case GC_FIND_NSID:
		NMgtAt( mysym, &port );
		if( !port || !*port )
			return GC_NM_SRVR_ID_ERR;
		STlcopy( port, gca_listen_id, length-1 );
		break;
	default:
		return GC_NM_SRVR_ID_ERR;
	}

	return OK;
} /* end GCnsid */

/*{
** Name: GChostname - Obtain the identification of the local host
**
** History:
**      22-May-1997 (allmi01)
**         For sos_us5, terminate hostname at first period (.)
**         thereby truncating domain name.
**	04-may-1998 (canor01)
**	    GChostname should default to using II_HOSTNAME, if it is set.
**	08-mar-2007 (smeke01) BUG 115434
**	    The size passed in is equivalent to sizeof(buf), and
**	    includes the space required for EOS marker. Since sizeof() is
**	    a 1-based count of elements we need to decrement by 1 when
**	    accessing the last element (to set EOS marker), and when
**	    passing to any functions that expect a 0-based count of
**	    elements.
*/

VOID
GChostname( char *buf, i4  size )
{
    char *ptr;
    i4  i;

    NMgtAt( "II_HOSTNAME", &ptr );
    if ( ptr && *ptr )
    {
        STncpy( buf, ptr, size - 1 );
	buf[ size - 1 ] = EOS;
    }
    else if ( gethostname( buf, size - 1 ) != 0 )
    {
        *buf = EOS;
    }

#ifdef sos_us5
for (i=0; i < size; i++)
	{
	if (buf[i] == '.')
		buf[i] = '\0';
	}
#endif

} /* end GChostname */


/*
** Name: GCusername
**
** Description:
**	Set the passed in argument 'name' to contain the name of the user 
**	who is executing this process.  Name is a pointer to an array 
**	large enough to hold the user name.  The user name is returned
**	NULL-terminated in lowercase with any trailing whitespace deleted.
**
**	On systems where relevant, this should return the effective rather 
**	than the real user ID. 
**
** Input:
**	size		Length of buffer for user name.
**
** Output:
**	name		The effective user name.
**
** Returns:
**	VOID
**
** History:
**	17-Mar-98 (gordy)
**	    Created.
*/

VOID
GCusername( char *name, i4  size )
{
    struct passwd	*p_pswd, pwd;
    char		pwuid_buf[BUFSIZ];

    p_pswd = iiCLgetpwuid( geteuid(), &pwd, pwuid_buf, sizeof( pwuid_buf ) );
    STncpy( name, (p_pswd == NULL) ? SystemAdminUser : p_pswd->pw_name, 
	     size - 1 );
    name[ size - 1 ] = EOS;

    return;
}
